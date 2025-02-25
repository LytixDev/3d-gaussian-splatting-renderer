#pragma once

#include <utilities/window.hpp>
#include "sceneGraph.hpp"

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 VP);
void init_game(GLFWwindow* window);
void update_frame(GLFWwindow* window);
void render_frame(GLFWwindow* window);
