#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 fragmentColor;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;

void main(){
    // Output color = color of the texture at the specified UV
    color = texture(myTextureSampler, UV) * vec4(fragmentColor, 1.0);
}
