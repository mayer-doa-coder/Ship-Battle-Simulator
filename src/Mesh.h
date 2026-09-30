#pragma once

// Phase 14: the project's first reusable mesh type.
//
// Every phase since 2 has drawn a shape by hand-writing its own GPU-handle
// struct (TriangleGpu, QuadGpu, CubeGpu) and its own matching pair of
// create*()/destroy*() functions - four or five lines apart, doing almost
// exactly the same thing each time. Mesh replaces the two structs that were
// already IDENTICAL in shape (QuadGpu and CubeGpu both just own a VAO, a
// VBO, and an EBO) with one real, reusable type.
//
// The reference project's Mesh eventually stores a NORMAL on every vertex,
// for lighting. This project has no lighting yet - that arrives in Stage C -
// so a normal here would be a field nothing reads, which is exactly the
// "do not build ahead" mistake this project's own rules warn against. This
// Vertex instead stores exactly what every mesh built so far actually uses:
// a position and a colour. A normal is added later, the same way every
// other field in this project has been added only once a real phase needed
// it.

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

// One corner of a mesh: where it is, and what colour it is. This is the
// exact same pair of attributes the triangle, quad, and cube have all used
// since Phase 2 - Mesh does not change what a vertex holds, only how the
// GPU objects that store it are owned and reused.
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

// Owns one indexed mesh's GPU objects - a VAO, a VBO, and an EBO - and knows
// how to draw and free itself. Every object built from here on that needs
// indexed drawing (Phase 9's idea) uses this ONE type instead of its own
// hand-written struct and function pair.
class Mesh {
public:
    Mesh() = default;

    // Uploads 'vertices' and 'indices' once, immediately, and remembers how
    // many indices to draw. This replaces the separate createQuad()/
    // createCube() functions - what they did by hand, this constructor does
    // once, for any mesh's data.
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    {
        create(vertices, indices);
    }

    // Frees the GPU objects automatically if destroy() was never called -
    // the same safety net ShaderProgram's destructor already provides.
    // main() still calls destroy() explicitly at shutdown, for the exact
    // reason it still calls shader.destroy() rather than only relying on
    // ShaderProgram's destructor: local C++ objects are destroyed in
    // reverse declaration order when a function returns, which in main()
    // happens AFTER glfwDestroyWindow()/glfwTerminate() run. Deleting a GPU
    // object with no OpenGL context left to delete it in is undefined
    // behaviour, so the explicit call - while the context still exists -
    // stays, and this destructor only ever has real work left to do if
    // something goes wrong before that call is reached.
    ~Mesh()
    {
        destroy();
    }

    // Copying a Mesh would let two C++ objects both believe they own, and
    // both try to delete, the SAME GPU handles - the identical reason
    // ShaderProgram (Phase 2) disables copying too.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Moving TRANSFERS ownership instead of duplicating the GPU objects, so
    // a function like makeCube() can build a Mesh and return it by value -
    // the exact "Mesh cube = makeCube();" idiom used below - without ever
    // creating a second, competing owner of the same VAO.
    Mesh(Mesh&& other) noexcept
    {
        moveFrom(other);
    }

    Mesh& operator=(Mesh&& other) noexcept
    {
        if (this != &other) {
            destroy();
            moveFrom(other);
        }
        return *this;
    }

    // glBindVertexArray + glDrawElements: the two lines every renderScene()
    // draw call used to write out by hand for the quad and the cube. The
    // caller is still responsible for setting uModel and uTint first, the
    // same as before - a Mesh only knows its own geometry, never where it
    // sits or what colour filter is on it that frame.
    void draw() const
    {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    // Mirrors ShaderProgram::valid(): true once the GPU objects exist. A
    // constructor cannot return a success/failure code the way createQuad()
    // and createCube() used to, so main() checks this afterward instead.
    bool valid() const
    {
        return m_vao != 0 && m_vbo != 0 && m_ebo != 0;
    }

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
        m_indexCount = 0;
    }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    int m_indexCount = 0;

