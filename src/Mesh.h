#pragma once

// Phase 14: one vertex layout, and one Mesh type that owns its own GPU
// buffers and draws itself.
//
// Phases 2, 9, and 10 each built the same OpenGL objects by hand - a VAO, a
// VBO, and (from Phase 9) an EBO - inside three near-identical
// create/destroy function pairs living in main.cpp. Nothing was wrong with
// any of them; there were simply three copies of one idea. This file
// replaces all three with a single type.
//
// Its job is to:
//   1. describe one vertex the same way for every object in the project;
//   2. own a VAO, a VBO, and an EBO;
//   3. draw itself with one call;
//   4. free every one of those GPU objects when asked.

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Hull.h"
#include "Planks.h"

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <map>
#include <utility>
#include <vector>

// One vertex, used by every mesh in the project from here on.
//
// Earlier phases wrote a vertex as six loose floats in a row and told OpenGL
// how to divide them up with hand-counted offsets. A struct says the same
// thing once, in a way the compiler checks: three named fields, each a
// glm::vec3, so a vertex is 9 floats and 36 bytes.
struct Vertex {
    // Where this corner sits, measured from the mesh's OWN origin - not from
    // the world origin. The model matrix decides where the mesh ends up.
    glm::vec3 position;

    // Which way the surface faces at this corner. Nothing reads this yet:
    // Phase 15's debug view is the first to use it, and the lighting phases
    // are what it really exists for. It is uploaded from this phase onward so
    // that every generator written from now on has to get it right at the
    // moment the geometry is built, rather than having normals bolted on
    // later, when a wrong one is far harder to notice.
    glm::vec3 normal;

    // This corner's own colour, exactly as Phases 2-10 used it. Materials
    // replace this in Phase 29; until then it is still how every object on
    // screen gets its colour.
    glm::vec3 color;
};

// A mesh: some vertices, some indices, and the three OpenGL objects that hold
// them on the GPU.
//
// This is shaped deliberately like ShaderProgram in src/Shader.h - the same
// constructor, destructor, deleted copy, valid(), and destroy() - because it
// has the same responsibility: one C++ object owning named OpenGL objects
// that must be deleted exactly once, while a context still exists.
class Mesh {
public:
    Mesh() = default;

    // The safety net, not the normal path. main() calls destroy() explicitly
    // while the OpenGL context is still alive; by the time this runs,
    // destroy() has already zeroed every handle, so it does nothing. Without
    // that explicit call, a Mesh declared in main() would be destroyed AFTER
    // glfwTerminate(), and deleting a GPU object with no context is not
    // valid.
    ~Mesh()
    {
        destroy();
    }

    // One Mesh owns one VAO, one VBO, and one EBO. Copying it would give two
    // objects the same handles, and the second destructor would delete GPU
    // objects that were already deleted, so copying is disabled - exactly the
    // reason ShaderProgram disables it.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Sends vertices and indices to the GPU and records the vertex layout.
    // 'name' appears in any message this function prints, so a failure says
    // WHICH mesh failed instead of just that one did.
    bool upload(const char* name,
                const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices)
    {
        // Uploading into a Mesh that already holds buffers would leak the old
        // ones. Freeing first also makes this safe to call twice, which
        // Phase 25's rebuildMeshes() will rely on.
        destroy();

        if (vertices.empty() || indices.empty()) {
            std::fprintf(stderr,
                         "[mesh] '%s' has no vertices or no indices\n",
                         name);
            return false;
        }

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        // The vertex data. vertices.data() is the address of the first
        // Vertex, and the structs sit end to end in memory, so the whole
        // array uploads in one call - exactly as the float arrays did.
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            GL_STATIC_DRAW);

        // The index data, the same GL_ELEMENT_ARRAY_BUFFER Phase 9
        // introduced.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
            GL_STATIC_DRAW);

        // The layout, described ONCE here instead of once per object.
        // sizeof(Vertex) is the stride, and offsetof asks the compiler where
        // each field starts, so nothing has to be counted by hand any more.
        // Miscounting those offsets was a real hazard in the old code; the
        // compiler cannot get it wrong.
        const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<const void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<const void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2, 3, GL_FLOAT, GL_FALSE, stride,
            reinterpret_cast<const void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(2);

        // The same rule Phases 9 and 10 followed: unbind the VBO, never the
        // EBO, and unbind the VAO last. A VAO remembers which
        // GL_ELEMENT_ARRAY_BUFFER was bound while it was bound, and unbinding
        // the EBO first would erase that memory.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_vertexCount = static_cast<GLsizei>(vertices.size());
        m_indexCount = static_cast<GLsizei>(indices.size());

        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::fprintf(stderr,
                         "[mesh] '%s' OpenGL setup error: 0x%04X\n",
                         name,
                         error);
            destroy();
            return false;
        }

        std::printf("[mesh] %-10s vertices=%d indices=%d triangles=%d\n",
                    name,
                    m_vertexCount,
                    m_indexCount,
                    triangleCount());
        return true;
    }

    // Draws the whole mesh. The VAO already remembers the vertex layout and
    // the index buffer, so binding it is all the setup a draw needs - which
    // is why the caller now only has to say "draw yourself" and set the
    // uniforms it cares about.
    void draw() const
    {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    bool valid() const
    {
        return m_vao != 0 && m_vbo != 0 && m_ebo != 0 && m_indexCount > 0;
    }

    GLsizei vertexCount() const
    {
        return m_vertexCount;
    }

    GLsizei indexCount() const
    {
        return m_indexCount;
    }

    // Every mesh in this project is built from triangles, so three indices
    // always describe one triangle.
    GLsizei triangleCount() const
    {
        return m_indexCount / 3;
    }

    // Deletes all three GPU objects and forgets their handles. Safe to call
    // twice: after the first call every handle is 0, and the `if` guards skip
    // the delete.
    void destroy()
    {
        if (m_ebo != 0)
            glDeleteBuffers(1, &m_ebo);

        if (m_vbo != 0)
            glDeleteBuffers(1, &m_vbo);

        if (m_vao != 0)
            glDeleteVertexArrays(1, &m_vao);

        m_ebo = 0;
        m_vbo = 0;
        m_vao = 0;
        m_vertexCount = 0;
        m_indexCount = 0;
    }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_vertexCount = 0;
    GLsizei m_indexCount = 0;
};

// ---- Generators ---------------------------------------------------------
//
// A generator writes out a shape's vertices and indices with a loop instead
// of by hand. makeCube is the first; makeQuad is the second (Phase 17);
// makeGrid is the third and the first PARAMETERISED one (Phase 18);
// makeCylinder is the fourth and the first CURVED one (Phase 20, side wall;
// Phase 21 closed it with end caps); makeSphere is the fifth and the first
// TWO-parameter one (Phase 22). Those five are every shape the project needs.
//
// Phase 23 adds no sixth shape. It adds computeSmoothNormals() - a second WAY of
// deciding a normal - and one more cube to demonstrate it on.
//
// Phase 16 - THE UNIT-MESH RULE. Every generator from here on produces a
// shape of size 1, centred on its own origin, and takes NO size parameter at
// all. An object's real size is decided by a glm::scale in its model matrix,
// at the moment it is drawn.
//
// The reason is not tidiness, it is reuse. A mesh is a block of memory on the
// graphics card; a matrix is sixteen numbers sent with a draw call. If size
// lived in the vertex data, every differently sized object would need its own
// upload. Because size lives in the matrix instead, ONE cube on the GPU can
// be drawn at any size, any number of times, in the same frame.
//
// Phase 14 still passed a halfSize into makeCube. Phase 16 takes it away,
// which is the whole point of this phase.

