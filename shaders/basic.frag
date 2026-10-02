#version 330 core

// The interpolated colour received from the vertex shader.
// This value is different for every pixel, because OpenGL blends it
// between the three corners of the triangle.
in vec3 vColor;

// Phase 15: the interpolated normal, in the mesh's own space.
in vec3 vNormal;

// Phase 3: a uniform is the opposite of the value above.
// It is set once from C++ and is the SAME for every pixel in the draw call.
// Here it acts as a colour filter: each channel is multiplied by it.
uniform vec3 uTint;

// Phase 15: 0 draws the normal picture, 1 draws normals as colours. It is an
// int rather than a bool because GLSL 330 has no bool uniform setter in the
// C++ code - this is the first real use of Shader.h's setInt(), which has been
// waiting unused since Phase 3.
uniform int uDebugNormals;

// The final red, green, blue, and alpha values written to the screen.
out vec4 FragColor;

void main()
{
    if (uDebugNormals == 1) {
        // A normal's components run from -1 to +1, but a colour channel runs
        // from 0 to 1. Halving and shifting maps one range onto the other:
        //   -1  ->  0.0   (no light in that channel)
        //    0  ->  0.5   (half)
        //   +1  ->  1.0   (full)
        // So a face pointing along +X comes out (1.0, 0.5, 0.5), a pale pink,
        // and one pointing along -X comes out (0.0, 0.5, 0.5), a dark teal.
        //
        // uTint is deliberately NOT applied here. The tint's job is to filter
        // an object's colour, and multiplying it into a normal would make the
        // colours unreadable - which would defeat the whole point of a view
        // whose only job is to be read accurately.
        //
        // vNormal is used as it arrives, with no normalize(). Every surface in
        // the project so far is flat, and interpolating between corners that
        // all share one normal gives that same normal back exactly. Phases 20
        // to 22 add curved surfaces, whose corners have DIFFERENT normals;
        // interpolating between those shortens the result, and normalize()
        // belongs here from that point on.
        FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
        return;
    }

    // Multiplying means a tint channel of 1.0 leaves that colour alone,
    // below 1.0 dims it, and above 1.0 brightens it.
    // OpenGL clamps the displayed result to 1.0.
    FragColor = vec4(vColor * uTint, 1.0);
}
