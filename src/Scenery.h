#pragma once

// Environment build: the SCENERY of each location - islands, mountains, cliffs, jungle, rocks, a pier, a lighthouse - built from the
// meshes the project already has. No OpenGL is used here, like src/Islands.h.
//
// NO NEW GEOMETRY. A hill is the unit sphere flattened; a trunk or a tower is the unit cylinder; a boulder, a cliff face, a plank and a
// roof are the unit cube, turned and stretched. "Procedural" here means exactly what it did for the ship: a short list of numbers and a
// few functions turn into dozens of placed parts. The whole forest on the jungle island is two meshes and a table of random numbers.
//
// A part is a PROP: which mesh, where (a matrix that already includes its size), which material, and a bounding sphere so the renderer
// can skip what is behind the camera or beyond the far plane. A prop is a LEAF of the scene - nothing hangs from it - so the project's
// rule "a frame stores no scale, scale only at the draw call" is not at risk: there is no child to inherit it. (A palm's fronds ARE
// children of the trunk's top in spirit, and are built from that point, never from the trunk's scaled matrix.)
//
// Everything is placed from fixed seeds, so the world is the same every time it is drawn. Nothing is animated here; the few things that
// move (trees swaying in the wind) are a function of the clock, applied at the draw call.
//
// The list of OBSTACLES is the land seen from above - a circle per island or cliff - so the ships can run aground on it and a cannonball
// can strike it. It is built from the same numbers as the props, so what you see is what you hit.

#include "Environment.h"
#include "Islands.h"
#include "Material.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

enum class PropMesh { SPHERE, CYLINDER, CUBE };

struct Prop {
    PropMesh mesh = PropMesh::SPHERE;
    glm::mat4 model = glm::mat4(1.0f);          // placed AND sized
    const Material* material = nullptr;
    glm::vec3 centre = glm::vec3(0.0f);         // bounding sphere, for culling
    float radius = 1.0f;
    float sway = 0.0f;                          // 0 = rigid; else the radians a gale tips it about `pivot`
    glm::vec3 pivot = glm::vec3(0.0f);
};

// The land seen from above. height is how far above the sea it rises (a cannonball below that height and inside the circle has hit it).
struct Obstacle {
    float x, z, radius, height;
};

// A light that glows at night or in fog: a lighthouse lamp, a lantern on a pier.
struct SceneryLamp {
    glm::vec3 position;
    float size;
};

// Where a treasure chest lies and how hard it is to find. BEACH: on the sand at the water's edge (the ship can reach it); CAVE: in a hollow of boulders; WRECK: on the deck
// of a sunken ship; SUMMIT: on top of a hill, too far from the sea for the ship - someone has to fly up (the free camera); UNDERWATER: on the seabed - someone has to dive.
enum class TreasureKind { BEACH, CAVE, WRECK, SUMMIT, UNDERWATER, SHIP };

struct TreasureSite {
    glm::vec3 pos = glm::vec3(0.0f);       // the chest's base (world; hull-local for TreasureKind::SHIP)
    float yaw = 0.0f;                      // which way its front faces, radians about y
    TreasureKind kind = TreasureKind::BEACH;
    bool hidden = false;                   // out of sight from the obvious approach: behind an island, inside a cave, under the sea
    int gold = 100;                        // what it is worth
    const char* name = "chest";
};

// A stream running down a mountain side: from `top` to `bottom` (world), `width` wide. Drawn as animated strands (drawWaterfalls in main.cpp), with spray at its foot.
struct Waterfall {
    glm::vec3 top, bottom;
    float width;
};

struct Scenery {
    std::vector<Prop> props;
    std::vector<Obstacle> obstacles;
    std::vector<SceneryLamp> lamps;
    std::vector<TreasureSite> treasures;
    std::vector<Waterfall> waterfalls;
};

namespace SceneryConfig {

// Keep full-size islands and treasure landmarks well apart.  Only translations are spread;
// individual trees, rocks, buildings and chests keep their authored sizes.
constexpr float POSITION_SPREAD = 1.65f;

// The camera now has a long world far plane.  This range keeps recognisable landmark silhouettes
// visible while their small detail is still controlled by the ordinary prop/LOD checks.
constexpr float CULL_DISTANCE = 230.0f;

// How far ahead of itself the enemy's steering keeps clear of land (the old single-circle clearance, now only a look-ahead margin).
constexpr float SHIP_CLEARANCE = 2.3f;

// A ship's SHAPE for collision: a chain of circles down its length, centred on its position and following its heading. The hull is 5.6 long
// and 1.44 wide (ShipConfig::DEFAULT_DIMENSIONS x ShowcaseConfig::SHIP_SCALE); four circles of radius 0.9 spaced 1.3 apart cover it from
// stern to stem with a little to spare. Change these if the ship's size changes.
constexpr int HULL_CIRCLE_COUNT = 4;
constexpr float HULL_CIRCLE_RADIUS = 0.9f;
constexpr float HULL_CIRCLE_OFFSETS[HULL_CIRCLE_COUNT] = { -2.0f, -0.7f, 0.7f, 2.0f };

// The two places the fight starts must stay open water: the player at the origin and the enemy at (9, 30).
constexpr float KEEP_CLEAR_RADIUS = 14.0f;

} // namespace SceneryConfig

// ---- a tiny deterministic random number generator ---------------------------------------------------------------------
//
// Not <random>: that library's distributions are free to differ between compilers, and the scenery must be the same everywhere. This is
// a xorshift on a hashed seed - the same seed gives the same forest, and a different seed gives a different one.
struct SceneryRng {
    unsigned int state;
    explicit SceneryRng(unsigned int seed) : state(environmentHash(seed * 2654435761u + 12345u) | 1u) {}
    float next()
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return static_cast<float>(state & 0xFFFFFFu) / 16777216.0f;
    }
    float range(float a, float b) { return a + (b - a) * next(); }
};

// ---- the building blocks ----------------------------------------------------------------------------------------------

inline void sceneryAdd(Scenery& s, PropMesh mesh, const glm::mat4& model, const Material& material,
                       const glm::vec3& centre, float radius, float sway = 0.0f, const glm::vec3& pivot = glm::vec3(0.0f))
{
    Prop p;
    p.mesh = mesh;
    p.model = model;
    p.material = &material;
    p.centre = centre;
    p.radius = radius;
    p.sway = sway;
    p.pivot = pivot;
    s.props.push_back(p);
}

inline glm::mat4 sceneryMove(const glm::vec3& p)
{
    return glm::translate(glm::mat4(1.0f), p);
}
inline glm::mat4 sceneryTurn(float angle, const glm::vec3& axis)
{
    return glm::rotate(glm::mat4(1.0f), angle, axis);
}
inline glm::mat4 sceneryStretch(const glm::vec3& s)
{
    return glm::scale(glm::mat4(1.0f), s);
}

// A sphere stretched into an ellipsoid. `size` is the three DIAMETERS. Centred where given.
inline void addEllipsoid(Scenery& s, const glm::vec3& centre, const glm::vec3& size, const Material& m,
                         float sway = 0.0f, const glm::vec3& pivot = glm::vec3(0.0f))
{
    sceneryAdd(s, PropMesh::SPHERE, sceneryMove(centre) * sceneryStretch(size), m, centre,
               0.5f * std::max(size.x, std::max(size.y, size.z)), sway, pivot);
}

// A rounded hill: a half-ellipsoid standing on the waterline (its lower half is under the sea, hidden by the sea's own surface), as the
// distant islands of Phase 46 are.
inline void addHill(Scenery& s, float seaY, float x, float z, float width, float height, const Material& m)
{
    addEllipsoid(s, glm::vec3(x, seaY, z), glm::vec3(width, 2.0f * height, width), m);
}

// A boulder or a block: the unit cube turned and stretched. The scale is on the RIGHT, as everywhere in the project.
inline void addBlock(Scenery& s, const glm::vec3& centre, const glm::vec3& size, float yaw, float tiltX, float tiltZ, const Material& m)
{
    const glm::mat4 turn = sceneryTurn(yaw, glm::vec3(0, 1, 0)) * sceneryTurn(tiltX, glm::vec3(1, 0, 0)) * sceneryTurn(tiltZ, glm::vec3(0, 0, 1));
    sceneryAdd(s, PropMesh::CUBE, sceneryMove(centre) * turn * sceneryStretch(size), m, centre,
               0.87f * std::max(size.x, std::max(size.y, size.z)));
}

// A ROCK: not one box but a heap of them. Three to five blocks, each turned a different way about all three axes and sunk into the ground by a different amount, in two
// tones, with a rounded boulder tucked in among them - so the outline is irregular and the faces catch the light at different angles, as broken stone does. `size` is
// the width of the heap. (The project's only shapes are the cube, the sphere and the cylinder; a natural rock is what you get from many of them at random.)
inline void addRockCluster(Scenery& s, const glm::vec3& base, float size, SceneryRng& rng, const Material& m)
{
    const int parts = (size < 1.1f) ? 3 : (size < 2.0f ? 4 : 5);
    for (int i = 0; i < parts; ++i) {
        const float h = size * rng.range(0.45f, 0.95f), w = size * rng.range(0.30f, 0.58f), d = size * rng.range(0.30f, 0.58f);
        const glm::vec3 at = base + glm::vec3(rng.range(-0.42f, 0.42f) * size, 0.20f * h, rng.range(-0.42f, 0.42f) * size);
        addBlock(s, at, glm::vec3(w, h, d), rng.range(0.0f, 3.14f), rng.range(-0.55f, 0.55f), rng.range(-0.55f, 0.55f), (i % 3 == 1) ? ROCK_DARK : m);
    }
    addEllipsoid(s, base + glm::vec3(rng.range(-0.25f, 0.25f) * size, 0.16f * size, rng.range(-0.25f, 0.25f) * size),
                 glm::vec3(rng.range(0.55f, 0.85f) * size, rng.range(0.30f, 0.50f) * size, rng.range(0.50f, 0.80f) * size), m);
}

// A crag: the unit cube stood on one CORNER, so its body diagonal is vertical and it ends in a point at the top and the bottom - a
// jagged spire out of a shape the project already had. The stretch is applied AFTER the turn (to the left of it) on purpose: it has to
// act along the WORLD's vertical, not along the cube's own axes. A crag is a leaf with no child to inherit the stretch.
//
// `topHeight` is how far the point reaches above the water, `width` how thick it is.
inline void addCragSpire(Scenery& s, float seaY, float x, float z, float width, float topHeight, float yaw, const Material& m)
{
    const glm::vec3 diagonal = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 axis = glm::normalize(glm::cross(diagonal, up));
    const float angle = std::acos(glm::dot(diagonal, up));
    const glm::mat4 corner = sceneryTurn(yaw, up) * sceneryTurn(angle, axis);
    // The cube's corner is 0.866 from its centre. Stretched to a height of `h`, the point is 0.866 h above the centre.
    const float h = topHeight * 0.95f;
    const float centreY = seaY + topHeight - 0.866f * h;
    sceneryAdd(s, PropMesh::CUBE, sceneryMove(glm::vec3(x, centreY, z)) * sceneryStretch(glm::vec3(width, h, width)) * corner, m,
               glm::vec3(x, seaY + 0.5f * topHeight, z), std::max(0.5f * topHeight, width));
}

