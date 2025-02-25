#version 430 core

layout (location = 0) in vec3 aPosition; // Gaussian position in world space

uniform layout(location = 3) mat4 VP;

void main() {
    gl_Position = VP * vec4(aPosition, 1.0); // Transform to clip space
    gl_PointSize = 10.0; // Set a fixed size for splat (ignoring scale)
}
