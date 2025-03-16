#version 430 core

layout (location = 2) in vec3 position_ws;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 scale;
layout (location = 5) in float alpha;

uniform layout(location = 3) mat4 VP;

out vec3 frag_color;

vec3 tone_map(vec3 c) {
    return c / (vec3(1.0) + abs(c));
}

void main() {
    gl_Position = VP * vec4(position_ws, 1.0);
    gl_PointSize = 10.0;
    frag_color = tone_map(color);
}
