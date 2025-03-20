// Local headers
#include "utilities/window.hpp"
#include "program.hpp"

// System headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Standard headers
#include <cstdlib>
#include <arrrgh.hpp>


// A callback which allows GLFW to report errors whenever they occur
static void glfwErrorCallback(int error, const char *description)
{
    fprintf(stderr, "GLFW returned an error:\n\t%s (%i)\n", description, error);
}


GLFWwindow* initialise()
{
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Could not start GLFW\n");
        exit(EXIT_FAILURE);
    }

    // Set core window options (adjust version numbers if needed)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Enable the GLFW runtime error callback function defined previously.
    glfwSetErrorCallback(glfwErrorCallback);
    
    // Set additional window options
    glfwWindowHint(GLFW_RESIZABLE, windowResizable);
    glfwWindowHint(GLFW_SAMPLES, windowSamples);  // MSAA

    // Create window using GLFW
    GLFWwindow* window = glfwCreateWindow(windowWidthDefault, windowHeightDefault, windowTitle.c_str(), nullptr, nullptr);

    // Ensure the window is set up correctly
    if (!window) {
        fprintf(stderr, "Could not open GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Let the window be the current OpenGL context and initialise glad
    glfwMakeContextCurrent(window);
    gladLoadGL();

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

    // Print various OpenGL information to stdout
    printf("%s: %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    printf("GLFW\t %s\n", glfwGetVersionString());
    printf("OpenGL\t %s\n", glGetString(GL_VERSION));
    printf("GLSL\t %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return window;
}


int main()
{
    // Initialise window using GLFW
    GLFWwindow* window = initialise();
    // Run an OpenGL application using this window
    run_program(window);
    // Terminate GLFW (no need to call glfwDestroyWindow)
    glfwTerminate();
	// Shutdown all ImGUI stuff
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

    return EXIT_SUCCESS;
}
