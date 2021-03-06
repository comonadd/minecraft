#version 330 core

in vec2 UV;

out vec4 outColor;
in vec3 fragment_color;

uniform vec3 sky_color;

void main() {
    vec3 color = sky_color;
    outColor = vec4(color, 1.0);
}
