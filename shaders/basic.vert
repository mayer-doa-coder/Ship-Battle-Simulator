#version 330 core
// ^ src/Lighting.h's shared lighting block is spliced in immediately below this
//   line, by ShaderProgram::loadFromFiles(). It brings in uShadingMode, the
//   material uniforms, the light uniforms, lightContribution() and
//   computeLighting(). Do NOT declare those again here.

// Phase 2: the three per-vertex inputs, matching the layout src/Mesh.h records.
// A vertex carries one position, one normal and one colour (Phase 14).
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

// Phases 4-7: the three matrices a vertex passes through on its way to the screen.
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// Phase 26: THE NORMAL MATRIX, (M^-1)^T, reduced to its top-left 3x3 and computed on
// the CPU once per object per frame.
//
// A position and a direction are different kinds of thing, and an uneven scale
// treats them as opposites. Squash a surface downward and its normals must become
// MORE vertical, not less - and (M^-1)^T is exactly the matrix that does that.
//
// For a rotation alone, or a uniform scale, it points the same way as uModel and
// changes nothing visible, which is why the bug hides until something is stretched
// unevenly. It is a mat3 because a normal is a direction: w = 0, so the translation
// column could never affect it anyway (Phase 4).
uniform mat3 uNormalMatrix;

// Phase 26: 1 uses the normal matrix, 0 deliberately uses mat3(uModel) - the naive,
// wrong way - so the two can be compared live with the 'M' key.
uniform int uUseNormalMatrix;

// Phase 29: still passed on, but nothing reads it. The per-vertex colour was the
// stand-in for a material from Phase 2 to Phase 28; src/Material.h has replaced it.
// The attribute stays because removing it would mean rebuilding every generator and
// would invalidate the byte-identical frame evidence from Phases 14 and 17.
out vec3 vColor;

// Phase 15/26: the normal, in WORLD space after the normal matrix. The fragment
// shader needs it there, because the light directions are in world space too.
out vec3 vNormal;

// Phase 28: this vertex's position in world space, for the view direction.
out vec3 vWorldPos;

// Phase 31: the GOURAUD result.
//
// When uShadingMode is Gouraud this holds the finished colour of this vertex, and
// the rasteriser blends those colours across the triangle. In the other two modes
// nothing is computed here and the fragment shader does the work instead.
out vec3 vLitColor;

// Environment build: the clip plane of the planar reflection pass (dot(world position, plane) < 0 is cut away; the pass switches GL_CLIP_DISTANCE0 on),
// and how much white water the sea has at this vertex. Both are inert outside those passes.
uniform vec4 uClipPlane;
out float vSeaFoam;

// Environment build: THE HOLD. The hull is a closed shell whose normals point out of the ship; seen from INSIDE (the camera is below decks) it is drawn with the faces
// culled the other way round and its normals turned to face the camera. 0 for everything else.
uniform int uFlipNormals;

void main()
{
    // Matrices go on the LEFT and are read right to left (Phase 6):
    //   1. uModel      - place the vertex in the WORLD;
    //   2. uView       - re-measure that position from the CAMERA;
    //   3. uProjection - flatten camera space into clip space.
    // w = 1.0 because this is a POSITION, and only w = 1 lets a matrix's
    // translation part move it.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    vColor = aColor;

    // Phase 28: w = 1.0 again, because this IS a position - the translation must
    // apply. Only uModel, never uView or uProjection: the normal, both light
    // positions and the view position all have to be in the same space, and this
    // project uses world space throughout.
    vWorldPos = vec3(uModel * vec4(aPosition, 1.0));
    vSeaFoam = 0.0;

    // Phase 26: mat3(uModel) is the naive version. It treats a direction exactly
    // like a position, which is only correct when the model matrix has no uneven
    // scale in it.
    mat3 normalTransform = (uUseNormalMatrix != 0) ? uNormalMatrix : mat3(uModel);

    // normalize() is not optional. The normal matrix is built from a matrix
    // containing a scale, so it stretches the vector's LENGTH even while it fixes its
    // DIRECTION - measured, by a factor of 2.4 to 3.1 on the stretched cube.
    vNormal = normalize(normalTransform * aNormal);

    // Environment build: THE SEA. When the sea is being drawn (uSeaMode = 1) every vertex is lifted to the height of the waves at its WORLD x and z -
    // the world's, not the mesh's, so the waves stay put in the world when the sea mesh slides along with the ship - and the normal comes from the
    // waves' slope instead of the mesh's flat +y. Everything lit afterwards (Flat, Gouraud, Phong, the haze, the specular streak) therefore sees the
    // swell. For every other object uSeaMode is 0 and this block does nothing at all, so the rest of the scene and the gallery are untouched.
    if (uSeaMode != 0) {
        float lift = seaWaveHeight(vWorldPos.xz);
        vWorldPos.y += lift;
        gl_Position = uProjection * uView * vec4(vWorldPos, 1.0);
        vNormal = seaWaveNormal(vWorldPos.xz);

        // White water: a vertex near the top of the highest possible crest, broken up by a slow pattern so the foam lies in patches and not in bands.
        float crest = lift / max(uWaveScale * SEA_AMPLITUDE_SUM, 0.0001);
        float patches = 0.65 + 0.35 * sin(vWorldPos.x * 1.7 + vWorldPos.z * 2.3 + uWaveTime * 1.1);
        vSeaFoam = uFoam * smoothstep(0.42, 0.85, crest) * patches;
    }

    if (uFlipNormals != 0)
        vNormal = -vNormal;

    // Planar reflection: the distance of this vertex from the mirror plane (see renderScene). Positive = kept.
    gl_ClipDistance[0] = dot(vec4(vWorldPos, 1.0), uClipPlane);

    // Phase 31: GOURAUD. The lighting is evaluated HERE, once per vertex, and the
    // result is interpolated across the triangle by the rasteriser.
    //
    // Note what gets interpolated: a COLOUR. That is the whole reason Gouraud misses
    // a highlight that falls between two vertices - there is no vertex there to
    // compute it at, so no amount of blending can invent it (L9 s28).
    //
    // computeLighting() comes from the shared block in src/Lighting.h, and the
    // fragment shader calls the SAME function for Phong. That is what makes the two
    // modes genuinely comparable rather than two separate pieces of code that happen
    // to look similar.
    //
    // The branch means the other two modes do not pay for work they will not use.
    // Phase 45: computeLighting() cannot see this stage's attribute, so the vertex colour is
    // handed to it through a global it reads (declared in the shared block). The fragment shader
    // does the same with the interpolated colour. Only a material with uKeFromVertexColor set
    // ever looks at it; for every other draw it is multiplied away or never read.
    gVertexColor = aColor;

    if (uShadingMode == 1) {
        vLitColor = computeLighting(vNormal, vWorldPos);
    } else {
        vLitColor = vec3(0.0);
    }
}
