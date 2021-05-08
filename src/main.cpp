#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <cstring>
#include <chrono>
#include <iostream>

#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "VertexArray.h"
#include "ShaderStorageBuffer.h"
#include "Shader.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "VoxelLoader.h"
#include "Octree.h"

#ifdef VOXEL_RENDERER_DEBUG
    #include "Debug.h"
#endif

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        return -1;
    }

    #ifdef VOXEL_RENDERER_DEBUG
    enableDebugging();
    #endif

    glm::ivec2 windowSize(1280, 720);
    window = glfwCreateWindow(windowSize.x, windowSize.y, "Voxel Renderer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    bool cursorHidden = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if(glewInit() != GLEW_OK) {
        return -1;
    }

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(NULL);

    #ifdef VOXEL_RENDERER_DEBUG
    initializeDebugger();
    #endif

    VoxelData voxelData = VoxelLoader::loadVoxelData("monu1.xraw", VoxelDataAxis::Z_Up);
    uint8_t* world = voxelData.voxelData;
    glm::vec3* palette = (glm::vec3*)voxelData.paletteData;

    if(voxelData.voxelData == nullptr) {
        return -1;
    }
    if(voxelData.sizeX != voxelData.sizeY || voxelData.sizeX != voxelData.sizeZ) {
        std::cout << "ERROR: world sides must have the same length" << std::endl;
        return -1;
    }


    Octree octree(world, voxelData.sizeX, 5);

    delete[] world;
    std::cout << octree.chunkData.size() << ", " << octree.nodes.size() << " : " << octree.chunkData.size() + octree.nodes.size() * sizeof(OctreeNode) << std::endl;

    float verticies[6 * 3] {
        -1.0, -1.0,  0.0,
         1.0, -1.0,  0.0,
         1.0,  1.0,  0.0,
        -1.0,  1.0,  0.0,
    };

    unsigned int indicies[6] {
        0, 1, 2,  2, 3, 0
    };

    VertexArray vao;

    std::vector<VertexAttribute> vboAttributes { VertexAttribute(3, VertexAttributeType::FLOAT, false) };
    VertexBuffer vbo(vboAttributes);
    vbo.setData(verticies, sizeof(verticies), BufferDataUsage::STATIC_DRAW);

    ElementBuffer ebo;
    ebo.setData(indicies, sizeof(indicies), BufferDataUsage::STATIC_DRAW);

    vao.unbind();

    ShaderStorageBuffer octreeNodesSSB(0);
    octreeNodesSSB.setData(octree.nodes.data(), octree.nodes.size() * sizeof(OctreeNode), BufferDataUsage::DYNAMIC_COPY);

    ShaderStorageBuffer chunkDataSSB(1);
    chunkDataSSB.setData(octree.chunkData.data(), octree.chunkData.size() * sizeof(uint8_t), BufferDataUsage::DYNAMIC_COPY);

    Framebuffer gBuffer;
    gBuffer.bind();

    std::shared_ptr<Texture> albedoTexture = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    albedoTexture->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    albedoTexture->setFilterMode(TextureFilterMode::NEAREST);

    std::shared_ptr<Texture> normalTexture = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    normalTexture->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    normalTexture->setFilterMode(TextureFilterMode::NEAREST);

    std::shared_ptr<Texture> posTexture = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    posTexture->textureImage2D(TextureFormat::RGBA32F, windowSize.x, windowSize.y, (float*)NULL);
    posTexture->setFilterMode(TextureFilterMode::NEAREST);

    gBuffer.attachTexture(albedoTexture.get(), 0);
    gBuffer.attachTexture(normalTexture.get(), 1);
    gBuffer.attachTexture(posTexture.get(), 2);

    gBuffer.unbind();

    Framebuffer lightingFrameBuffer;
    lightingFrameBuffer.bind();
    std::shared_ptr<Texture> frameTexture = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    frameTexture->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    frameTexture->setFilterMode(TextureFilterMode::NEAREST);
    lightingFrameBuffer.attachTexture(frameTexture.get(), 0);
    lightingFrameBuffer.unbind();

    Shader gBufferShader("shader.glsl");
    if(!gBufferShader.compiledSuccessfully()) return -1;
    Shader lightingShader("lightingShader.glsl");
    if(!lightingShader.compiledSuccessfully()) return -1;
    Shader postProcessShader("postProcessShader.glsl");
    if(!postProcessShader.compiledSuccessfully()) return -1;

    gBufferShader.useShader();
    gBufferShader.setUniform1ui("u_worldWidth", octree.worldWidth);
    gBufferShader.setUniform1ui("u_maxOctreeDepth", octree.maxDepth);
    gBufferShader.setUniform1ui("u_chunkWidth", octree.worldWidth / std::pow(2, octree.maxDepth));
    gBufferShader.setUniform3fv("u_palette", 256, (float*)palette);
    gBufferShader.setUniform2i("u_windowSize", windowSize.x, windowSize.y);
    gBufferShader.setUniform1f("u_fov", 1.0);

    lightingShader.useShader();
    lightingShader.setUniform1ui("u_worldWidth", octree.worldWidth);
    lightingShader.setUniform1ui("u_maxOctreeDepth", octree.maxDepth);
    lightingShader.setUniform1ui("u_chunkWidth", octree.worldWidth / std::pow(2, octree.maxDepth));

    lightingShader.setTexture(albedoTexture, 0, "u_gAlbedo");
    lightingShader.setTexture(normalTexture, 1, "u_gNormal");
    lightingShader.setTexture(posTexture, 2, "u_gPos");

    postProcessShader.useShader();
    postProcessShader.setTexture(frameTexture, 0, "u_frameTexture");
    postProcessShader.setTexture(albedoTexture, 1, "u_gAlbedo");
    postProcessShader.setTexture(normalTexture, 2, "u_gNormal");

    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    double cameraAngle = 8.8025;
    double xMousePos, yMousePos;
    glfwGetCursorPos(window, &xMousePos, &yMousePos);


    std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> previousTime = currentTime;
    double timeAccumulator = 0.0;
    int frameCounter = 0;

    double deltaTime = 0.0;

    int outputImageSelection = 0;

    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto d = currentTime - previousTime;
        previousTime = currentTime;
        currentTime = std::chrono::high_resolution_clock::now();
        frameCounter++;
        timeAccumulator += d.count();
        if(timeAccumulator > 1000000000) {
            timeAccumulator -= 1000000000;
            std::cout << "Fps: " << frameCounter << ", Frame time: " << 1.0 / frameCounter << std::endl;
            frameCounter = 0;
        }
        deltaTime = d.count() / 1000000000.0;

        gBuffer.bind();
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        gBufferShader.useShader();
        vao.bind();

        if(cursorHidden) cameraAngle = xMousePos / 400.0;
        float movementSpeed = 0.1;
        glm::vec3 forwardVector = glm::vec3(sin(cameraAngle), 0.0, -cos(cameraAngle));
        glm::vec3 strafeVector = glm::cross(forwardVector, glm::vec3(0.0, 1.0, 0.0));
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) movementSpeed *= 5;
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) position.y += movementSpeed;
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position.y -= movementSpeed;

        glfwGetCursorPos(window, &xMousePos, &yMousePos);
        gBufferShader.setUniform3f("u_cameraPos", position.x, position.y, position.z);
        gBufferShader.setUniform1f("u_cameraDir", cameraAngle);

        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            if(cursorHidden) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorHidden = !cursorHidden;
        }

        gBuffer.setDrawBuffers();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        lightingFrameBuffer.bind();
        lightingShader.useShader();
        lightingShader.setUniform1f("u_deltaTime", float(deltaTime));
        lightingShader.bindTextures();
        
        lightingFrameBuffer.setDrawBuffers();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        lightingFrameBuffer.unbind();
        postProcessShader.useShader();
        postProcessShader.bindTextures();
        postProcessShader.setUniform1i("u_frameTexture", outputImageSelection);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::RadioButton("Show final image", &outputImageSelection, 0);
        ImGui::RadioButton("Show albedo buffer", &outputImageSelection, 1);
        ImGui::RadioButton("Show normal buffer", &outputImageSelection, 2);

        if(ImGui::Button("Hide cursor")) {
            cursorHidden = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(NULL);

    glfwTerminate();
    return 0;
}