    void create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            GL_STATIC_DRAW);

        // Same rule every EBO in this project has followed since Phase 9:
        // it is bound here, while this VAO is bound, and it is NOT unbound
        // before the VAO is - unbinding it first would erase the VAO's own
        // memory of which index buffer belongs to it.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
            GL_STATIC_DRAW);

        // offsetof works here because Vertex is a plain aggregate - two
        // glm::vec3 members, no constructors, no hidden bookkeeping - which
        // is exactly what keeps its memory layout predictable enough for
        // this to be well defined.
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_indexCount = static_cast<int>(indices.size());
    }

    void moveFrom(Mesh& other)
    {
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
};

// Phase 14's other half of the plan: the cube's geometry moves out of
// main.cpp entirely and into this one function. Calling it twice would
// produce two independent cubes sharing nothing - there is only one call
// site today, but that reuse is exactly the point of a generator function
// instead of writing the vertex list out again by hand.
//
// This is a true UNIT cube: HALF_SIZE is fixed at 0.5, so every face sits
// exactly 0.5 units from the centre and the cube is exactly 1 unit wide,
// tall, and deep. A different final size will come from a scale in the
// model matrix (Phase 16), the same "one unit mesh, resized only when
// drawn" idea CubeConfig::HALF_SIZE could not yet demonstrate on its own.
// Changing this project's cube size today still means editing the one
// constant below, until that scale control exists.
inline Mesh makeCube()
{
    constexpr float h = 0.5f;

    // Six faces, 4 vertices each - not 8 shared corners - for the exact
    // reason Phase 10 explained: a corner shared by three faces could only
    // ever carry ONE colour, and each face here needs its own.
    const std::vector<Vertex> vertices = {
        // +Z face (front, facing the camera) - blue
        { {-h, -h,  h}, {0.0f, 0.0f, 1.0f} },
        { { h, -h,  h}, {0.0f, 0.0f, 1.0f} },
        { { h,  h,  h}, {0.0f, 0.0f, 1.0f} },
        { {-h,  h,  h}, {0.0f, 0.0f, 1.0f} },

        // -Z face (back) - yellow
        { { h, -h, -h}, {1.0f, 1.0f, 0.0f} },
        { {-h, -h, -h}, {1.0f, 1.0f, 0.0f} },
        { {-h,  h, -h}, {1.0f, 1.0f, 0.0f} },
        { { h,  h, -h}, {1.0f, 1.0f, 0.0f} },

        // +X face (right) - red
        { { h, -h,  h}, {1.0f, 0.0f, 0.0f} },
        { { h, -h, -h}, {1.0f, 0.0f, 0.0f} },
        { { h,  h, -h}, {1.0f, 0.0f, 0.0f} },
        { { h,  h,  h}, {1.0f, 0.0f, 0.0f} },

        // -X face (left) - cyan
        { {-h, -h, -h}, {0.0f, 1.0f, 1.0f} },
        { {-h, -h,  h}, {0.0f, 1.0f, 1.0f} },
        { {-h,  h,  h}, {0.0f, 1.0f, 1.0f} },
        { {-h,  h, -h}, {0.0f, 1.0f, 1.0f} },

        // +Y face (top) - green
        { {-h,  h,  h}, {0.0f, 1.0f, 0.0f} },
        { { h,  h,  h}, {0.0f, 1.0f, 0.0f} },
        { { h,  h, -h}, {0.0f, 1.0f, 0.0f} },
        { {-h,  h, -h}, {0.0f, 1.0f, 0.0f} },

        // -Y face (bottom) - magenta
        { {-h, -h, -h}, {1.0f, 0.0f, 1.0f} },
        { { h, -h, -h}, {1.0f, 0.0f, 1.0f} },
        { { h, -h,  h}, {1.0f, 0.0f, 1.0f} },
        { {-h, -h,  h}, {1.0f, 0.0f, 1.0f} },
    };

    // The exact same {corner,corner,corner, corner,corner,corner} pattern
    // Phase 9's quad introduced, six times over - one per face, starting at
    // that face's own first vertex.
    const std::vector<unsigned int> indices = {
         0,  1,  2,   2,  3,  0,    // +Z face
         4,  5,  6,   6,  7,  4,    // -Z face
         8,  9, 10,  10, 11,  8,    // +X face
        12, 13, 14,  14, 15, 12,    // -X face
        16, 17, 18,  18, 19, 16,    // +Y face
        20, 21, 22,  22, 23, 20,    // -Y face
    };

    return Mesh(vertices, indices);
}