// A crag is now a GROUP: the tall spire, a shorter one leaning against it, and a heap of broken stone round its foot, so the silhouette has more than one summit and
// a rubble skirt instead of one perfect point standing on the water. The random numbers come from the crag's own position, so the same crag is the same everywhere.
inline void addCrag(Scenery& s, float seaY, float x, float z, float width, float topHeight, float yaw, const Material& m)
{
    SceneryRng rng(static_cast<unsigned int>(std::fabs(x * 73.0f) + std::fabs(z * 131.0f) + topHeight * 17.0f) + 5u);
    addCragSpire(s, seaY, x, z, width, topHeight, yaw, m);
    if (topHeight > 2.0f) {
        const float a = rng.range(0.0f, 6.28f), d = 0.42f * width;
        addCragSpire(s, seaY - 0.2f, x + std::cos(a) * d, z + std::sin(a) * d, width * rng.range(0.50f, 0.68f), topHeight * rng.range(0.50f, 0.74f), yaw + rng.range(-0.9f, 0.9f), (&m == &ROCK_DARK) ? CLIFF_ROCK : ROCK_DARK);
        const float b = a + rng.range(2.0f, 4.0f);
        addCragSpire(s, seaY - 0.2f, x + std::cos(b) * d * 1.1f, z + std::sin(b) * d * 1.1f, width * rng.range(0.38f, 0.5f), topHeight * rng.range(0.30f, 0.5f), yaw + rng.range(-1.2f, 1.2f), m);
    }
    addRockCluster(s, glm::vec3(x + rng.range(-0.3f, 0.3f) * width, seaY - 0.15f, z + rng.range(-0.3f, 0.3f) * width), 0.55f * width + 0.35f * topHeight * 0.3f, rng, m);
}

// The height of the ground at horizontal distance d from the middle of a half-ellipsoid hill with horizontal RADIUS a and height b.
inline float ellipsoidHeight(float d, float a, float b)
{
    if (d >= a)
        return 0.0f;
    return b * std::sqrt(1.0f - (d / a) * (d / a));
}

inline void addObstacle(Scenery& s, float x, float z, float radius, float height)
{
    s.obstacles.push_back({ x, z, radius, height });
}

// ---- vegetation -------------------------------------------------------------------------------------------------------

// A palm: a leaning trunk and a crown of drooping fronds. The fronds are built from the TOP OF THE TRUNK (a point), never from the
// trunk's scaled matrix - the parent-scale-leak rule of Stage D, holding in the scenery too. The crown sways about that point.
inline void addPalm(Scenery& s, const glm::vec3& base, float height, float yaw, float lean, SceneryRng& rng)
{
    const glm::mat4 root = sceneryMove(base) * sceneryTurn(yaw, glm::vec3(0, 1, 0)) * sceneryTurn(lean, glm::vec3(1, 0, 0));
    const glm::vec3 top = glm::vec3(root * glm::vec4(0.0f, height, 0.0f, 1.0f));

    sceneryAdd(s, PropMesh::CYLINDER, root * sceneryMove(glm::vec3(0.0f, 0.5f * height, 0.0f)) * sceneryStretch(glm::vec3(0.30f, height, 0.30f)),
               TREE_TRUNK, base + glm::vec3(0.0f, 0.5f * height, 0.0f), 0.5f * height + 0.3f);

    constexpr int FRONDS = 5;
    for (int i = 0; i < FRONDS; ++i) {
        const float fy = yaw + (static_cast<float>(i) + rng.range(-0.2f, 0.2f)) * 6.2831853f / FRONDS;
        const float droop = rng.range(0.40f, 0.75f);
        const float len = rng.range(1.9f, 2.6f);
        const glm::mat4 frond = sceneryMove(top) * sceneryTurn(fy, glm::vec3(0, 1, 0)) * sceneryTurn(droop, glm::vec3(1, 0, 0))
                              * sceneryMove(glm::vec3(0.0f, 0.0f, 0.5f * len)) * sceneryStretch(glm::vec3(0.62f, 0.07f, len));
        sceneryAdd(s, PropMesh::SPHERE, frond, PALM_FROND, top, 0.6f * len, 0.05f, top);
    }
    addEllipsoid(s, top + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(0.42f), TREE_TRUNK);
}

// A broadleaf jungle tree: a trunk and two overlapping blobs of foliage.
inline void addBroadleaf(Scenery& s, const glm::vec3& base, float height, float crown, const Material& leaves, SceneryRng& rng)
{
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(base + glm::vec3(0.0f, 0.5f * height, 0.0f)) * sceneryStretch(glm::vec3(0.34f, height, 0.34f)),
               TREE_TRUNK, base + glm::vec3(0.0f, 0.5f * height, 0.0f), 0.5f * height + 0.2f);
    const glm::vec3 top = base + glm::vec3(0.0f, height, 0.0f);
    addEllipsoid(s, top, glm::vec3(2.0f * crown, 1.5f * crown, 2.0f * crown), leaves, 0.025f, base);
    addEllipsoid(s, top + glm::vec3(rng.range(-0.5f, 0.5f) * crown, 0.55f * crown, rng.range(-0.5f, 0.5f) * crown),
                 glm::vec3(1.3f * crown, 1.05f * crown, 1.3f * crown), JUNGLE_LIGHT, 0.03f, base);
}

// A pine: a short trunk under three stacked, narrowing tiers of needles - the tallest and thinnest on top.
inline void addPine(Scenery& s, const glm::vec3& base, float height)
{
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(base + glm::vec3(0.0f, 0.2f * height, 0.0f)) * sceneryStretch(glm::vec3(0.22f, 0.4f * height, 0.22f)),
               TREE_TRUNK, base + glm::vec3(0.0f, 0.2f * height, 0.0f), 0.3f * height);
    for (int i = 0; i < 3; ++i) {
        const float w = height * 0.50f * (1.0f - 0.27f * static_cast<float>(i));
        const float cy = height * (0.30f + 0.24f * static_cast<float>(i));
        addEllipsoid(s, base + glm::vec3(0.0f, cy, 0.0f), glm::vec3(w, height * 0.40f, w), PINE_GREEN, 0.02f, base);
    }
}

// A bush: three or four overlapping flattened blobs of leaf, no trunk. Low and wide, for the edge of the jungle and the foot of a slope.
inline void addBush(Scenery& s, const glm::vec3& base, float size, const Material& leaves, SceneryRng& rng)
{
    const int blobs = 3 + static_cast<int>(rng.next() * 1.99f);
    for (int i = 0; i < blobs; ++i) {
        const float a = rng.range(0.0f, 6.2832f), d = rng.range(0.0f, 0.40f) * size, k = rng.range(0.55f, 0.95f);
        addEllipsoid(s, base + glm::vec3(std::cos(a) * d, 0.28f * size * k, std::sin(a) * d), glm::vec3(0.95f * size * k, 0.62f * size * k, 0.95f * size * k),
                     (i == 1) ? JUNGLE_LIGHT : leaves, 0.04f, base);
    }
}

// A fern or broad-leaved plant: six long blades fanned out from one point, each leaning up and away and drooping at its tip.
inline void addFern(Scenery& s, const glm::vec3& base, float size, float yaw, SceneryRng& rng)
{
    for (int i = 0; i < 6; ++i) {
        const float fy = yaw + (static_cast<float>(i) + rng.range(-0.25f, 0.25f)) * 6.2831853f / 6.0f;
        const float lean = rng.range(0.55f, 1.05f), len = size * rng.range(0.8f, 1.2f);
        const glm::mat4 blade = sceneryMove(base) * sceneryTurn(fy, glm::vec3(0, 1, 0)) * sceneryTurn(lean, glm::vec3(1, 0, 0))
                              * sceneryMove(glm::vec3(0.0f, 0.0f, 0.5f * len)) * sceneryStretch(glm::vec3(0.22f * size, 0.04f * size, len));
        sceneryAdd(s, PropMesh::SPHERE, blade, (i % 2 == 0) ? JUNGLE_LIGHT : JUNGLE_MID, base, 0.6f * len, 0.07f, base);
    }
}

// A banana-leaf plant is a fern on a short stalk, bigger: used among the trees.
inline void addBroadLeafPlant(Scenery& s, const glm::vec3& base, float height, SceneryRng& rng)
{
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(base + glm::vec3(0.0f, 0.5f * height, 0.0f)) * sceneryStretch(glm::vec3(0.14f, height, 0.14f)),
               TREE_TRUNK, base + glm::vec3(0.0f, 0.5f * height, 0.0f), 0.5f * height + 0.2f);
    addFern(s, base + glm::vec3(0.0f, height, 0.0f), 1.5f * rng.range(0.8f, 1.2f), rng.range(0.0f, 6.28f), rng);
}

// A rock outcrop: a boulder, sunk a little into the ground.
inline void addBoulder(Scenery& s, const glm::vec3& base, float size, SceneryRng& rng, const Material& m = CLIFF_ROCK)
{
    addRockCluster(s, base, size, rng, m);
}

// A lighthouse: a pale tower, a red band, a lamp. Same parts as the open ocean's lighthouse of the playable build.
inline void addLighthouse(Scenery& s, float x, float baseY, float z, float height)
{
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(x, baseY + 0.5f * height, z)) * sceneryStretch(glm::vec3(1.4f, height, 1.4f)),
               SAILCLOTH, glm::vec3(x, baseY + 0.5f * height, z), 0.5f * height + 1.0f);
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(x, baseY + 0.62f * height, z)) * sceneryStretch(glm::vec3(1.48f, 0.16f * height, 1.48f)),
               FLAG_CLOTH, glm::vec3(x, baseY + 0.62f * height, z), 1.5f);
    sceneryAdd(s, PropMesh::SPHERE, sceneryMove(glm::vec3(x, baseY + height + 0.35f, z)) * sceneryStretch(glm::vec3(1.1f)),
               LIGHTHOUSE_LAMP, glm::vec3(x, baseY + height + 0.35f, z), 0.6f);
    s.lamps.push_back({ glm::vec3(x, baseY + height + 0.35f, z), 3.2f });
}

// ---- islands ----------------------------------------------------------------------------------------------------------

struct TropicalIsland {
    float x, z;
    float radius;      // of the beach
    float height;      // of the jungle hill
    int broadleaf;     // trees in the jungle
    int palms;         // trees on the beach
    int boulders;
    unsigned int seed;
};

