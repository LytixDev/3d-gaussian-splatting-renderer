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
    GaussianSplat new_model;
    if (model_path == "test") {
        std::vector<glm::vec3> ws_positions = {
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            // {0.0f, 1.0f, 0.0f},
            // {0.0f, 0.0f, 1.0f}
        };
        std::vector<glm::vec4> rotations = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f, 0.0f},
            // {1.0f, 0.0f, 0.0f, 0.0f},
            // {1.0f, 0.0f, 0.0f, 0.0f}
        };
        std::vector<glm::vec3> scales = {
            {0.03f, 0.03f, 0.03f},
            {0.2f,  0.03f, 0.03f},
            // {0.03f, 0.2f,  0.03f},
            // {0.03f, 0.03f, 0.2f}
        };
        std::vector<glm::vec3> colors = {
            {(0.0f - 0.5f) / 0.28209f, (0.0f - 0.5f) / 0.28209f, (1.0f - 0.5f) / 0.28209f},
            {(1.0f - 0.5f) / 0.28209f, (0.0f - 0.5f) / 0.28209f, (0.0f - 0.5f) / 0.28209f},
            // {(0.0f - 0.5f) / 0.28209f, (1.0f - 0.5f) / 0.28209f, (0.0f - 0.5f) / 0.28209f},
            // {(0.0f - 0.5f) / 0.28209f, (0.0f - 0.5f) / 0.28209f, (1.0f - 0.5f) / 0.28209f}
        };
        std::vector<float> opacities = {1.0f, 1.0f};//, 1.0f, 1.0f};
        new_model.count = 2;
        new_model.ws_positions = ws_positions;
        new_model.scales = scales;
        new_model.rotations = rotations;
        new_model.colors = colors;
        new_model.opacities = opacities;
    } else {
        new_model = gaussian_splat_from_file(model_path);
    }

    std::cout << "Loaded new model:" << std::endl;
    gaussian_splat_print(new_model);
    state->loaded_model = new_model;
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

    ImGui::Begin("Program Settings");
    float fps = ImGui::GetIO().Framerate;
    float delta_time = ImGui::GetIO().DeltaTime * 1000.0f;
    ImGui::Text("FPS: %.1f | Frametime: %.3f ms", fps, delta_time);
    
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
        ImGui::Text("Load time: %f (ms)", state->loaded_model.load_time_in_ms);
        ImGui::Text("Depth sort time: %f (ms)", state->depth_sort_time_in_ms);
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

    ImGui::SliderFloat("Scale multipler", &state->scale_multiplier, 0.1, 3.0);
    ImGui::Checkbox("Depth sort", &state->depth_sort);

    // Draw mode
    const char *draw_modes[] = { "Normal", "Quad", "Albedo", "Depth", "Point Cloud" };
    int current_draw_mode = static_cast<int>(state->draw_mode);
    if (ImGui::Combo("Draw Mode", &current_draw_mode, draw_modes, IM_ARRAYSIZE(draw_modes))) {
        state->draw_mode = static_cast<DrawMode>(current_draw_mode);
    }

    ImGui::Text("Help:");
    const char *help_text =
        "- Use the dropdown menu to select different models.\n"
        "- Depth sorting is super slow, so you may want to use it sparingly :-).\n"
        "- Camera controls:\n"
        "  * Move: WASD, Space (up), Left Shift (down)\n"
        "  * Look: Hold Right Mouse Button and move the mouse\n"
        "\n";

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly;
    ImGui::InputTextMultiline("##help_text", (char*)help_text, strlen(help_text) + 1,
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8), flags);
    ImGui::End();
}

void run_program(GLFWwindow* window)
{
    // Disable vsync
    glfwSwapInterval(0);

    // Enable depth (Z) buffer (accept "closest" fragment)
    // glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);

    // Configure miscellaneous OpenGL settings
    glEnable(GL_CULL_FACE);
    //glDisable(GL_CULL_FACE);

    // Disable built-in dithering
    glDisable(GL_DITHER);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);

    // Set default colour after clearing the colour buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Initialise global program state
    ProgramState state;
    state.all_models = list_ply_and_splat_files("../res/");
    // Test gaussians for debugging
    state.all_models.push_back("test");
    // Set default model
    std::string default_model = "../res/father-day.ply";
    // std::string default_model = "test";
    auto it = std::find(state.all_models.begin(), state.all_models.end(), default_model);

    if (it != state.all_models.end()) {
        load_model(&state, *it);
    } else if (!state.all_models.empty()) {
        load_model(&state, state.all_models.at(0));
    } else {
        std::cerr << "ERROR: No .ply or .splat models found in ../res/" << std::endl;
        exit(1);
    }

    init_game(window, state);

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
	    // Clear colour and depth buffers
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        update_frame(window, &state);
        render_frame(window, &state);

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
