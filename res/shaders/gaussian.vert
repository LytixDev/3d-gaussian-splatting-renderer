#version 430 core
layout (location = 0) in vec2 quadVertex;

// Per-instance attributes
layout (location = 2) in vec3 position_ws;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 scale;
layout (location = 5) in float alpha;
layout (location = 4) in vec4 rotation;

uniform layout(location = 0) mat4 VP;
uniform layout(location = 1) float scale_multipler;
uniform layout(location = 2) mat4 view_matrix;
uniform layout(location = 3) mat4 projection_matrix;
uniform layout(location = 4) vec3 hfov_focal;

// To fragment shader
out vec3 frag_color;
out float frag_alpha;
out vec3 conic;
out vec2 coordxy;

mat3 cov3d(vec4 r, vec3 s) {
    // NOTE: Could these be constructed ahead of time?
    // Rotation matrix from quaternion
    mat3 rotation_matrix = mat3(
        1.0 - 2.0 * (r.z * r.z + r.w * r.w),
        2.0 * (r.y * r.z + r.x * r.w),
        2.0 * (r.y * r.w - r.x * r.z),
        
        2.0 * (r.y * r.z - r.x * r.w),
        1.0 - 2.0 * (r.y * r.y + r.w * r.w),
        2.0 * (r.z * r.w + r.x * r.y),
        
        2.0 * (r.y * r.w + r.x * r.z),
        2.0 * (r.z * r.w - r.x * r.y),
        1.0 - 2.0 * (r.y * r.y + r.z * r.z)
    );
    
    mat3 scale_matrix = mat3(
        scale_multipler * s.x, 0.0, 0.0,
        0.0, scale_multipler * s.y, 0.0,
        0.0, 0.0, scale_multipler * s.z
    );
    
    mat3 transformation = scale_matrix * rotation_matrix;
    
    // Final 3d covariance matrix
    return transpose(transformation) * transformation;
}

// Based on: https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/main/cuda_rasterizer/forward.cu
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


void main() {
    mat3 cov3d = cov3d(rotation, scale);

    // To camera space
    vec4 position_cs = view_matrix * vec4(position_ws, 1.0);
    vec4 position_2d = projection_matrix * position_cs;
    position_2d.xyz = position_2d.xyz / position_2d.w;
    position_2d.w = 1.f;
    vec2 wh = 2 * hfov_focal.xy * hfov_focal.z;

    // Near-plane 0.1f, far-plane 200f
    // TODO: Reject gaussians with position_ws close to the near or far planes
    // float near_threshold = 0.15f; // Slightly beyond near plane
    // float far_threshold = 195.5f;  // Slightly before far plane
    // 
    // if (-position_cs.z < near_threshold || -position_cs.z > far_threshold) {
    //     gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
    //     return;
    // }
    // if (all(lessThan(abs(position_2d.xyz), vec3(0.1)))) {
    //     gl_Position = vec4(0, 0, 0, 0);
    //     return;	
    // }
    if (all(greaterThan(abs(position_2d.xyz), vec3(1.3)))) {
		gl_Position = vec4(0, 0, 0, 0);
        return;	
    }


    vec3 cov2d = cov2d(position_cs, hfov_focal.z, hfov_focal.z, hfov_focal.x, hfov_focal.y, cov3d, view_matrix);
	float det = (cov2d.x * cov2d.z - cov2d.y * cov2d.y);
    // Gaussian is not visible from this view.
	if (det == 0.0f) {
		gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
        return;
    }
    
    float det_inv = 1.f / det;
	conic = vec3(cov2d.z * det_inv, -cov2d.y * det_inv, cov2d.x * det_inv);
    
    // Size of quad in screen space. Multiplying by 3 means 99% of the Gaussian is covered by the quad.
    vec2 quad_ss = vec2(3.0f * sqrt(cov2d.x), 3.0f * sqrt(cov2d.z));
    // Size of quad in ndc
    vec2 quad_ndc = quad_ss / wh * 2;

    position_2d.xy = position_2d.xy + quadVertex * quad_ndc;

    gl_Position = position_2d;

    // Send values to fragment shader 
    frag_color = color;
    frag_alpha = alpha;
    // Pixel coordinates
    coordxy = quadVertex * quad_ss;
}