// A second unit cube, identical in size and shape to makeCube() above, but
// plain WHITE on every vertex instead of one hardcoded colour per face.
// makeCube()'s rainbow colouring exists for one reason - Phase 10/11 needed
// six visibly different faces to prove winding and culling - and that
// colouring would look wrong on a real object like a ship's hull, which is
// supposed to be one solid colour.
//
// A white mesh multiplied by 'uTint' (the colour filter shaders/basic.frag
// has applied since Phase 3) becomes exactly whatever colour uTint is. That
// is what lets ONE cube mesh be reused, unchanged, for a brown hull, a tan
// deck, a white sail, and a grey cannon mount - the colour is chosen at
// DRAW time, not baked into the geometry.
inline Mesh makeUnitCube()
{
    constexpr float h = 0.5f;
    const glm::vec3 white(1.0f, 1.0f, 1.0f);

    const std::vector<Vertex> vertices = {
        { {-h, -h,  h}, white }, { { h, -h,  h}, white }, { { h,  h,  h}, white }, { {-h,  h,  h}, white },
        { { h, -h, -h}, white }, { {-h, -h, -h}, white }, { {-h,  h, -h}, white }, { { h,  h, -h}, white },
        { { h, -h,  h}, white }, { { h, -h, -h}, white }, { { h,  h, -h}, white }, { { h,  h,  h}, white },
        { {-h, -h, -h}, white }, { {-h, -h,  h}, white }, { {-h,  h,  h}, white }, { {-h,  h, -h}, white },
        { {-h,  h,  h}, white }, { { h,  h,  h}, white }, { { h,  h, -h}, white }, { {-h,  h, -h}, white },
        { {-h, -h, -h}, white }, { { h, -h, -h}, white }, { { h, -h,  h}, white }, { {-h, -h,  h}, white },
    };

    const std::vector<unsigned int> indices = {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20,
    };

    return Mesh(vertices, indices);
}

// A unit "tapered box": the same shape as makeUnitCube(), except every
// bottom corner (y = -0.5) is pulled inward toward the centre, by
// 'bottomScale' - 1.0 gives back a plain cube, and anything smaller gives a
// shape that is full width at the top and narrower at the bottom, the same
// idea a real ship's hull uses: wide at the deck, narrow at the keel. Only
// the bottom ring moves; the top ring stays at the full, un-tapered size.
inline Mesh makeUnitTaperedBox(float bottomScale)
{
    constexpr float h = 0.5f;
    const float b = h * bottomScale;
    const glm::vec3 white(1.0f, 1.0f, 1.0f);

    const std::vector<Vertex> vertices = {
        // +Z face: bottom-left, bottom-right, top-right, top-left
        { {-b, -h,  b}, white }, { { b, -h,  b}, white }, { { h,  h,  h}, white }, { {-h,  h,  h}, white },
        // -Z face
        { { b, -h, -b}, white }, { {-b, -h, -b}, white }, { {-h,  h, -h}, white }, { { h,  h, -h}, white },
        // +X face
        { { b, -h,  b}, white }, { { b, -h, -b}, white }, { { h,  h, -h}, white }, { { h,  h,  h}, white },
        // -X face
        { {-b, -h, -b}, white }, { {-b, -h,  b}, white }, { {-h,  h,  h}, white }, { {-h,  h, -h}, white },
        // +Y face (top) - untapered, the full-size ring
        { {-h,  h,  h}, white }, { { h,  h,  h}, white }, { { h,  h, -h}, white }, { {-h,  h, -h}, white },
        // -Y face (bottom) - the tapered ring
        { {-b, -h, -b}, white }, { { b, -h, -b}, white }, { { b, -h,  b}, white }, { {-b, -h,  b}, white },
    };

    const std::vector<unsigned int> indices = {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20,
    };

    return Mesh(vertices, indices);
}

