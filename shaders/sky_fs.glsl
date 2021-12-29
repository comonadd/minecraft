#version 330 core

in vec2 UV;

out vec4 outColor;
in vec3 fragment_color;

/* uniform sampler2D myTextureSampler; */
/* uniform sampler2D myTextureSampler; */

uniform vec3 sky_color;

void main() {
    // vec3 color = vec3(0, 0, 0);
    // if (fragment_color == vec3(1.0, 0.0, 1.0)) {
    /* vec3 color = vec3(texture2D(myTextureSampler, UV)); */

    vec3 color = sky_color;
    // } else {
    //     color = fragment_color;
    // }
    outColor = vec4(color, 1.0);
}
