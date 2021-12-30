#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texUV;

uniform mat4 mvp;

out vec2 UV;
out vec3 fragment_color;

void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    UV = texUV;
    fragment_color = color;
}
