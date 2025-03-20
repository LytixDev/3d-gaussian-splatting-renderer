#version 430 core
in vec3 frag_color;
in float frag_alpha;
in vec3 conic;
in vec2 coordxy;

out vec4 frag_color_out;

void main() {
    float power = -0.5f * (conic.x * coordxy.x * coordxy.x + conic.z * coordxy.y * coordxy.y) - conic.y * coordxy.x * coordxy.y;
    if(power > 0.0f) discard;
    float alpha = min(0.99f, frag_alpha * exp(power));
    if(alpha < 1.f / 255.f) discard;
    frag_color_out = vec4(frag_color, alpha);
}