// How far a unit mesh reaches from its own origin along each axis. 0.5 each
// way makes the shape exactly 1 unit across, so a scale of 3 in the model
// matrix means "3 units across" with no arithmetic in between.
constexpr float UNIT_HALF_EXTENT = 0.5f;

// Builds the unit cube's 24 vertices and 36 indices on the CPU, without
// touching OpenGL at all. Keeping this separate from the upload means the
// numbers can be checked, printed, or modified before they ever reach the GPU
// - which is exactly what Phase 23's computeSmoothNormals() will need to do.
//
// There is no size parameter: the cube is always 1 x 1 x 1. 'faceColors'
// holds one colour per face, in the same order the faces are listed below.
inline void buildCubeGeometry(const glm::vec3 faceColors[6],
                              std::vector<Vertex>& vertices,
                              std::vector<unsigned int>& indices)
{
    // Each face is described by three directions rather than four typed-out
    // corners:
    //   normal - which way the face looks, straight out of the cube;
    //   right  - one step across the face;
    //   up     - one step up the face.
    //
    // These are chosen so that `right` crossed with `up` gives `normal`,
    // which is what makes the corner loop below come out counter-clockwise
    // AS SEEN FROM OUTSIDE - the winding GL_CULL_FACE needs to keep the face
    // (Phases 9 and 11). Getting this right once, here, replaces checking it
    // six times by hand.
    struct Face {
        glm::vec3 normal;
        glm::vec3 right;
        glm::vec3 up;
    };

    const Face faces[6] = {
        { {  0.0f,  0.0f,  1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },  // +Z front
        { {  0.0f,  0.0f, -1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f,  0.0f } },  // -Z back
        { {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f,  0.0f } },  // +X right
        { { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, 1.0f,  0.0f } },  // -X left
        { {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f, -1.0f } },  // +Y top
        { {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f,  1.0f } },  // -Y bottom
    };

    vertices.clear();
    indices.clear();
    vertices.reserve(24);
    indices.reserve(36);

    for (int f = 0; f < 6; ++f) {
        const Face& face = faces[f];

        // The four corners, walked counter-clockwise from the bottom-left.
        // Starting at the face's centre (normal * UNIT_HALF_EXTENT) and
        // stepping half a face width along `right` and `up` lands exactly on a
        // corner, because the three directions are perpendicular and unit
        // length.
        const glm::vec3 centre = face.normal * UNIT_HALF_EXTENT;
        const glm::vec3 across = face.right * UNIT_HALF_EXTENT;
        const glm::vec3 upward = face.up * UNIT_HALF_EXTENT;

        const glm::vec3 corners[4] = {
            centre - across - upward,   // bottom-left
            centre + across - upward,   // bottom-right
            centre + across + upward,   // top-right
            centre - across + upward,   // top-left
        };

        // Each face keeps its OWN four vertices, never shared with a
        // neighbouring face, for the reason Phase 10 gave: a vertex carries
        // one colour and one normal, and a cube corner belongs to three
        // faces that each need different values for both.
        const unsigned int base = static_cast<unsigned int>(vertices.size());

        for (int c = 0; c < 4; ++c)
            vertices.push_back({ corners[c], face.normal, faceColors[f] });

        // The same six-index, two-triangle pattern the quad introduced in
        // Phase 9, used once per face.
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }
}

// Builds the unit cube and uploads it. This is the call main() makes, exactly
// once, however many cubes end up on screen.
inline bool makeCube(Mesh& mesh, const glm::vec3 faceColors[6])
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildCubeGeometry(faceColors, vertices, indices);
    return mesh.upload("cube", vertices, indices);
}

// Phase 17: the simplest generator there is - four vertices, six indices, one
// flat surface. It exists to show the generator recipe with nothing else in
// the way: no loop over faces, no table of directions, just the four corners.
//
// Like the cube it follows the unit-mesh rule (Phase 16): it takes no size,
// and the quad is always 1 x 1, centred on its own origin, lying in the XY
// plane. A quad's real size is a glm::scale in its model matrix.
//
// A quad has no thickness, so "1 x 1" is its width and height; its depth is
// exactly 0. The scale to use is therefore (width, height, 1) - the 1 on z
// leaves the flat quad flat, and scaling zero by anything is still zero.
//
// Which way it faces: every corner's normal is (0, 0, 1), straight out of the
// screen along +Z. The winding has to AGREE with that normal - the corners
// must run counter-clockwise as seen from +Z - or GL_CULL_FACE would throw the
// quad away when viewed from the front and keep it when viewed from behind.
// Both facts are written down once, here, and Phase 17's checkpoint is that
// the Phase 15 normals view confirms the first one on screen.
//
// 'cornerColors' holds one colour per corner, in the order the corners are
// listed below: bottom-left, bottom-right, top-right, top-left.
inline void buildQuadGeometry(const glm::vec3 cornerColors[4],
                              std::vector<Vertex>& vertices,
                              std::vector<unsigned int>& indices)
{
    const glm::vec3 normal(0.0f, 0.0f, 1.0f);

    // The four corners, walked counter-clockwise (as seen from +Z) from the
    // bottom-left - the same walk the cube uses for each of its faces.
    const glm::vec3 corners[4] = {
        { -UNIT_HALF_EXTENT, -UNIT_HALF_EXTENT, 0.0f },   // 0: bottom-left
        {  UNIT_HALF_EXTENT, -UNIT_HALF_EXTENT, 0.0f },   // 1: bottom-right
        {  UNIT_HALF_EXTENT,  UNIT_HALF_EXTENT, 0.0f },   // 2: top-right
        { -UNIT_HALF_EXTENT,  UNIT_HALF_EXTENT, 0.0f },   // 3: top-left
    };

    vertices.clear();
    indices.clear();
    vertices.reserve(4);
    indices.reserve(6);

    for (int c = 0; c < 4; ++c)
        vertices.push_back({ corners[c], normal, cornerColors[c] });

    // Two triangles sharing the diagonal from corner 1 (bottom-right) to
    // corner 3 (top-left). Corners 1 and 3 are each stored once but used
    // twice - Phase 9's indexed-drawing idea, unchanged.
    //
    // This is NOT the cube's {0,1,2, 2,3,0} pattern, which would cut the quad
    // along the OTHER diagonal (corner 0 to corner 2). Cutting it the other
    // way changes how the four corner colours blend across the surface, and
    // the quad has looked the same way since Phase 9; keeping its diagonal
    // keeps its picture. Both triangles are counter-clockwise from +Z:
    //   (0, 1, 3)  bottom-left,  bottom-right, top-left
    //   (1, 2, 3)  bottom-right, top-right,    top-left
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(3);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);
}

// Builds the unit quad and uploads it. This is the call main() makes.
inline bool makeQuad(Mesh& mesh, const glm::vec3 cornerColors[4])
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildQuadGeometry(cornerColors, vertices, indices);
    return mesh.upload("quad", vertices, indices);
}