// A jungle island: a skirt of sand, a green hill with three lumps on it, a dense jungle of broadleaf trees standing on the hill's
// surface, a ring of palms on the beach and boulders at the water's edge.
inline void addTropicalIsland(Scenery& s, float seaY, const TropicalIsland& I)
{
    SceneryRng rng(I.seed);
    const float hillRadius = I.radius * 0.80f;

    addHill(s, seaY, I.x, I.z, 2.0f * I.radius * 1.12f, 0.55f, SAND);
    addHill(s, seaY, I.x, I.z, 2.0f * hillRadius, I.height, JUNGLE_MID);

    // Three lumps so the skyline is not one dome.
    struct Lump { float x, z, a, b; };
    Lump lumps[3];
    for (int k = 0; k < 3; ++k) {
        const float ang = rng.range(0.0f, 6.28f), d = I.radius * rng.range(0.25f, 0.45f);
        lumps[k] = { I.x + std::cos(ang) * d, I.z + std::sin(ang) * d, hillRadius * rng.range(0.40f, 0.55f), I.height * rng.range(0.55f, 0.85f) };
        addHill(s, seaY, lumps[k].x, lumps[k].z, 2.0f * lumps[k].a, lumps[k].b, k == 1 ? JUNGLE_DARK : JUNGLE_LIGHT);
    }

    // The ground's height at a point: the tallest of the hill and its lumps.
    auto groundAt = [&](float px, float pz) {
        float h = ellipsoidHeight(std::hypot(px - I.x, pz - I.z), hillRadius, I.height);
        for (const Lump& l : lumps)
            h = std::max(h, ellipsoidHeight(std::hypot(px - l.x, pz - l.z), l.a, l.b));
        return h;
    };

    const Material* greens[3] = { &JUNGLE_DARK, &JUNGLE_MID, &JUNGLE_LIGHT };
    for (int i = 0; i < I.broadleaf; ++i) {
        const float ang = rng.range(0.0f, 6.2832f), d = hillRadius * 0.93f * std::sqrt(rng.next());
        const float px = I.x + std::cos(ang) * d, pz = I.z + std::sin(ang) * d;
        const float h = groundAt(px, pz);
        if (h < 0.5f) continue;                                   // not on the slope's foot, where the sand is
        const float tall = (rng.next() < 0.18f) ? rng.range(3.0f, 4.2f) : rng.range(1.3f, 2.7f);           // now and then an emergent giant
        addBroadleaf(s, glm::vec3(px, seaY + h - 0.25f, pz), tall, rng.range(1.0f, 1.9f) * (0.8f + 0.12f * tall), *greens[static_cast<int>(rng.next() * 2.99f)], rng);
    }
    // The canopy: a skin of small blobs of leaf lying on the hill, in several greens, so the hill is a forest and not a smooth green dome with trees stuck on it.
    for (int i = 0; i < I.broadleaf * 2; ++i) {
        const float ang = rng.range(0.0f, 6.2832f), d = hillRadius * 0.97f * std::sqrt(rng.next());
        const float px = I.x + std::cos(ang) * d, pz = I.z + std::sin(ang) * d;
        const float h = groundAt(px, pz);
        if (h < 0.35f) continue;
        const float sz = rng.range(1.1f, 2.2f);
        addEllipsoid(s, glm::vec3(px, seaY + h + 0.15f * sz, pz), glm::vec3(sz, 0.62f * sz, sz), *greens[static_cast<int>(rng.next() * 2.99f)], 0.02f, glm::vec3(I.x, seaY, I.z));
    }
    // Undergrowth: ferns and broad-leaved plants between the trees on the slope, and bushes where the jungle meets the sand.
    for (int i = 0; i < I.broadleaf / 2; ++i) {
        const float ang = rng.range(0.0f, 6.2832f), d = hillRadius * rng.range(0.55f, 1.05f);
        const float px = I.x + std::cos(ang) * d, pz = I.z + std::sin(ang) * d;
        const float h = groundAt(px, pz);
        if (h < 0.15f) { addBush(s, glm::vec3(px, seaY + 0.25f, pz), rng.range(0.9f, 1.5f), JUNGLE_DARK, rng); continue; }
        if (i % 3 == 0) addBroadLeafPlant(s, glm::vec3(px, seaY + h - 0.15f, pz), rng.range(0.5f, 1.0f), rng);
        else addFern(s, glm::vec3(px, seaY + h - 0.05f, pz), rng.range(0.8f, 1.5f), rng.range(0.0f, 6.28f), rng);
    }
    for (int i = 0; i < I.palms; ++i) {
        const float ang = (static_cast<float>(i) + rng.range(-0.3f, 0.3f)) * 6.2832f / static_cast<float>(std::max(1, I.palms));
        const float d = I.radius * rng.range(0.80f, 1.02f);
        addPalm(s, glm::vec3(I.x + std::cos(ang) * d, seaY + 0.35f, I.z + std::sin(ang) * d), rng.range(3.0f, 4.6f),
                ang + rng.range(-0.6f, 0.6f), rng.range(0.10f, 0.32f), rng);
    }
    for (int i = 0; i < I.boulders; ++i) {
        const float ang = rng.range(0.0f, 6.2832f), d = I.radius * rng.range(1.02f, 1.16f);
        addBoulder(s, glm::vec3(I.x + std::cos(ang) * d, seaY - 0.1f, I.z + std::sin(ang) * d), rng.range(0.9f, 1.8f), rng);
    }
    addObstacle(s, I.x, I.z, I.radius * 1.05f, I.height);
}

struct RockMass {
    float x, z;
    float radius;      // of its foot
    float height;      // of its tallest crag
    int crags;         // spires round it
    int boulders;
    bool grass;        // a green cap and a few stunted trees on top
    unsigned int seed;
};

// A rocky island: one tall crag in the middle, a ring of shorter ones, a skirt of boulders, and (optionally) a cap of grass.
inline void addRockMass(Scenery& s, float seaY, const RockMass& R)
{
    SceneryRng rng(R.seed);
    // The foot: a squat dark mound the crags rise out of, so they do not stand on the water like pegs.
    addHill(s, seaY, R.x, R.z, 2.0f * R.radius * 0.9f, R.height * 0.30f, ROCK_DARK);
    addCrag(s, seaY, R.x, R.z, R.radius * 0.95f, R.height, rng.range(0.0f, 1.5f), CLIFF_ROCK);
    for (int i = 0; i < R.crags; ++i) {
        const float ang = (static_cast<float>(i) + rng.range(-0.25f, 0.25f)) * 6.2832f / static_cast<float>(std::max(1, R.crags));
        const float d = R.radius * rng.range(0.55f, 0.85f);
        addCrag(s, seaY, R.x + std::cos(ang) * d, R.z + std::sin(ang) * d, R.radius * rng.range(0.45f, 0.65f),
                R.height * rng.range(0.40f, 0.78f), rng.range(0.0f, 1.5f), (i % 3 == 0) ? ROCK_DARK : CLIFF_ROCK);
    }
    for (int i = 0; i < R.boulders; ++i) {
        const float ang = rng.range(0.0f, 6.2832f), d = R.radius * rng.range(0.75f, 1.15f);
        addBoulder(s, glm::vec3(R.x + std::cos(ang) * d, seaY - 0.15f, R.z + std::sin(ang) * d), rng.range(0.9f, 2.0f) * (0.5f + R.radius * 0.07f), rng,
                   (i % 2 == 0) ? CLIFF_ROCK : ROCK_DARK);
    }
    if (R.grass || R.radius < 8.5f) {                                  // a beach on the sheltered side: a crescent of sand with a few stones and a palm
        const float ba = rng.range(0.0f, 6.2832f), bd = R.radius * 0.98f;
        const glm::vec3 bc(R.x + std::cos(ba) * bd, seaY + 0.1f, R.z + std::sin(ba) * bd);
        addEllipsoid(s, bc, glm::vec3(R.radius * 0.75f, 0.9f, R.radius * 0.55f), SAND);
        addRockCluster(s, bc + glm::vec3(0.5f, 0.2f, 0.4f), 0.8f, rng, CLIFF_ROCK);
        if (R.radius > 7.0f)
            addPalm(s, bc + glm::vec3(-0.6f, 0.35f, 0.2f), rng.range(2.8f, 3.8f), rng.range(0.0f, 6.28f), 0.22f, rng);
    }
    if (R.grass) {
        const float capY = seaY + R.height * 0.30f * 0.9f;
        addEllipsoid(s, glm::vec3(R.x, capY, R.z), glm::vec3(R.radius * 1.15f, 0.9f, R.radius * 1.15f), DUNE_GRASS);
        for (int i = 0; i < 3; ++i) {
            const float ang = rng.range(0.0f, 6.2832f), d = R.radius * rng.range(0.1f, 0.4f);
            addBroadleaf(s, glm::vec3(R.x + std::cos(ang) * d, capY + 0.2f, R.z + std::sin(ang) * d), rng.range(1.0f, 1.6f), rng.range(0.7f, 1.0f),
                         JUNGLE_DARK, rng);
        }
    }
    addObstacle(s, R.x, R.z, R.radius * 1.05f, R.height * 0.8f);
}

// A mountain with a shape: a wide forested foot, three tiers of bare rock narrowing to a jagged crown, spurs running off its flanks, outcrops of broken stone on the slopes,
// snow on the summit of the tall ones, and (if tall) a stream running down the side that faces the sea. Everything but the proportions comes from the mountain's own position,
// so no two are alike. `rock` is the colour of the upper rock; the foot is green.
inline void addMountain(Scenery& s, float seaY, float x, float z, float width, float height, const Material& rock)
{
    SceneryRng rng(static_cast<unsigned int>(std::fabs(x * 37.0f) + std::fabs(z * 59.0f) + width * 3.0f) + 11u);
    static const float W[3] = { 1.00f, 0.62f, 0.34f };      // width of each tier, as a share of the base
    static const float C[3] = { 0.00f, 0.30f, 0.57f };      // height of each tier's centre
    static const float V[3] = { 0.62f, 0.50f, 0.43f };      // its vertical radius
    for (int i = 0; i < 3; ++i) {
        const float jitter = (i == 1) ? 0.06f * width : (i == 2 ? -0.04f * width : 0.0f);
        const Material& m = (i == 0) ? (height > 20.0f ? JUNGLE_DARK : HILL_GREEN) : rock;           // the foot is forest or meadow, the upper tiers are stone
        addEllipsoid(s, glm::vec3(x + jitter, seaY + C[i] * height, z), glm::vec3(W[i] * width, 2.0f * V[i] * height, W[i] * width), m);
    }
    // Spurs: shoulders of rock off the flanks, each a smaller ellipsoid at its own height, so the skyline steps down in ridges.
    const int spurs = 3 + static_cast<int>(rng.next() * 2.99f);
    for (int k = 0; k < spurs; ++k) {
        const float a = rng.range(0.0f, 6.2832f), d = rng.range(0.22f, 0.36f) * width;
        const float sw = rng.range(0.38f, 0.58f) * width, sh = rng.range(0.34f, 0.60f) * height;
        addEllipsoid(s, glm::vec3(x + std::cos(a) * d, seaY + 0.10f * sh, z + std::sin(a) * d), glm::vec3(sw, 1.9f * sh, sw * rng.range(0.7f, 1.0f)), (k % 2) ? rock : CLIFF_ROCK);
    }
    // The crown: instead of a smooth dome, a jagged group of spires on the top tier.
    const float crownBase = 0.70f * height;
    addCragSpire(s, seaY + crownBase, x - 0.03f * width, z, 0.22f * width, 0.36f * height, rng.range(0.0f, 1.5f), ROCK_DARK);
    addCragSpire(s, seaY + 0.66f * height, x + 0.08f * width, z + 0.05f * width, 0.14f * width, 0.26f * height, rng.range(0.0f, 1.5f), rock);
    addCragSpire(s, seaY + 0.64f * height, x - 0.10f * width, z - 0.06f * width, 0.12f * width, 0.22f * height, rng.range(0.0f, 1.5f), CLIFF_ROCK);
    if (height >= 20.0f) {
        addEllipsoid(s, glm::vec3(x - 0.03f * width, seaY + 0.90f * height, z), glm::vec3(0.16f * width, 0.20f * height, 0.16f * width), SNOW);
        addEllipsoid(s, glm::vec3(x + 0.08f * width, seaY + 0.80f * height, z + 0.05f * width), glm::vec3(0.09f * width, 0.12f * height, 0.09f * width), SNOW);
    }
    // Broken stone on the slopes, and trees and bushes on the green foot.
    for (int k = 0; k < 6; ++k) {
        const float a = rng.range(0.0f, 6.2832f), d = rng.range(0.20f, 0.42f) * width;
        const float h = V[0] * height * std::sqrt(std::max(0.0f, 1.0f - (2.0f * d / width) * (2.0f * d / width)));
        addRockCluster(s, glm::vec3(x + std::cos(a) * d, seaY + h, z + std::sin(a) * d), rng.range(1.3f, 2.6f), rng, (k % 2) ? ROCK_DARK : CLIFF_ROCK);
    }
    // The stream: down the flank that faces the middle of the map, from the shoulder to the sea.
    if (height >= 22.0f) {
        const float away = std::atan2(-z, -x);
        const float a = away + rng.range(-0.35f, 0.35f);
        const auto onFoot = [&](float d) { return V[0] * height * std::sqrt(std::max(0.0f, 1.0f - (2.0f * d / width) * (2.0f * d / width))); };
        const float d0 = 0.385f * width, d1 = 0.4975f * width;
        Waterfall w;
        w.top = glm::vec3(x + std::cos(a) * (d0 + 0.5f), seaY + onFoot(d0) + 0.3f, z + std::sin(a) * (d0 + 0.5f));
        w.bottom = glm::vec3(x + std::cos(a) * (d1 + 0.5f), seaY + onFoot(d1) + 0.1f, z + std::sin(a) * (d1 + 0.5f));
        w.width = 1.1f + 0.015f * width;
        s.waterfalls.push_back(w);
    }
    addObstacle(s, x, z, 0.40f * width, height);
}

