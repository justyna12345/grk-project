#version 330 core

uniform vec3 particleColor;

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec4 color;

void main(){
    // Output color = color of the texture at the specified UV
    // color = texture( myTextureSampler, UV ) * particlecolor;
    color = vec4(particleColor, 1.0);

}