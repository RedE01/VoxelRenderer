#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>

#include "VertexBuffer.h"
#include "VertexArray.h"
#include "ShaderStorageBuffer.h"
#include "Shader.h"

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

    window = glfwCreateWindow(640, 480, "Voxel Renderer", NULL, NULL);
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
         1.0,  1.0,  0.0,
        -1.0,  1.0,  0.0,
        -1.0, -1.0,  0.0,
    };

    VertexArray vao;

    std::vector<VertexAttribute> vboAttributes { VertexAttribute(3, VertexAttributeType::FLOAT, false) };
    VertexBuffer vbo(vboAttributes);
    vbo.setData(verticies, sizeof(verticies), BufferDataUsage::STATIC_DRAW);

    vao.unbind();

    Shader shader("shader.glsl");

    ShaderStorageBuffer octreeNodesSSB(0);
    octreeNodesSSB.setData(octree.nodes.data(), octree.nodes.size() * sizeof(OctreeNode), BufferDataUsage::DYNAMIC_COPY);

    ShaderStorageBuffer chunkDataSSB(1);
    chunkDataSSB.setData(octree.chunkData.data(), octree.chunkData.size() * sizeof(uint8_t), BufferDataUsage::DYNAMIC_COPY);

    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    double cameraAngle = 0.0;
    double xMousePos, yMousePos;
    glfwGetCursorPos(window, &xMousePos, &yMousePos);

    shader.setUniform1ui("u_worldWidth", octree.worldWidth);
    shader.setUniform1ui("u_maxOctreeDepth", octree.maxDepth);
    shader.setUniform1ui("u_chunkWidth", WORLD_WIDTH / std::pow(2, octree.maxDepth));
    shader.setUniform3fv("u_palette", 256, (float*)palette);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        shader.useShader();
        vao.bind();

        cameraAngle = xMousePos / 400.0;
        float movementSpeed = 0.5;
        glm::vec3 forwardVector = glm::vec3(sin(cameraAngle), 0.0, cos(cameraAngle));
        glm::vec3 strafeVector = glm::cross(glm::vec3(0.0, 1.0, 0.0), forwardVector);
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= movementSpeed * forwardVector;
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += movementSpeed * strafeVector;
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) position.y += movementSpeed;
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position.y -= movementSpeed;

        double lastMouseXPos = xMousePos;
        glfwGetCursorPos(window, &xMousePos, &yMousePos);
        double deltaMouseX = xMousePos - lastMouseXPos;
        shader.setUniform3f("u_cameraPos", position.x, position.y, position.z);
        shader.setUniform1f("u_cameraDir", cameraAngle);

        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            if(cursorHidden) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorHidden = !cursorHidden;
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}