// A sea cliff: three blocks of rock of different heights, turned a little against each other like a stack of broken slabs, under a cap of
// grass, with boulders at its foot. `yaw` turns the whole cliff; width and depth are the footprint of its widest block.
inline void addCliff(Scenery& s, float seaY, float x, float z, float width, float depth, float height, float yaw, SceneryRng& rng)
{
    const float c = std::cos(yaw), sn = std::sin(yaw);
    const auto place = [&](float ox, float oz) { return glm::vec2(x + c * ox + sn * oz, z - sn * ox + c * oz); };
    // The face: a ragged wall of leaning blocks side by side, of different heights, each turned a little against its neighbours; the tallest are towards the back.
    constexpr int BLOCKS = 9;
    for (int i = 0; i < BLOCKS; ++i) {
        const float u = (static_cast<float>(i) + 0.5f) / BLOCKS - 0.5f;                       // -0.5 .. 0.5 along the cliff
        const float back = rng.range(-0.30f, 0.25f) * depth;
        const float h = height * rng.range(0.50f, 1.0f) * (0.75f + 0.5f * (back / depth + 0.3f));
        const glm::vec2 p = place(u * width, back);
        addBlock(s, glm::vec3(p.x, seaY + 0.5f * h - 0.6f, p.y), glm::vec3(width / BLOCKS * rng.range(1.3f, 1.9f), h + 1.2f, depth * rng.range(0.45f, 0.8f)),
                 yaw + rng.range(-0.6f, 0.6f), rng.range(-0.28f, 0.28f), rng.range(-0.28f, 0.28f), (i % 3 == 1) ? ROCK_DARK : CLIFF_ROCK);
    }
    // Ledges and a pale band of strata: thin slabs jutting from the face at two heights.
    for (int k = 0; k < 4; ++k) {
        const glm::vec2 p = place(rng.range(-0.45f, 0.45f) * width, rng.range(0.3f, 0.5f) * depth);
        addBlock(s, glm::vec3(p.x, seaY + height * rng.range(0.25f, 0.65f), p.y), glm::vec3(width * 0.22f, 0.45f, depth * 0.42f), yaw + rng.range(-0.5f, 0.5f), rng.range(-0.1f, 0.1f), rng.range(-0.1f, 0.1f), CLIFF_ROCK);
    }
    // The top: grass over the cliff, with a few stunted trees and bushes; rubble at the foot.
    const glm::vec2 top = place(0.05f * width, -0.1f * depth);
    addEllipsoid(s, glm::vec3(top.x, seaY + height * 0.92f, top.y), glm::vec3(width * 0.80f, 2.2f, depth * 0.95f), HILL_GREEN);
    for (int i = 0; i < 3; ++i) {
        const glm::vec2 p = place(rng.range(-0.35f, 0.35f) * width, rng.range(-0.3f, 0.2f) * depth);
        addBush(s, glm::vec3(p.x, seaY + height * 0.97f, p.y), rng.range(0.9f, 1.5f), PINE_GREEN, rng);
    }
    for (int i = 0; i < 4; ++i) {
        const float off = rng.range(-0.5f, 0.5f) * width;
        addBoulder(s, glm::vec3(x + c * off + sn * depth * 0.6f, seaY - 0.1f, z - sn * off + c * depth * 0.6f), rng.range(1.0f, 2.0f), rng);
    }
    addObstacle(s, x, z, 0.5f * std::max(width, depth) + 0.8f, height);
}

// ---- the five locations --------------------------------------------------------------------------------------------------

// The open ocean is the showcase as it was: the island table of Phase 46, the buoys, sea stacks, lighthouse and clouds that the playable
// build draws. Those stay where they are drawn (renderScene and drawScenery); the only thing built here is the land seen from above, so
// the ships can run aground on the same islands the table draws.
// One of the open ocean's islands (the table of src/Islands.h), now built from the same parts as the others: a rock stack becomes a crag group; a low hill becomes a sandy
// islet with a grassy back, palms, bushes and a ring of stones; a higher one a green island with lumps, trees on top and a beach.
inline void addOceanIsland(Scenery& s, float seaY, const Island& I, unsigned int seed)
{
    SceneryRng rng(seed);
    const glm::vec2 o = islandOffsetFromShip(I);
    const float x = o.x, z = o.y, W = I.width, H = I.height;
    if (I.kind == IslandKind::ROCK) {
        addCrag(s, seaY, x, z, std::max(1.4f * W, 1.8f), I.height + 0.4f, rng.range(0.0f, 1.5f), CLIFF_ROCK);
        return;
    }
    addHill(s, seaY, x, z, 1.14f * W, std::min(0.55f, 0.4f * H), SAND);
    addHill(s, seaY, x, z, 0.80f * W, H, (H < 1.5f) ? DUNE_GRASS : HILL_GREEN);
    if (H >= 2.0f)
        for (int k = 0; k < 2; ++k) {
            const float a = rng.range(0.0f, 6.28f), d = 0.18f * W;
            addHill(s, seaY, x + std::cos(a) * d, z + std::sin(a) * d, 0.40f * W, H * rng.range(0.65f, 0.85f), k ? JUNGLE_MID : JUNGLE_LIGHT);
        }
    const float top = 0.5f * 0.80f * W;
    const int palms = std::max(1, static_cast<int>(W / 5.0f));
    for (int i = 0; i < palms; ++i) {
        const float a = (static_cast<float>(i) + rng.range(-0.3f, 0.3f)) * 6.2832f / static_cast<float>(palms), d = W * rng.range(0.34f, 0.50f);
        addPalm(s, glm::vec3(x + std::cos(a) * d, seaY + 0.28f, z + std::sin(a) * d), rng.range(2.4f, 3.8f), a + rng.range(-0.6f, 0.6f), rng.range(0.10f, 0.30f), rng);
    }
    for (int i = 0; i < std::max(2, static_cast<int>(W / 3.5f)); ++i) {
        const float a = rng.range(0.0f, 6.2832f), d = W * rng.range(0.15f, 0.42f);
        const float h = ellipsoidHeight(d, top, H);
        addBush(s, glm::vec3(x + std::cos(a) * d, seaY + h - 0.1f, z + std::sin(a) * d), rng.range(0.8f, 1.4f), (i % 2) ? JUNGLE_DARK : JUNGLE_MID, rng);
    }
    if (H >= 2.4f)
        for (int i = 0; i < 3; ++i) {
            const float a = rng.range(0.0f, 6.2832f), d = top * rng.range(0.1f, 0.45f);
            addBroadleaf(s, glm::vec3(x + std::cos(a) * d, seaY + ellipsoidHeight(d, top, H) - 0.2f, z + std::sin(a) * d), rng.range(1.2f, 2.0f), rng.range(0.8f, 1.2f), JUNGLE_DARK, rng);
        }
    for (int i = 0; i < std::max(2, static_cast<int>(W / 4.0f)); ++i) {
        const float a = rng.range(0.0f, 6.2832f), d = W * rng.range(0.52f, 0.60f);
        addRockCluster(s, glm::vec3(x + std::cos(a) * d, seaY - 0.1f, z + std::sin(a) * d), rng.range(0.9f, 1.8f), rng, CLIFF_ROCK);
    }
}

inline void buildOpenOcean(Scenery& s, float seaY)
{
    for (int i = 0; i < IslandConfig::COUNT; ++i) {
        const Island& island = IslandConfig::ISLANDS[i];
        const glm::vec2 o = islandOffsetFromShip(island);
        addOceanIsland(s, seaY, island, 600u + static_cast<unsigned int>(i) * 13u);
        addObstacle(s, o.x, o.y, islandRadius(island), island.height);
    }
}

// Tropical ocean with dense jungle islands.
inline void buildJungleIslands(Scenery& s, float seaY)
{
    // x, z, beach radius, hill height, jungle trees, beach palms, boulders, seed
    const TropicalIsland ISLANDS[] = {
        {  33.0f,  41.0f, 15.0f, 5.4f, 48, 11, 8, 11u },     // the big one, ahead and to the right
        { -30.0f,  36.0f, 12.0f, 4.4f, 30,  8, 6, 23u },     // ahead and to the left
        {  42.0f, -10.0f, 10.0f, 4.0f, 24,  7, 5, 37u },
        { -42.0f, -14.0f, 13.0f, 5.0f, 36, 10, 7, 41u },
        {  -6.0f, -54.0f, 15.0f, 5.2f, 42, 10, 8, 53u },     // astern
        {  50.0f,  58.0f, 12.0f, 4.6f, 28,  8, 5, 67u },
        { -18.0f,  68.0f, 11.0f, 4.0f, 26,  7, 5, 71u },     // far ahead
    };
    for (const TropicalIsland& I : ISLANDS)
        addTropicalIsland(s, seaY, I);

    // Far hills on the horizon behind the islands: big, low, rounded, each a different green, so the jungle goes back in layers into the haze.
    static const float FAR[][4] = { { -70.0f, 86.0f, 34.0f, 9.0f }, { -34.0f, 96.0f, 40.0f, 12.0f }, { 14.0f, 98.0f, 46.0f, 10.0f }, { 58.0f, 92.0f, 38.0f, 13.0f },
                                    { 88.0f, 60.0f, 36.0f, 9.0f }, { -86.0f, 52.0f, 34.0f, 11.0f }, { -90.0f, -4.0f, 38.0f, 8.0f }, { 92.0f, -20.0f, 40.0f, 10.0f } };
    {
        SceneryRng farRng(173u);
        int k = 0;
        for (const auto& f : FAR) {
            addHill(s, seaY, f[0], f[1], f[2], f[3], (k % 2) ? JUNGLE_MID : JUNGLE_DARK);
            addObstacle(s, f[0], f[1], 0.46f * f[2], f[3]);
            for (int j = 0; j < 4; ++j)
                addHill(s, seaY, f[0] + farRng.range(-0.3f, 0.3f) * f[2], f[1] + farRng.range(-0.2f, 0.2f) * f[2], f[2] * farRng.range(0.35f, 0.55f), f[3] * farRng.range(0.7f, 1.15f), (j % 2) ? JUNGLE_LIGHT : JUNGLE_MID);
            ++k;
        }
    }

    // Sandbars: a low bank of sand with two or three palms, nothing more.
    SceneryRng rng(97u);
    static const float BARS[][2] = { { 8.0f, 62.0f }, { -14.0f, 18.0f }, { 22.0f, -34.0f }, { -30.0f, -42.0f }, { 14.0f, 100.0f }, { 70.0f, 20.0f } };
    for (const auto& b : BARS) {
        addHill(s, seaY, b[0], b[1], 8.0f, 0.5f, SAND);
        for (int i = 0; i < 2; ++i)
            addPalm(s, glm::vec3(b[0] + rng.range(-1.5f, 1.5f), seaY + 0.3f, b[1] + rng.range(-1.5f, 1.5f)), rng.range(3.0f, 4.0f), rng.range(0.0f, 6.0f), 0.2f, rng);
        addObstacle(s, b[0], b[1], 4.2f, 0.5f);
    }
}

