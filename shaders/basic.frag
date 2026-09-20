#version 330 core

// The interpolated colour received from the vertex shader.
in vec3 vColor;

// The final red, green, blue, and alpha values written to the screen.
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
