#version 430 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 squareVertices;
layout(location = 1) in vec4 xyzs; // Position of the center of the particule and size of the square
layout(location = 2) in vec4 color; // Position of the center of the particule and size of the square

// Output data ; will be interpolated for each fragment.
out vec2 UV;

uniform mat4 VP; // Model-View-Projection matrix, but without the Model (the position is in BillboardPos; the orientation depends on the camera)

void main()
{
    float particleSize = xyzs.w; // because we encoded it this way.

    // Output position of the vertex
    gl_Position = VP * vec4(vertexPosition * particleSize + xyzs.xyz, 1.0f);

    // UV of the vertex. No special space for this one.
    UV = vertexPosition.xy + vec2(0.5, 0.5);
}