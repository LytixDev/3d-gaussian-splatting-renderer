#version 430 core
in vec3 frag_color;
out vec4 frag_color_out;

void main() {
    // NOTE: When we create the ellipsis, this can go
    // float dist = length(gl_PointCoord - vec2(0.5)); // Circular alpha mask
    // if (dist > 0.5) discard; // Soft edges for the splat
    frag_color_out = vec4(frag_color, 1.0);
}
