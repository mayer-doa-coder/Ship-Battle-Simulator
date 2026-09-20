#version 330 core

// The interpolated colour received from the vertex shader.
// This value is different for every pixel, because OpenGL blends it
// between the three corners of the triangle.
in vec3 vColor;

// Phase 3: a uniform is the opposite of the value above.
// It is set once from C++ and is the SAME for every pixel in the draw call.
// Here it acts as a colour filter: each channel is multiplied by it.
uniform vec3 uTint;

// The final red, green, blue, and alpha values written to the screen.
out vec4 FragColor;

void main()
{
    // Multiplying means a tint channel of 1.0 leaves that colour alone,
    // below 1.0 dims it, and above 1.0 brightens it.
    // OpenGL clamps the displayed result to 1.0.
    FragColor = vec4(vColor * uTint, 1.0);
}
