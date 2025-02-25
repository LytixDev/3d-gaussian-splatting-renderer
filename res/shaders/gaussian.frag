#version 430 core
out vec4 FragColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); // Circular alpha mask
    if (dist > 0.5) discard; // Soft edges for the splat

    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red Gaussian
}
