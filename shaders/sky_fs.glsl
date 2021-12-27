#version 330 core

in vec2 UV;

out vec4 outColor;

uniform sampler2D myTextureSampler;


void main(){
    vec3 color = vec3(texture2D(myTextureSampler, UV));
    outColor = vec4(0.0, 0.8, 0.8, 1.0);
}
