#version 430 core
in vec3 frag_color;
in float frag_alpha;
in vec3 conic;
in vec2 coordxy;
flat in int frag_draw_mode;

out vec4 frag_color_out;

void main() {
    float power = -0.5f * (conic.x * coordxy.x * coordxy.x + conic.z * coordxy.y * coordxy.y) - conic.y * coordxy.x * coordxy.y;
    float alpha = min(0.99f, frag_alpha * exp(power));

    // Normal
    if (frag_draw_mode == 0) {
        if(power > 0.0f) discard;
        if(alpha < 1.f / 255.f) discard;
        frag_color_out = vec4(frag_color, alpha);
    } 
    // Quad
    else if (frag_draw_mode == 1) {
        frag_color_out = vec4(frag_color, 1);
    }
    // Albedo 
    else if (frag_draw_mode == 2) {
        //frag_color_out.rgb = frag_color;
        frag_color_out.rgb = frag_color * exp(power);
        frag_color_out.a = alpha > 0.22 ? 1 : 0;
    } 
    // Depth
    else if (frag_draw_mode == 3) {
        frag_color_out = vec4(frag_color, alpha);
    }
}
