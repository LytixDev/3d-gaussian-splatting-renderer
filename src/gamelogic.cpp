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
#include "glm/fwd.hpp"
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

Gloom::Camera *camera = new Gloom::Camera(glm::vec3(0.3f, 0.0f, 2.5f), 2.0f, 0.075f);
double lastFrameTime = 0.0;
SceneNode* rootNode;
// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* shader3D;
Gloom::Shader* shader_gaussian;
Gloom::Shader* shader_point_cloud;

// Projection matrix variables
float field_of_view = glm::radians(80.0f);
float near_clipping_plane = 0.1f;
float far_clipping_plane = 200.0f;

GaussianSplat splat;

GLuint vao, vbo, ebo, positionVBO, colorVBO, scaleVBO, alphaVBO, rotationVBO;
GLuint instancedVAO;


void mouseCallback(GLFWwindow* window, double x, double y) 
{
    camera->handleCursorPosInput(x, y);
    // Forward to ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
{
    camera->handleMouseButtonInputs(button, action);
    // Forward to ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[button] = action;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
    camera->handleKeyboardInputs(key, action);
    // NOTE: We don't forward any keys to ImGui. Maybe we want to do that later.
}

void setup_quad() 
{
    float quad_vertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };

    int quad_indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // Generate and bind VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Generate and bind VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    // Generate and bind EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
}

void setup_gaussians() 
{
    // First, set up the quad
    setup_quad();
    
    // Set up instanced rendering
    glGenVertexArrays(1, &instancedVAO);
    glBindVertexArray(instancedVAO);
    
    // Bind the quad's VBO and EBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    
    // NOTE: Clean up this
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    
    // Position buffer setup
    glGenBuffers(1, &positionVBO);
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 splat.ws_positions.size() * sizeof(glm::vec3), 
                 splat.ws_positions.data(), 
                 GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // Tell OpenGL this is an instanced attribute
    
    // Color buffer setup
    glGenBuffers(1, &colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 splat.colors.size() * sizeof(glm::vec3),
                 splat.colors.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1); // Tell OpenGL this is an instanced attribute
    
    // Scale buffer setup
    glGenBuffers(1, &scaleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 splat.scales.size() * sizeof(glm::vec3),
                 splat.scales.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1); // Tell OpenGL this is an instanced attribute
    
    // Alpha buffer setup
    glGenBuffers(1, &alphaVBO);
    glBindBuffer(GL_ARRAY_BUFFER, alphaVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 splat.opacities.size() * sizeof(float),
                 splat.opacities.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1); // Tell OpenGL this is an instanced attribute
    
    // Rotation buffer setup
    glGenBuffers(1, &rotationVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rotationVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 splat.rotations.size() * sizeof(glm::vec4),
                 splat.rotations.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1); // Tell OpenGL this is an instanced attribute
}

void free_gaussians() 
{
    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &instancedVAO);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &positionVBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &scaleVBO);
    glDeleteBuffers(1, &alphaVBO);
    glDeleteBuffers(1, &rotationVBO);
}

void render_gaussians(ProgramState *state, bool render_as_point_cloud) 
{
    // Calculate view projection matrix
    float aspect_ratio = float(state->windowWidth) / float(state->windowHeight);
    glm::mat4 projection = glm::perspective(field_of_view, aspect_ratio, near_clipping_plane, far_clipping_plane);
    glm::mat4 VP = projection * camera->getViewMatrix();
    
    // Set VP matrix uniform
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(VP));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(camera->getViewMatrix()));
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Bind the instanced VAO
    glBindVertexArray(instancedVAO);
   
    if (render_as_point_cloud) {
        // Draw as points
        glEnable(GL_PROGRAM_POINT_SIZE);
        //glDrawArrays(GL_POINTS, 0, splat.ws_positions.size());
        glDrawArraysInstanced(GL_POINTS, 0, 1, splat.ws_positions.size());
    } else {
        // Draw all Gaussians as instanced quads
        // 6 vertices per quad 
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, splat.ws_positions.size());
    }
}


void init_game(GLFWwindow* window, ProgramState state) 
{
    splat = state.loaded_model;
    //gaussian_splat_print(splat);
    setup_gaussians();

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Setup shaders
    shader3D = new Gloom::Shader();
    shader3D->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader_gaussian = new Gloom::Shader();
    shader_gaussian->makeBasicShader("../res/shaders/gaussian.vert", "../res/shaders/gaussian.frag");
    shader_point_cloud = new Gloom::Shader();
    shader_point_cloud->makeBasicShader("../res/shaders/point_cloud.vert", "../res/shaders/point_cloud.frag");

    shader3D->activate();

    // Regular geometry
    rootNode = createSceneNode(GEOMETRY);
    SceneNode *triNode = createSceneNode(GEOMETRY);
    Mesh tri = quadrilateral(glm::vec3(50));
    unsigned int triVAO = generateBuffer(tri);
    triNode->position = glm::vec3(10.0, 0.0, -40.0);
    triNode->vertexArrayObjectID  = triVAO;
    triNode->VAOIndexCount        = tri.indices.size();
    rootNode->children.push_back(triNode);

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;
}

