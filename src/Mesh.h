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

#include <cstddef>
#include <cstdio>
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

        std::printf("[mesh] %-8s vertices=%d indices=%d triangles=%d\n",
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
// of by hand. makeCube is the first; makeQuad, makeGrid, makeCylinder, and
// makeSphere follow in Phases 17-22.

// Builds the cube's 24 vertices and 36 indices on the CPU, without touching
// OpenGL at all. Keeping this separate from the upload means the numbers can
// be checked, printed, or modified before they ever reach the GPU - which is
// exactly what Phase 23's computeSmoothNormals() will need to do.
//
// 'halfSize' is how far each face sits from the centre, so the cube is
// 2 * halfSize across. 'faceColors' holds one colour per face, in the same
// order the faces are listed below.
inline void buildCubeGeometry(float halfSize,
                              const glm::vec3 faceColors[6],
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
        // Starting at the face's centre (normal * halfSize) and stepping
        // half a face width along `right` and `up` lands exactly on a corner,
        // because the three directions are perpendicular and unit length.
        const glm::vec3 centre = face.normal * halfSize;
        const glm::vec3 across = face.right * halfSize;
        const glm::vec3 upward = face.up * halfSize;

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

// Builds the cube and uploads it. This is the call main() makes.
inline bool makeCube(Mesh& mesh, float halfSize, const glm::vec3 faceColors[6])
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    buildCubeGeometry(halfSize, faceColors, vertices, indices);
    return mesh.upload("cube", vertices, indices);
}
