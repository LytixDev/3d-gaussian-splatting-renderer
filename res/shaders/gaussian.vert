#version 430 core
layout (location = 0) in vec3 aPosition; // Gaussian position in world space
layout (location = 1) in vec3 aColor;    // Gaussian color

uniform layout(location = 3) mat4 VP;

// Pass color to fragment shader
out vec3 fragColor;

// Function to process colors that may be outside 0-1 range
vec3 processColor(vec3 inputColor) {
    // Tone mapping approach
    return inputColor / (vec3(1.0) + abs(inputColor));
}

void main() {
    gl_Position = VP * vec4(aPosition, 1.0); // Transform to clip space
    gl_PointSize = 10.0; // Set a fixed size for all Gaussians
    
    // Process the unusual color format
    fragColor = processColor(aColor);
}