// Phase 18: the first PARAMETERISED generator.
//
// makeCube and makeQuad always produce the same amount of geometry: 24
// vertices and 4 vertices, every time. makeGrid is handed a number and builds
// a different amount depending on it, which is what "parameterised" means.
//
// 'cells' is the plan's N: how many quads fit along each side of the grid. So
// the grid is cells x cells quads, and that decides everything else:
//
//     vertices  = (cells + 1) * (cells + 1)
//     triangles = 2 * cells * cells
//     indices   = 6 * cells * cells
//
// The "+ 1" is the fence-post count: a side cut into 4 pieces has 5 posts
// along it. Those three formulas are worth being able to recite, because
// Phase 19's checkpoint is to work the triangle count out by eye and check it.
//
// It is still a UNIT mesh (Phase 16): exactly 1 x 1, centred on its own
// origin, and no size parameter. 'cells' changes how FINELY it is divided, not
// how big it is - two completely separate ideas that are easy to confuse.
//
// It lies in the XZ plane with the normal (0, 1, 0): FLAT AND HORIZONTAL, like
// a floor, not upright like the quad. That is deliberate and the two are not
// interchangeable. This mesh becomes the sea in Phase 81, where the wave
// displaces y from functions of x and z, so its two parameters have to be x
// and z. Building it upright now would mean rebuilding it then.
//
// Why a grid is not just "lots of quads": neighbouring cells SHARE their
// corner vertices. At cells = 32 a grid holds 33 * 33 = 1089 vertices and
// 2048 triangles. Four separate vertices per triangle corner would need
// 2048 * 3 = 6144. Sharing is what indexed drawing (Phase 9) bought, and it is
// why the sea can be finely divided without the vertex count exploding.
//
// 'cornerColors' holds one colour per CORNER OF THE WHOLE GRID, in the same
// cyclic order makeQuad uses: (-x,-z), (+x,-z), (+x,+z), (-x,+z). Every
// vertex in between is blended from all four, so the surface reads as one
// continuous sheet rather than a patchwork.
inline void buildGridGeometry(int cells,
                              const glm::vec3 cornerColors[4],
                              std::vector<Vertex>& vertices,
                              std::vector<unsigned int>& indices)
{
    // One quad is the smallest grid there is. Clamping also avoids dividing by
    // zero below, which would make every position a NaN and draw nothing at
    // all. Phase 25 gives this a proper maximum as well, when '+' and '-'
    // start rebuilding meshes at runtime.
    if (cells < 1)
        cells = 1;

    // Every vertex of a flat horizontal surface faces the same way: straight
    // up. (Phase 82 is where the wave's slope makes this vary per vertex.)
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);

    const int side = cells + 1;          // vertices along one edge: the fence posts
    const float step = 1.0f / static_cast<float>(cells);

    vertices.clear();
    indices.clear();
    vertices.reserve(static_cast<std::size_t>(side) * static_cast<std::size_t>(side));
    indices.reserve(static_cast<std::size_t>(cells) * static_cast<std::size_t>(cells) * 6u);

    // ---- the vertices: a (cells + 1) x (cells + 1) lattice, row by row ----
    //
    // The outer loop walks z, the inner loop walks x, so vertex (i, j) ends up
    // at index j * side + i. Knowing that one formula is what makes the index
    // loop below straightforward.
    for (int j = 0; j < side; ++j) {
        const float v = static_cast<float>(j) * step;     // 0 .. 1 across z
        for (int i = 0; i < side; ++i) {
            const float u = static_cast<float>(i) * step; // 0 .. 1 across x

            // u and v run 0..1, so subtracting the half extent centres the
            // grid on its own origin: x and z run -0.5 .. +0.5.
            const glm::vec3 position(
                -UNIT_HALF_EXTENT + u,
                0.0f,
                -UNIT_HALF_EXTENT + v);

            // Blend all four corner colours - across x first, then across z.
            // This is one bilinear mix, the same idea as asking "what colour
            // is this point on a four-cornered sheet?"
            const glm::vec3 nearEdge = glm::mix(cornerColors[0], cornerColors[1], u);
            const glm::vec3 farEdge  = glm::mix(cornerColors[3], cornerColors[2], u);
            const glm::vec3 color    = glm::mix(nearEdge, farEdge, v);

            vertices.push_back({ position, normal, color });
        }
    }

    // ---- the indices: two triangles per cell ----
    //
    // Each cell is a little quad with four already-uploaded corners:
    //
    //     v01 --- v11        v01 = (i,     j + 1)
    //      |  \    |         v11 = (i + 1, j + 1)
    //      |    \  |         v00 = (i,     j)
    //     v00 --- v10        v10 = (i + 1, j)
    //
    // Both triangles are wound counter-clockwise AS SEEN FROM ABOVE (+Y),
    // which is what GL_CULL_FACE needs to keep a surface whose normal points
    // up (Phases 9 and 11). Walking v00 -> v01 -> v11 turns that way; the
    // obvious-looking v00 -> v10 -> v11 turns the other way and would make the
    // whole grid invisible from above and visible only from underneath.
    for (int j = 0; j < cells; ++j) {
        for (int i = 0; i < cells; ++i) {
            const unsigned int v00 = static_cast<unsigned int>(j * side + i);
            const unsigned int v10 = v00 + 1u;
            const unsigned int v01 = v00 + static_cast<unsigned int>(side);
            const unsigned int v11 = v01 + 1u;

            indices.push_back(v00);
            indices.push_back(v01);
            indices.push_back(v11);

            indices.push_back(v00);
            indices.push_back(v11);
            indices.push_back(v10);
        }
    }
}

// Builds the unit grid and uploads it. This is the call main() makes.
inline bool makeGrid(Mesh& mesh, int cells, const glm::vec3 cornerColors[4])
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildGridGeometry(cells, cornerColors, vertices, indices);
    return mesh.upload("grid", vertices, indices);
}

// The angle all the way round a circle, in radians. Dividing it by the number
// of segments gives the step from one vertex of a ring to the next.
constexpr float TWO_PI = 6.28318530717958648f;

