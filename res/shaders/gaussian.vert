#version 430 core
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

// To fragment shader
out vec3 frag_color;
out float frag_alpha;
out vec3 conic;
out vec2 coordxy;
flat out int frag_draw_mode;


mat3 compute_cov3d(vec4 R, vec3 s) {
    //mat3 scale = mat3(
    //    scale_multipler * s.x, 0.0, 0.0,
    //    0.0, scale_multipler * s.y, 0.0,
    //    0.0, 0.0, scale_multipler * s.z
    //);
    mat3 scale = mat3(
        s.x, 0.0, 0.0,
        0.0, s.y, 0.0,
        0.0, 0.0, s.z
    );


	float r = R.x;
	float x = R.y;
	float y = R.z;
	float z = R.w;

    mat3 rotation = mat3(
		1.f - 2.f * (y * y + z * z), 2.f * (x * y - r * z), 2.f * (x * z + r * y),
		2.f * (x * y + r * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z - r * x),
		2.f * (x * z - r * y), 2.f * (y * z + r * x), 1.f - 2.f * (x * x + y * y)
	);

    mat3 M = scale * rotation;
    return transpose(M) * M;
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

mat4 defaultViewMatrix() {
    vec3 eye = vec3(0.0, 0.0, 5.0);
    vec3 center = vec3(0.0, 0.0, 0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);

    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    return mat4(
        vec4(s, 0.0),
        vec4(u, 0.0),
        vec4(-f, 0.0),
        vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0)
    );
}

mat4 defaultProjectionMatrix() {
    float fov = radians(45.0);
    float aspect = 16.0 / 9.0;
    float near = 0.1;
    float far = 100.0;

    float tanHalfFov = tan(fov / 2.0);
    float range = near - far;

    return mat4(
        1.0 / (aspect * tanHalfFov), 0.0, 0.0, 0.0,
        0.0, 1.0 / tanHalfFov, 0.0, 0.0,
        0.0, 0.0, (near + far) / range, -1.0,
        0.0, 0.0, (2.0 * near * far) / range, 0.0
    );
}

vec3 defaultHFovFocal() {
    float htany = tan(radians(45.0) / 2.0);
    float htanx = htany / (1080.0 * 1.5) * (1920.0 * 1.5);
    float focal_z = (1080 * 1.5) / (2.0 * htany);
    return vec3(htanx, htany, focal_z);
}

void main() { 
    mat4 view = defaultViewMatrix();
    mat4 proj = defaultProjectionMatrix();
    vec3 hfov = defaultHFovFocal();
    //view = view_matrix;
    proj = projection_matrix;

    // Frustum culling
	//vec4 p_view = view_matrix * vec4(position_ws, 1);
    //if (p_view.z < -200.0f) {
    //    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
    //    return;
    //}

    mat3 cov3d = compute_cov3d(rotation, scale);
    //if (scale_multipler == 1.f) {
    //    cov3d = compute_cov3d(vec4(0.f, 0.f, 0.f, 0.f), scale);
    //}

    // To camera space
    vec4 position_cs = view * vec4(position_ws, 1.0);
    vec4 position_2d = proj * position_cs;
    position_2d.xyz = position_2d.xyz / position_2d.w;
    position_2d.w = 1.0f;
    vec2 wh = 2 * hfov.xy * hfov.z;

    vec3 cov2d = cov2d(position_cs, hfov.z, hfov.z, hfov.x, hfov.y, cov3d, view);
	float det = (cov2d.x * cov2d.z - cov2d.y * cov2d.y);
    // Gaussian is not visible from this view.
	if (det == 0.0f) {
		gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
        return;
    }
    
    float det_inv = 1.0f / det;
	conic = vec3(cov2d.z * det_inv, -cov2d.y * det_inv, cov2d.x * det_inv);

    // Size of quad in screen space. Multiplying by 3 means 99% of the Gaussian is covered by the quad.
    vec2 quad_ss = vec2(3.0f * sqrt(cov2d.x), 3.0f * sqrt(cov2d.z));
    // If quad is huge, something has gone bad
    // if (abs(cov2d.x * cov2d.z) > 100000) {
	// 	gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
    //     return;
    // }

    // Size of quad in ndc
    vec2 quad_ndc = quad_ss / wh * 2;

    position_2d.xy = position_2d.xy + quadVertex * quad_ndc;

    gl_Position = position_2d;

    // Send values to fragment shader 
    frag_color = color;
    frag_alpha = alpha;
    // Pixel coordinates
    coordxy = quadVertex * quad_ss;
    frag_draw_mode = draw_mode;
}
