#version 430 core
in vec2 frag_texCoord;
in vec3 frag_color;
in float frag_alpha;

out vec4 FragColor;

void main() {
    // Convert texture coordinates to center-relative coordinates (-1 to 1)
    vec2 centered = 2.0 * (frag_texCoord - 0.5);
    
    // Calculate radial distance from center (squared)
    float r2 = dot(centered, centered);
    
    // Gaussian falloff function
    // exp(-0.5 * r^2) gives us the classic Gaussian bell curve
    float gaussianFalloff = exp(-2.0 * r2);
    
    // Apply Gaussian falloff to the alpha
    float finalAlpha = frag_alpha * gaussianFalloff;
    
    // Discard very transparent pixels
    if (finalAlpha < 0.01) {
        discard;
    }
    
    // Output final color with Gaussian alpha falloff
    FragColor = vec4(frag_color, finalAlpha);
}