// Phase 20: the first CURVED surface, and the project's first mesh whose
// normals are not all the same.
//
// Phase 20 built the SIDE WALL. Phase 21 added the two END CAPS, so this is now
// a closed solid: a tube you cannot see through from any angle.
//
// The three pieces are built from one shared ring of positions but keep separate
// vertices, because the wall's normals point sideways and each cap's points
// straight up or down - see the comment on the caps below.
//
// Unit-mesh rule (Phase 16): no size parameter. The tube is exactly 1 unit
// across and 1 unit tall, centred on its own origin, with its AXIS ALONG Y -
// standing upright like a mast. Its real size is a glm::scale of
// (diameter, height, diameter) at draw time.
//
// 'segments' is how many flat strips the circle is cut into, which is the same
// kind of value as makeGrid's 'cells': it changes how finely the surface is
// divided, not how big it is. A circle cut into 6 looks like a hexagon; cut
// into 64 it looks round. Nothing about it is ever actually curved - every
// triangle is flat - which is the whole reason Phase 33's Demo B uses a
// low-segment barrel to show faceting.
//
//     wall        2 * segments vertices, 2 * segments triangles
//     + 2 caps    2 * (1 + segments) vertices, 2 * segments triangles
//     ---------------------------------------------------------------
//     vertices  = 4 * segments + 2      =  66 at segments = 16
//     triangles = 4 * segments          =  64 at segments = 16
//     indices   = 12 * segments         = 192 at segments = 16
//
// Note the "+ 2" is for the two cap centres, and that there is no "+ 1" per
// ring, unlike makeGrid. A grid's row of posts has two ends, so it needs
// cells + 1 of them. A ring has no ends - it closes on itself - so segment
// 'segments - 1' joins straight back to segment 0 with a modulo, and the seam
// vertices are SHARED rather than duplicated. That works because the normal at
// angle 0 and at angle 2*pi really are the same direction. (A textured cylinder
// would have to duplicate the seam, because the texture coordinate there jumps
// from 1 back to 0. This project has no textures, so sharing is correct and
// cheaper.)
inline void buildCylinderGeometry(int segments,
                                  const glm::vec3& bottomColor,
                                  const glm::vec3& topColor,
                                  std::vector<Vertex>& vertices,
                                  std::vector<unsigned int>& indices)
{
    // Two segments would be a flat flap and one would be nothing at all, so
    // three is the smallest ring that encloses any space. Clamping also keeps
    // the division below safe. Phase 25 adds a maximum as well.
    if (segments < 3)
        segments = 3;

    vertices.clear();
    indices.clear();
    vertices.reserve(static_cast<std::size_t>(segments) * 4u + 2u);
    indices.reserve(static_cast<std::size_t>(segments) * 12u);

    // The ring of (x, z) positions, worked out ONCE and then used three times:
    // for the wall, for the bottom cap's rim, and for the top cap's rim.
    //
    // The three sets of vertices built from it are separate - they have to be,
    // because they need different normals - but they are built from the SAME
    // numbers. That is what guarantees a cap's edge lands exactly on the wall's
    // edge, with no hairline crack between them. Calling sin and cos again for
    // each would almost certainly give the same answers, but "almost certainly"
    // is not a good foundation for a seam.
    std::vector<glm::vec2> ring;
    ring.reserve(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);

        // A point on a circle of radius UNIT_HALF_EXTENT. sin for x and cos for
        // z means angle 0 starts on the +Z axis and the angle advances toward
        // +X, which is the direction that makes the windings below come out
        // facing outward.
        ring.push_back(glm::vec2(std::sin(angle) * UNIT_HALF_EXTENT,
                                 std::cos(angle) * UNIT_HALF_EXTENT));
    }

    // ---- 1. the side wall: two rings, built one segment at a time ----
    for (int i = 0; i < segments; ++i) {
        const float x = ring[static_cast<std::size_t>(i)].x;
        const float z = ring[static_cast<std::size_t>(i)].y;

        // THE ANALYTIC NORMAL. For a cylinder standing on the Y axis the
        // surface normal points straight out from the axis, so it has no y
        // component at all: it is just the point's own (x, z) direction, made
        // unit length.
        //
        // "Analytic" means it comes from knowing what the shape IS, rather than
        // from measuring the triangles that approximate it. It is exactly right
        // for the smooth cylinder even though the mesh is a ring of flat
        // strips, and that mismatch is the point: the lighting will behave as
        // though the surface were truly round. Phase 23's computeSmoothNormals()
        // is the other approach - averaging the faces that meet at a vertex -
        // for shapes whose normals cannot simply be written down.
        //
        // Both vertices of this segment share it, because a cylinder's normal
        // does not depend on height.
        const glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        vertices.push_back({ { x, -UNIT_HALF_EXTENT, z }, normal, bottomColor });
        vertices.push_back({ { x,  UNIT_HALF_EXTENT, z }, normal, topColor });
    }

    // ---- 2. the wall's indices: one quad per segment ----
    //
    // Vertex 2*i is the bottom of segment i and 2*i + 1 is its top, so a strip
    // between segment i and the next one has these four corners:
    //
    //     t0 --- t1        b0 = 2*i          t0 = 2*i + 1
    //     |  \    |        b1 = 2*next       t1 = 2*next + 1
    //     |    \  |
    //     b0 --- b1
    //
    // 'next' wraps with a modulo, which is what closes the tube: the last
    // strip joins back to vertex 0 instead of needing a duplicate ring.
    //
    // b0 -> b1 -> t1 and b0 -> t1 -> t0 are counter-clockwise seen from
    // OUTSIDE the tube, which is what GL_CULL_FACE needs (Phases 9 and 11).
    for (int i = 0; i < segments; ++i) {
        const int next = (i + 1) % segments;

        const unsigned int b0 = static_cast<unsigned int>(i * 2);
        const unsigned int t0 = b0 + 1u;
        const unsigned int b1 = static_cast<unsigned int>(next * 2);
        const unsigned int t1 = b1 + 1u;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t1);

        indices.push_back(b0);
        indices.push_back(t1);
        indices.push_back(t0);
    }

    // ---- 3. the end caps (Phase 21) ----
    //
    // WHY THE CAPS CANNOT REUSE THE WALL'S RIM VERTICES. The wall's rim vertex
    // at segment i already sits in exactly the right place for the cap's rim.
    // It cannot be used, because a vertex carries ONE normal and these two
    // surfaces need different ones: the wall's points sideways, out from the
    // axis, and the cap's points straight up or straight down. Sharing would
    // force one of them to be wrong.
    //
    // This is the Phase 10 cube lesson again, and it is the reason this is its
    // own phase rather than four more lines in the one above. Every cap rim
    // vertex is a duplicate of a wall rim vertex in POSITION and a different
    // vertex in MEANING:
    //
    //     wall        2 * segments            vertices
    //     each cap    1 centre + segments     vertices
    //     total       4 * segments + 2        vertices, 4 * segments triangles
    //
    // A TRIANGLE FAN is the shape: one centre vertex, and a triangle from it to
    // each neighbouring pair of rim vertices. It is drawn here as ordinary
    // indexed triangles rather than with GL_TRIANGLE_FAN, because this mesh is
    // one glDrawElements call for the whole cylinder and a fan primitive would
    // need its own call - and because Mesh only knows how to draw GL_TRIANGLES.
    //
    // The windings are OPPOSITE to each other. Counter-clockwise seen from
    // outside means counter-clockwise seen from ABOVE for the top cap and from
    // BELOW for the bottom one, so the two fans list their corners in reverse
    // order. Getting this wrong is invisible from the front and leaves the other
    // end of the tube see-through, which is exactly the hole Phase 20 measured.
    for (int end = 0; end < 2; ++end) {
        const bool top = (end == 1);
        const float y = top ? UNIT_HALF_EXTENT : -UNIT_HALF_EXTENT;
        const glm::vec3 normal(0.0f, top ? 1.0f : -1.0f, 0.0f);
        const glm::vec3& color = top ? topColor : bottomColor;

        // The centre of the disc, then its rim - all with the cap's flat normal.
        const unsigned int centre = static_cast<unsigned int>(vertices.size());
        vertices.push_back({ glm::vec3(0.0f, y, 0.0f), normal, color });

        const unsigned int rimStart = static_cast<unsigned int>(vertices.size());
        for (int i = 0; i < segments; ++i)
            vertices.push_back({ glm::vec3(ring[static_cast<std::size_t>(i)].x,
                                           y,
                                           ring[static_cast<std::size_t>(i)].y),
                                 normal, color });

        for (int i = 0; i < segments; ++i) {
            const unsigned int a = rimStart + static_cast<unsigned int>(i);
            const unsigned int b = rimStart + static_cast<unsigned int>((i + 1) % segments);

            indices.push_back(centre);
            // The only difference between the two fans: which way round the pair
            // goes. The top needs (centre, a, b); the bottom needs (centre, b, a).
            indices.push_back(top ? a : b);
            indices.push_back(top ? b : a);
        }
    }
}

