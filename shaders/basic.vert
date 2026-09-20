#version 330 core

// Attribute 0: the test vertex position, measured from the triangle's own centre.
layout (location = 0) in vec3 aPosition;

// Attribute 1: the red, green, and blue colour of this vertex.
layout (location = 1) in vec3 aColor;

// Phase 4: the model matrix. It is a uniform (Phase 3), so all three vertices
// receive the SAME matrix and the triangle moves as one rigid piece.
// It answers one question: where does this object sit, and how is it turned?
// Phase 5 needed NO change to this file: a matrix can hold a rotation as well
// as a move, and this shader multiplies by whatever matrix it is given.
uniform mat4 uModel;

// Sent to the fragment shader. OpenGL smoothly blends this value between vertices.
out vec3 vColor;

void main()
{
    // A position must have four components before OpenGL can use it.
    // w = 1.0 means this value represents a position, and only w = 1 lets the
    // translation part of the matrix move it.
    // The matrix goes on the LEFT of the vector: matrix * vector.
    gl_Position = uModel * vec4(aPosition, 1.0);
    vColor = aColor;
}
