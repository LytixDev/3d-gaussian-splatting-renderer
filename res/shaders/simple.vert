#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 texture_coordinates_in;
in layout(location = 3) vec3 tangent_in;
in layout(location = 4) vec3 bitangent_in;

uniform layout(location = 3) mat4 MVP;
uniform layout(location = 4) mat4 model_matrix;
uniform layout(location = 5) mat3 normal_matrix;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 texture_coordinates_out;
out layout(location = 2) vec3 frag_pos_out;
out layout(location = 3) mat3 TBN_out;

void main()
{
    // To frag shader
    normal_out = normalize(normal_matrix * normal_in);
    texture_coordinates_out = texture_coordinates_in;
    //TBN_out = transpose(mat3(
    //    tangent_in,
    //    bitangent_in,
    //    normal_in)); 
    // Unsure why we need to normalize the tangent and bitangent when I thought we specified them
    // to be normalized in the generateAttribute call.
    TBN_out = mat3(normalize(tangent_in), normalize(bitangent_in), normal_out);

    frag_pos_out = vec3(model_matrix * vec4(position, 1.0f));

    gl_Position = MVP * vec4(position, 1.0f);
}