// Builds the unit cylinder - side wall and both end caps - and uploads it.
// This is the call main() makes.
inline bool makeCylinder(Mesh& mesh,
                         int segments,
                         const glm::vec3& bottomColor,
                         const glm::vec3& topColor)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildCylinderGeometry(segments, bottomColor, topColor, vertices, indices);
    return mesh.upload("cylinder", vertices, indices);
}

// Phase 22: the first generator with TWO parameters, and the shape whose normal
// is the simplest of all.
//
// A UV sphere is built the way a globe is drawn: rings of latitude stacked from
// one pole to the other, each ring cut into the same number of steps of
// longitude.
//
//     stacks is latitude  - how many bands from the north pole to the south.
//     slices is longitude - how many steps around each ring.
//
// They are genuinely independent, which is what makes this the first
// two-parameter surface: raising one makes the ball smoother top-to-bottom and
// the other smoother round the middle.
//
// Unit-mesh rule (Phase 16): no size parameter. The ball is exactly 1 unit
// across, centred on its own origin, so its radius is UNIT_HALF_EXTENT.
//
//     rings     = stacks - 1                    (the poles are not rings)
//     vertices  = 2 + (stacks - 1) * slices     =  200 at 12 x 18
//     triangles = 2 * slices * (stacks - 1)     =  396 at 12 x 18
//     indices   = 6 * slices * (stacks - 1)     = 1188 at 12 x 18
//
// THE NORMAL IS THE POSITION. For a sphere centred on its own origin, the
// direction from the centre out to a point on the surface IS the surface normal
// there - so the normal is just normalize(position), and nothing else has to be
// worked out. That is the simplest analytic normal in the project, and it is
// only this simple because the mesh is a unit mesh centred on the origin: a
// sphere built off-centre would need normalize(position - centre).
//
// THE POLES ARE SHARED, UNLIKE THE CYLINDER'S CAP RIMS. Each pole is ONE vertex
// serving 'slices' triangles. That is correct here, and the contrast with
// Phase 21 is worth understanding:
//
//     cylinder cap rim   the wall wants a sideways normal and the cap wants an
//                        axial one. Genuinely different directions at the same
//                        point, so they need separate vertices.
//     sphere pole        the true normal at the north pole is (0, 1, 0), and
//                        every triangle meeting there agrees with it. One
//                        vertex, one normal, correct.
//
// The naive way to build a sphere is a single rectangular lattice of
// (stacks + 1) * slices vertices with the top and bottom rows collapsed onto the
// poles. That is shorter to write, but it produces a ring of DEGENERATE
// zero-area triangles at each pole, where two of the three corners are the same
// point. They draw nothing, cost a little, and make a mesh that cannot pass a
// "no degenerate triangles" check. Treating the two pole bands as triangle FANS
// instead - exactly like the cylinder's caps - avoids them entirely.
inline void buildSphereGeometry(int stacks,
                                int slices,
                                const glm::vec3& bottomColor,
                                const glm::vec3& topColor,
                                std::vector<Vertex>& vertices,
                                std::vector<unsigned int>& indices)
{
    // Two stacks is the smallest sphere there is: two pole fans meeting at a
    // single ring, which is a diamond rather than a ball but is still a closed
    // solid. Fewer than three slices could not close round. Phase 25 adds
    // maxima to go with these minima.
    if (stacks < 2) stacks = 2;
    if (slices < 3) slices = 3;

    const float radius = UNIT_HALF_EXTENT;
    const float halfTurn = TWO_PI * 0.5f;      // pi: pole to pole is half a turn

    vertices.clear();
    indices.clear();
    vertices.reserve(2u + static_cast<std::size_t>(stacks - 1) * static_cast<std::size_t>(slices));
    indices.reserve(static_cast<std::size_t>(slices) * static_cast<std::size_t>(stacks - 1) * 6u);

    // ---- 1. the north pole: vertex 0 ----
    vertices.push_back({ glm::vec3(0.0f, radius, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), topColor });

    // ---- 2. the rings of latitude, north to south ----
    //
    // phi is the angle down from the north pole, so it runs from 0 to pi. The
    // poles themselves are handled separately, so this loop covers only the
    // rings strictly between them: j = 1 .. stacks - 1.
    for (int j = 1; j < stacks; ++j) {
        const float phi = halfTurn * static_cast<float>(j) / static_cast<float>(stacks);
        const float y = radius * std::cos(phi);
        const float ringRadius = radius * std::sin(phi);

        // Blend the two colours from pole to pole: 1 at the north, 0 at the south.
        const glm::vec3 color = glm::mix(bottomColor, topColor, 1.0f - phi / halfTurn);

        for (int i = 0; i < slices; ++i) {
            const float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);

            // The same sin-for-x, cos-for-z convention the cylinder uses, so
            // the windings below come out the same way round.
            const glm::vec3 position(ringRadius * std::sin(theta),
                                     y,
                                     ringRadius * std::cos(theta));

            vertices.push_back({ position, glm::normalize(position), color });
        }
    }

    // ---- 3. the south pole: the last vertex ----
    vertices.push_back({ glm::vec3(0.0f, -radius, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), bottomColor });

    const unsigned int northPole = 0u;
    const unsigned int southPole = static_cast<unsigned int>(vertices.size() - 1u);

    // Ring j (1 .. stacks - 1) begins at this vertex. Ring 1 is just below the
    // north pole; ring stacks - 1 is just above the south pole.
    const auto ringStart = [slices](int j) {
        return 1u + static_cast<unsigned int>((j - 1) * slices);
    };

    // ---- 4. the north pole fan ----
    //
    // Walking the ring in increasing order and finishing at the pole comes out
    // counter-clockwise seen from outside, matching the middle bands below.
    for (int i = 0; i < slices; ++i) {
        indices.push_back(ringStart(1) + static_cast<unsigned int>(i));
        indices.push_back(ringStart(1) + static_cast<unsigned int>((i + 1) % slices));
        indices.push_back(northPole);
    }

    // ---- 5. the middle bands: a quad per slice, two triangles each ----
    //
    // Exactly the cylinder wall's pattern, with 'upper' and 'lower' being two
    // rings of DIFFERENT radius instead of two rings of the same radius. At
    // stacks = 2 there are no middle bands and this loop does not run at all.
    for (int j = 1; j < stacks - 1; ++j) {
        for (int i = 0; i < slices; ++i) {
            const unsigned int next = static_cast<unsigned int>((i + 1) % slices);
            const unsigned int upperI = ringStart(j) + static_cast<unsigned int>(i);
            const unsigned int upperN = ringStart(j) + next;
            const unsigned int lowerI = ringStart(j + 1) + static_cast<unsigned int>(i);
            const unsigned int lowerN = ringStart(j + 1) + next;

            indices.push_back(lowerI);
            indices.push_back(lowerN);
            indices.push_back(upperN);

            indices.push_back(lowerI);
            indices.push_back(upperN);
            indices.push_back(upperI);
        }
    }

    // ---- 6. the south pole fan ----
    //
    // The reverse order of the north fan, for the same reason the cylinder's two
    // caps wind oppositely: "counter-clockwise seen from outside" points a
    // different way at the two ends of a solid.
    for (int i = 0; i < slices; ++i) {
        indices.push_back(southPole);
        indices.push_back(ringStart(stacks - 1) + static_cast<unsigned int>((i + 1) % slices));
        indices.push_back(ringStart(stacks - 1) + static_cast<unsigned int>(i));
    }
}

