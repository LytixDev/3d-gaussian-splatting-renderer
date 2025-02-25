#version 430 core


// Inputs
in layout(location = 0) vec3 normal_in;
in layout(location = 1) vec2 texture_coordinates_in;
in layout(location = 2) vec3 frag_pos_in;

out vec4 color;

void main()
{
    color = vec4(0.8, 0.2, 0.0, 1.0);
}
