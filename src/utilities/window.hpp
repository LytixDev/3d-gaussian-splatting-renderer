#pragma once

// System Headers
#include <glad/glad.h>

// Standard headers
#include <string>

// Constants
const int windowWidthDefault      = 1920 * 1.5;
const int windowHeightDefault     = 1080 * 1.5;
const std::string windowTitle     = "Glowbox";
const GLint       windowResizable = GL_FALSE;
const int         windowSamples   = 4;

struct CommandLineOptions {
    bool enableMusic;
    bool enableAutoplay;
};
