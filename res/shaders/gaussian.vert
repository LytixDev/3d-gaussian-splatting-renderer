#version 430 core
layout (location = 0) in vec2 quadVertex; // Local quad coordinates (-0.5 to 0.5)
layout (location = 1) in vec2 texture_coord;

// Per-instance attributes
layout (location = 2) in vec3 position_ws;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 scale;
layout (location = 5) in float alpha;

uniform layout(location = 0) mat4 VP;
uniform layout(location = 1) float scale_multipler;

// To fragment shader !
out vec2 frag_texture_coord;
out vec3 frag_color;
out float frag_alpha;

void main() {
    vec2 scaled_quad_vertex = quadVertex * scale.xy * scale_multipler;
    
    // Transform the Gaussian center position to clip space
    vec4 clip_center = VP * vec4(position_ws, 1.0);
    
    // TODO: rotation
    float aspect = 1.0; // Adjust if needed based on your viewport aspect ratio
    float perspectiveScale = 1.0 / clip_center.w;
    
    // Offset the vertex position in screen space
    vec4 clip_pos = clip_center;
    clip_pos.xy += scaled_quad_vertex;// * perspectiveScale;
    
    gl_Position = clip_pos;
    
    frag_texture_coord = texture_coord;
    frag_color = color;
    frag_alpha = alpha;
}