void update_frame(GLFWwindow* window, ProgramState *state)
{
    if (state->change_model) {
        free_gaussians();
        state->change_model = false;
        splat = state->loaded_model;
        //std::cout << "Changing model!" << std::endl;
        //gaussian_splat_print(splat);
        setup_gaussians();
    }

    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastFrameTime);
    lastFrameTime = currentTime;
    
    camera->updateCamera(deltaTime);

    float aspect_ratio = float(state->windowWidth) / float(state->windowHeight);
    glm::mat4 projection = glm::perspective(field_of_view, aspect_ratio, near_clipping_plane, far_clipping_plane);
    glm::mat4 VP = projection * camera->getViewMatrix();

    glm::mat4 identity = glm::mat4(1);
    updateNodeTransformations(rootNode, identity, VP);
}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 VP) 
{
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

glm::mat4 lastViewMatrix;

typedef struct {
    size_t index;
    float depth;
} GaussianDepth;

bool depth_sort_and_update_buffers() 
{
    // Only perform the depth sort if camera has moved
    glm::mat4 currentViewMatrix = camera->getViewMatrix();
    bool viewMatrixChanged = true;
    viewMatrixChanged = false;
    for (int i = 0; i < 4 && !viewMatrixChanged; i++) {
        for (int j = 0; j < 4 && !viewMatrixChanged; j++) {
            // Use a small epsilon for floating point comparison
            if (std::abs(currentViewMatrix[i][j] - lastViewMatrix[i][j]) > 0.0001f) {
                viewMatrixChanged = true;
            }
        }
    }
    if (!viewMatrixChanged) {
        return false;
    }

    lastViewMatrix = currentViewMatrix;

    std::vector<GaussianDepth> depthSortData(splat.ws_positions.size());
    glm::mat4 viewMatrix = camera->getViewMatrix();
    
    // ~300 ms
    // Calculate view-space depth for each Gaussian
    for (size_t i = 0; i < splat.ws_positions.size(); i++) {
        glm::vec4 viewPos = viewMatrix * glm::vec4(splat.ws_positions[i], 1.0f);
        depthSortData[i].index = i;
        depthSortData[i].depth = -viewPos.z;  // Negative because view space goes into  the negative Z
    }
    
    // ~600 ms
    // TODO: Count sort is faster, and then consider sorting on the GPU
    std::sort(depthSortData.begin(), depthSortData.end(),
        [](const GaussianDepth& a, const GaussianDepth& b) {
            return a.depth > b.depth;
        });

    
    // ~500 ms
    // Create temporary vectors for sorted data
    std::vector<glm::vec3> sorted_positions(splat.ws_positions.size());
    std::vector<glm::vec3> sorted_colors(splat.colors.size());
    std::vector<glm::vec3> sorted_scales(splat.scales.size());
    std::vector<float> sorted_opacities(splat.opacities.size());
    std::vector<glm::vec4> sorted_rotations(splat.rotations.size());
    
    // Reorder data based on sorted depths
    for (size_t i = 0; i < depthSortData.size(); i++) {
        size_t idx = depthSortData[i].index;
        sorted_positions[i] = splat.ws_positions[idx];
        sorted_colors[i] = splat.colors[idx];
        sorted_scales[i] = splat.scales[idx];
        sorted_opacities[i] = splat.opacities[idx];
        sorted_rotations[i] = splat.rotations[idx];
    }
    
    // TODO: Is this necessary? Can we do some trickery to make this fast?
    // Update buffers
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, sorted_positions.size() * sizeof(glm::vec3),
                 sorted_positions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, sorted_colors.size() * sizeof(glm::vec3),
                 sorted_colors.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
    glBufferData(GL_ARRAY_BUFFER, sorted_scales.size() * sizeof(glm::vec3),
                 sorted_scales.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, alphaVBO);
    glBufferData(GL_ARRAY_BUFFER, sorted_opacities.size() * sizeof(float),
                 sorted_opacities.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, rotationVBO);
    glBufferData(GL_ARRAY_BUFFER, sorted_rotations.size() * sizeof(glm::vec4),
                 sorted_rotations.data(), GL_STATIC_DRAW);

    return true;
}

void render_frame(GLFWwindow* window, ProgramState *state) 
{
    if (state->depth_sort) {
        auto start_time = std::chrono::high_resolution_clock::now();
        if (depth_sort_and_update_buffers()) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            state->depth_sort_time_in_ms = duration;
        }
    }

    glfwGetWindowSize(window, &state->windowWidth, &state->windowHeight);
    glViewport(0, 0, state->windowWidth, state->windowHeight);

    shader3D->activate();
    glUniform3fv(shader3D->getUniformFromName("camera_position"), 1, glm::value_ptr(camera->getPosition()));
    renderNode3D(rootNode);

    if (state->render_as_point_cloud) {
        shader_point_cloud->activate();
    } else {
        shader_gaussian->activate();
    }
    //glUniform1f(shader_gaussian->getUniformFromName("scale_multipler"), state->scale_multiplier);
    glUniform1f(1, state->scale_multiplier);

    // Camera params used to calculate the Jacobian from view space to screen space
    float htany = tan(field_of_view / 2);
    float htanx = htany / float(state->windowHeight) * float(state->windowHeight);
    // Distance to the focal plane based on the vertical fov
    float focal_z = float(state->windowHeight) / (2 * htany);
    glm::vec3 focal_fov = glm::vec3(htanx, htany, focal_z);
    glUniform3fv(4, 1, glm::value_ptr(focal_fov));


    glUniform1i(5, state->draw_mode);

    render_gaussians(state, state->render_as_point_cloud);
}