// Mountain or hilly coastal region with cliffs and rocks.
inline void buildMountainCoast(Scenery& s, float seaY)
{
    // x, z, width, height
    static const float MOUNTAINS[][4] = {
        { -44.0f, -26.0f, 30.0f, 17.0f }, { -48.0f,  -4.0f, 36.0f, 26.0f }, { -42.0f,  18.0f, 30.0f, 20.0f }, { -46.0f,  40.0f, 34.0f, 29.0f },
        { -38.0f,  60.0f, 28.0f, 16.0f }, { -26.0f, -46.0f, 26.0f, 14.0f }, { -12.0f,  74.0f, 36.0f, 23.0f }, {  18.0f,  80.0f, 40.0f, 31.0f },
        {  46.0f,  70.0f, 34.0f, 19.0f }, {  62.0f,  18.0f, 30.0f, 12.0f }, {  60.0f, -18.0f, 30.0f, 15.0f }, {  -8.0f, -68.0f, 40.0f, 21.0f },
    };
    for (const auto& m : MOUNTAINS)
        addMountain(s, seaY, m[0], m[1], m[2], m[3], (m[3] > 22.0f) ? ROCK_DARK : CLIFF_ROCK);

    // Foothills, green, with pines on them. x, z, width, height
    static const float FOOTHILLS[][4] = {
        { -34.0f, -18.0f, 18.0f, 5.0f }, { -38.0f,   8.0f, 20.0f, 6.5f }, { -33.0f,  34.0f, 18.0f, 5.0f }, { -30.0f,  56.0f, 16.0f, 4.5f },
        { -22.0f,  76.0f, 20.0f, 6.0f }, {   8.0f,  72.0f, 24.0f, 5.0f }, {  34.0f,  70.0f, 18.0f, 4.0f }, {  50.0f,  30.0f, 16.0f, 4.0f },
    };
    SceneryRng rng(131u);
    for (const auto& f : FOOTHILLS) {
        addHill(s, seaY, f[0], f[1], f[2], f[3], HILL_GREEN);
        const int pines = 7;
        for (int i = 0; i < pines; ++i) {
            const float ang = rng.range(0.0f, 6.2832f), d = 0.5f * f[2] * 0.8f * std::sqrt(rng.next());
            const float h = ellipsoidHeight(d, 0.5f * f[2], f[3]);
            if (h < 0.8f) continue;
            addPine(s, glm::vec3(f[0] + std::cos(ang) * d, seaY + h - 0.2f, f[1] + std::sin(ang) * d), rng.range(2.4f, 3.8f));
        }
        addObstacle(s, f[0], f[1], 0.5f * f[2] * 0.95f, f[3]);
    }

    // Sea cliffs along the shore, and a rocky headland to starboard. x, z, width, depth, height, yaw
    static const float CLIFFS[][6] = {
        { -27.0f,  -4.0f,  8.0f, 5.0f, 6.5f,  0.10f }, { -26.0f,  22.0f,  9.0f, 5.0f, 7.5f, -0.20f }, { -28.0f,  46.0f,  8.0f, 5.0f, 6.0f, 0.25f },
        { -16.0f,  64.0f,  8.0f, 5.0f, 6.5f,  0.90f }, {  38.0f,  50.0f,  6.0f, 4.0f, 5.0f, 0.40f }, {  46.0f,  42.0f,  5.0f, 4.0f, 6.5f, 0.10f },
    };
    for (const auto& c : CLIFFS)
        addCliff(s, seaY, c[0], c[1], c[2], c[3], c[4], c[5], rng);

    // Sea stacks and boulders in the shallows.
    static const float STACKS[][3] = { { -18.0f, -26.0f, 6.0f }, { -14.0f, 40.0f, 7.0f }, { 24.0f, 54.0f, 5.0f }, { 30.0f, -22.0f, 6.0f }, { 40.0f, 8.0f, 5.0f } };
    for (const auto& k : STACKS) {
        addCrag(s, seaY, k[0], k[1], 2.4f, k[2], rng.range(0.0f, 1.5f), CLIFF_ROCK);
        addObstacle(s, k[0], k[1], 1.7f, k[2]);
    }
}

// Rocky islands and narrow sea passages. The two big masses ahead leave a channel about 17 units wide, straight on from where the
// player starts; the enemy lies just before its mouth.
inline void buildRockyIslands(Scenery& s, float seaY)
{
    const RockMass MASSES[] = {
        // x, z, foot radius, tallest crag, spires, boulders, grass, seed
        { -22.0f,  58.0f, 14.0f, 10.0f, 7, 12, true,  201u },     // left of the channel
        {  22.0f,  62.0f, 13.0f, 11.0f, 7, 12, true,  211u },     // right of the channel
        { -22.0f,  84.0f, 12.0f, 12.0f, 6, 10, false, 223u },
        {  21.0f,  88.0f, 11.0f,  9.0f, 6, 10, true,  227u },
        { -36.0f,  34.0f,  9.0f,  7.0f, 5,  8, false, 233u },
        {  38.0f,  38.0f,  9.0f,  8.0f, 5,  8, true,  239u },
        { -46.0f,  -8.0f,  8.0f,  6.5f, 4,  6, false, 241u },
        {  48.0f, -10.0f,  8.0f,  6.0f, 4,  6, true,  251u },
        { -12.0f, -42.0f,  6.0f,  4.5f, 3,  5, false, 257u },
        {  16.0f, -36.0f,  7.0f,  5.0f, 4,  6, true,  263u },
        { -50.0f,  22.0f,  9.0f,  7.5f, 5,  8, true,  269u },
        {  54.0f,  24.0f,  8.0f,  6.0f, 4,  6, false, 271u },
        {   2.0f, 104.0f, 10.0f,  9.0f, 5,  8, false, 277u },
    };
    for (const RockMass& R : MASSES)
        addRockMass(s, seaY, R);

    // Reef rocks scattered through the shallows and a sea stack or two in the channel, to be steered round.
    SceneryRng rng(307u);
    static const float REEFS[][3] = { { -6.0f, 36.0f, 1.4f }, { 12.0f, 48.0f, 1.6f }, { 2.0f, 74.0f, 1.8f }, { -10.0f, 12.0f, 1.2f }, { 22.0f, 12.0f, 1.3f },
                                      { -28.0f, 8.0f, 1.5f }, { 34.0f, -2.0f, 1.4f }, { 4.0f, -22.0f, 1.3f }, { -24.0f, -26.0f, 1.6f }, { 30.0f, -30.0f, 1.2f } };
    for (const auto& r : REEFS) {
        addCrag(s, seaY, r[0], r[1], 1.2f + r[2] * 0.4f, 2.2f + r[2], rng.range(0.0f, 1.5f), ROCK_DARK);
        addBoulder(s, glm::vec3(r[0] + 1.3f, seaY - 0.1f, r[1] - 0.6f), r[2] * 0.9f, rng);
        addObstacle(s, r[0], r[1], 1.5f, 2.0f + r[2]);
    }
}

// Foggy / misty coastline: a low shore of dunes and dark pines, a wooden pier running out towards the player, a lighthouse on a
// headland, and pilings and buoys in the water. Meant to be seen through haze; the location also adds fog of its own.
inline void buildFoggyCoast(Scenery& s, float seaY)
{
    SceneryRng rng(401u);

    // The shore: dunes in a line across the bow, then dark pines behind them, then a ridge.
    for (int i = 0; i < 12; ++i) {
        const float x = -66.0f + 12.0f * static_cast<float>(i) + rng.range(-3.0f, 3.0f);
        const float z = 49.0f + rng.range(-2.5f, 2.5f);
        const float w = rng.range(15.0f, 22.0f), h = rng.range(1.6f, 2.8f);
        addHill(s, seaY, x, z, w, h, (i % 3 == 0) ? DUNE_GRASS : SAND);
        addObstacle(s, x, z, 0.5f * w * 0.9f, h);
        for (int k = 0; k < 3; ++k) {
            const float px = x + rng.range(-0.35f, 0.35f) * w, pz = z + rng.range(2.0f, 6.0f);
            addPine(s, glm::vec3(px, seaY + 0.9f, pz), rng.range(3.0f, 5.2f));
        }
    }
    for (int i = 0; i < 6; ++i)
        addHill(s, seaY, -60.0f + 24.0f * static_cast<float>(i) + rng.range(-4.0f, 4.0f), 68.0f + rng.range(-3.0f, 3.0f), rng.range(26.0f, 34.0f), rng.range(5.0f, 9.0f), HILL_GREEN);

    for (int i = 0; i < 5; ++i)                                                              // a second, higher ridge behind the first, so the land goes back in layers
        addHill(s, seaY, -64.0f + 30.0f * static_cast<float>(i) + rng.range(-5.0f, 5.0f), 86.0f + rng.range(-4.0f, 4.0f), rng.range(34.0f, 44.0f), rng.range(8.0f, 14.0f), (i % 2) ? HILL_GREEN : PINE_GREEN);
    for (int i = 0; i < 9; ++i)
        addRockCluster(s, glm::vec3(-60.0f + 14.0f * static_cast<float>(i) + rng.range(-3.0f, 3.0f), seaY - 0.1f, 45.0f + rng.range(-2.0f, 1.0f)), rng.range(1.0f, 2.2f), rng, CLIFF_ROCK);

    // A shingle spit curving round to the left, with rocks.
    for (int i = 0; i < 5; ++i) {
        const float x = -48.0f + rng.range(-3.0f, 3.0f), z = 34.0f - 12.0f * static_cast<float>(i);
        addHill(s, seaY, x, z, rng.range(9.0f, 13.0f), 1.2f, SAND);
        addBoulder(s, glm::vec3(x + 3.0f, seaY - 0.1f, z), rng.range(1.2f, 2.2f), rng);
        addObstacle(s, x, z, 5.0f, 1.2f);
    }

    // The headland and its lighthouse, to starboard.
    addRockMass(s, seaY, { 32.0f, 46.0f, 8.0f, 4.0f, 4, 8, true, 421u });
    addLighthouse(s, 32.0f, seaY + 2.4f, 46.0f, 7.5f);

    // The pier: planks on posts, from the shore at z = 46 out to z = 23, with a hut at the landward end and a lantern at the far end.
    const float pierX = -14.0f;
    const float deckY = seaY + 0.9f;
    for (int i = 0; i < 8; ++i) {
        const float z = 46.0f - 2.75f * static_cast<float>(i) - 1.3f;
        addBlock(s, glm::vec3(pierX, deckY, z), glm::vec3(2.4f, 0.14f, 2.7f), 0.0f, 0.0f, 0.0f, DECK_WOOD);
        for (int side = -1; side <= 1; side += 2)
            sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(pierX + 1.05f * static_cast<float>(side), seaY + 0.1f, z + 1.0f)) * sceneryStretch(glm::vec3(0.26f, 2.1f, 0.26f)),
                       HULL_WOOD, glm::vec3(pierX, seaY + 0.1f, z), 1.5f);
        addObstacle(s, pierX, z, 1.5f, 1.0f);
    }
    addBlock(s, glm::vec3(pierX, seaY + 2.3f, 44.5f), glm::vec3(3.2f, 2.6f, 3.4f), 0.0f, 0.0f, 0.0f, HULL_WOOD);                 // hut
    addBlock(s, glm::vec3(pierX, seaY + 3.9f, 44.5f), glm::vec3(3.9f, 0.5f, 4.0f), 0.0f, 0.0f, 0.0f, CASTLE_WOOD);               // roof
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(pierX + 1.0f, seaY + 1.8f, 22.8f)) * sceneryStretch(glm::vec3(0.16f, 1.8f, 0.16f)),
               HULL_WOOD, glm::vec3(pierX, seaY + 1.8f, 22.8f), 1.2f);
    sceneryAdd(s, PropMesh::SPHERE, sceneryMove(glm::vec3(pierX + 1.0f, seaY + 2.85f, 22.8f)) * sceneryStretch(glm::vec3(0.5f)),
               LIGHTHOUSE_LAMP, glm::vec3(pierX + 1.0f, seaY + 2.85f, 22.8f), 0.4f);
    s.lamps.push_back({ glm::vec3(pierX + 1.0f, seaY + 2.85f, 22.8f), 1.6f });

    // Pilings standing alone in the water, and marker buoys.
    for (int i = 0; i < 9; ++i) {
        const float x = 8.0f + rng.range(-1.0f, 1.0f) + 3.0f * static_cast<float>(i % 3), z = 36.0f + 3.0f * static_cast<float>(i / 3) + rng.range(-1.0f, 1.0f);
        sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(x, seaY + 0.5f, z)) * sceneryStretch(glm::vec3(0.30f, rng.range(1.6f, 2.6f), 0.30f)),
                   HULL_WOOD, glm::vec3(x, seaY + 0.5f, z), 1.5f);
    }
    static const float BUOYS[][2] = { { -4.0f, 18.0f }, { 14.0f, 22.0f }, { -24.0f, 20.0f }, { 24.0f, 28.0f }, { 2.0f, 38.0f } };
    for (const auto& b : BUOYS) {
        addEllipsoid(s, glm::vec3(b[0], seaY + 0.12f, b[1]), glm::vec3(0.95f), FLAG_CLOTH);
        sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(glm::vec3(b[0], seaY + 1.1f, b[1])) * sceneryStretch(glm::vec3(0.1f, 1.5f, 0.1f)),
                   SAILCLOTH, glm::vec3(b[0], seaY + 1.1f, b[1]), 1.0f);
    }
}

