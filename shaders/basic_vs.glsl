#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texUV;

out vec3 fragmentColor;

uniform mat4 MVP;

// Output data ; will be interpolated for each fragment.
out vec2 UV;

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
    fragmentColor = vec3(1.0, 1.0, 1.0);

    // UV of the vertex. No special space for this one.
    UV = texUV;
}
