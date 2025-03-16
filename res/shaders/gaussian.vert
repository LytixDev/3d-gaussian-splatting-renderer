#version 430 core
// Quad vertex attributes
layout (location = 0) in vec2 quadVertex;      // Local quad coordinates (-0.5 to 0.5)
layout (location = 1) in vec2 texCoord;        // Texture coordinates for the quad

// Per-instance Gaussian attributes
layout (location = 2) in vec3 position_ws;     // World-space position of Gaussian
layout (location = 3) in vec3 color;           // Color of Gaussian
layout (location = 4) in vec3 scale;           // Scale of Gaussian
layout (location = 5) in float alpha;          // Opacity of Gaussian

uniform layout(location = 0) mat4 VP;          // View-projection matrix
uniform float scale_multiplier;

// Output to fragment shader
out vec2 frag_texCoord;
out vec3 frag_color;
out float frag_alpha;

// Tone mapping function (unchanged)
// vec3 tone_map(vec3 c) {
//     return c / (vec3(1.0) + abs(c));
// }

void main() {
    // Scale the quad based on Gaussian scale and perspective division
    vec2 scaledQuadVertex = quadVertex * scale.xy * scale_multiplier;
    
    // Transform the Gaussian center position to clip space
    vec4 centerPositionClip = VP * vec4(position_ws, 1.0);
    
    // Calculate the billboard orientation (screen-aligned quad)
    // This keeps the quad facing the camera
    float aspect = 1.0; // Adjust if needed based on your viewport aspect ratio
    
    // Apply proper scaling for perspective (objects further away appear smaller)
    float perspectiveScale = 1.0 / centerPositionClip.w;
    
    // Offset the vertex position in screen space
    vec4 positionClip = centerPositionClip;
    positionClip.xy += scaledQuadVertex;// * perspectiveScale;
    
    gl_Position = positionClip;
    
    // Pass data to fragment shader
    frag_texCoord = texCoord;
    frag_color = color;//tone_map(color);
    frag_alpha = alpha;
}
