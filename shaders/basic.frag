#version 330 core
// ^ src/Lighting.h's shared lighting block is spliced in immediately below this
//   line, by ShaderProgram::loadFromFiles(). It brings in uShadingMode, the
//   material uniforms, the light uniforms, lightContribution() and
//   computeLighting(). Do NOT declare those again here.

// Phase 29: the per-vertex colour still arrives but is no longer read. The material
// decides an object's appearance now.

// Phase 15/26: the interpolated normal, in world space.
//
// INTERPOLATED is the key word for Phase 31. In Phong mode this is what gets blended
// across the triangle, and the lighting is then worked out per pixel from it. In
// Gouraud mode the colour was blended instead and this is unused.
in vec3 vNormal;

// Phase 28: this fragment's world position, for the view direction and for the point
// light's distance.
in vec3 vWorldPos;

// Phase 31: the Gouraud result, already lit in the vertex shader and blended here.
in vec3 vLitColor;

// Phase 15: 0 draws the normal picture, 1 paints normals as colours.
uniform int uDebugNormals;

// The final red, green, blue, and alpha values written to the screen.
out vec4 FragColor;

void main()
{
    // ---- FLAT shading (Phase 31) ------------------------------------------
    //
    // A flat-shaded face uses ONE normal for the whole triangle - the triangle's own
    // geometric normal. The mesh does not store that anywhere, so it has to be
    // recovered here.
    //
    // dFdx and dFdy are screen-space derivatives: they report how a value changes
    // between this pixel and the one next to it, horizontally and vertically. The GPU
    // can do this because it shades pixels in 2x2 blocks. Applied to the world
    // position they give two vectors lying IN the surface - and the cross product of
    // two vectors in a plane is perpendicular to that plane.
    //
    // So this is the triangle's true facet normal, constant across the whole triangle
    // because the surface is planar between its three corners.
    vec3 faceNormal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));

    // The cross product's SIGN depends on screen-space handedness, so it can come out
    // pointing into the surface instead of out of it. Comparing against the
    // interpolated normal - which is known to point outward - and flipping if they
    // disagree fixes that without caring about winding order or which way the
    // derivatives happened to run.
    if (dot(faceNormal, vNormal) < 0.0)
        faceNormal = -faceNormal;

    // Phase 26/27: both shaders normalize, for two different reasons. The vertex
    // shader does it because the normal matrix changes a normal's length. This does it
    // because INTERPOLATING between two unit vectors gives something shorter than 1 -
    // the straight line between two points on a circle passes inside it. On a flat
    // face whose corners share one normal it costs nothing; on the sphere it is the
    // difference between correct and slightly too dark.
    vec3 N = normalize(vNormal);

    if (uDebugNormals == 1) {
        // A normal's components run from -1 to +1, a colour channel from 0 to 1.
        // Halving and shifting maps one onto the other, so a face pointing along +X
        // comes out (1.0, 0.5, 0.5).
        //
        // No lighting and no material here, deliberately: the only job of this view is
        // to be read accurately.
        //
        // In Flat mode it shows the FACET normal, which is itself worth seeing - the
        // sphere stops being a smooth colour sweep and becomes a patchwork.
        vec3 shown = (uShadingMode == 0) ? faceNormal : N;
        FragColor = vec4(shown * 0.5 + 0.5, 1.0);
        return;
    }

    // Phase 31: ONE branch, in ONE program, choosing between three shading models.
    //
    // All three use the SAME computeLighting() from src/Lighting.h - Gouraud simply
    // called it earlier, in the vertex shader. The models differ only in WHAT normal
    // is used and WHERE the function runs:
    //
    //   Flat    - the facet normal, constant per triangle, so every pixel of a
    //             triangle comes out the same: visible faceting, and Mach banding
    //             where neighbouring facets meet (L9 s27).
    //   Gouraud - per VERTEX, then the COLOUR is interpolated. Cheap, but a highlight
    //             falling between vertices is never computed at all (L9 s28).
    //   Phong   - the NORMAL is interpolated, then lit per fragment. Correct, and the
    //             most expensive: the whole model is re-evaluated for every pixel
    //             (L9 s36).
    vec3 lit;
    if (uShadingMode == 0) {
        lit = computeLighting(faceNormal, vWorldPos);
    } else if (uShadingMode == 1) {
        lit = vLitColor;
    } else {
        lit = computeLighting(N, vWorldPos);
    }

    FragColor = vec4(lit, 1.0);
}
