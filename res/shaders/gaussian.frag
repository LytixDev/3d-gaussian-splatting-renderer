#version 430 core
in vec3 frag_color;
in vec3 frag_scale;
in float frag_alpha;

out vec4 frag_color_out;

void main() {
    // NOTE: When we create the ellipsis, this can go
    // float dist = length(gl_PointCoord - vec2(0.5)); // Circular alpha mask
    // if (dist > 0.5) discard; // Soft edges for the splat

    vec2 uv = gl_PointCoord * 2.0 - 1.0;

    // Isotropic
    float dist2 = dot(uv * frag_scale.xy, uv * frag_scale.xy);
    float alpha = exp(-0.5 * dist2);


    frag_color_out = vec4(frag_color.rgb, alpha);
    //frag_color_out = vec4(frag_color.rgb, frag_alpha * alpha);
    // frag_color_out = vec4(frag_color, 1.0);
}
