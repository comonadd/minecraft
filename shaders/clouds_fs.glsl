#version 330 core

in vec2 UV;

out vec4 outColor;
in vec3 fragment_color;

void main() {
    vec3 color = fragment_color;
    outColor = vec4(color, 1.0);
}
