#pragma once

// System Headers
#include <glad/glad.h>

// Standard headers
#include <string>

// Constants
const int         windowWidth     = 1366;
const int         windowHeight    = 768;
const std::string windowTitle     = "Glowbox";
const GLint       windowResizable = GL_FALSE;
const int         windowSamples   = 4;

struct CommandLineOptions {
    bool enableMusic;
    bool enableAutoplay;
};