// A unit ship's hull: unlike makeUnitTaperedBox(), which keeps the same
// width along its whole length, this shape's width ALSO changes along Z -
// pinched to a point at the bow, full width at its widest point, and a
// narrower flat face (a transom) at the stern - the silhouette that
// actually reads as "a boat" instead of "a box with sloped sides". At every
// point along that length, the bottom is still pulled in toward the keel by
// 'bottomScale', the same idea makeUnitTaperedBox() uses.
inline Mesh makeUnitShipHull(float bottomScale)
{
    struct Station { float z; float halfWidth; };

    // Five cross-sections along the hull's length, from bow (z = -0.5) to
    // stern (z = +0.5). The widest point sits slightly aft of the centre,
    // and the stern stays a real width instead of coming to a point - a
    // sailing ship's hull is not symmetric front-to-back.
    constexpr int stationCount = 5;
    const Station stations[stationCount] = {
        { -0.50f, 0.00f },   // bow - a point
        { -0.25f, 0.42f },
        {  0.05f, 0.50f },   // widest point
        {  0.30f, 0.44f },
        {  0.50f, 0.34f },   // stern - a flat transom, not a point
    };

    const glm::vec3 white(1.0f, 1.0f, 1.0f);
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Four corners per station - top-left, top-right, bottom-right,
    // bottom-left - exactly like one "ring" of makeUnitTaperedBox(), just
    // repeated at each station instead of only at the top and bottom.
    for (const Station& s : stations) {
        const float topW = s.halfWidth;
        const float botW = s.halfWidth * bottomScale;
        vertices.push_back({ {-topW,  0.5f, s.z}, white });   // top-left
        vertices.push_back({ { topW,  0.5f, s.z}, white });   // top-right
        vertices.push_back({ { botW, -0.5f, s.z}, white });   // bottom-right
        vertices.push_back({ {-botW, -0.5f, s.z}, white });   // bottom-left
    }

    // Top, bottom, left, and right quads joining each station to the next.
    // At the bow, halfWidth is 0, so that station's four corners already
    // coincide in pairs - the quads touching it collapse into triangles on
    // their own, closing the bow to a point with no separate cap needed.
    for (int i = 0; i < stationCount - 1; ++i) {
        const unsigned int a = static_cast<unsigned int>(i) * 4;
        const unsigned int b = a + 4;

        indices.insert(indices.end(), { a + 0, b + 0, b + 1,  b + 1, a + 1, a + 0 }); // top
        indices.insert(indices.end(), { a + 3, b + 3, b + 2,  b + 2, a + 2, a + 3 }); // bottom
        indices.insert(indices.end(), { a + 0, a + 3, b + 3,  b + 3, b + 0, a + 0 }); // left
        indices.insert(indices.end(), { a + 1, b + 1, b + 2,  b + 2, a + 2, a + 1 }); // right
    }

    // The stern does not taper to a point, so unlike the bow it needs an
    // actual face closing it off - the last station's four corners.
    const unsigned int last = static_cast<unsigned int>(stationCount - 1) * 4;
    indices.insert(indices.end(), { last + 0, last + 1, last + 2,  last + 2, last + 3, last + 0 });

    return Mesh(vertices, indices);
}

