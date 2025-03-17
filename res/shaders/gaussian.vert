#version 430 core
layout (location = 0) in vec2 quadVertex; // Local quad coordinates (-0.5 to 0.5)
layout (location = 1) in vec2 texture_coord;

// Per-instance attributes
layout (location = 2) in vec3 position_ws;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 scale;
layout (location = 5) in float alpha;
layout (location = 4) in vec4 rotation;

uniform layout(location = 0) mat4 VP;
uniform layout(location = 1) float scale_multipler;

// To fragment shader !
out vec2 frag_texture_coord;
out vec3 frag_color;
out float frag_alpha;

void main() {
    // TODO: rotation
    // float x = rotation.x;
    // float y = rotation.y;
    // float z = rotation.z;
    // float w = rotation.w;
    // mat2 rotMat = mat2(
    //     1.0 - 2.0 * (y * y + z * z),  2.0 * (x * y - w * z),
    //     2.0 * (x * y + w * z),  1.0 - 2.0 * (x * x + z * z)
    // );

    vec2 scaled_quad_vertex = quadVertex * scale.xy * scale_multipler;
    // vec2 rotated_quad_vertex = rotMat * scaled_quad_vertex;

    // vec2 rotated_quad_vertex = rotMat * quadVertex;
    // vec2 transformed_quad_vertex = rotated_quad_vertex * scale.xy * scale_multipler;

    // Transform the Gaussian center position to clip space
    vec4 clip_center = VP * vec4(position_ws, 1.0);
    
    float perspectiveScale = 1.0 / clip_center.w;
    
    // Offset the vertex position in screen space
    vec4 clip_pos = clip_center;
    clip_pos.xy += scaled_quad_vertex;// * perspectiveScale;
    //clip_pos.xy += rotated_quad_vertex;// * perspectiveScale;
    //clip_pos.xy += transformed_quad_vertex;
    
    gl_Position = clip_pos;
    
    frag_texture_coord = texture_coord;
    frag_color = color;
    frag_alpha = alpha;
}
