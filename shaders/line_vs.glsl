#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 mvp;

out vec3 fragment_color;

void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    fragment_color = vec3(255, 0, 0);
}
