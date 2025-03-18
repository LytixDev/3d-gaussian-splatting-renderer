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
uniform layout(location = 2) mat4 view_matrix;
uniform layout(location = 3) mat4 projection_matrix;
uniform layout(location = 4) vec3 hfov_focal;

// To fragment shader !
out vec2 frag_texture_coord;
out vec3 frag_color;
out float frag_alpha;
out vec3 conic;
out vec2 coordxy;


mat3 computeCov3D(vec4 rots, vec3 scales) {
  float scaleMod = 1.0f;

  vec3 firstRow = vec3(
    1.f - 2.f * (rots.z * rots.z + rots.w * rots.w),
    2.f * (rots.y * rots.z - rots.x * rots.w),      
    2.f * (rots.y * rots.w + rots.x * rots.z)       
  );

  vec3 secondRow = vec3(
    2.f * (rots.y * rots.z + rots.x * rots.w),       
    1.f - 2.f * (rots.y * rots.y + rots.w * rots.w), 
    2.f * (rots.z * rots.w - rots.x * rots.y)        
  );

  vec3 thirdRow = vec3(
    2.f * (rots.y * rots.w - rots.x * rots.z),       
    2.f * (rots.z * rots.w + rots.x * rots.y),     
    1.f - 2.f * (rots.y * rots.y + rots.z * rots.z) 
  );


  mat3 scaleMatrix = mat3(
    scaleMod * scales.x, 0, 0, 
    0, scaleMod * scales.y, 0,
    0, 0, scaleMod * scales.z
  );

  mat3 rotMatrix = mat3(
    firstRow,
    secondRow,
    thirdRow
  );

  mat3 mMatrix = scaleMatrix * rotMatrix;

  mat3 sigma = transpose(mMatrix) * mMatrix;
  return sigma;
}

void main() {
    mat3 cov3d = computeCov3D(rotation, scale);

    // position camera space
    vec4 cam = view_matrix * vec4(position_ws, 1.0);
    vec4 pos2d = projection_matrix * cam;
    pos2d.xyz = pos2d.xyz / pos2d.w;
    pos2d.w = 1.f;
    vec2 wh = 2 * hfov_focal.xy * hfov_focal.z;

    float limx = 1.3 * hfov_focal.x;
    float limy = 1.3 * hfov_focal.y;

    float txtz = cam.x / cam.z;
    float tytz = cam.y / cam.z;

    // Clamped versions of txtz and tytz 
    float tx = min(limx, max(-limx, txtz)) * cam.z;
    float ty = min(limy, max(-limy, tytz)) * cam.z; 


    if (any(greaterThan(abs(pos2d.xyz), vec3(1.3)))) {
        gl_Position = vec4(-100, -100, -100, 1);
        return;	
    }

    mat3 J = mat3(
      hfov_focal.z / cam.z, 0., -(hfov_focal.z * tx) / (cam.z * cam.z),
      0., hfov_focal.z / cam.z, -(hfov_focal.z * ty) / (cam.z * cam.z),
      0., 0., 0.
    );
                
                
    mat3 T = transpose(mat3(view_matrix)) * J;

    mat3 cov2d = transpose(T) * transpose(cov3d) * T;

    cov2d[0][0] += 0.3f;
    cov2d[1][1] += 0.3f; 

    float det = cov2d[0][0] * cov2d[1][1] - cov2d[0][1] * cov2d[1][0];

    if (det == 0.0f)
        gl_Position = vec4(0.f, 0.f, 0.f, 0.f);

    float det_inv = 1.f / det;
    conic = vec3(cov2d[1][1] * det_inv, -cov2d[0][1] * det_inv, cov2d[0][0] * det_inv);


    // Project quad into screen space
    vec2 quadwh_scr = vec2(3.f * sqrt(cov2d[0][0]), 3.f * sqrt(cov2d[1][1]));

    // Convert screenspace quad to NDC
    vec2 quadwh_ndc = quadwh_scr / wh * 2;


    // Update gaussian's position w.r.t the quad in NDC
    pos2d.xy = pos2d.xy + quadVertex * quadwh_ndc;

    // Calculate where this quad lies in pixel coordinates 
    coordxy = quadVertex * quadwh_scr;

    // Set position
    gl_Position = pos2d;

    // Send values to fragment shader 
    frag_color = color;
    frag_alpha = alpha;




    // vec2 scaled_quad_vertex = quadVertex * scale.xy * scale_multipler;
    // // vec2 rotated_quad_vertex = rotMat * scaled_quad_vertex;

    // // vec2 rotated_quad_vertex = rotMat * quadVertex;
    // // vec2 transformed_quad_vertex = rotated_quad_vertex * scale.xy * scale_multipler;

    // // Transform the Gaussian center position to clip space
    // vec4 clip_center = VP * vec4(position_ws, 1.0);
    // 
    // float perspectiveScale = 1.0 / clip_center.w;
    // 
    // // Offset the vertex position in screen space
    // vec4 clip_pos = clip_center;
    // clip_pos.xy += scaled_quad_vertex;// * perspectiveScale;
    // //clip_pos.xy += rotated_quad_vertex;// * perspectiveScale;
    // //clip_pos.xy += transformed_quad_vertex;
    // 
    // gl_Position = clip_pos;
    // 
    // frag_texture_coord = texture_coord;
    // frag_color = color;
    // frag_alpha = alpha;
}