// ---- treasure: where it lies, and the scenery that gives each hiding place its character ---------------------------------------------------

inline void addTreasure(Scenery& s, TreasureKind kind, const glm::vec3& pos, float yaw, bool hidden, int gold, const char* name)
{
    TreasureSite t;
    t.kind = kind; t.pos = pos; t.yaw = yaw; t.hidden = hidden; t.gold = gold; t.name = name;
    s.treasures.push_back(t);
}

// A patch of pale sand at the water's edge with a few stones round it.
inline void addTreasureBeach(Scenery& s, float seaY, float x, float z, SceneryRng& rng)
{
    addEllipsoid(s, glm::vec3(x, seaY + 0.15f, z), glm::vec3(3.4f, 1.0f, 3.4f), SAND);          // a low mound, high enough to stay dry in a sunny swell
    for (int i = 0; i < 3; ++i)
        addBoulder(s, glm::vec3(x + rng.range(-1.8f, 1.8f), seaY - 0.05f, z + rng.range(-1.8f, 1.8f)), rng.range(0.5f, 0.9f), rng);
}

// A cave: a back wall and two side walls of rock under a slab, open on the side the chest faces. The ship cannot enter it, but can lie alongside.
inline void addTreasureCave(Scenery& s, float seaY, float x, float z, float yaw, SceneryRng& rng)
{
    const glm::vec3 fwd(std::sin(yaw), 0.0f, std::cos(yaw)), side(std::cos(yaw), 0.0f, -std::sin(yaw));
    const auto at = [&](float a, float b, float y) { return glm::vec3(x, 0.0f, z) + fwd * a + side * b + glm::vec3(0.0f, y, 0.0f); };
    for (int k = -1; k <= 1; ++k)                                                                                              // the back wall: three heaps, one behind the other
        addRockCluster(s, at(-1.7f, 1.35f * static_cast<float>(k), seaY - 0.1f + 0.4f * static_cast<float>(1 - std::abs(k))), 2.9f, rng, (k == 0) ? ROCK_DARK : CLIFF_ROCK);
    for (int sd = -1; sd <= 1; sd += 2)                                                                                        // the two sides
        for (int k = 0; k < 2; ++k)
            addRockCluster(s, at(-0.7f + 1.5f * static_cast<float>(k), 2.0f * static_cast<float>(sd), seaY - 0.1f), 2.4f, rng, (k == sd) ? ROCK_DARK : CLIFF_ROCK);
    addBlock(s, at(-0.3f, 0.0f, seaY + 2.4f), glm::vec3(4.4f, 0.9f, 3.8f), yaw + 0.12f, 0.06f, 0.05f, ROCK_DARK);              // the fallen slabs of the roof
    addBlock(s, at(0.5f, 0.6f, seaY + 2.7f), glm::vec3(3.0f, 0.7f, 2.4f), yaw - 0.35f, -0.08f, 0.10f, CLIFF_ROCK);
    addRockCluster(s, at(-0.8f, -0.4f, seaY + 2.8f), 2.6f, rng, CLIFF_ROCK);
    addEllipsoid(s, at(0.2f, 0.0f, seaY + 0.05f), glm::vec3(3.0f, 0.4f, 3.0f), SAND);                                        // the floor
    addRockCluster(s, at(1.7f, 1.3f, seaY - 0.1f), 1.3f, rng, CLIFF_ROCK);
    addObstacle(s, x, z, 2.4f, 2.6f);
}

// A wreck: the broken bottom of a ship, canted over, with a stump of mast, loose planks, and a deck to stand a chest on.
inline void addTreasureWreck(Scenery& s, float seaY, float x, float z, float yaw, SceneryRng& rng)
{
    const glm::vec3 fwd(std::sin(yaw), 0.0f, std::cos(yaw)), side(std::cos(yaw), 0.0f, -std::sin(yaw));
    const auto at = [&](float a, float b, float y) { return glm::vec3(x, 0.0f, z) + fwd * a + side * b + glm::vec3(0.0f, y, 0.0f); };
    addBlock(s, at(0.0f, 0.0f, seaY + 0.15f), glm::vec3(2.8f, 0.7f, 6.0f), yaw, 0.05f, 0.14f, HULL_WOOD);                      // the keel and floor
    addBlock(s, at(0.0f, 1.45f, seaY + 0.65f), glm::vec3(0.22f, 1.3f, 5.4f), yaw, 0.04f, 0.16f, CASTLE_WOOD);                  // the left side, broken short
    addBlock(s, at(-0.6f, -1.45f, seaY + 0.55f), glm::vec3(0.22f, 1.0f, 3.4f), yaw, 0.05f, 0.1f, CASTLE_WOOD);                 // the right side, broken shorter
    addBlock(s, at(-0.8f, 0.0f, seaY + 0.62f), glm::vec3(2.4f, 0.14f, 2.0f), yaw, 0.04f, 0.12f, DECK_WOOD);                    // what is left of the deck
    sceneryAdd(s, PropMesh::CYLINDER, sceneryMove(at(1.4f, 0.2f, seaY + 1.5f)) * sceneryTurn(0.35f, glm::vec3(0, 0, 1)) * sceneryStretch(glm::vec3(0.24f, 3.0f, 0.24f)),
               HULL_WOOD, at(1.4f, 0.2f, seaY + 1.5f), 1.8f);                                                                  // the mast stump
    for (int i = 0; i < 4; ++i)
        addBlock(s, at(rng.range(-3.0f, 3.0f), rng.range(-3.0f, 3.0f), seaY + 0.08f), glm::vec3(1.4f, 0.08f, 0.2f), rng.range(0.0f, 3.14f), 0.0f, 0.0f, DECK_WOOD);
    addObstacle(s, x, z, 3.0f, 1.2f);
}

inline void addTreasureSummit(Scenery& s, float x, float y, float z, SceneryRng& rng)
{
    addBlock(s, glm::vec3(x, y - 0.12f, z), glm::vec3(2.6f, 0.3f, 2.6f), rng.range(0.0f, 1.5f), 0.0f, 0.0f, CLIFF_ROCK);
    for (int i = 0; i < 3; ++i)
        addBoulder(s, glm::vec3(x + rng.range(-1.6f, 1.6f), y - 0.2f, z + rng.range(-1.6f, 1.6f)), rng.range(0.5f, 0.9f), rng);
}

inline void addTreasureSeabed(Scenery& s, float x, float y, float z, SceneryRng& rng)
{
    addEllipsoid(s, glm::vec3(x, y - 0.1f, z), glm::vec3(4.4f, 0.8f, 4.4f), SAND);
    for (int i = 0; i < 3; ++i)
        addBoulder(s, glm::vec3(x + rng.range(-2.2f, 2.2f), y - 0.2f, z + rng.range(-2.2f, 2.2f)), rng.range(0.6f, 1.3f), rng, ROCK_DARK);
}

// A point on an island's coast: `towards` +1 for the shore facing the origin (where the player starts), -1 for the far shore.
inline glm::vec3 coastPoint(float cx, float cz, float radius, float towards)
{
    const float d = std::max(std::hypot(cx, cz), 1.0f);
    return glm::vec3(cx - towards * cx / d * radius, 0.0f, cz - towards * cz / d * radius);
}

