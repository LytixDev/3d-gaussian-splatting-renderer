#version 430 core
in vec3 fragColor;
out vec4 FragColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); // Circular alpha mask
    if (dist > 0.5) discard; // Soft edges for the splat
    FragColor = vec4(fragColor, 1.0); // Use the processed color
}
