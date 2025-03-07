#version 430 core
in vec3 frag_color;
//in vec3 frag_scale;
//in float frag_alpha;

out vec4 frag_color_out;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); // Circular alpha mask
    if (dist > 0.5) discard; // Soft edges for the splat
    frag_color_out = vec4(frag_color, 1.0f);
}