// Builds the unit sphere and uploads it. This is the call main() makes.
inline bool makeSphere(Mesh& mesh,
                       int stacks,
                       int slices,
                       const glm::vec3& bottomColor,
                       const glm::vec3& topColor)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildSphereGeometry(stacks, slices, bottomColor, topColor, vertices, indices);
    return mesh.upload("sphere", vertices, indices);
}

// ---- Phase 23: the OTHER way of deciding a normal ----------------------
//
// Every normal so far has been ANALYTIC - written down because the shape was
// known: an axis constant for the cube, the quad, the grid and the cylinder's
// caps, normalize(vec3(x, 0, z)) for the cylinder wall, normalize(position) for
// the sphere. That is always the better answer when it is available, because it
// is exact and does not depend on how finely the mesh is divided.
//
// It is not always available. For a shape with no formula - a hull hand-built
// from arbitrary points, or a surface that has been deformed - the only
// information left is the triangles themselves. So: take the normal of every
// triangle that MEETS a vertex and average them.
//
//     N_v = (sum of N_i) / || sum of N_i ||        - L9 slide 20
//
// Dividing by the length of the sum is the same as dividing by the count and
// then normalizing, so the formula is written in the compact form the slide
// uses. This function is a direct implementation of that slide and belongs in
// the report.
//
// It overwrites whatever normals the vertices already had, and it only makes a
// difference when vertices are SHARED between faces. On a mesh where every face
// owns its own vertices - the 24-vertex cube, the cylinder's cap rims - each
// vertex is touched by only one face's triangles, so the "average" is that one
// face's normal and nothing changes. That is the whole point of the comparison
// in Phase 23, and the reason the flat cube cannot be smoothed.
inline void computeSmoothNormals(std::vector<Vertex>& vertices,
                                const std::vector<unsigned int>& indices)
{
    // Start from zero so the sum can be accumulated in place.
    for (Vertex& vert : vertices)
        vert.normal = glm::vec3(0.0f);

    // Add each triangle's own normal to all three of its corners.
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3& a = vertices[indices[i]].position;
        const glm::vec3& b = vertices[indices[i + 1]].position;
        const glm::vec3& c = vertices[indices[i + 2]].position;

        // The face normal, from the winding - exactly the cross product the
        // winding tests in Phases 9 to 22 have been computing all along.
        const glm::vec3 faceNormal = glm::normalize(glm::cross(b - a, c - a));

        vertices[indices[i]].normal += faceNormal;          // accumulate sum N_i
        vertices[indices[i + 1]].normal += faceNormal;
        vertices[indices[i + 2]].normal += faceNormal;
    }

    // Divide by the length of the sum, which is what normalize does.
    for (Vertex& vert : vertices)
        vert.normal = glm::normalize(vert.normal);
}

// Phase 23: a cube built from 8 SHARED corners instead of 24 separate ones.
//
// This is the mesh Phase 10 rejected. Phase 10 wanted one flat colour per face,
// and a vertex carries one colour, so a corner shared by three faces could not
// serve all three - hence 24 vertices. Everything that followed inherited that
// reasoning, including the cylinder's cap rims in Phase 21.
//
// Sharing is exactly what makes smoothing possible. With 8 vertices, each corner
// is touched by the triangles of three different faces, so averaging them gives
// a normal that points diagonally outward and is shared by all three. Normals
// then INTERPOLATE across each face instead of being constant over it, and the
// cube shades as though it were a rounded blob.
//
// The same sharing takes the per-face colours away again, which is why this cube
// is coloured by corner height instead: 'bottomColor' on the four lower corners
// and 'topColor' on the four upper ones. You cannot give a shared-vertex cube six
// face colours, and failing to is not a limitation of this code - it is the
// Phase 10 lesson, seen from the other side.
//
//     vertices  =  8   (against the flat cube's 24)
//     triangles = 12   (the same 12)
//     indices   = 36   (the same 36)
inline void buildSharedCubeGeometry(const glm::vec3& bottomColor,
                                    const glm::vec3& topColor,
                                    std::vector<Vertex>& vertices,
                                    std::vector<unsigned int>& indices)
{
    const float h = UNIT_HALF_EXTENT;

    // The eight corners of a cube, in a fixed order the index list below relies
    // on: 0-3 are the bottom face walked anticlockwise from (-x,-z), and 4-7 are
    // the top four directly above them.
    const glm::vec3 corners[8] = {
        { -h, -h, -h },   // 0
        {  h, -h, -h },   // 1
        {  h,  h, -h },   // 2
        { -h,  h, -h },   // 3
        { -h, -h,  h },   // 4
        {  h, -h,  h },   // 5
        {  h,  h,  h },   // 6
        { -h,  h,  h },   // 7
    };

    vertices.clear();
    indices.clear();
    vertices.reserve(8);
    indices.reserve(36);

    // The normal is left at zero: computeSmoothNormals() fills it in. Writing a
    // placeholder here rather than guessing is deliberate - there is no single
    // correct normal for a shared corner until the faces around it are known.
    for (int i = 0; i < 8; ++i)
        vertices.push_back({ corners[i],
                             glm::vec3(0.0f),
                             corners[i].y < 0.0f ? bottomColor : topColor });

    // Twelve triangles, two per face, every one wound counter-clockwise as seen
    // from OUTSIDE - checked face by face the same way Phase 10's were.
    const unsigned int faces[36] = {
        4, 5, 6,   4, 6, 7,     // +Z front
        1, 0, 3,   1, 3, 2,     // -Z back
        5, 1, 2,   5, 2, 6,     // +X right
        0, 4, 7,   0, 7, 3,     // -X left
        7, 6, 2,   7, 2, 3,     // +Y top
        0, 1, 5,   0, 5, 4,     // -Y bottom
    };
    for (unsigned int i : faces)
        indices.push_back(i);
}

// Builds the shared-corner cube, averages its normals, and uploads it.
inline bool makeSmoothCube(Mesh& mesh,
                           const glm::vec3& bottomColor,
                           const glm::vec3& topColor)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildSharedCubeGeometry(bottomColor, topColor, vertices, indices);
    computeSmoothNormals(vertices, indices);
    return mesh.upload("smoothcube", vertices, indices);
}

