#version 430 core
layout (location = 0) in vec3 position_ws;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 scale;
layout (location = 3) in float alpha;

uniform layout(location = 3) mat4 VP;

out vec3 frag_color;
out vec3 frag_scale;
out float frag_alpha;

// NOTE: The color is stored store as rgb from 0 - 1, but some srgb stuff. Temporary tone mapping.
vec3 tone_map(vec3 c) {
    return c / (vec3(1.0) + abs(c));
}

void main() {
    gl_Position = VP * vec4(position_ws, 1.0);
    gl_PointSize = 1.0;
    // NOTE: This constant should be able to vary
    // gl_PointSize = max(1.0, length(scale.xy) * 100.0 / gl_Position.w); 
    
    frag_color = tone_map(color);
    frag_scale = scale;
    frag_alpha = alpha;
}