// A unit cylinder: radius 0.5, height 1.0, standing along the Y axis,
// centred on the origin - so its two flat ends sit at y = -0.5 and y = +0.5.
// Like makeUnitCube(), every vertex is white; the colour a mast, a cannon
// barrel, or anything else built from this shape actually shows comes from
// 'uTint' at draw time, chosen fresh for each part.
//
// 'segments' is how many points make up the round cross-section - the same
// idea as a clock face made of 'segments' evenly spaced hours. More segments
// means a rounder-looking cylinder and more triangles; this project has no
// on-screen triangle-count display yet, so there is nothing to trade off
// against beyond "does it still look round" - 16 comfortably does.
inline Mesh makeCylinder(int segments = 16)
{
    constexpr float radius = 0.5f;
    constexpr float halfHeight = 0.5f;
    constexpr float twoPi = 6.28318530718f;
    const glm::vec3 white(1.0f, 1.0f, 1.0f);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // One bottom vertex and one top vertex per segment, at matching angles
    // around the circle, so vertex (2*i) and (2*i + 1) are always the bottom
    // and top of the SAME point on the rim.
    for (int i = 0; i < segments; ++i) {
        const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float x = radius * std::cos(angle);
        const float z = radius * std::sin(angle);
        vertices.push_back({ {x, -halfHeight, z}, white });
        vertices.push_back({ {x,  halfHeight, z}, white });
    }

    // The side wall: one two-triangle quad between each rim point and the
    // next one around the circle, wrapping back to point 0 after the last.
    for (int i = 0; i < segments; ++i) {
        const int next = (i + 1) % segments;
        const int bottomA = i * 2;
        const int topA = i * 2 + 1;
        const int bottomB = next * 2;
        const int topB = next * 2 + 1;

        indices.push_back(bottomA); indices.push_back(bottomB); indices.push_back(topB);
        indices.push_back(topB);    indices.push_back(topA);    indices.push_back(bottomA);
    }

    // The two flat caps: one centre vertex each, then a triangle fan out to
    // its own rim - the exact same fan idea Phase 21 (docs/PHASE_PLAN.md)
    // plans to use for this same shape.
    const unsigned int bottomCentreIndex = static_cast<unsigned int>(vertices.size());
    vertices.push_back({ {0.0f, -halfHeight, 0.0f}, white });
    const unsigned int topCentreIndex = static_cast<unsigned int>(vertices.size());
    vertices.push_back({ {0.0f, halfHeight, 0.0f}, white });

    for (int i = 0; i < segments; ++i) {
        const int next = (i + 1) % segments;
        indices.push_back(bottomCentreIndex); indices.push_back(next * 2); indices.push_back(i * 2);
        indices.push_back(topCentreIndex);    indices.push_back(i * 2 + 1); indices.push_back(next * 2 + 1);
    }

    return Mesh(vertices, indices);
}

// A unit sphere: radius 0.5, centred on the origin, built the standard
// "world globe" way - rings of latitude ('stacks', pole to pole) each split
// into 'slices' points of longitude. Also plain white, tinted at draw time
// like the shapes above.
inline Mesh makeSphere(int stacks = 12, int slices = 16)
{
    constexpr float radius = 0.5f;
    constexpr float pi = 3.14159265358979f;
    const glm::vec3 white(1.0f, 1.0f, 1.0f);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // phi sweeps from 0 (the top pole) to pi (the bottom pole); theta sweeps
    // all the way around each ring it produces.
    for (int stack = 0; stack <= stacks; ++stack) {
        const float phi = pi * static_cast<float>(stack) / static_cast<float>(stacks);
        const float ringY = radius * std::cos(phi);
        const float ringRadius = radius * std::sin(phi);

        for (int slice = 0; slice <= slices; ++slice) {
            const float theta = 2.0f * pi * static_cast<float>(slice) / static_cast<float>(slices);
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);
            vertices.push_back({ {x, ringY, z}, white });
        }
    }

    // Each ring has (slices + 1) vertices, not 'slices', because the last
    // point of each ring is a second copy of its first point - needed so the
    // ring can have its own seam instead of sharing one edge with a
    // neighbour, keeping every quad in this grid a plain, uncomplicated one.
    const int verticesPerRing = slices + 1;
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const int a = stack * verticesPerRing + slice;
            const int b = a + verticesPerRing;

            indices.push_back(a);     indices.push_back(b);     indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b);     indices.push_back(b + 1);
        }
    }

    return Mesh(vertices, indices);
}
