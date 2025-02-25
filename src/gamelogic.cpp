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
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "fmt/core.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp> // Enables to_string on glm types, handy for debugging

#include "imgui.h"

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"


// TODO: Move
#include "utilities/camera.hpp"
Gloom::Camera *camera = new Gloom::Camera(glm::vec3(0.0f, 0.0f, 2.0f), 50.0f, 0.1f);


double lastFrameTime = 0.0;  // Change from GLfloat to double for better precision


SceneNode* rootNode;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader3D;
Gloom::Shader* shader2D;

CommandLineOptions options;

// Modify if you want the music to start further on in the track. Measured in seconds.
double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;

#define LIGHT_SOURCES 1
SceneNode *lightSources[LIGHT_SOURCES];


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

unsigned int imageToTexture(PNGImage image) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Copy image into texture
    glTexImage2D(GL_TEXTURE_2D,
                 0, // Level of detail. 0 since we are only creating a single texture.
                 GL_RGBA, // Internal format
                 image.width, // Width
                 image.height, // Height
                 0, // Border
                 GL_RGBA, // Input format the input data is stored in
                 GL_UNSIGNED_BYTE, // Data and type per channel
                 image.pixels.data()); // Image data

    glGenerateMipmap(GL_TEXTURE_2D);
    // Configure the sampling options for the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}


void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    options = gameOptions;

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    shader3D = new Gloom::Shader();
    shader3D->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader3D->activate();

    shader2D = new Gloom::Shader();
    shader2D->makeBasicShader("../res/shaders/2d.vert", "../res/shaders/2d.frag");

    rootNode = createSceneNode(GEOMETRY);

    SceneNode *triNode = createSceneNode(GEOMETRY);
    Mesh tri = triangle(glm::vec3(50));
    unsigned int triVAO = generateBuffer(tri);
    triNode->position = glm::vec3(10.0, 0.0, -80.0);
    triNode->vertexArrayObjectID  = triVAO;
    triNode->VAOIndexCount        = tri.indices.size();

    rootNode->children.push_back(triNode);

    SceneNode *padLight = createSceneNode(POINT_LIGHT);
    padLight->position = glm::vec3(5.0, 5.0, 20.0);
    padLight->lightColor = glm::vec3(1.0, 1.0, 1.0);
    rootNode->children.push_back(padLight);
    lightSources[padLight->lightNodeID] = padLight;

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;
}

void updateFrame(GLFWwindow* window) {
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

    /* Set normal mapped flag */
    if (node->nodeType == NORMAL_MAPPED) {
        glUniform1i(7, 1);
    } else {
        glUniform1i(7, 0);
    }

    switch(node->nodeType) {
        case NORMAL_MAPPED:
            glBindTextureUnit(0, node->textureID);
            glBindTextureUnit(1, node->textureNormalID);
            glBindTextureUnit(2, node->roughnessID);
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

void render3D(SceneNode *root) {
    shader3D->activate();
    
    glUniform3fv(shader3D->getUniformFromName("camera_position"), 1, glm::value_ptr(camera->getPosition()));

    // Pass light positions to fragment shader
    for (int i = 0; i < LIGHT_SOURCES; i++) {
        SceneNode *node = lightSources[i];
        auto prefix = fmt::format("light_sources[{}]", i);
        // Position
        auto location_position = shader3D->getUniformFromName(prefix + ".position");
        glUniform3fv(location_position, 1, glm::value_ptr(node->lightPosition));
        // Color
        auto location_color = shader3D->getUniformFromName(prefix + ".color");
        glUniform3fv(location_color, 1, glm::value_ptr(node->lightColor));
    }

    renderNode3D(root);
}

void renderNode2D(SceneNode* node) {
    switch(node->nodeType) {
        case GEOMETRY_2D: {
            if (node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
        default: break;
        }
    }

    for(SceneNode* child : node->children) {
        renderNode2D(child);
    }
}

void render2D(SceneNode *root) {
    shader2D->activate();
    /* Orthographic project with center (0,0) at the bottom left corner */
    glm::mat4 orthographicProjection = glm::ortho(0.0f,
                                                  (float)windowWidth,
                                                  0.0f,
                                                  (float)windowHeight,
                                                  0.0f, // Near plane
                                                  1.0f); // Far plane
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(orthographicProjection));

    /* 
     * NOTE: We only draw text for the 2D part of rendering right now, so we can just set the 
     * texture here and not dynamically in the scene node switch.
     */
    //glBindTextureUnit(0, charMapTextureID);
    renderNode2D(root);
}


#define KEY_PRESSED(key) (glfwGetKey(window, (key)) == GLFW_PRESS)
#define KEY_RELEASED(key) (glfwGetKey(window, (key)) == GLFW_RELEASE)

int lastKeyX = -1;
int lastKeyZ = -1;

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    render3D(rootNode);
    //render2D(rootNode);
}