// The treasure of one location. About half is in plain sight from a likely approach (a chest on a beach); the rest has to be looked for: behind an island, in a cave,
// on a summit, on the seabed. Positions are in the same frame as the scenery above (the player at the origin, facing +z), so they move with the scenery when it is carried to
// the player (transformScenery).
inline void addTreasureSites(Scenery& s, int location, float seaY)
{
    SceneryRng rng(900u + static_cast<unsigned int>(location));
    switch (location) {
    case LocationId::JUNGLE_ISLAND: {
        glm::vec3 p = coastPoint(33.0f, 41.0f, 15.0f, 1.0f);   p.y = seaY + 0.55f;
        addTreasureBeach(s, seaY, p.x, p.z, rng);              addTreasure(s, TreasureKind::BEACH, p, -2.2f, false, 250, "beach chest");
        p = coastPoint(-30.0f, 36.0f, 12.0f, -1.0f);           p.y = seaY + 0.55f;
        addTreasureBeach(s, seaY, p.x, p.z, rng);              addTreasure(s, TreasureKind::BEACH, p, 0.6f, true, 400, "hidden cove chest");
        addTreasureSummit(s, -42.0f, seaY + 5.1f, -14.0f, rng); addTreasure(s, TreasureKind::SUMMIT, glm::vec3(-42.0f, seaY + 5.15f, -14.0f), 1.0f, true, 600, "summit chest");
        addTreasureSeabed(s, 51.0f, seaY - 5.5f, -7.0f, rng);  addTreasure(s, TreasureKind::UNDERWATER, glm::vec3(51.0f, seaY - 5.5f, -7.0f), 2.0f, true, 500, "sunken chest");
        break;
    }
    case LocationId::MOUNTAIN_COAST: {
        glm::vec3 p(-20.5f, seaY + 0.55f, 22.0f);
        addTreasureBeach(s, seaY, p.x, p.z, rng);              addTreasure(s, TreasureKind::BEACH, p, 1.57f, false, 250, "cliff-foot chest");
        addTreasureCave(s, seaY, -29.0f, 8.0f, 1.57f, rng);    addTreasure(s, TreasureKind::CAVE, glm::vec3(-29.4f, seaY + 0.3f, 8.0f), 1.57f, true, 450, "cave chest");
        addTreasureSummit(s, -48.0f, seaY + 25.4f, -4.0f, rng); addTreasure(s, TreasureKind::SUMMIT, glm::vec3(-48.0f, seaY + 25.45f, -4.0f), 0.4f, true, 700, "peak chest");
        addTreasureSeabed(s, 27.0f, seaY - 5.5f, 54.0f, rng);  addTreasure(s, TreasureKind::UNDERWATER, glm::vec3(27.0f, seaY - 5.5f, 54.0f), 3.0f, true, 500, "sunken chest");
        break;
    }
    case LocationId::ROCKY_ISLANDS: {
        addTreasureWreck(s, seaY, -9.0f, 26.0f, 0.5f, rng);    addTreasure(s, TreasureKind::WRECK, glm::vec3(-9.2f, seaY + 0.95f, 25.4f), 0.5f, false, 300, "wreck chest");
        addTreasureCave(s, seaY, -8.5f, 57.0f, 1.57f, rng);    addTreasure(s, TreasureKind::CAVE, glm::vec3(-8.9f, seaY + 0.3f, 57.0f), 1.57f, true, 450, "cave chest");
        addTreasureSummit(s, 22.0f, seaY + 9.0f, 62.0f, rng);  addTreasure(s, TreasureKind::SUMMIT, glm::vec3(22.0f, seaY + 9.05f, 62.0f), 0.8f, true, 650, "crag chest");
        addTreasureSeabed(s, -8.0f, seaY - 5.5f, 14.0f, rng);  addTreasure(s, TreasureKind::UNDERWATER, glm::vec3(-8.0f, seaY - 5.5f, 14.0f), 2.2f, true, 500, "sunken chest");
        break;
    }
    case LocationId::FOGGY_COAST: {
        addTreasure(s, TreasureKind::BEACH, glm::vec3(-14.0f, seaY + 1.0f, 22.9f), 3.14f, false, 250, "pier chest");     // on the pier's end deck
        glm::vec3 p(32.0f, seaY + 0.55f, 51.5f);
        addTreasureBeach(s, seaY, p.x, p.z, rng);              addTreasure(s, TreasureKind::BEACH, p, 0.0f, true, 400, "headland chest");
        addTreasureWreck(s, seaY, 14.0f, 46.0f, -0.4f, rng);   addTreasure(s, TreasureKind::WRECK, glm::vec3(13.9f, seaY + 0.95f, 45.4f), -0.4f, true, 500, "wreck chest");
        addTreasureSeabed(s, 12.0f, seaY - 4.8f, 34.0f, rng);  addTreasure(s, TreasureKind::UNDERWATER, glm::vec3(12.0f, seaY - 4.8f, 34.0f), 1.0f, true, 550, "sunken chest");
        break;
    }
    default: {   // the open ocean: placed in the world with the Phase 46 island table
        glm::vec3 p(24.0f, seaY + 0.55f, 9.0f);
        addTreasureBeach(s, seaY, p.x, p.z, rng);           addTreasure(s, TreasureKind::BEACH, p, -1.2f, false, 250, "beach chest");
        p = glm::vec3(30.0f, seaY + 0.55f, -30.0f);
        addTreasureBeach(s, seaY, p.x, p.z, rng);              addTreasure(s, TreasureKind::BEACH, p, 0.5f, true, 400, "far-shore chest");
        addTreasureSummit(s, 23.6f, seaY + 3.0f, -4.2f, rng);  addTreasure(s, TreasureKind::SUMMIT, glm::vec3(23.6f, seaY + 3.05f, -4.2f), 0.9f, true, 500, "sea-stack chest");
        addTreasureSeabed(s, -21.0f, seaY - 5.5f, 18.0f, rng); addTreasure(s, TreasureKind::UNDERWATER, glm::vec3(-21.0f, seaY - 5.5f, 18.0f), 2.0f, true, 500, "sunken chest");
        addTreasureWreck(s, seaY, 15.0f, 52.0f, 2.2f, rng);    addTreasure(s, TreasureKind::WRECK, glm::vec3(14.9f, seaY + 0.95f, 51.4f), 2.2f, false, 350, "wreck chest");
        break;
    }
    }
}

// Builds the scenery (and the land's outline) of one location. Called when the player picks a location, never per frame.
inline Scenery buildScenery(int location, float seaY)
{
    Scenery s;
    switch (location) {
    case LocationId::JUNGLE_ISLAND:  buildJungleIslands(s, seaY); break;
    case LocationId::MOUNTAIN_COAST: buildMountainCoast(s, seaY); break;
    case LocationId::ROCKY_ISLANDS:  buildRockyIslands(s, seaY); break;
    case LocationId::FOGGY_COAST:    buildFoggyCoast(s, seaY); break;
    default:                         buildOpenOcean(s, seaY); break;
    }
    addTreasureSites(s, location, seaY);

    // The original environment was authored as a compact showcase around the origin.  Spread the
    // translations into sailing-scale distances without scaling the procedural models themselves.
    // Every representation is moved together so rendered land, collision, lamps, waterfalls and
    // treasure remain exactly aligned.
    const float spread = SceneryConfig::POSITION_SPREAD;
    for (Prop& p : s.props) {
        p.model[3].x *= spread; p.model[3].z *= spread;
        p.centre.x *= spread; p.centre.z *= spread;
        p.pivot.x *= spread; p.pivot.z *= spread;
    }
    for (Obstacle& o : s.obstacles) { o.x *= spread; o.z *= spread; }
    for (SceneryLamp& l : s.lamps) { l.position.x *= spread; l.position.z *= spread; }
    for (Waterfall& w : s.waterfalls) {
        w.top.x *= spread; w.top.z *= spread;
        w.bottom.x *= spread; w.bottom.z *= spread;
    }
    for (TreasureSite& t : s.treasures) { t.pos.x *= spread; t.pos.z *= spread; }
    return s;
}

// ---- solid things: ships and land never overlap -------------------------------------------------------------------------------------
//
// A SHIP'S SHAPE for collision is a chain of four circles down its length (SceneryConfig::HULL_CIRCLES), placed from the ship's position and
// heading - the hull is 5.6 long and 1.4 wide, so a chain follows it closely where a single circle would either leave the bow inside a rock
// or keep the ship a ship's-length from everything. LAND is a list of circles seen from above (Obstacle). Two solid things that overlap are
// pushed apart until they do not, so one never ends up inside the other.
//
// Pure arithmetic, no OpenGL, so a CPU test can drop thousands of ships on the map and check that none is left inside anything.

// Where circle i of a ship's chain is, in the world (x and z).
inline glm::vec2 hullCircleCentre(const glm::vec3& position, float heading, int i)
{
    const float along = SceneryConfig::HULL_CIRCLE_OFFSETS[i];
    return glm::vec2(position.x + std::sin(heading) * along, position.z + std::cos(heading) * along);
}

// Is the ship touching any of the land? `slack` shrinks the test (so a ship exactly at the edge is not "touching").
inline bool shipTouchesLand(const std::vector<Obstacle>& obstacles, const glm::vec3& position, float heading, float slack = 0.0f)
{
    for (int i = 0; i < SceneryConfig::HULL_CIRCLE_COUNT; ++i) {
        const glm::vec2 c = hullCircleCentre(position, heading, i);
        for (const Obstacle& o : obstacles) {
            const float dx = c.x - o.x, dz = c.y - o.z;
            const float limit = o.radius + SceneryConfig::HULL_CIRCLE_RADIUS - slack;
            if (dx * dx + dz * dz < limit * limit)
                return true;
        }
    }
    return false;
}

// If a ship is touching the land, pushes it out and returns true.
//
// Each circle of the chain that overlaps an obstacle asks for the push that would clear IT, along the line from the obstacle's centre. The
// requests of one pass are AVERAGED and applied together. (Applied one after another they would let two overlapping circles shove the ship
// back and forth between them, creeping towards the corner where they cross; added up, a ship touching with two circles would be thrown twice
// as far as it needs; averaged, one contact is cleared in one pass and several settle in a few.) If the ship is still on land after the passes
// - a wedge between circles that nearly touch - a search finds the nearest open water, trying 24 directions at growing distances. It runs only
// on those rare frames.
inline bool pushShipOffLand(const std::vector<Obstacle>& obstacles, glm::vec3& position, float heading = 0.0f)
{
    bool touched = false;
    for (int pass = 0; pass < 6; ++pass) {
        float pushX = 0.0f, pushZ = 0.0f;
        int contacts = 0;
        for (int i = 0; i < SceneryConfig::HULL_CIRCLE_COUNT; ++i) {
            const glm::vec2 c = hullCircleCentre(position, heading, i);
            for (const Obstacle& o : obstacles) {
                const float dx = c.x - o.x, dz = c.y - o.z;
                const float limit = o.radius + SceneryConfig::HULL_CIRCLE_RADIUS;
                const float d2 = dx * dx + dz * dz;
                if (d2 >= limit * limit)
                    continue;
                const float d = std::sqrt(d2);
                const float nx = (d > 1e-4f) ? dx / d : 1.0f, nz = (d > 1e-4f) ? dz / d : 0.0f;
                pushX += nx * (limit - d + 0.02f);          // 0.02: end just clear of the edge, not exactly on it
                pushZ += nz * (limit - d + 0.02f);
                ++contacts;
            }
        }
        if (contacts == 0)
            break;
        position.x += pushX / static_cast<float>(contacts);
        position.z += pushZ / static_cast<float>(contacts);
        touched = true;
    }

    if (touched && shipTouchesLand(obstacles, position, heading)) {
        const glm::vec3 from = position;
        for (float r = 0.4f; r <= 30.0f; r += 0.4f) {
            bool found = false;
            for (int k = 0; k < 24 && !found; ++k) {
                const float a = static_cast<float>(k) * 0.2617994f;       // 15 degrees
                const glm::vec3 candidate(from.x + std::cos(a) * r, from.y, from.z + std::sin(a) * r);
                if (!shipTouchesLand(obstacles, candidate, heading)) {
                    position = candidate;
                    found = true;
                }
            }
            if (found)
                break;
        }
    }
    return touched;
}

