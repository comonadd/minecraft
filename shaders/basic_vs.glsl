#version 150 core

in vec3 position;
in vec3 color;

out vec3 fragmentColor;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
    fragmentColor = color;
}
