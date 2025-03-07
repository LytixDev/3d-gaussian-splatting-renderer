// Local headers
#include "program.hpp"
#include "utilities/plyParser.hpp"
#include "utilities/window.hpp"
#include "gamelogic.h"
#include <glm/glm.hpp>
// glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <utilities/shader.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utilities/timeutils.h>

#include <thread>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;


static std::vector<std::string> list_ply_and_splat_files(const std::string& directory) {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".ply" || entry.path().extension() == ".splat") {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

static void load_model(ProgramState *state, std::string model_path)
{
    GaussianSplat loaded_model;
    // NOTE: Lol ...
    if (model_path.back() == 'y') {
        loaded_model = gaussian_splat_from_ply_file(model_path);
    } else {
        loaded_model = gaussian_splat_from_splat_file(model_path);
    }
    std::cout << "Loaded new model:" << std::endl;
    gaussian_splat_print(loaded_model);
    state->loaded_model = loaded_model;
    state->current_model = model_path;
    state->change_model = true;
    state->is_loading_model = false;
}


static void imgui_draw(ProgramState *state)
{
    // Find currently selected model so we can default the dropdown list of models to it
    size_t selected_model_index = 0;
    for (size_t i = 0; i < state->all_models.size(); i++) {
        if (state->all_models[i] == state->current_model) {
            selected_model_index = i;
            break;
        }
    }

    bool b;
    ImGui::Begin("Program Settings");
    float fps = ImGui::GetIO().Framerate;
    float delta_time = ImGui::GetIO().DeltaTime * 1000.0f;
    ImGui::Text("FPS: %.1f | Frametime: %.3f ms", fps, delta_time);
    ImGui::Checkbox("Draw Triangle", &b);
    
    // Display loading thingy if a model is currently being loaded
    if (state->is_loading_model) {
        ImGui::Text("Loading model... Please wait");
        ImGui::SameLine();
        float time = ImGui::GetTime();
        char spinner[4] = {"|/-\\"[(int)(time * 10) % 4], 0};
        ImGui::Text("%s", spinner);
    }

    // TODO: For large models, we could consider caching the parsed splat file
    if (!state->all_models.empty()) {
        char *preview_value = (char *)state->all_models[selected_model_index].c_str();
        if (ImGui::BeginCombo("Select Model", preview_value)) {
            for (size_t i = 0; i < state->all_models.size(); i++) {
                bool is_selected = selected_model_index == i;
                // Disable selection if currently loading a model
                if (!state->is_loading_model && 
                    ImGui::Selectable(state->all_models[i].c_str(), is_selected) &&
                    selected_model_index != i) {
                    // This path is only taken if a new model is selected and no other models
                    // are already loading. Since we only allow one thread we don't need
                    // any synchronization mechanisms here.
                    selected_model_index = i;
                    std::string selected_model = state->all_models[i];
                    
                    // Create a new detatched thread for loading the splat file
                    state->is_loading_model = true;
                    std::thread loading_thread([state, selected_model]() {
                        load_model(state, selected_model);
                    });
                    loading_thread.detach();
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    // Display data for the currently chosen model
    if (ImGui::CollapsingHeader("Model Statistics")) {
        ImGui::Text("Vertex Count: %zu", state->loaded_model.count);
    }

    // Display any warnings or errors for the currently chosen model
    if (state->loaded_model.had_error) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Model loaded with errors:");
        if (ImGui::BeginChild("ModelErrors", ImVec2(0, 100), true)) {
            for (const auto& message : state->loaded_model.warning_and_error_messages) {
                // TODO: should probably have a tag or something on the error message
                //       stupid to have to to string compares all the time

                // Color code warnings and errors differently
                if (message.find("Warning:") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", message.c_str());
                } else if (message.find("Error:") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", message.c_str());
                } else {
                    ImGui::Text("%s", message.c_str());
                }
            }
            ImGui::EndChild();
        }
    } else if (!state->loaded_model.warning_and_error_messages.empty()) {
        // No errors, but some warnings
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Model loaded with warnings:");
        if (ImGui::BeginChild("ModelWarnings", ImVec2(0, 100), true)) {
            for (const auto& message : state->loaded_model.warning_and_error_messages) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", message.c_str());
            }
            ImGui::EndChild();
        }
    }

    ImGui::End();
}

void run_program(GLFWwindow* window)
{
    // Disable vsync
    glfwSwapInterval(0);

    // Enable depth (Z) buffer (accept "closest" fragment)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Configure miscellaneous OpenGL settings
    glEnable(GL_CULL_FACE);

    // Disable built-in dithering
    glDisable(GL_DITHER);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set default colour after clearing the colour buffer
    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);

    // Initialise global program state
    ProgramState state;
    state.all_models = list_ply_and_splat_files("../res/");
    load_model(&state, state.all_models.at(0));

	init_game(window, state);

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
	    // Clear colour and depth buffers
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

        update_frame(window, &state);
        render_frame(window);

        imgui_draw(&state);

        // Handle input events
        glfwPollEvents();
        handleKeyboardInput(window);

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Flip buffers
        glfwSwapBuffers(window);
    }
}


void handleKeyboardInput(GLFWwindow* window)
{
    // Use escape key for terminating the GLFW window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
