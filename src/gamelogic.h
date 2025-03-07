#pragma once

#include <GLFW/glfw3.h>
#include <utilities/window.hpp>
#include "sceneGraph.hpp"
#include "program.hpp"

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 VP);
void init_game(GLFWwindow* window, ProgramState state);
void update_frame(GLFWwindow* window, ProgramState *state);
void render_frame(GLFWwindow* window, ProgramState *state);
