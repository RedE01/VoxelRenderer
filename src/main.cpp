#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

#ifdef VOXEL_RENDERER_DEBUG
    #include "Debug.h"
#endif

const unsigned int WORLD_WIDTH = 256;
uint8_t world[WORLD_WIDTH][WORLD_WIDTH][WORLD_WIDTH];
glm::vec3 palette[256];

struct OctreeNode {
    OctreeNode(unsigned int parentIndex) : parentIndex(parentIndex), childrenIndices{0, 0, 0, 0, 0, 0, 0, 0}, dataIndex(0), isSolidColor(1) {}
    const unsigned int parentIndex;
    unsigned int childrenIndices[8];
    int isSolidColor;
    unsigned int dataIndex;
};

class Octree {
public:
    Octree(uint8_t* world, unsigned int worldWidth, unsigned int maxDepth)
        : worldWidth(worldWidth), maxDepth(maxDepth) {

        nodes.push_back(OctreeNode(0));
        initOctree(0, 0, 0, 0, 0);
    }

private:
    void initOctree(unsigned int currentIndex, int depth, int startx, int starty, int startz) {
        int width = worldWidth / std::pow(2, depth);
        int hWidth = width / 2;
        
        if(!isSolidColor(width, startx, starty, startz)) {
            nodes[currentIndex].isSolidColor = 0;
            if(depth >= maxDepth) {
                initData(nodes[currentIndex], width, startx, starty, startz);
            }
            else {
                for(int i = 0; i < 8; ++i) {
                    nodes[currentIndex].childrenIndices[i] = nodes.size();
                    nodes.push_back(OctreeNode(currentIndex));

                    int cStartx = startx + ((i % 2 == 0) ? 0 : hWidth);
                    int cStarty = starty + ((i % 4 <  2) ? 0 : hWidth);
                    int cStartz = startz + ((i % 8 <  4) ? 0 : hWidth);
                    initOctree(nodes[currentIndex].childrenIndices[i], depth + 1, cStartx, cStarty, cStartz);
                }
            }
        }
        else {
            nodes[currentIndex].dataIndex = world[startz][starty][startx];
        }
    }

    bool isSolidColor(int width, int startx, int starty, int startz) {
        int endx = startx + width;
        int endy = starty + width;
        int endz = startz + width;
        uint8_t uniqueColor = world[startz][starty][startx];
        for(int z = startz; z < endz; z++) {
            for(int y = starty; y < endy; y++) {
                for(int x = startx; x < endx; x++) {
                    if(world[z][y][x] != uniqueColor) return false;
                }
            }
        }
        return true;
    }

    void initData(OctreeNode& node, int width, int startx, int starty, int startz) {
        int endx = startx + width;
        int endy = starty + width;
        int endz = startz + width;
        node.dataIndex = chunkData.size();
        for(int z = startz; z < endz; z++) {
            for(int y = starty; y < endy; y++) {
                for(int x = startx; x < endx; x++) {
                    chunkData.push_back(world[z][y][x]);
                }
            }
        }
    }

public:
    const unsigned int worldWidth;
    const unsigned int maxDepth;
    
    std::vector<uint8_t> chunkData;
    std::vector<OctreeNode> nodes;
};

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

    #ifdef VOXEL_RENDERER_DEBUG
    initializeDebugger();
    #endif

    for(int x = 0; x < WORLD_WIDTH; x++) {
        for(int y = 0; y < WORLD_WIDTH; y++) {
            for(int z = 0; z < WORLD_WIDTH; z++) {
                world[z][y][x] = 0;

                int h = 50;
                if((x - h)*(x - h) + (y - h)*(y - h) + (z - h)*(z - h) <= 20*20) {
                    world[z][y][x] = 1 + std::rand() % 3;
                }
            }
        }
    }

    palette[1] = glm::vec3(0.8, 0.0, 0.0);
    palette[2] = glm::vec3(0.0, 0.8, 0.0);
    palette[3] = glm::vec3(0.0, 0.0, 0.8);

    Octree octree((uint8_t*)world, WORLD_WIDTH, 5);
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
    Shader lightingShader("lightingShader.glsl");
    Shader postProcessShader("postProcessShader.glsl");

    gBufferShader.useShader();
    gBufferShader.setUniform1ui("u_worldWidth", octree.worldWidth);
    gBufferShader.setUniform1ui("u_maxOctreeDepth", octree.maxDepth);
    gBufferShader.setUniform1ui("u_chunkWidth", WORLD_WIDTH / std::pow(2, octree.maxDepth));
    gBufferShader.setUniform3fv("u_palette", 256, (float*)palette);
    gBufferShader.setUniform2i("u_windowSize", windowSize.x, windowSize.y);
    gBufferShader.setUniform1f("u_fov", 1.0);

    lightingShader.useShader();
    lightingShader.setUniform1ui("u_worldWidth", octree.worldWidth);
    lightingShader.setUniform1ui("u_maxOctreeDepth", octree.maxDepth);
    lightingShader.setUniform1ui("u_chunkWidth", WORLD_WIDTH / std::pow(2, octree.maxDepth));

    lightingShader.setTexture(albedoTexture, 0, "u_gAlbedo");
    lightingShader.setTexture(normalTexture, 1, "u_gNormal");
    lightingShader.setTexture(posTexture, 2, "u_gPos");

    postProcessShader.useShader();
    postProcessShader.setTexture(frameTexture, 0, "u_frameTexture");

    glm::vec3 position = glm::vec3(-107.341, 33.5, -126.044);
    double cameraAngle = 8.8025;
    double xMousePos, yMousePos;
    glfwGetCursorPos(window, &xMousePos, &yMousePos);


    std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> previousTime = currentTime;
    double timeAccumulator = 0.0;
    int frameCounter = 0;

    double deltaTime = 0.0;

    while (!glfwWindowShouldClose(window)) {
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

        cameraAngle = xMousePos / 400.0;
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

        double lastMouseXPos = xMousePos;
        glfwGetCursorPos(window, &xMousePos, &yMousePos);
        double deltaMouseX = xMousePos - lastMouseXPos;
        gBufferShader.setUniform3f("u_cameraPos", position.x, position.y, position.z);
        gBufferShader.setUniform1f("u_cameraDir", cameraAngle);

        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            if(cursorHidden) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorHidden = !cursorHidden;
        }

        unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        lightingFrameBuffer.bind();
        lightingShader.useShader();
        lightingShader.setUniform1f("u_deltaTime", float(deltaTime));
        lightingShader.bindTextures();
        
        unsigned int attachments2[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, attachments2);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        lightingFrameBuffer.unbind();
        postProcessShader.useShader();
        postProcessShader.bindTextures();
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}