#version 430 core

in layout(location = 0) vec2 texture_coords_in;

layout(binding = 0) uniform sampler2D text_sampler;

out vec4 color;

void main()
{
    vec4 texture_color = texture(text_sampler, texture_coords_in);
    color = texture_color; //vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
