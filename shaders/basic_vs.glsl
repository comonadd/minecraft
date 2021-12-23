#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texUV;
layout(location = 3) in float ao;
layout(location = 4) in float light;

uniform mat4 MVP;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out float diffuse;
out float fragment_light;
out float fragment_ao;

const vec3 light_direction = normalize(vec3(-1.0, 1.0, -1.0));

void main()
{
    diffuse = max(0.0, dot(normal, light_direction));
    fragment_light = light;
    fragment_ao = ao;
    gl_Position = MVP * vec4(position, 1.0);
    UV = texUV;
}
