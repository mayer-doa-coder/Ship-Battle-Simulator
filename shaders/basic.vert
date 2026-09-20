#version 330 core

// Attribute 0: the test vertex position, measured from the triangle's own centre.
layout (location = 0) in vec3 aPosition;

// Attribute 1: the red, green, and blue colour of this vertex.
layout (location = 1) in vec3 aColor;

// Phase 4: the model matrix. It is a uniform (Phase 3), so all three vertices
// receive the SAME matrix and the triangle moves as one rigid piece.
// It answers one question: where does this object sit, and how is it turned?
// Phase 5 and 6 needed NO change to this file: a matrix can hold a move, a
// turn, and a resize together, and this shader multiplies by whatever matrix
// it is given.
uniform mat4 uModel;

// Phase 7: two more matrices, applied in the same way.
//   uView       - turns WORLD-space coordinates into CAMERA-space coordinates.
//                 It answers "where is the camera, and which way is it facing?"
//   uProjection - turns camera-space coordinates into clip space, adding the
//                 perspective divide that makes far things look smaller.
// Before this phase, uModel's output went straight to gl_Position, so every
// object lived in the same flat -1..+1 box the screen shows directly. Now a
// vertex passes through three matrices before it reaches the screen.
uniform mat4 uView;
uniform mat4 uProjection;

// Sent to the fragment shader. OpenGL smoothly blends this value between vertices.
out vec3 vColor;

void main()
{
    // A position must have four components before OpenGL can use it.
    // w = 1.0 means this value represents a position, and only w = 1 lets the
    // translation part of a matrix move it.
    //
    // Matrices go on the LEFT of the vector, and are read right to left:
    //   1. uModel      - place the vertex in the WORLD (Phases 4-6);
    //   2. uView        - re-measure that world position from the CAMERA;
    //   3. uProjection  - flatten camera space into clip space, with the
    //                      perspective divide GPU hardware performs
    //                      automatically after this shader runs.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vColor = aColor;
}
