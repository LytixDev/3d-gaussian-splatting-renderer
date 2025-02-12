#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 texture_coords_in;

uniform layout(location = 3) mat4 ortho;  // Orthographic projection

out layout(location = 0) vec2 texture_coords_out;

void main()
{
    texture_coords_out = texture_coords_in;
    gl_Position = ortho * vec4(position, 1.0f);
}
