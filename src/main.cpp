#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>

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

    VoxelData voxelData = VoxelLoader::loadVoxelData("assets/world.xraw", VoxelDataAxis::Z_Up);
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
    albedoTexture->textureImage2D(TextureFormat::RGB16F, windowSize.x, windowSize.y, (float*)NULL);
    albedoTexture->setFilterMode(TextureFilterMode::NEAREST);

    std::shared_ptr<Texture> normalTexture0 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    normalTexture0->textureImage2D(TextureFormat::RGB16F, windowSize.x, windowSize.y, (float*)NULL);
    normalTexture0->setFilterMode(TextureFilterMode::NEAREST);
    std::shared_ptr<Texture> normalTexture1 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    normalTexture1->textureImage2D(TextureFormat::RGB16F, windowSize.x, windowSize.y, (float*)NULL);
    normalTexture1->setFilterMode(TextureFilterMode::NEAREST);
    std::weak_ptr<Texture> normalTexture = normalTexture0;
    std::weak_ptr<Texture> prevNormalTexture = normalTexture1;

    std::shared_ptr<Texture> posTexture0 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    posTexture0->textureImage2D(TextureFormat::RGB32F, windowSize.x, windowSize.y, (float*)NULL);
    posTexture0->setFilterMode(TextureFilterMode::NEAREST);
    std::shared_ptr<Texture> posTexture1 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    posTexture1->textureImage2D(TextureFormat::RGB32F, windowSize.x, windowSize.y, (float*)NULL);
    posTexture1->setFilterMode(TextureFilterMode::NEAREST);
    std::weak_ptr<Texture> posTexture = posTexture0;
    std::weak_ptr<Texture> prevPosTexture = posTexture1;

    gBuffer.attachTexture(albedoTexture.get(), 0);
    gBuffer.attachTexture(normalTexture.lock().get(), 1);
    gBuffer.attachTexture(posTexture.lock().get(), 2);

    gBuffer.unbind();

    Framebuffer lightingFrameBuffer;
    lightingFrameBuffer.bind();

    std::shared_ptr<Texture> frameTexture0 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    frameTexture0->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    frameTexture0->setFilterMode(TextureFilterMode::NEAREST);
    std::shared_ptr<Texture> frameTexture1 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    frameTexture1->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    frameTexture1->setFilterMode(TextureFilterMode::NEAREST);
    std::weak_ptr<Texture> frameTexture = frameTexture0;
    std::weak_ptr<Texture> prevFrameTexture = frameTexture1;

    std::shared_ptr<Texture> blueNoiseTexture = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    blueNoiseTexture->textureImage2D("assets/blueNoise.png", 3);
    blueNoiseTexture->setFilterMode(TextureFilterMode::NEAREST);
    blueNoiseTexture->setWrapModeR(TextureWrapMode::REPEAT);
    blueNoiseTexture->setWrapModeS(TextureWrapMode::REPEAT);

    std::shared_ptr<Texture> denoisedFrame0 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    denoisedFrame0->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    denoisedFrame0->setFilterMode(TextureFilterMode::NEAREST);
    denoisedFrame0->setWrapModeR(TextureWrapMode::REPEAT);
    denoisedFrame0->setWrapModeS(TextureWrapMode::REPEAT);

    std::shared_ptr<Texture> denoisedFrame1 = std::make_shared<Texture>(TextureType::TEXTURE_2D);
    denoisedFrame1->textureImage2D(TextureFormat::RGBA16F, windowSize.x, windowSize.y, (float*)NULL);
    denoisedFrame1->setFilterMode(TextureFilterMode::NEAREST);
    denoisedFrame1->setWrapModeR(TextureWrapMode::REPEAT);
    denoisedFrame1->setWrapModeS(TextureWrapMode::REPEAT);

    std::weak_ptr<Texture> denoisedFrameDst = denoisedFrame0;
    std::weak_ptr<Texture> denoisedFrameSrc = denoisedFrame1;

    lightingFrameBuffer.attachTexture(frameTexture.lock().get(), 0);
    lightingFrameBuffer.unbind();

    Shader gBufferShader("shader.glsl");
    if(!gBufferShader.compiledSuccessfully()) return -1;
    Shader lightingShader("lightingShader.glsl");
    if(!lightingShader.compiledSuccessfully()) return -1;
    Shader taaShader("taaShader.glsl");
    if(!taaShader.compiledSuccessfully()) return -1;
    Shader denoisingShader("denoisingShader.glsl");
    if(!denoisingShader.compiledSuccessfully()) return -1;
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
    lightingShader.setTexture(blueNoiseTexture, 8, "u_blueNoiseTexture");

    postProcessShader.useShader();
    postProcessShader.setTexture(frameTexture, 0, "u_frameTexture");
    postProcessShader.setTexture(albedoTexture, 1, "u_gAlbedo");
    postProcessShader.setTexture(normalTexture, 2, "u_gNormal");
    postProcessShader.setTexture(posTexture, 3, "u_gPos");

    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 prevPosition = position;
    double cameraAngle = 8.8025;
    glm::mat3 cameraRotMatrix; 
    glm::mat3 prevCameraRotMatrix;
    double xMousePos, yMousePos;
    glfwGetCursorPos(window, &xMousePos, &yMousePos);


    std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> previousTime = currentTime;
    double timeAccumulator = 0.0;
    int frameCounter = 0;
    double deltaTime = 0.0;
    int frame = 0;

    int outputImageSelection = 0;
    float taaAlpha = 0.1;
    float taaDistWeightScaler = 0.1;
    float taaNormalWeightScaler = 0.02;
    float taaColorWeightScaler = 0.01;

    bool enableDenoising = true;
    float denoisingColorWeightScaler = 0.01;
    float denoisingNormalWeightScaler = 0.01;
    float denoisingPosWeightScaler = 0.5;

    int denoiseIterations = 3;

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
        frame++;

        // Camera movement
        if(cursorHidden) cameraAngle = xMousePos / 400.0;
        float movementSpeed = 0.1;
        glm::vec3 forwardVector = glm::vec3(sin(cameraAngle), 0.0, -cos(cameraAngle));
        glm::vec3 strafeVector = glm::cross(forwardVector, glm::vec3(0.0, 1.0, 0.0));
        prevPosition = position;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) movementSpeed *= 5;
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) position.y += movementSpeed;
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position.y -= movementSpeed;
        
        prevCameraRotMatrix = cameraRotMatrix;
        cameraRotMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), (float)-cameraAngle, glm::vec3(0.0, 1.0, 0.0)));

        glfwGetCursorPos(window, &xMousePos, &yMousePos);

        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            if(cursorHidden) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorHidden = !cursorHidden;
        }

        // Render g buffer
        gBuffer.bind();
        // glClear(GL_COLOR_BUFFER_BIT);
        gBufferShader.useShader();
        vao.bind();

        gBufferShader.setUniform3f("u_cameraPos", position.x, position.y, position.z);
        gBufferShader.setUniformMat3("u_cameraRotMatrix", cameraRotMatrix);

        gBuffer.attachTexture(normalTexture.lock().get(), 1);
        gBuffer.attachTexture(posTexture.lock().get(), 2);
        
        gBuffer.setDrawBuffers();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Lighting calculations
        lightingFrameBuffer.bind();
        lightingShader.useShader();
        lightingShader.setUniform1f("u_frame", float(frame));
        lightingShader.setUniform2f("u_noiseTextureScale", (float)windowSize.x / (float)blueNoiseTexture->getWidth(), (float)windowSize.y / (float)blueNoiseTexture->getHeight());

        lightingFrameBuffer.attachTexture(frameTexture.lock().get(), 0);
        lightingShader.setTexture(normalTexture, 1, "u_gNormal");
        lightingShader.setTexture(posTexture, 2, "u_gPos");

        lightingShader.bindTextures();
        
        lightingFrameBuffer.setDrawBuffers();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // TAA
        if(taaAlpha < 1.0) {
            taaShader.useShader();
            taaShader.setTexture(frameTexture, 0, "u_frameTexture");
            taaShader.setTexture(normalTexture, 1, "u_normalTexture");
            taaShader.setTexture(posTexture, 2, "u_posTexture");
            taaShader.setTexture(prevFrameTexture, 4, "u_prevFrameTexture");
            taaShader.setTexture(prevNormalTexture, 5, "u_prevNormalTexture");
            taaShader.setTexture(prevPosTexture, 6, "u_prevPosTexture");

            taaShader.setUniform3f("u_cameraPos", position.x, position.y, position.z);
            taaShader.setUniform3f("u_prevCameraPos", prevPosition.x, prevPosition.y, prevPosition.z);
            taaShader.setUniformMat3("u_prevCameraRotMatrix", prevCameraRotMatrix);
            taaShader.setUniform2f("u_windowSize", (float)windowSize.x, (float)windowSize.y);
            taaShader.setUniform1f("u_fov", 1.0);
            taaShader.setUniform1f("u_taaAlpha", taaAlpha);
            taaShader.setUniform1f("u_taaDistWeightScaler", taaDistWeightScaler);
            taaShader.setUniform1f("u_taaNormalWeightScaler", taaNormalWeightScaler);
            taaShader.setUniform1f("u_taaColorNormalScaler", taaColorWeightScaler);

            taaShader.bindTextures();

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        std::weak_ptr<Texture> result = frameTexture;

        if(enableDenoising) {
            denoisingShader.useShader();
            denoisingShader.setUniform2f("u_windowSize", (float)windowSize.x, (float)windowSize.y);
            denoisingShader.setUniform1f("u_colorWeightScaler", denoisingColorWeightScaler);
            denoisingShader.setUniform1f("u_normalWeightScaler", denoisingNormalWeightScaler);
            denoisingShader.setUniform1f("u_posWeightScaler", denoisingPosWeightScaler);
            denoisingShader.setTexture(albedoTexture, 1, "u_albedoTexture");
            denoisingShader.setTexture(normalTexture, 2, "u_normalTexture");
            denoisingShader.setTexture(posTexture, 3, "u_posTexture");

            for(int iteration = 0; iteration < denoiseIterations; ++iteration) {
                std::swap(denoisedFrameSrc, denoisedFrameDst);
                std::weak_ptr<Texture> srcTexture = (iteration == 0) ? frameTexture : denoisedFrameSrc;

                denoisingShader.setUniform1f("u_scale", (float)(std::pow(2.0, iteration)));

                denoisingShader.setTexture(srcTexture, 0, "u_frameTexture");
                lightingFrameBuffer.attachTexture(denoisedFrameDst.lock().get(), 0);

                denoisingShader.bindTextures();
                lightingFrameBuffer.setDrawBuffers();
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            result = denoisedFrameDst;
        }

        // Render final frame
        lightingFrameBuffer.unbind();
        postProcessShader.useShader();
        postProcessShader.setTexture(result, 0, "u_frameTexture");
        postProcessShader.setTexture(normalTexture, 2, "u_gNormal");
        postProcessShader.bindTextures();
        postProcessShader.setUniform1i("u_frameTexture", outputImageSelection);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Render GUI
        ImGui::RadioButton("Show final image", &outputImageSelection, 0);
        ImGui::RadioButton("Show albedo buffer", &outputImageSelection, 1);
        ImGui::RadioButton("Show normal buffer", &outputImageSelection, 2);

        ImGui::SliderFloat("TAA alpha", &taaAlpha, 0.0, 1.0, "%f");
        ImGui::SliderFloat("TAA dist weight scaler", &taaDistWeightScaler, 0.0, 1.0);
        ImGui::SliderFloat("TAA normal weight scaler", &taaNormalWeightScaler, 0.0, 1.0);
        ImGui::SliderFloat("TAA color weight scaler", &taaColorWeightScaler, 0.0, 1.0);
        ImGui::SliderFloat("Denoising color weight scaler", &denoisingColorWeightScaler, 0.01, 1.0);
        ImGui::SliderFloat("Denoising normal weight scaler", &denoisingNormalWeightScaler, 0.01, 1.0);
        ImGui::SliderFloat("Denoising pos weight scaler", &denoisingPosWeightScaler, 0.01, 1.0);
        ImGui::Checkbox("Enable denoising", &enableDenoising);
        ImGui::SliderInt("Denoise iterations", &denoiseIterations, 0, 10);

        if(ImGui::Button("Hide cursor")) {
            cursorHidden = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        std::swap(frameTexture, prevFrameTexture);
        std::swap(normalTexture, prevNormalTexture);
        std::swap(posTexture, prevPosTexture);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(NULL);

    glfwTerminate();
    return 0;
}