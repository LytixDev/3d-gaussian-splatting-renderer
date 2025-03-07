#version 430 core
in vec2 frag_texture_coord;
in vec3 frag_color;
in float frag_alpha;

out vec4 frag_color_out;

void main() {
    // Convert texture coordinates to center-relative coordinates (-1 to 1)
    vec2 centered = 2.0 * (frag_texture_coord - 0.5);
    // Distance from center
    float r2 = dot(centered, centered);
    float gaussian_fallow = exp(-2.0 * r2);
    float final_alpha = frag_alpha * gaussian_fallow;

    if (final_alpha < 0.1) {
        discard;
    }
    
    frag_color_out = vec4(frag_color, final_alpha);
}
