#version 330 core

// Attribute 0: the test vertex position in clip space.
layout (location = 0) in vec3 aPosition;

// Attribute 1: the red, green, and blue colour of this vertex.
layout (location = 1) in vec3 aColor;

// Sent to the fragment shader. OpenGL smoothly blends this value between vertices.
out vec3 vColor;

void main()
{
    // A position must have four components before OpenGL can use it.
    // w = 1.0 means this value represents a position.
    gl_Position = vec4(aPosition, 1.0);
    vColor = aColor;
}