// Two ships are solid to each other. Every circle of one is tested against every circle of the other; where they overlap, the ships are
// pushed apart along the line between the circles' centres, each by half (they have the same mass). Averaged and repeated like the land
// push, so a ship ramming the other broadside, bow on, or sliding alongside is separated and never ends up inside it. Returns true if the
// ships were touching.
inline bool separateShips(glm::vec3& a, float headingA, glm::vec3& b, float headingB)
{
    bool touched = false;
    const float limit = 2.0f * SceneryConfig::HULL_CIRCLE_RADIUS;
    for (int pass = 0; pass < 12; ++pass) {
        float pushX = 0.0f, pushZ = 0.0f, deepest = 0.0f;
        int contacts = 0;
        for (int i = 0; i < SceneryConfig::HULL_CIRCLE_COUNT; ++i) {
            const glm::vec2 ca = hullCircleCentre(a, headingA, i);
            for (int j = 0; j < SceneryConfig::HULL_CIRCLE_COUNT; ++j) {
                const glm::vec2 cb = hullCircleCentre(b, headingB, j);
                const float dx = ca.x - cb.x, dz = ca.y - cb.y;
                const float d2 = dx * dx + dz * dz;
                if (d2 >= limit * limit)
                    continue;
                const float d = std::sqrt(d2);
                // Dead centre (two circles exactly on top of each other): push along the first ship's own side so the answer is not arbitrary.
                const float nx = (d > 1e-4f) ? dx / d : std::cos(headingA), nz = (d > 1e-4f) ? dz / d : -std::sin(headingA);
                pushX += nx * (limit - d + 0.02f);
                pushZ += nz * (limit - d + 0.02f);
                deepest = std::max(deepest, limit - d + 0.02f);
                ++contacts;
            }
        }
        if (contacts == 0)
            break;
        float ax = pushX / static_cast<float>(contacts), az = pushZ / static_cast<float>(contacts);
        // Two hulls crossing like an X overlap all round their meeting point, and the pushes of the circles on opposite sides cancel to almost
        // nothing. Then the circles say nothing useful, so the ships are pushed apart along the line between their own centres instead.
        if (ax * ax + az * az < 0.0625f * deepest * deepest) {
            float cx = a.x - b.x, cz = a.z - b.z;
            const float cl = std::sqrt(cx * cx + cz * cz);
            if (cl > 1e-4f) { cx /= cl; cz /= cl; } else { cx = std::cos(headingA); cz = -std::sin(headingA); }
            ax = cx * deepest;
            az = cz * deepest;
        }
        a.x += 0.5f * ax; a.z += 0.5f * az;
        b.x -= 0.5f * ax; b.z -= 0.5f * az;
        touched = true;
    }
    return touched;
}

// Has a cannonball at `p` struck the land? Inside an obstacle's circle and lower than its top.
inline bool ballHitsLand(const std::vector<Obstacle>& obstacles, const glm::vec3& p, float seaY)
{
    for (const Obstacle& o : obstacles) {
        const float dx = p.x - o.x, dz = p.z - o.z;
        if (dx * dx + dz * dz < o.radius * o.radius * 0.64f && p.y - seaY < o.height * 0.8f)
            return true;
    }
    return false;
}

// The nearest land ahead of a ship, for the enemy's steering: returns the signed side the land is on (positive = to the left of the
// heading) and how far away it is, or distance = 1e9 if the way is clear within `lookAhead`.
inline void landAhead(const std::vector<Obstacle>& obstacles, const glm::vec3& position, float heading, float lookAhead, float& side, float& distance)
{
    side = 0.0f;
    distance = 1e9f;
    const float fx = std::sin(heading), fz = std::cos(heading);
    for (const Obstacle& o : obstacles) {
        const float dx = o.x - position.x, dz = o.z - position.z;
        const float along = dx * fx + dz * fz;
        const float across = dx * fz - dz * fx;                      // positive when the land is to the ship's left (+x side at heading 0 is "left")
        const float reach = o.radius + SceneryConfig::SHIP_CLEARANCE + 2.5f;
        if (along < 0.0f || along > lookAhead + o.radius || std::fabs(across) > reach)
            continue;
        if (along < distance) {
            distance = along;
            side = across;
        }
    }
}

// ---- predictive ship navigation ---------------------------------------------------------------------------------------------------
//
// landAhead() is useful for a simple warning, but a single ray cannot choose a route through a harbour or between two islands.  The enemy
// used to see the nearest circle, turn away from it, see the circle on the other side on the next frame, and repeat that decision forever.
// These CPU-only helpers instead try complete, gently turning courses.  Every sample tests the same four-circle hull used by collision, so
// a course is considered open only when the whole ship fits.  The caller supplies a preferred side to keep successive frames from changing
// their mind in a narrow passage.

inline float navigationWrap(float a)
{
    while (a > 3.14159265f) a -= 6.28318531f;
    while (a < -3.14159265f) a += 6.28318531f;
    return a;
}

inline float shipLandClearance(const std::vector<Obstacle>& obstacles, const glm::vec3& position, float heading)
{
    float best = 1e9f;
    for (int i = 0; i < SceneryConfig::HULL_CIRCLE_COUNT; ++i) {
        const glm::vec2 c = hullCircleCentre(position, heading, i);
        for (const Obstacle& o : obstacles) {
            const float dx = c.x - o.x, dz = c.y - o.z;
            best = std::min(best, std::sqrt(dx * dx + dz * dz) - o.radius - SceneryConfig::HULL_CIRCLE_RADIUS);
        }
    }
    return best;
}

// Minimum hull-to-land gap along a course.  The simulated bow turns at a ship-like rate instead of being snapped to candidateHeading, so
// the swept bow and stern are checked as well as the eventual straight path.
inline float shipCourseClearance(const std::vector<Obstacle>& obstacles, const glm::vec3& start, float startHeading,
                                 float candidateHeading, float speed, float seconds = 6.0f)
{
    glm::vec3 p = start;
    float h = startHeading;
    float clearance = shipLandClearance(obstacles, p, h);
    constexpr float STEP = 0.25f;
    constexpr float PREDICTED_TURN_RATE = 0.62f; // rad/s: below the playable ship's maximum, so the prediction does not assume a pivot turn
    const int steps = std::max(1, static_cast<int>(std::ceil(seconds / STEP)));
    const float travelSpeed = std::clamp(std::fabs(speed), 1.35f, 2.6f);
    for (int i = 0; i < steps; ++i) {
        const float error = navigationWrap(candidateHeading - h);
        h += std::clamp(error, -PREDICTED_TURN_RATE * STEP, PREDICTED_TURN_RATE * STEP);
        p += glm::vec3(std::sin(h), 0.0f, std::cos(h)) * (travelSpeed * STEP);
        clearance = std::min(clearance, shipLandClearance(obstacles, p, h));
    }
    return clearance;
}

struct ShipCoursePlan {
    float heading = 0.0f;
    float clearance = 1e9f;
    float speedScale = 1.0f;
    int side = 0;                 // +1 left, -1 right, 0 direct
    bool directClear = true;
    bool reverse = false;         // no safe forward course: make room before trying again
};

inline ShipCoursePlan planShipCourse(const std::vector<Obstacle>& obstacles, const glm::vec3& position, float currentHeading,
                                     float wantedHeading, float speed, int preferredSide)
{
    constexpr float SAFE_GAP = 0.32f;
    constexpr float DEG = 3.14159265f / 180.0f;
    constexpr float OFFSETS[] = {
        0.0f, 15.0f*DEG, -15.0f*DEG, 30.0f*DEG, -30.0f*DEG, 45.0f*DEG, -45.0f*DEG,
        62.0f*DEG, -62.0f*DEG, 82.0f*DEG, -82.0f*DEG, 105.0f*DEG, -105.0f*DEG,
        130.0f*DEG, -130.0f*DEG
    };

    ShipCoursePlan plan;
    plan.heading = wantedHeading;
    const float direct = shipCourseClearance(obstacles, position, currentHeading, wantedHeading, speed);
    plan.directClear = direct >= SAFE_GAP;
    if (plan.directClear) {
        plan.clearance = direct;
        return plan;
    }

    float bestScore = -1e9f;
    for (float offset : OFFSETS) {
        const float candidate = wantedHeading + offset;
        const float clearance = shipCourseClearance(obstacles, position, currentHeading, candidate, speed);
        const float goalProgress = std::cos(navigationWrap(candidate - wantedHeading));
        const float turnCost = std::fabs(navigationWrap(candidate - currentHeading)) / 3.14159265f;
        const int side = offset > 0.001f ? 1 : (offset < -0.001f ? -1 : 0);
        const float sideCost = preferredSide != 0 && side != 0 && side != preferredSide ? 0.85f : 0.0f;
        const float unsafeCost = clearance < SAFE_GAP ? 20.0f + 8.0f * (SAFE_GAP - clearance) : 0.0f;
        const float score = 3.2f * goalProgress + 0.18f * std::clamp(clearance, -2.0f, 5.0f) - 0.35f * turnCost - sideCost - unsafeCost;
        if (score > bestScore) {
            bestScore = score;
            plan.heading = candidate;
            plan.clearance = clearance;
            plan.side = side;
        }
    }

    plan.reverse = plan.clearance < 0.04f;
    plan.speedScale = plan.reverse ? 0.0f : std::clamp(0.28f + 0.34f * plan.clearance, 0.28f, 1.0f);
    return plan;
}

// ---- a straight rod between two points ------------------------------------------------------------------------------------------

// The matrix that turns the unit cylinder (1 tall, along y, centred on its origin) into a rod of the given thickness running from a to b:
// moved to the midpoint, turned so its axis lies along b - a, stretched to the length. Used for a bolt of lightning, whose zigzag is a
// chain of such rods. The turn is built from the cross product of the two directions - the axis that carries y onto the rod.
inline glm::mat4 sceneryBetween(const glm::vec3& a, const glm::vec3& b, float thickness)
{
    const glm::vec3 d = b - a;
    const float length = glm::length(d);
    const glm::vec3 dir = (length > 1e-5f) ? d / length : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 axis = glm::cross(up, dir);
    glm::mat4 turn(1.0f);
    if (glm::dot(axis, axis) > 1e-8f)
        turn = glm::rotate(glm::mat4(1.0f), std::acos(std::clamp(glm::dot(up, dir), -1.0f, 1.0f)), glm::normalize(axis));
    else if (dir.y < 0.0f)
        turn = glm::rotate(glm::mat4(1.0f), 3.14159265f, glm::vec3(1.0f, 0.0f, 0.0f));
    return sceneryMove(0.5f * (a + b)) * turn * sceneryStretch(glm::vec3(thickness, length, thickness));
}

// ---- anchoring the scenery to where the player is -------------------------------------------------------------------------------------

// The layouts above are written for a player at the origin facing +z (the start of a fight). If the player has sailed away before choosing
// a new location, the land would be built where they are not looking. So it is carried to the player: moved to (anchor.x, anchor.z) and
// turned to the player's heading, as one rigid motion. Rotating about +y by the heading is the same convention as the ships' own
// (heading 0 faces +z, a positive heading turns towards +x).
inline void transformScenery(Scenery& s, const glm::vec3& anchor, float heading)
{
    const glm::mat4 M = sceneryMove(glm::vec3(anchor.x, 0.0f, anchor.z)) * sceneryTurn(heading, glm::vec3(0.0f, 1.0f, 0.0f));
    for (Prop& p : s.props) {
        p.model = M * p.model;
        p.centre = glm::vec3(M * glm::vec4(p.centre, 1.0f));
        p.pivot = glm::vec3(M * glm::vec4(p.pivot, 1.0f));
    }
    for (Obstacle& o : s.obstacles) {
        const glm::vec3 c = glm::vec3(M * glm::vec4(o.x, 0.0f, o.z, 1.0f));
        o.x = c.x;
        o.z = c.z;
    }
    for (SceneryLamp& l : s.lamps)
        l.position = glm::vec3(M * glm::vec4(l.position, 1.0f));
    for (Waterfall& w : s.waterfalls) {
        w.top = glm::vec3(M * glm::vec4(w.top, 1.0f));
        w.bottom = glm::vec3(M * glm::vec4(w.bottom, 1.0f));
    }
    for (TreasureSite& t : s.treasures) {
        t.pos = glm::vec3(M * glm::vec4(t.pos, 1.0f));
        t.yaw += heading;                       // the same turn as everything else, about y
    }
}