// ---- Phase 47: the hull - the first of the three generators beyond the original five ---------
//
// The plan allows exactly three shapes that a scaled primitive cannot make: this hull, the plank
// strip (Phase 51) and the sail (Phase 60). A cube scaled to a hull's size is a barge; a hull is a
// SHAPE, and a shape needs its own generator.
//
// Built the way it is described in src/Hull.h: a ring of points for each cross-section from stern
// to stem, and a quad between every pair of neighbouring points on neighbouring rings.
//
// THE FOUR SURFACES. Like the cylinder, whose wall and caps cannot share vertices because they need
// different normals, the hull is four surfaces that meet at CREASES:
//
//     the sides     one smooth surface from the left gunwale, under the keel, to the right gunwale
//     the cap       the flat-across top between the two gunwales, normal up - the deck level
//     the transom   the flat stern, normal aft
//     (the stem     is not a surface: at the bow the width reaches zero and the two sides meet)
//
// A vertex carries ONE normal. The sides and the cap meet at the gunwale at roughly a right angle;
// sharing the gunwale's vertices would average the two normals and round the edge off, so the cap and
// the transom get their OWN copies of the vertices along the edges they share with the sides. The
// positions are identical; the meaning is different (the Phase 10 cube lesson, again).
//
//     unit mesh      x, y, z all in [-0.5, 0.5]; the draw-time scale is (beam, depth, length)
//     normals        computeSmoothNormals() - the L9 slide 20 averaging formula - run over each surface
//     winding        counter-clockwise seen from OUTSIDE, as every mesh in the project
inline void buildHullGeometry(const std::vector<HullRing>& rings,
                              const HullColors& colors,
                              std::vector<Vertex>& outVertices,
                              std::vector<unsigned int>& outIndices)
{
    // Phase 49: the hull is built in TWO passes. Pass one (everything below, down to the normals) builds the
    // surface with every ring-point shared between the quads around it, so that computeSmoothNormals() blends
    // across the planks and the hull shades as one smooth curve. Pass two (at the end) copies it out and gives each
    // PLANK its own vertices, so that each can carry its own colour with a crisp edge, and takes the normals
    // from pass one so the edge is invisible to the lighting. These two are the temporary mesh of pass one:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int R = static_cast<int>(rings.size());
    const int S = HullShape::STRAKES;
    const int perRing = 2 * S + 1;    // left gunwale ... keel ... right gunwale

    const auto at = [](const HullRing& r, int k, float side) {
        const glm::vec2 p = hullSectionPoint(r, k);
        return glm::vec3(side * p.x, p.y, r.z);
    };
    const auto push = [&](const glm::vec3& p) {
        vertices.push_back({ p, glm::vec3(0.0f), glm::vec3(1.0f) });
        return static_cast<unsigned int>(vertices.size() - 1u);
    };
    // A triangle is only kept if it has an area: at the stem the left and right gunwale points of the
    // last ring are the same place, and the quad that ends there collapses to a triangle.
    const auto addTriangle = [&](unsigned int a, unsigned int b, unsigned int c) {
        const glm::vec3 e1 = vertices[b].position - vertices[a].position;
        const glm::vec3 e2 = vertices[c].position - vertices[a].position;
        if (glm::length(glm::cross(e1, e2)) < 1e-9f)
            return;
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    };

    // ---- 1. the sides: rings of 2S+1 points, left gunwale (j = 0) to keel (j = S) to right gunwale ----
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < perRing; ++j) {
            if (j < S)        push(at(rings[static_cast<std::size_t>(i)], S - j, -1.0f));
            else if (j == S)  push(at(rings[static_cast<std::size_t>(i)], 0, 1.0f));
            else              push(at(rings[static_cast<std::size_t>(i)], j - S, 1.0f));
        }
    }
    const auto side = [perRing](int i, int j) { return static_cast<unsigned int>(i * perRing + j); };

    // The same index pattern is counter-clockwise from outside on BOTH flanks (worked out in the
    // plan for this phase: on the right, j rises with the gunwale; on the left it falls, and that
    // reverses the viewer's handedness exactly as much as it reverses the direction).
    for (int i = 0; i + 1 < R; ++i)
        for (int j = 0; j + 1 < perRing; ++j) {
            addTriangle(side(i, j), side(i, j + 1), side(i + 1, j + 1));
            addTriangle(side(i, j), side(i + 1, j + 1), side(i + 1, j));
        }

    // ---- 2. the cap: left and right gunwale points, their own vertices, normal up ----
    std::vector<unsigned int> capL(static_cast<std::size_t>(R)), capR(static_cast<std::size_t>(R));
    for (int i = 0; i < R; ++i) {
        const HullRing& r = rings[static_cast<std::size_t>(i)];
        capL[static_cast<std::size_t>(i)] = push(at(r, S, -1.0f));
        // At the stem the two gunwale points coincide; one vertex serves both so none is left unused.
        capR[static_cast<std::size_t>(i)] = (r.halfWidth < 1e-9f) ? capL[static_cast<std::size_t>(i)] : push(at(r, S, 1.0f));
    }
    for (int i = 0; i + 1 < R; ++i) {
        const std::size_t a = static_cast<std::size_t>(i), b = a + 1;
        addTriangle(capL[a], capL[b], capR[b]);
        addTriangle(capL[a], capR[b], capR[a]);
    }

    // ---- 3. the transom: the flat stern, closed across the first ring ----
    const std::size_t transomFirst = vertices.size();    // pass two needs to know where the transom's vertices begin
    {
        const HullRing& r = rings.front();
        const unsigned int keel = push(at(r, 0, 1.0f));
        std::vector<unsigned int> tl(static_cast<std::size_t>(S + 1)), tr(static_cast<std::size_t>(S + 1));
        tl[0] = tr[0] = keel;
        for (int k = 1; k <= S; ++k) {
            tl[static_cast<std::size_t>(k)] = push(at(r, k, -1.0f));
            tr[static_cast<std::size_t>(k)] = push(at(r, k, 1.0f));
        }
        for (int k = 0; k < S; ++k) {
            const std::size_t a = static_cast<std::size_t>(k), b = a + 1;
            addTriangle(tl[a], tl[b], tr[b]);
            addTriangle(tl[a], tr[b], tr[a]);
        }
    }

    computeSmoothNormals(vertices, indices);

    // ---- pass two: give every plank its own vertices, and paint it ----
    //
    // A vertex carries ONE colour. A row of the loft is shared by the plank below it and the plank above, which
    // are different colours, so the row's vertices are duplicated - once per plank that uses them. The position and
    // the NORMAL are copied from pass one, so the lighting sees one smooth surface and only the paint changes.
    //
    // The cap and the transom are not planked: they keep their vertices, one colour each.
    outVertices.clear();
    outIndices.clear();
    const std::size_t sideVertexCount = static_cast<std::size_t>(R) * static_cast<std::size_t>(perRing);
    std::map<std::pair<unsigned int, int>, unsigned int> plankVertex;       // (pass-one vertex, strake) -> output vertex
    std::map<unsigned int, unsigned int> otherVertex;                        // pass-one vertex (cap, transom) -> output vertex

    const auto paintedSide = [&](unsigned int shared, int strake) {
        const auto key = std::make_pair(shared, strake);
        const auto found = plankVertex.find(key);
        if (found != plankVertex.end())
            return found->second;
        const Vertex& src = vertices[shared];
        outVertices.push_back({ src.position, src.normal, colors.strake[static_cast<std::size_t>(strake)] });
        const unsigned int made = static_cast<unsigned int>(outVertices.size() - 1u);
        plankVertex[key] = made;
        return made;
    };
    const auto keptVertex = [&](unsigned int shared) {
        const auto found = otherVertex.find(shared);
        if (found != otherVertex.end())
            return found->second;
        const Vertex& src = vertices[shared];
        const bool isTransom = static_cast<std::size_t>(shared) >= transomFirst;
        outVertices.push_back({ src.position, src.normal, isTransom ? colors.transom : colors.cap });
        const unsigned int made = static_cast<unsigned int>(outVertices.size() - 1u);
        otherVertex[shared] = made;
        return made;
    };
    for (std::size_t t = 0; t + 2 < indices.size(); t += 3) {
        const unsigned int a = indices[t], b = indices[t + 1], c = indices[t + 2];
        if (a < sideVertexCount) {
            // A side triangle: its two ring-points j and j + 1 decide the strake.
            const int ja = static_cast<int>(a % static_cast<unsigned int>(perRing));
            const int jb = static_cast<int>(b % static_cast<unsigned int>(perRing));
            const int jc = static_cast<int>(c % static_cast<unsigned int>(perRing));
            const int strake = hullStrakeOfQuad(std::min(ja, std::min(jb, jc)), S);
            outIndices.push_back(paintedSide(a, strake));
            outIndices.push_back(paintedSide(b, strake));
            outIndices.push_back(paintedSide(c, strake));
        } else {
            outIndices.push_back(keptVertex(a));
            outIndices.push_back(keptVertex(b));
            outIndices.push_back(keptVertex(c));
        }
    }
}

