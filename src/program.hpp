#ifndef PROGRAM_HPP
#define PROGRAM_HPP
#pragma once


// System headers
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string>
#include <vector>
#include <utilities/window.hpp>
#include <utilities/plyParser.hpp>


typedef struct {
    std::string current_model;
    std::vector<std::string> all_models;
    GaussianSplat loaded_model;
    // If true then the renderer will start to render the loaded_model and set the change_model 
    // flag back to false
    bool change_model = false;
    bool is_loading_model = false;
    
    float scale_multiplier = 6;
    bool render_as_point_cloud = false;
    bool depth_sort = false;
} ProgramState;

// Main OpenGL program
void run_program(GLFWwindow* window);

// Function for handling keypresses
void handleKeyboardInput(GLFWwindow* window);

// Checks for whether an OpenGL error occurred. If one did,
// it prints out the error type and ID
inline void printGLError() {
    int errorID = glGetError();

    if(errorID != GL_NO_ERROR) {
        std::string errorString;

        switch(errorID) {
            case GL_INVALID_ENUM:
                errorString = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_OPERATION:
                errorString = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorString = "GL_OUT_OF_MEMORY";
                break;
            case GL_STACK_UNDERFLOW:
                errorString = "GL_STACK_UNDERFLOW";
                break;
            case GL_STACK_OVERFLOW:
                errorString = "GL_STACK_OVERFLOW";
                break;
            default:
                errorString = "[Unknown error ID]";
                break;
        }

        fprintf(stderr, "An OpenGL error occurred (%i): %s.\n",
                errorID, errorString.c_str());
    }
}


#endif
