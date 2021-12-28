#version 330 core

in vec2 UV;

out vec4 outColor;
in vec3 fragment_color;

uniform sampler2D myTextureSampler; 

void main() {
    vec3 color = vec3(texture2D(myTextureSampler, UV));
    outColor = vec4(color, 1.0);
}
