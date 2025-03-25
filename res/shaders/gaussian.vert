#version 430 core
// This entire file is heavily based on: https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu
//  which is the original CUDA renderer from Kerb et al.

layout (location = 0) in vec2 quadVertex;

// Per-instance attributes
layout (location = 2) in vec3 position_ws;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 scale;
layout (location = 5) in float alpha;
layout (location = 6) in vec4 rotation;

uniform layout(location = 0) mat4 VP;
uniform layout(location = 1) float scale_multipler;
uniform layout(location = 2) mat4 view_matrix;
uniform layout(location = 3) mat4 projection_matrix;
uniform layout(location = 4) vec3 hfov_focal;
uniform layout(location = 5) int draw_mode;
// Draw modes:
//     Normal = 0
//     Quad = 1
//     Albedo = 2
//     Depth = 3
//     Point_Cloud = 4

// To fragment shader
out vec3 frag_color;
out float frag_alpha;
out vec3 conic;
out vec2 coordxy;
flat out int frag_draw_mode;


// TODO: Assuming we don't care about the scale_multipler we can do this ahead of time done 
mat3 compute_cov3d(vec4 R, vec3 s) {
    mat3 scale = mat3(
        scale_multipler * s.x, 0.0, 0.0,
        0.0, scale_multipler * s.y, 0.0,
        0.0, 0.0, scale_multipler * s.z
    );
    // mat3 scale = mat3(
    //     s.x, 0.0, 0.0,
    //     0.0, s.y, 0.0,
    //     0.0, 0.0, s.z
    // );

    // Arghhh, this took way to long to figure out.
    // I think it's due to a mismatch between my view matrix and the original papers view matrix
    float r = R.x;  // Real-part
    float x = -R.y; // i
    float y = -R.z; // j
    float z = R.w;  // k

    mat3 rotation = mat3(
        1.f - 2.f * (y * y + z * z), 2.f * (x * y - r * z), 2.f * (x * z + r * y),
        2.f * (x * y + r * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - r * x),
        2.f * (x * z - r * y), 2.f * (y * z + r * x), 1.f - 2.f * (x * x + y * y)
	);

    mat3 M = scale * rotation;
    return transpose(M) * M;
}


// Pretty much copy paste from https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu
vec3 cov2d(vec4 mean, float focal_x, float focal_y, float tan_fovx, float tan_fovy, mat3 cov3D, mat4 viewmatrix)
{
    vec4 t = mean;
    float limx = 1.3f * tan_fovx;
    float limy = 1.3f * tan_fovy;
    float txtz = t.x / t.z;
    float tytz = t.y / t.z;
    t.x = min(limx, max(-limx, txtz)) * t.z;
    t.y = min(limy, max(-limy, tytz)) * t.z;

    mat3 J = mat3(
        focal_x / t.z, 0.0f, -(focal_x * t.x) / (t.z * t.z),
        0.0f, focal_y / t.z, -(focal_y * t.y) / (t.z * t.z),
        0, 0, 0
    );
    mat3 W = transpose(mat3(viewmatrix));
    mat3 T = W * J;

    mat3 Vrk = mat3(
        cov3D[0][0], cov3D[0][1], cov3D[0][2],
        cov3D[1][0], cov3D[1][1], cov3D[1][2],
        cov3D[2][0], cov3D[2][1], cov3D[2][2]
    );

    //mat3 cov = transpose(T) * transpose(Vrk) * T;
    mat3 cov = transpose(T) * transpose(cov3D) * T;
	// Apply low-pass filter: every Gaussian should be at least
	// one pixel wide/high. Discard 3rd row and column.
	cov[0][0] += 0.3f;
	cov[1][1] += 0.3f;
    return vec3(cov[0][0], cov[0][1], cov[1][1]);
}

// For some unkniwn reason passing hvof_focal as a uniform doesn't work properly ...
// Even when it has the exact same values ...
vec3 default_hvof_focal() {
    float htany = tan(radians(60.0) / 2.0);
    float htanx = htany / (1080.0) * (1920.0);
    float focal_z = (1080) / (2.0 * htany);
    return vec3(htanx, htany, focal_z);
}

void main() { 
    vec3 hfov = default_hvof_focal();

    // Near culling, made no performance benefit
    // vec4 p_view = view_matrix * vec4(position_ws, 1);
    // if (p_view.z <= 0.2f) {
    //     gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
    //     return;
    // }

    mat3 cov3d = compute_cov3d(rotation, scale);
    // Transform position to camera space
    vec4 position_cs = view_matrix * vec4(position_ws, 1.0);

    // Compute 2d covariance
    vec3 cov2d = cov2d(position_cs, hfov.z, hfov.z, hfov.x, hfov.y, cov3d, view_matrix);
    float det = (cov2d.x * cov2d.z - cov2d.y * cov2d.y);
    // Gaussian is not visible from this view, so we can early stop. Made no performance benefit.
    // if (det == 0.0f) {
    //     gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
    //     return;
    // }
    // EWA algorithm
    float det_inv = 1.0f / det;
    conic = vec3(cov2d.z * det_inv, -cov2d.y * det_inv, cov2d.x * det_inv);

    // Size of quad (splat footprint) in screen space. Multiplying by 3 means 99% of the Gaussian is covered by the quad.
    vec2 quad_ss = vec2(3.0f * sqrt(cov2d.x), 3.0f * sqrt(cov2d.z));
    // If quad is huge, something has gone bad
    // if (abs(cov2d.x * cov2d.z) > 100000) {
    //      gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
    //      return;
    // }
    vec2 wh = 2 * hfov.xy * hfov.z;
    // Size of quad in NDC
    vec2 quad_ndc = quad_ss / wh * 2;

    // Position transformed from camera space into clip space using the projection matrix
    vec4 position_2d = projection_matrix * position_cs;
    // Do the perspective division. Now in NDC.
    position_2d.xyz = position_2d.xyz / position_2d.w;
    position_2d.w = 1.0f;
    position_2d.xy = position_2d.xy + quadVertex * quad_ndc;
    gl_Position = position_2d;

    // Send values to fragment shader 

    if (draw_mode == 3) {
        float depth_reciprocal = 1 / -position_cs.z;
        frag_color = vec3(depth_reciprocal, depth_reciprocal, depth_reciprocal);
    } else {
        frag_color = color;
    }
    frag_alpha = alpha;

    // Pixel coordinates
    coordxy = quadVertex * quad_ss;

    frag_draw_mode = draw_mode;
}
