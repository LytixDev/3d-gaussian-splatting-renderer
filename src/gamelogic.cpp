#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <utilities/imageLoader.hpp>
#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"
#include "utilities/plyParser.hpp"
#include "utilities/camera.hpp"
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "fmt/core.h"
#include "imgui.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp> // Enables to_string on glm types, handy for debugging

Gloom::Camera *camera = new Gloom::Camera(glm::vec3(0.0f, 0.0f, 2.0f), 50.0f, 0.1f);
double lastFrameTime = 0.0;  // Change from GLfloat to double for better precision
SceneNode* rootNode;
// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* shader3D;
Gloom::Shader* shaderGaussian;


void mouseCallback(GLFWwindow* window, double x, double y) {
    camera->handleCursorPosInput(x, y);
    // Forward to ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    camera->handleMouseButtonInputs(button, action);
    // Forward to ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button] = action;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    camera->handleKeyboardInputs(key, action);
    // NOTE: We don't forward any keys to ImGui. Maybe we want to do that later.
}




GLuint vao, vbo;
glm::vec3 gaussianPos = glm::vec3(0.0f, 0.0f, -5.0f); // World position

void setupGaussian() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &gaussianPos, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void renderGaussian(glm::vec3 gaussianPos) {
    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);
    glm::mat4 VP = projection * camera->getViewMatrix();

    // Create model matrix to place Gaussian at its world position
    glm::mat4 model = glm::translate(glm::mat4(1.0f), gaussianPos);
    glm::mat4 MVP = VP * model; // Full transformation

    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(vao);
    glEnable(GL_PROGRAM_POINT_SIZE); // Enable point size control
    glDrawArrays(GL_POINTS, 0, 1);
}


void init_game(GLFWwindow* window) {
    GaussianSplat splat = gaussian_splat_from_ply_file("../res/ornaments.ply");
    gaussian_splat_print(splat);

    setupGaussian();

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    shader3D = new Gloom::Shader();
    shader3D->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader3D->activate();

    shaderGaussian = new Gloom::Shader();
    shaderGaussian->makeBasicShader("../res/shaders/gaussian.vert", "../res/shaders/gaussian.frag");


    rootNode = createSceneNode(GEOMETRY);

    SceneNode *triNode = createSceneNode(GEOMETRY);
    Mesh tri = triangle(glm::vec3(50));
    unsigned int triVAO = generateBuffer(tri);
    triNode->position = glm::vec3(10.0, 0.0, -80.0);
    triNode->vertexArrayObjectID  = triVAO;
    triNode->VAOIndexCount        = tri.indices.size();

    rootNode->children.push_back(triNode);

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;
}

void update_frame(GLFWwindow* window) {
    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastFrameTime);
    lastFrameTime = currentTime;
    
    camera->updateCamera(deltaTime);

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);
    glm::mat4 VP = projection * camera->getViewMatrix();

    glm::mat4 identity = glm::mat4(1);
    updateNodeTransformations(rootNode, identity, VP);
}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 VP) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->modelMatrix = transformationThusFar * transformationMatrix;
    node->currentTransformationMatrix = VP * node->modelMatrix;
    node->normalMatrix = glm::mat3(glm::transpose(glm::inverse(node->modelMatrix)));

    switch(node->nodeType) {
        case NORMAL_MAPPED: break;
        case GEOMETRY_2D: break;
        case GEOMETRY: break;
        case POINT_LIGHT:
            node->lightPosition = glm::vec3(node->modelMatrix * glm::vec4(0, 0, 0, 1));
            break;
        case SPOT_LIGHT: {
        } break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->modelMatrix, VP);
    }
}


void renderNode3D(SceneNode* node) {
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->modelMatrix));
    glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(node->normalMatrix));

    switch(node->nodeType) {
        case NORMAL_MAPPED:
            /* Intentional fallthrough */
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case GEOMETRY_2D: {
        } break;
        case POINT_LIGHT: {
        } break;
        case SPOT_LIGHT: {
        } break;
    }

    for(SceneNode* child : node->children) {
        renderNode3D(child);
    }
}

void render_frame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    shader3D->activate();
    glUniform3fv(shader3D->getUniformFromName("camera_position"), 1, glm::value_ptr(camera->getPosition()));
    renderNode3D(rootNode);

    shaderGaussian->activate();
    renderGaussian(gaussianPos);
}
