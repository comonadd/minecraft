#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texUV;
layout(location = 3) in float ao;
layout(location = 4) in float light;

uniform mat4 MVP;
uniform vec3 light_pos;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out float fragment_light;
out float fragment_ao;
out vec3 fragment_normal;
out vec3 FragPos;
out vec3 fragment_light_pos;

void main()
{
    fragment_light = light;
    fragment_ao = ao;
    fragment_normal = normal;
    gl_Position = MVP * vec4(position, 1.0);
    FragPos = position;
    UV = texUV;
    fragment_light_pos = light_pos;
}