// Builds the hull from a station table and uploads it.
inline bool makeHull(Mesh& mesh, const HullProfile& profile, const HullColors& colors)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildHullGeometry(hullRings(profile), colors, vertices, indices);
    return mesh.upload("hull", vertices, indices);
}

// ---- Phase 51: the deck plank sheet -----------------------------------------------------------------------------------
//
// The second of the three generators the plan allows beyond the original five meshes (the hull was the first). One flat
// sheet in the xz plane at y = 0, 1 x 1, normal +y, wound counter-clockwise as seen from above - so it is visible from above
// and culled from below, like a real deck. It is made of the strips in src/Planks.h: a plank, a seam, a plank, ... each a
// quad running the full length (z = -0.5 to +0.5) with its own four vertices, so the colour changes at a seam EDGE instead
// of blending across it (a vertex has one colour; compare Phase 49's second pass).
//
//   vertices = 4 x (planks + seams)       triangles = 2 x (planks + seams)
inline void buildDeckPlankGeometry(int planks,
                                   float seamFraction,
                                   std::vector<Vertex>& vertices,
                                   std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    for (const DeckStrip& s : deckStrips(planks, seamFraction)) {
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({ glm::vec3(s.x0, 0.0f, -UNIT_HALF_EXTENT), up, s.color });   // 0: port edge, stern
        vertices.push_back({ glm::vec3(s.x0, 0.0f,  UNIT_HALF_EXTENT), up, s.color });   // 1: port edge, bow
        vertices.push_back({ glm::vec3(s.x1, 0.0f,  UNIT_HALF_EXTENT), up, s.color });   // 2: starboard edge, bow
        vertices.push_back({ glm::vec3(s.x1, 0.0f, -UNIT_HALF_EXTENT), up, s.color });   // 3: starboard edge, stern
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
    }
}

inline bool makeDeckPlanks(Mesh& mesh)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildDeckPlankGeometry(DeckPlanking::PLANKS, DeckPlanking::SEAM_FRACTION, vertices, indices);
    return mesh.upload("deckplanks", vertices, indices);
}

// ---- Phase 60-61: bowed, weathered, deterministically torn sail -----------------------------
//
// A 1 x 1 cloth in the xy plane, centred like the original quad.  Its z coordinate
// is a smooth parabola across the width multiplied by a vertical sine, which pins
// the cloth to the yard at its top edge and gives it a belly lower down.  The
// derivatives of that same function give the analytic normal.  Front and back
// vertices are separate so culling and lighting are correct from either side.
// Three fixed cells are omitted as worn holes, and the bottom edge uses a fixed
// jagged table: the result is deterministic in screenshots and needs no texture.
inline void buildSailGeometry(int cellsX, int cellsY, float belly,
                              std::vector<Vertex>& vertices,
                              std::vector<unsigned int>& indices)
{
    constexpr float SAIL_PI = 3.14159265358979f;
    cellsX = std::max(4, cellsX);
    cellsY = std::max(4, cellsY);
    vertices.clear(); indices.clear();
    const int row = cellsX + 1;
    const glm::vec3 shades[4] = {
        glm::vec3(0.52f, 0.48f, 0.39f), glm::vec3(0.65f, 0.60f, 0.49f),
        glm::vec3(0.57f, 0.52f, 0.43f), glm::vec3(0.70f, 0.64f, 0.52f)
    };
    for (int face = 0; face < 2; ++face) {
        const float sign = face == 0 ? 1.0f : -1.0f;
        for (int y = 0; y <= cellsY; ++y) {
            const float v = static_cast<float>(y) / static_cast<float>(cellsY);
            for (int x = 0; x <= cellsX; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(cellsX);
                float py = 0.5f - v;
                if (y == cellsY) {
                    static const float JAG[8] = { 0.00f, 0.045f, 0.012f, 0.070f, 0.018f, 0.055f, 0.008f, 0.035f };
                    py += JAG[x & 7];
                }
                const float q = 2.0f * u - 1.0f;
                const float wave = std::sin(SAIL_PI * v);
                const float pz = belly * (1.0f - q * q) * wave;
                const float dzdu = -4.0f * belly * q * wave;
                const float dzdv = belly * (1.0f - q * q) * SAIL_PI * std::cos(SAIL_PI * v);
                glm::vec3 n = glm::normalize(glm::vec3(-dzdu, dzdv, 1.0f)) * sign;
                vertices.push_back({ glm::vec3(u - 0.5f, py, pz), n,
                                     shades[(x + 2 * y) & 3] });
            }
        }
    }
    const int faceVertices = row * (cellsY + 1);
    const auto hole = [cellsX, cellsY](int x, int y) {
        return (x == cellsX / 3 && y == cellsY / 2)
            || (x == (2 * cellsX) / 3 && y == cellsY / 3)
            || (x == cellsX / 2 && y == (2 * cellsY) / 3);
    };
    for (int face = 0; face < 2; ++face) {
        const unsigned int off = static_cast<unsigned int>(face * faceVertices);
        for (int y = 0; y < cellsY; ++y) for (int x = 0; x < cellsX; ++x) {
            if (hole(x, y)) continue;
            const unsigned int a = off + static_cast<unsigned int>(y * row + x);
            const unsigned int b = a + 1u, d = a + static_cast<unsigned int>(row), c = d + 1u;
            if (face == 0) {
                indices.insert(indices.end(), { a, d, c, a, c, b });
            } else {
                indices.insert(indices.end(), { a, c, d, a, b, c });
            }
        }
    }
}

inline bool makeSail(Mesh& mesh, int cellsX = 12, int cellsY = 10, float belly = 0.12f)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildSailGeometry(cellsX, cellsY, belly, vertices, indices);
    return mesh.upload("sail", vertices, indices);
}
