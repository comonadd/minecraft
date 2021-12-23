#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in float diffuse;
in float fragment_ao;
in float fragment_light;

// Ouput data
out vec4 outColor;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

const float daylight = 1.0;

void main(){
    // Take color from the atlas texture
    vec3 color = vec3(texture2D(myTextureSampler, UV));


    float df = 0.2 * diffuse;
    float ao = fragment_ao;
    ao = min(1.0, ao + fragment_light);
    df = min(1.0, df + fragment_light);

    float value = min(1.0, daylight + fragment_light);
    vec3 light_color = vec3(value * 0.3 + 0.2);
    vec3 ambient = vec3(value * 0.3 + 0.2);
    vec3 light = ambient + light_color * df;
    color = clamp(color * light * ao, vec3(0.0), vec3(1.0));
    // vec3 sky_color = vec3(texture2D(sky_sampler, vec2(timer, fog_height)));
    // color = mix(color, sky_color, fog_factor);

    // Output
    outColor = vec4(color, 1.0);
}
