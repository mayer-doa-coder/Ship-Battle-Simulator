#pragma once

// Environment build: THE SHIP'S INTERIOR and the player who walks in it. No OpenGL here (like Crew.h): the layout of the rooms, the furniture as a list of primitives placed in the
// HULL'S OWN FRAME, the ladders and stairs, the walls you cannot walk through, and the walker.
//
// IN THE SHIP'S FRAME. Every position here is in the hull's frame, the same frame the crew and the furniture of the decks are in (Crew.h). A chair at (0.1, -0.2, -2.1) is therefore
// 2.1 units from the middle of the hull towards the stern, wherever the ship is, however it is heading, and however it pitches and rolls on the waves: the interior is drawn as
// `hull * position`, and a player standing in it is carried with the ship. That is also what makes it consistent with the outside: the rooms are bounded by the SAME hull the outside is
// drawn from (the hull's own section curve, src/Hull.h, tells how wide it is at any height and station), so a wall cannot stand where the planking is not.
//
// THE LEVELS. The hold lies below the main deck, 0.30 below the hull's centre: from the stern, the CAPTAIN'S CABIN, the CREW QUARTERS, the CARGO HOLD (with the keelson and bilge), the GALLEY
// and the POWDER MAGAZINE, divided by bulkheads with doorways. Above it are the decks the ship already has: the main deck, the quarterdeck, the poop and the forecastle, joined to each other and
// to the hold by ladders, a hatch, and a companionway of stairs. A Connector says where the foot and the head of each are, and the Explorer climbs between them.
//
// THE HEADROOM is small (the hull is shallow: 0.45 of a unit at the waist, taller at the ends), so the eye is low (0.36) and the furniture in the waist rooms is knee-high. The roof is the hull's
// own cap, seen from underneath.

#include "Crew.h"
#include "Scenery.h"
#include "Ship.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace InteriorConfig {
constexpr float FLOOR_Y = -0.30f;           // the hold's floor, in the hull's frame
constexpr float EYE_HEIGHT = 0.36f;         // a standing person's eye above their feet
constexpr float BODY_RADIUS = 0.05f;
constexpr float WALK_SPEED = 0.85f;
constexpr float RUN_SPEED = 1.7f;
constexpr float CLIMB_SPEED = 0.55f;
constexpr float USE_REACH = 0.42f;          // how near a ladder you must be to use it
constexpr float CHEST_REACH = 0.95f;        // how near a chest in the ship you must be to open it
}

enum InteriorLevel { LVL_HOLD = 0, LVL_DECK = 1, LVL_QUARTER = 2, LVL_POOP = 3, LVL_FORE = 4, LVL_COUNT = 5 };

struct Box2 { float x0, z0, x1, z1; };

enum class ConnectorKind { LADDER, HATCH, STAIRS };

struct Connector {
    int lower = LVL_HOLD, upper = LVL_DECK;
    glm::vec3 lowerFoot = glm::vec3(0.0f);     // where you stand at the bottom, and
    glm::vec3 upperFoot = glm::vec3(0.0f);     // at the top (feet, hull-local)
    ConnectorKind kind = ConnectorKind::LADDER;
    const char* name = "ladder";
};

struct InteriorLayout {
    // The hull, for asking how wide it is.
    HullProfile profile{};
    float beam = 1.0f, depth = 1.0f, length = 1.0f;

    float levelY[LVL_COUNT] = {};
    float zMin[LVL_COUNT] = {}, zMax[LVL_COUNT] = {};
    float xLimit[LVL_COUNT] = {};              // the greatest |x| on a deck level (the hold uses the hull's width instead)

    Scenery props;                             // everything fixed, hull-local; drawn as hull * model
    std::vector<Box2> colliders[LVL_COUNT];
    std::vector<Connector> connectors;
    std::vector<TreasureSite> chests;          // the ship's own treasure (hull-local)
    struct Sleeper { glm::vec3 pos; float yaw; };
    std::vector<Sleeper> sleepers;
    glm::vec3 cookPos = glm::vec3(0.0f);
    float cookYaw = 0.0f;
    std::vector<glm::vec3> lamps;              // the hold's lanterns, hull-local (the scene's one point light is lent to the nearest)

    float halfWidthAt(float z, float y) const;
    float ceilingAt(float z) const;
};

// How far the hull reaches sideways at height y (hull-local) at station z: found by walking up the section\'s own curve.
inline float InteriorLayout::halfWidthAt(float z, float y) const
{
    const HullStation s = hullStationAt(profile, std::clamp(z / length + 0.5f, 0.0f, 1.0f));
    HullRing r{ z / length, 0.5f * s.halfBeam, -0.5f + s.keelRise, -0.5f + s.sheer };
    const float yu = y / depth;
    if (yu < r.bottom)
        return 0.0f;
    for (float k = 0.0f; k <= static_cast<float>(HullShape::STRAKES); k += 0.125f) {
        const glm::vec2 p = hullSectionPointAt(r, k);
        if (p.y >= yu)
            return p.x * beam;
    }
    return r.halfWidth * (1.0f - HullShape::TUMBLEHOME) * beam;
}

// The underside of the hull's cap at station z: the roof of the hold.
inline float InteriorLayout::ceilingAt(float z) const
{
    const HullStation s = hullStationAt(profile, std::clamp(z / length + 0.5f, 0.0f, 1.0f));
    return depth * (-0.5f + s.sheer) - 0.012f;
}

inline float explorerLimitX(const InteriorLayout& L, int level, float z);         // (defined with the walker, below)

// Whether a walker's feet may stand at (x, z) on `level`: inside the deck or the hull, and clear of every obstacle by the width of the body.
inline bool standableAt(const InteriorLayout& L, int level, float x, float z)
{
    if (z < L.zMin[level] || z > L.zMax[level] || std::fabs(x) > explorerLimitX(L, level, z))
        return false;
    const float r = InteriorConfig::BODY_RADIUS;
    for (const Box2& b : L.colliders[level])
        if (x > b.x0 - r && x < b.x1 + r && z > b.z0 - r && z < b.z1 + r)
            return false;
    return true;
}

// The nearest standable place to `p` on `level` (p itself if it is one): a ladder's foot is wanted beside the ladder, but never inside a crate.
inline glm::vec3 snapToStandable(const InteriorLayout& L, int level, const glm::vec3& p)
{
    if (standableAt(L, level, p.x, p.z))
        return p;
    for (float radius = 0.02f; radius < 0.7f; radius += 0.02f)
        for (int k = 0; k < 24; ++k) {
            const float a = 6.2831853f * static_cast<float>(k) / 24.0f;
            const float x = p.x + radius * std::cos(a), z = p.z + radius * std::sin(a);
            if (standableAt(L, level, x, z))
                return glm::vec3(x, p.y, z);
        }
    return p;
}

// ---- building the interior --------------------------------------------------------------------------------------------------------

namespace InteriorBuild {

struct Ctx {
    InteriorLayout& L;
    Scenery& s;
    SceneryRng rng;
    float floorY;
    explicit Ctx(InteriorLayout& layout) : L(layout), s(layout.props), rng(4242u), floorY(InteriorConfig::FLOOR_Y) {}
};

inline void box(Ctx& c, const glm::vec3& centre, const glm::vec3& size, const Material& m, float yaw = 0.0f)
{
    addBlock(c.s, centre, size, yaw, 0.0f, 0.0f, m);
}
inline void rod(Ctx& c, const glm::vec3& a, const glm::vec3& b, float thickness, const Material& m)
{
    sceneryAdd(c.s, PropMesh::CYLINDER, sceneryBetween(a, b, thickness), m, 0.5f * (a + b), 0.5f * glm::length(b - a) + thickness);
}
inline void ball(Ctx& c, const glm::vec3& centre, const glm::vec3& size, const Material& m)
{
    addEllipsoid(c.s, centre, size, m);
}
inline void drum(Ctx& c, const glm::vec3& centre, float diameter, float height, const Material& m)
{
    sceneryAdd(c.s, PropMesh::CYLINDER, sceneryMove(centre) * sceneryStretch(glm::vec3(diameter, height, diameter)), m, centre, 0.5f * std::max(height, diameter));
}
// Something you cannot walk through: the same box, and its footprint as a collider on `level`.
inline void solid(Ctx& c, int level, const glm::vec3& centre, const glm::vec3& size, const Material& m, float yaw = 0.0f)
{
    box(c, centre, size, m, yaw);
    const float hx = 0.5f * size.x, hz = 0.5f * size.z;
    c.L.colliders[level].push_back({ centre.x - hx, centre.z - hz, centre.x + hx, centre.z + hz });
}

// A treasure chest standing on the floor of the hold: it is registered as the ship's own treasure and is an obstacle the size of its footprint.
inline void chestAt(Ctx& c, int level, const glm::vec3& pos, float yaw, int gold, const char* name, bool hidden)
{
    c.L.chests.push_back({ pos, yaw, TreasureKind::SHIP, hidden, gold, name });
    const bool turned = std::fabs(std::sin(yaw)) > 0.7f;
    const float hx = turned ? 0.10f : 0.15f, hz = turned ? 0.15f : 0.10f;
    c.L.colliders[level].push_back({ pos.x - hx, pos.z - hz, pos.x + hx, pos.z + hz });
}

// A barrel: a drum, two iron hoops, and a lid. `solidToo` makes it an obstacle.
inline void barrel(Ctx& c, int level, const glm::vec3& base, float diameter, float height, const Material& wood, bool solidToo = true)
{
    drum(c, base + glm::vec3(0.0f, 0.5f * height, 0.0f), diameter, height, wood);
    for (int h = 0; h < 2; ++h)
        drum(c, base + glm::vec3(0.0f, height * (0.25f + 0.5f * static_cast<float>(h)), 0.0f), diameter * 1.05f, 0.025f, IRON_DARK);
    drum(c, base + glm::vec3(0.0f, height + 0.004f, 0.0f), diameter * 0.9f, 0.01f, HULL_WOOD);
    if (solidToo)
        c.L.colliders[level].push_back({ base.x - 0.5f * diameter, base.z - 0.5f * diameter, base.x + 0.5f * diameter, base.z + 0.5f * diameter });
}

inline void crate(Ctx& c, int level, const glm::vec3& base, float size, float yaw = 0.0f, bool solidToo = true)
{
    box(c, base + glm::vec3(0.0f, 0.5f * size, 0.0f), glm::vec3(size), CHEST_WOOD, yaw);
    box(c, base + glm::vec3(0.0f, 0.5f * size, 0.0f), glm::vec3(size * 1.02f, size * 0.12f, size * 1.02f), IRON_DARK, yaw);
    if (solidToo)
        c.L.colliders[level].push_back({ base.x - 0.5f * size, base.z - 0.5f * size, base.x + 0.5f * size, base.z + 0.5f * size });
}

// A lantern hung from the roof on a chain, and registered as a light.
inline void hangingLantern(Ctx& c, const glm::vec3& at, float scale = 1.0f)
{
    const float top = c.L.ceilingAt(at.z);
    rod(c, glm::vec3(at.x, top, at.z), glm::vec3(at.x, at.y + 0.07f * scale, at.z), 0.008f, IRON_DARK);
    box(c, at + glm::vec3(0.0f, 0.065f * scale, 0.0f), glm::vec3(0.07f, 0.014f, 0.07f) * scale, BRASS);
    ball(c, at, glm::vec3(0.07f, 0.09f, 0.07f) * scale, LANTERN_GLASS);
    box(c, at + glm::vec3(0.0f, -0.055f * scale, 0.0f), glm::vec3(0.05f, 0.012f, 0.05f) * scale, BRASS);
    c.s.lamps.push_back({ at, 1.0f });
    c.L.lamps.push_back(at);
}

// A ladder between two points: two rails and rungs.
inline void ladder(Ctx& c, const glm::vec3& foot, const glm::vec3& head, float width = 0.15f)
{
    const glm::vec3 d = head - foot;
    const float len = glm::length(d);
    glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), d / len));
    if (glm::dot(side, side) < 0.5f) side = glm::vec3(1.0f, 0.0f, 0.0f);
    for (int s2 = -1; s2 <= 1; s2 += 2)
        rod(c, foot + side * (0.5f * width * static_cast<float>(s2)), head + side * (0.5f * width * static_cast<float>(s2)), 0.016f, HULL_WOOD);
    const int rungs = std::max(3, static_cast<int>(len / 0.075f));
    for (int i = 1; i < rungs; ++i) {
        const glm::vec3 p = foot + d * (static_cast<float>(i) / static_cast<float>(rungs));
        rod(c, p - side * (0.5f * width), p + side * (0.5f * width), 0.012f, HULL_WOOD);
    }
}

inline void stairs(Ctx& c, const glm::vec3& bottom, const glm::vec3& top, float width, int steps)
{
    const glm::vec3 d = top - bottom;
    glm::vec3 run(d.x, 0.0f, d.z);
    const float runLen = glm::length(run);
    const glm::vec3 dir = run / runLen;
    const float yaw = std::atan2(dir.x, dir.z);
    for (int i = 0; i < steps; ++i) {
        const float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(steps);
        const glm::vec3 centre = bottom + glm::vec3(d.x * t, 0.0f, d.z * t);
        const float topY = bottom.y + d.y * (static_cast<float>(i) + 1.0f) / static_cast<float>(steps);
        const float h = topY - bottom.y + 0.02f;
        box(c, glm::vec3(centre.x, bottom.y + 0.5f * h - 0.01f, centre.z), glm::vec3(width, h, runLen / static_cast<float>(steps) + 0.004f), HOLD_PLANK, yaw);
    }
    for (int s2 = -1; s2 <= 1; s2 += 2) {
        const glm::vec3 off = glm::vec3(dir.z, 0.0f, -dir.x) * (0.5f * width * static_cast<float>(s2));
        rod(c, bottom + off + glm::vec3(0.0f, 0.2f, 0.0f), top + off + glm::vec3(0.0f, 0.2f, 0.0f), 0.014f, HULL_WOOD);          // the handrail
    }
}

} // namespace InteriorBuild

inline InteriorLayout buildInteriorLayout(const ShipDimensions& d, const CrewLayout& crew)
{
    using namespace InteriorBuild;
    InteriorLayout L;
    L.profile = d.hullProfile;
    L.beam = d.hullSize.x; L.depth = d.hullSize.y; L.length = d.hullSize.z;
    const float F = InteriorConfig::FLOOR_Y;

    L.levelY[LVL_HOLD] = F;
    L.levelY[LVL_DECK] = crew.waistY;
    L.levelY[LVL_QUARTER] = crew.quarterY;
    L.levelY[LVL_POOP] = crew.poopY;
    L.levelY[LVL_FORE] = crew.foreY;
    const DeckLevelDimensions& quarter = d.deckLevels[DECK_QUARTERDECK];
    const DeckLevelDimensions& poop = d.deckLevels[DECK_POOP];
    const DeckLevelDimensions& fore = d.deckLevels[DECK_FORECASTLE];
    L.zMin[LVL_HOLD] = -2.48f;      L.zMax[LVL_HOLD] = 2.0f;
    L.zMin[LVL_DECK] = quarter.zTo + 0.06f;   L.zMax[LVL_DECK] = fore.zFrom - 0.06f;
    L.zMin[LVL_QUARTER] = poop.zTo + 0.06f;   L.zMax[LVL_QUARTER] = quarter.zTo - 0.04f;
    L.zMin[LVL_POOP] = poop.zFrom + 0.06f;    L.zMax[LVL_POOP] = poop.zTo - 0.04f;
    L.zMin[LVL_FORE] = fore.zFrom + 0.06f;    L.zMax[LVL_FORE] = fore.zTo - 0.06f;
    L.xLimit[LVL_DECK] = 0.0f;                                                               // the waist follows the gunwale (see explorerLimitX)
    L.xLimit[LVL_QUARTER] = 0.5f * shipDeckLevelWidth(d, DECK_QUARTERDECK) - 0.08f;
    L.xLimit[LVL_POOP] = 0.5f * shipDeckLevelWidth(d, DECK_POOP) - 0.08f;
    L.xLimit[LVL_FORE] = 0.5f * shipDeckLevelWidth(d, DECK_FORECASTLE) - 0.08f;

    Ctx c(L);
    Scenery& s = L.props;
    const auto hw = [&](float z, float y) { return std::max(0.0f, L.halfWidthAt(z, y) - 0.03f); };

    // ================= the floor, the keelson, the frames and the roof beams =================
    // Floor boards: one slab per half-metre of the hold, each as wide as the hull is at floor height there.
    for (float z = -2.46f; z < 1.99f; z += 0.25f) {
        const float zc = z + 0.125f, w = hw(zc, F + 0.01f);
        if (w < 0.05f) continue;
        box(c, glm::vec3(0.0f, F - 0.02f, zc), glm::vec3(2.0f * w, 0.04f, 0.252f), HOLD_PLANK);
    }
    box(c, glm::vec3(0.0f, F + 0.012f, -0.25f), glm::vec3(0.08f, 0.05f, 4.4f), HULL_WOOD);                    // the keelson, the long timber down the middle
    for (float z = -2.3f; z < 1.9f; z += 0.45f)                                                                // floor timbers across it
        box(c, glm::vec3(0.0f, F + 0.003f, z), glm::vec3(2.0f * hw(z, F + 0.01f) * 0.96f, 0.012f, 0.045f), HULL_WOOD);
    // The frames (ribs) of the hull along both walls, and the beams that cross the roof.
    for (float z = -2.4f; z < 1.95f; z += 0.42f) {
        const float ceil = L.ceilingAt(z);
        for (int sd = -1; sd <= 1; sd += 2) {
            const float xs = static_cast<float>(sd);
            rod(c, glm::vec3(xs * (hw(z, F + 0.02f) - 0.005f), F + 0.02f, z), glm::vec3(xs * (hw(z, ceil - 0.02f) - 0.005f), ceil - 0.02f, z), 0.034f, HULL_WOOD);
        }
        box(c, glm::vec3(0.0f, ceil - 0.025f, z), glm::vec3(2.0f * hw(z, ceil - 0.03f), 0.05f, 0.05f), HULL_WOOD);
    }

    // ================= bulkheads: a wall across the hold with a doorway (and a door in the last) =================
    const float bulkZ[4] = { -1.58f, -0.58f, 0.58f, 1.32f };
    const float doorHalf[4] = { 0.17f, 0.17f, 0.17f, 0.14f };
    const float doorX[4] = { 0.0f, 0.0f, 0.0f, -0.05f };
    for (int b = 0; b < 4; ++b) {
        const float z = bulkZ[b], ceil = L.ceilingAt(z), w = hw(z, F + 0.01f), wTop = hw(z, ceil - 0.02f);
        const float doorTop = std::min(F + 0.36f, ceil - 0.04f);
        const float x0 = doorX[b] - doorHalf[b], x1 = doorX[b] + doorHalf[b];
        const float wallW = std::max(w, wTop) + 0.02f;
        // left panel (negative x), right panel (positive x), and the lintel over the door.
        solid(c, LVL_HOLD, glm::vec3(0.5f * (-wallW + x0), 0.5f * (F + ceil), z), glm::vec3(x0 + wallW, ceil - F, 0.05f), HULL_WOOD);
        solid(c, LVL_HOLD, glm::vec3(0.5f * (x1 + wallW), 0.5f * (F + ceil), z), glm::vec3(wallW - x1, ceil - F, 0.05f), HULL_WOOD);
        box(c, glm::vec3(doorX[b], 0.5f * (doorTop + ceil), z), glm::vec3(x1 - x0, ceil - doorTop, 0.05f), HULL_WOOD);
        box(c, glm::vec3(x0 - 0.012f, 0.5f * (F + doorTop), z), glm::vec3(0.024f, doorTop - F, 0.07f), CASTLE_WOOD);        // door frame
        box(c, glm::vec3(x1 + 0.012f, 0.5f * (F + doorTop), z), glm::vec3(0.024f, doorTop - F, 0.07f), CASTLE_WOOD);
        box(c, glm::vec3(doorX[b], doorTop + 0.012f, z), glm::vec3(x1 - x0 + 0.05f, 0.024f, 0.07f), CASTLE_WOOD);
        if (b == 3) {                                                  // the magazine\'s door, standing open against the wall, with its iron lock
            box(c, glm::vec3(x1 + 0.0f, 0.5f * (F + doorTop), z + 0.13f), glm::vec3(0.025f, doorTop - F - 0.02f, 0.22f), CASTLE_WOOD);
            box(c, glm::vec3(x1 + 0.02f, F + 0.17f, z + 0.2f), glm::vec3(0.02f, 0.05f, 0.05f), IRON_DARK);
        }
    }

    // ================= 1. the CAPTAIN'S CABIN (z -2.48 to -1.6) =================
    {
        box(c, glm::vec3(0.0f, F + 0.004f, -2.05f), glm::vec3(0.34f, 0.008f, 0.62f), FLAG_CLOTH);                              // a red runner down the middle
        // the bed along the starboard wall, with a blanket and a pillow
        const float bx = -0.30f, bz = -1.95f;
        solid(c, LVL_HOLD, glm::vec3(bx, F + 0.06f, bz), glm::vec3(0.34f, 0.12f, 0.58f), HULL_WOOD);
        box(c, glm::vec3(bx, F + 0.135f, bz + 0.05f), glm::vec3(0.32f, 0.045f, 0.42f), BLANKET);
        box(c, glm::vec3(bx, F + 0.16f, bz - 0.22f), glm::vec3(0.26f, 0.05f, 0.12f), SAILCLOTH);
        for (int sx = -1; sx <= 1; sx += 2) box(c, glm::vec3(bx + 0.17f * static_cast<float>(sx), F + 0.14f, bz - 0.29f), glm::vec3(0.02f, 0.28f, 0.02f), HULL_WOOD);   // bedposts
        // the desk across the stern with its chair and the things on it
        const float dz = -2.38f;
        solid(c, LVL_HOLD, glm::vec3(0.12f, F + 0.14f, dz), glm::vec3(0.58f, 0.04f, 0.26f), CASTLE_WOOD);
        box(c, glm::vec3(-0.15f, F + 0.07f, dz), glm::vec3(0.04f, 0.14f, 0.24f), CASTLE_WOOD);
        box(c, glm::vec3(0.39f, F + 0.07f, dz), glm::vec3(0.04f, 0.14f, 0.24f), CASTLE_WOOD);
        const float seatX = 0.24f, seatZ = dz + 0.28f;
        box(c, glm::vec3(seatX, F + 0.09f, seatZ), glm::vec3(0.18f, 0.03f, 0.18f), CASTLE_WOOD);                                // the chair
        box(c, glm::vec3(seatX, F + 0.17f, seatZ + 0.085f), glm::vec3(0.18f, 0.15f, 0.025f), CASTLE_WOOD);
        for (int a = -1; a <= 1; a += 2) for (int b2 = -1; b2 <= 1; b2 += 2)
            rod(c, glm::vec3(seatX + 0.08f * static_cast<float>(a), F, seatZ + 0.08f * static_cast<float>(b2)), glm::vec3(seatX + 0.08f * static_cast<float>(a), F + 0.09f, seatZ + 0.08f * static_cast<float>(b2)), 0.012f, CASTLE_WOOD);
        c.L.colliders[LVL_HOLD].push_back({ seatX - 0.09f, seatZ - 0.09f, seatX + 0.09f, seatZ + 0.09f });
        box(c, glm::vec3(0.30f, F + 0.19f, dz), glm::vec3(0.07f, 0.05f, 0.09f), BOOK_RED);                                       // books on the desk
        box(c, glm::vec3(0.30f, F + 0.235f, dz), glm::vec3(0.06f, 0.04f, 0.085f), BOOK_BLUE);
        box(c, glm::vec3(0.22f, F + 0.17f, dz + 0.04f), glm::vec3(0.15f, 0.004f, 0.11f), PAPER);                                 // a letter
        drum(c, glm::vec3(0.02f, F + 0.19f, dz - 0.05f), 0.035f, 0.05f, IRON_DARK);                                              // the inkwell
        rod(c, glm::vec3(0.02f, F + 0.215f, dz - 0.05f), glm::vec3(0.0f, F + 0.30f, dz - 0.08f), 0.006f, SAILCLOTH);             // a quill
        drum(c, glm::vec3(-0.08f, F + 0.19f, dz + 0.03f), 0.025f, 0.05f, SAILCLOTH);                                             // a candle
        ball(c, glm::vec3(-0.08f, F + 0.23f, dz + 0.03f), glm::vec3(0.018f, 0.03f, 0.018f), SUN_DISC);
        // the chart table beside the aisle: a big map, dividers, a compass, a sextant
        const float tx = 0.31f, tz = -1.80f;
        solid(c, LVL_HOLD, glm::vec3(tx, F + 0.12f, tz), glm::vec3(0.34f, 0.03f, 0.30f), CASTLE_WOOD);
        for (int a = -1; a <= 1; a += 2) for (int b2 = -1; b2 <= 1; b2 += 2) rod(c, glm::vec3(tx + 0.14f * static_cast<float>(a), F, tz + 0.12f * static_cast<float>(b2)), glm::vec3(tx + 0.14f * static_cast<float>(a), F + 0.12f, tz + 0.12f * static_cast<float>(b2)), 0.02f, CASTLE_WOOD);
        box(c, glm::vec3(tx, F + 0.1365f, tz), glm::vec3(0.32f, 0.004f, 0.26f), PAPER);
        box(c, glm::vec3(tx + 0.02f, F + 0.14f, tz - 0.02f), glm::vec3(0.19f, 0.002f, 0.003f), FLAG_CLOTH, 0.5f);               // a plotted course
        rod(c, glm::vec3(tx - 0.08f, F + 0.15f, tz - 0.05f), glm::vec3(tx - 0.05f, F + 0.2f, tz - 0.07f), 0.006f, BRASS);          // dividers
        rod(c, glm::vec3(tx - 0.08f, F + 0.15f, tz - 0.05f), glm::vec3(tx - 0.10f, F + 0.2f, tz - 0.07f), 0.006f, BRASS);
        drum(c, glm::vec3(tx + 0.08f, F + 0.15f, tz + 0.04f), 0.06f, 0.025f, BRASS);                                              // a compass
        ball(c, glm::vec3(tx + 0.08f, F + 0.163f, tz + 0.04f), glm::vec3(0.045f, 0.01f, 0.045f), POLISHED_SILVER);
        box(c, glm::vec3(tx + 0.10f, F + 0.15f, tz - 0.08f), glm::vec3(0.07f, 0.03f, 0.05f), BRASS);                              // the sextant
        rod(c, glm::vec3(tx + 0.10f, F + 0.17f, tz - 0.08f), glm::vec3(tx + 0.15f, F + 0.2f, tz - 0.12f), 0.006f, BRASS);
        // a globe on a stand by the bed's foot, and the bookshelf on the port wall
        const float gx = -0.12f, gz = -1.65f;
        rod(c, glm::vec3(gx, F, gz), glm::vec3(gx, F + 0.20f, gz), 0.016f, BRASS);
        ball(c, glm::vec3(gx, F + 0.26f, gz), glm::vec3(0.13f), POLISHED_SILVER);
        ball(c, glm::vec3(gx, F + 0.26f, gz), glm::vec3(0.135f, 0.02f, 0.135f), BRASS);
        const float sx = hw(-1.85f, F + 0.18f) - 0.06f;
        for (int lvl = 0; lvl < 2; ++lvl) {
            const float y = F + 0.12f + 0.15f * static_cast<float>(lvl);
            box(c, glm::vec3(sx, y, -1.85f), glm::vec3(0.12f, 0.02f, 0.46f), CASTLE_WOOD);
            const Material* bk[4] = { &BOOK_RED, &BOOK_GREEN, &BOOK_BLUE, &LEATHER };
            for (int k = 0; k < 7; ++k)
                box(c, glm::vec3(sx, y + 0.045f + 0.005f * static_cast<float>(k % 2), -1.85f - 0.19f + 0.063f * static_cast<float>(k)), glm::vec3(0.075f, 0.075f + 0.012f * static_cast<float>(k % 3), 0.045f), *bk[(k + lvl) % 4]);
        }
        rod(c, glm::vec3(sx - 0.02f, F, -1.85f), glm::vec3(sx - 0.02f, F + 0.3f, -1.85f), 0.012f, CASTLE_WOOD);
        hangingLantern(c, glm::vec3(0.0f, F + 0.34f, -2.05f));
        chestAt(c, LVL_HOLD, glm::vec3(-0.31f, F, -2.37f), 0.0f, 600, "captain's chest", true);                                    // tucked in the stern corner behind the bed
    }

    // ================= 2. the CREW QUARTERS (z -1.55 to -0.6) =================
    {
        // five hammocks slung between the frames, three of them with a sleeper in them
        for (int sd = -1; sd <= 1; sd += 2) {
            for (int k = 0; k < 3; ++k) {
                const float z = -1.40f + 0.27f * static_cast<float>(k);
                const float x = static_cast<float>(sd) * (hw(z, F + 0.2f) - 0.17f);
                if (sd < 0 && k == 2) continue;
                ball(c, glm::vec3(x, F + 0.185f, z), glm::vec3(0.17f, 0.06f, 0.26f), BLANKET);
                rod(c, glm::vec3(x, F + 0.185f, z - 0.13f), glm::vec3(x + static_cast<float>(sd) * 0.1f, F + 0.30f, z - 0.15f), 0.007f, ROPE_MAT);
                rod(c, glm::vec3(x, F + 0.185f, z + 0.13f), glm::vec3(x + static_cast<float>(sd) * 0.1f, F + 0.30f, z + 0.15f), 0.007f, ROPE_MAT);
                if (k == 0 || (sd > 0 && k == 1))
                    L.sleepers.push_back({ glm::vec3(x, F + 0.215f, z), 0.0f });
            }
        }
        // the mess table and its tin plates, by the port wall
        const float mx = -0.40f, mz = -1.10f;
        solid(c, LVL_HOLD, glm::vec3(mx, F + 0.115f, mz), glm::vec3(0.28f, 0.025f, 0.44f), CASTLE_WOOD);
        for (int a = -1; a <= 1; a += 2) for (int b2 = -1; b2 <= 1; b2 += 2) rod(c, glm::vec3(mx + 0.11f * static_cast<float>(a), F, mz + 0.19f * static_cast<float>(b2)), glm::vec3(mx + 0.11f * static_cast<float>(a), F + 0.115f, mz + 0.19f * static_cast<float>(b2)), 0.02f, CASTLE_WOOD);
        for (int k = 0; k < 3; ++k) { drum(c, glm::vec3(mx, F + 0.14f, mz - 0.14f + 0.14f * static_cast<float>(k)), 0.07f, 0.012f, POLISHED_SILVER); drum(c, glm::vec3(mx - 0.08f, F + 0.15f, mz - 0.1f + 0.12f * static_cast<float>(k)), 0.03f, 0.04f, BRASS); }
        // supplies: crates in the stern corner, barrels, sacks, coiled rope
        crate(c, LVL_HOLD, glm::vec3(-0.46f, F, -1.50f), 0.17f);
        crate(c, LVL_HOLD, glm::vec3(-0.46f, F + 0.17f, -1.50f), 0.15f, 0.3f, false);
        crate(c, LVL_HOLD, glm::vec3(-0.29f, F, -1.50f), 0.15f, 0.2f);
        barrel(c, LVL_HOLD, glm::vec3(0.47f, F, -1.50f), 0.17f, 0.22f, HULL_WOOD);
        barrel(c, LVL_HOLD, glm::vec3(0.50f, F, -1.31f), 0.17f, 0.22f, HULL_WOOD);
        barrel(c, LVL_HOLD, glm::vec3(0.38f, F, -1.50f), 0.15f, 0.2f, HULL_WOOD, false);
        for (int k = 0; k < 4; ++k) ball(c, glm::vec3(-0.52f + 0.07f * static_cast<float>(k % 2), F + 0.04f + 0.06f * static_cast<float>(k / 2), -0.75f + 0.05f * static_cast<float>(k)), glm::vec3(0.14f, 0.1f, 0.12f), SACK);
        drum(c, glm::vec3(0.40f, F + 0.025f, -0.72f), 0.2f, 0.05f, ROPE_MAT);
        // the companionway: stairs up to the main deck, straight ahead of the door from the cargo hold, with a handrail
        stairs(c, glm::vec3(0.0f, F, -0.85f), glm::vec3(0.0f, crew.waistY - 0.05f, -1.25f), 0.22f, 7);
        c.L.colliders[LVL_HOLD].push_back({ -0.12f, -1.30f, 0.12f, -0.85f });
        hangingLantern(c, glm::vec3(0.0f, F + 0.32f, -1.0f));
        chestAt(c, LVL_HOLD, glm::vec3(0.45f, F, -0.80f), -1.5708f, 350, "crew's chest", true);
    }

    // ================= 3. the CARGO HOLD (z -0.55 to 0.55): the hatch, chests, weapons, food, rope =================
    {
        ladder(c, glm::vec3(0.0f, F, 0.18f), glm::vec3(0.0f, L.ceilingAt(0.18f) + 0.02f, 0.18f));
        box(c, glm::vec3(0.0f, L.ceilingAt(0.18f) - 0.004f, 0.18f), glm::vec3(0.30f, 0.01f, 0.28f), GUNPORT_DARK);             // the hatch above, a dark square in the roof
        // cargo along both walls: crates, barrels; the port side leaves a pocket for a chest
        for (int sd = -1; sd <= 1; sd += 2) {
            const float xs = static_cast<float>(sd), x = xs * (hw(0.0f, F + 0.1f) - 0.20f);
            crate(c, LVL_HOLD, glm::vec3(x, F, -0.38f), 0.2f, 0.1f * xs);
            crate(c, LVL_HOLD, glm::vec3(x, F + 0.2f, -0.38f), 0.16f, 0.5f * xs, false);
            barrel(c, LVL_HOLD, glm::vec3(x, F, -0.12f), 0.19f, 0.25f, HULL_WOOD);
            if (sd > 0) barrel(c, LVL_HOLD, glm::vec3(x, F, 0.10f), 0.19f, 0.25f, HULL_WOOD);
            barrel(c, LVL_HOLD, glm::vec3(x - xs * 0.09f, F, 0.40f), 0.17f, 0.22f, HULL_WOOD, true);
        }
        // food: sacks of flour
        for (int k = 0; k < 5; ++k) ball(c, glm::vec3(0.34f + 0.05f * static_cast<float>(k % 2), F + 0.05f + 0.07f * static_cast<float>(k / 2), 0.47f - 0.05f * static_cast<float>(k)), glm::vec3(0.16f, 0.1f, 0.13f), SACK);
        // the weapons rack on the starboard wall: cutlasses and muskets in rows
        const float rx = hw(0.2f, F + 0.2f) - 0.04f;
        box(c, glm::vec3(rx, F + 0.22f, 0.2f), glm::vec3(0.025f, 0.03f, 0.5f), HULL_WOOD);
        box(c, glm::vec3(rx, F + 0.10f, 0.2f), glm::vec3(0.025f, 0.03f, 0.5f), HULL_WOOD);
        for (int k = 0; k < 5; ++k) {
            const float z = -0.02f + 0.1f * static_cast<float>(k);
            if (k < 3) rod(c, glm::vec3(rx - 0.01f, F + 0.07f, z), glm::vec3(rx - 0.01f, F + 0.27f, z), 0.014f, HULL_WOOD);            // a musket
            else { rod(c, glm::vec3(rx - 0.01f, F + 0.12f, z), glm::vec3(rx - 0.01f, F + 0.26f, z), 0.01f, POLISHED_SILVER); box(c, glm::vec3(rx - 0.01f, F + 0.115f, z), glm::vec3(0.03f, 0.012f, 0.045f), BRASS); }
        }
        // rope: coils on the floor
        drum(c, glm::vec3(-0.30f, F + 0.025f, 0.30f), 0.22f, 0.05f, ROPE_MAT);
        drum(c, glm::vec3(-0.30f, F + 0.07f, 0.30f), 0.17f, 0.04f, ROPE_MAT);
        // TWO treasure chests: one tucked in the pocket among the port barrels, one in plain sight on the floor
        chestAt(c, LVL_HOLD, glm::vec3(-0.50f, F, 0.12f), 1.5708f, 500, "chest among the barrels", true);
        chestAt(c, LVL_HOLD, glm::vec3(0.22f, F, -0.28f), 0.0f, 300, "cargo chest", false);
        hangingLantern(c, glm::vec3(0.0f, F + 0.30f, 0.0f));
    }

    // ================= 4. the GALLEY, below decks (z 0.62 to 1.3) =================
    {
        const float sx = -(hw(1.1f, F + 0.15f) - 0.17f);
        solid(c, LVL_HOLD, glm::vec3(sx, F + 0.09f, 1.05f), glm::vec3(0.30f, 0.18f, 0.34f), CLIFF_ROCK);                           // the stove
        box(c, glm::vec3(sx, F + 0.19f, 1.05f), glm::vec3(0.32f, 0.02f, 0.36f), IRON_DARK);
        box(c, glm::vec3(sx + 0.151f, F + 0.07f, 1.05f), glm::vec3(0.012f, 0.09f, 0.14f), GUNPORT_DARK);                          // the fire door
        box(c, glm::vec3(sx + 0.157f, F + 0.055f, 1.05f), glm::vec3(0.012f, 0.05f, 0.1f), SUN_DISC);                              // the fire inside
        drum(c, glm::vec3(sx, F + 0.20f + 0.03f, 1.05f), 0.16f, 0.08f, IRON_DARK);                                                // the cauldron
        rod(c, glm::vec3(sx - 0.05f, F + 0.2f, 1.17f), glm::vec3(sx - 0.05f, L.ceilingAt(1.17f), 1.17f), 0.04f, CLIFF_ROCK);       // the flue
        // the work table with a cutting board and knives, water and flour barrels
        solid(c, LVL_HOLD, glm::vec3(0.22f, F + 0.12f, 0.98f), glm::vec3(0.30f, 0.03f, 0.2f), CASTLE_WOOD);
        for (int a = -1; a <= 1; a += 2) for (int b2 = -1; b2 <= 1; b2 += 2) rod(c, glm::vec3(0.22f + 0.12f * static_cast<float>(a), F, 0.98f + 0.08f * static_cast<float>(b2)), glm::vec3(0.22f + 0.12f * static_cast<float>(a), F + 0.12f, 0.98f + 0.08f * static_cast<float>(b2)), 0.016f, CASTLE_WOOD);
        box(c, glm::vec3(0.20f, F + 0.138f, 0.98f), glm::vec3(0.14f, 0.012f, 0.10f), DECK_WOOD);
        box(c, glm::vec3(0.30f, F + 0.14f, 0.97f), glm::vec3(0.012f, 0.008f, 0.09f), POLISHED_SILVER);
        drum(c, glm::vec3(0.14f, F + 0.15f, 1.02f), 0.06f, 0.03f, BRASS);
        barrel(c, LVL_HOLD, glm::vec3(0.46f, F, 1.18f), 0.2f, 0.26f, HULL_WOOD);
        barrel(c, LVL_HOLD, glm::vec3(0.46f, F, 0.72f), 0.2f, 0.26f, HULL_WOOD);
        // shelves of pots on the wall, utensils hanging from the roof, dried fish
        const float shx = hw(1.1f, F + 0.2f) - 0.07f;
        box(c, glm::vec3(shx, F + 0.20f, 1.0f), glm::vec3(0.10f, 0.02f, 0.5f), CASTLE_WOOD);
        for (int k = 0; k < 4; ++k) drum(c, glm::vec3(shx, F + 0.24f, 0.82f + 0.12f * static_cast<float>(k)), 0.07f, 0.06f, (k % 2) ? BRASS : IRON_DARK);
        for (int k = 0; k < 4; ++k) {
            const float ux = 0.06f + 0.07f * static_cast<float>(k), uz = 0.74f;
            rod(c, glm::vec3(ux, L.ceilingAt(uz) - 0.04f, uz), glm::vec3(ux, F + 0.32f - 0.02f * static_cast<float>(k), uz), 0.005f, IRON_DARK);
        }
        for (int k = 0; k < 3; ++k) ball(c, glm::vec3(0.14f + 0.05f * static_cast<float>(k), F + 0.30f, 1.22f), glm::vec3(0.1f, 0.03f, 0.03f), SAILCLOTH);
        hangingLantern(c, glm::vec3(0.05f, F + 0.30f, 1.0f));
        L.cookPos = glm::vec3(sx + 0.24f, F, 1.05f);
        L.cookYaw = -1.5708f;
    }

    // ================= 5. the POWDER MAGAZINE (z 1.36 to 2.0), at the bow =================
    {
        // kegs in two columns, one each side of the aisle; a keg on top of the stack; a crate; a pyramid of round shot
        for (int row = 0; row < 2; ++row)
            for (int sd = -1; sd <= 1; sd += 2) {
                const float kz = 1.56f + 0.19f * static_cast<float>(row), kx = static_cast<float>(sd) * (hw(kz, F + 0.12f) - 0.17f);
                if (hw(kz, F + 0.12f) < 0.3f) continue;
                barrel(c, LVL_HOLD, glm::vec3(kx, F, kz), 0.15f, 0.2f, KEG_RED);
                drum(c, glm::vec3(kx, F + 0.206f, kz), 0.07f, 0.014f, IRON_DARK);                                                // the bung
            }
        barrel(c, LVL_HOLD, glm::vec3(0.25f, F + 0.2f, 1.56f), 0.14f, 0.18f, KEG_RED, false);                                      // one on top
        crate(c, LVL_HOLD, glm::vec3(-0.25f, F, 1.88f), 0.16f);
        for (int row = 0; row < 3; ++row)
            for (int k = 0; k < 3 - row; ++k)
                ball(c, glm::vec3(0.12f + 0.07f * static_cast<float>(k) + 0.035f * static_cast<float>(row), F + 0.035f + 0.06f * static_cast<float>(row), 1.88f), glm::vec3(0.065f), BLACK_PLASTIC);
        drum(c, glm::vec3(0.05f, F + 0.025f, 1.40f), 0.07f, 0.05f, BRASS);                                                          // a powder measure
        // the magazine lamp is outside the room, behind glass in the bulkhead, so no flame is ever near the powder
        box(c, glm::vec3(0.0f, F + 0.30f, 1.33f), glm::vec3(0.10f, 0.12f, 0.03f), IRON_DARK);
        ball(c, glm::vec3(0.0f, F + 0.30f, 1.36f), glm::vec3(0.07f, 0.09f, 0.04f), LANTERN_GLASS);
        c.s.lamps.push_back({ glm::vec3(0.0f, F + 0.30f, 1.40f), 1.0f });
        L.lamps.push_back(glm::vec3(0.0f, F + 0.30f, 1.46f));
        chestAt(c, LVL_HOLD, glm::vec3(0.0f, F, 1.94f), 3.14159f, 700, "chest behind the powder", true);
    }

    // ================= the decks above: the hatch and the companionway on the main deck, the ladders to the castles =================
    {
        Connector hatchA, stairs2;
        hatchA.lower = LVL_HOLD; hatchA.upper = LVL_DECK; hatchA.kind = ConnectorKind::HATCH; hatchA.name = "cargo hatch";
        hatchA.lowerFoot = glm::vec3(0.0f, F, 0.32f); hatchA.upperFoot = glm::vec3(0.0f, crew.waistY, 0.18f);
        stairs2.lower = LVL_HOLD; stairs2.upper = LVL_DECK; stairs2.kind = ConnectorKind::STAIRS; stairs2.name = "companionway";
        stairs2.lowerFoot = glm::vec3(0.0f, F, -0.74f); stairs2.upperFoot = glm::vec3(0.0f, crew.waistY, -1.22f);
        L.connectors.push_back(hatchA); L.connectors.push_back(stairs2);
        Connector q; q.lower = LVL_DECK; q.upper = LVL_QUARTER; q.kind = ConnectorKind::LADDER; q.name = "ladder to the quarterdeck";
        q.lowerFoot = crew.quarterLadder[0] + glm::vec3(-0.12f, 0.0f, -0.02f); q.upperFoot = crew.quarterLadder[1] + glm::vec3(-0.10f, 0.0f, -0.03f);
        Connector p; p.lower = LVL_QUARTER; p.upper = LVL_POOP; p.kind = ConnectorKind::LADDER; p.name = "ladder to the poop deck";
        p.lowerFoot = crew.poopLadder[0] + glm::vec3(-0.12f, 0.0f, -0.02f); p.upperFoot = crew.poopLadder[1] + glm::vec3(-0.10f, 0.0f, -0.03f);
        Connector f; f.lower = LVL_DECK; f.upper = LVL_FORE; f.kind = ConnectorKind::LADDER; f.name = "ladder to the forecastle";
        f.lowerFoot = crew.foreLadder[0] + glm::vec3(-0.12f, 0.0f, 0.02f); f.upperFoot = crew.foreLadder[1] + glm::vec3(-0.10f, 0.0f, 0.04f);
        L.connectors.push_back(q); L.connectors.push_back(p); L.connectors.push_back(f);
    }

    // ================= the deck furniture you cannot walk through =================
    {
        // Footprints of what the crew layout (src/Crew.h) stands on the decks - the sizes it is drawn at in src/CrewDraw.h, a little generous.
        std::vector<Box2>& D = L.colliders[LVL_DECK];
        const auto at = [](const glm::vec3& p, float hx, float z0, float z1) { return Box2{ p.x - hx, p.z + z0, p.x + hx, p.z + z1 }; };
        for (int i = 0; i < SHIP_MAST_COUNT; ++i) D.push_back(at(crew.mastFoot[i], 0.05f, -0.05f, 0.05f));
        D.push_back(at(crew.cannonPos, 0.11f, -0.20f, 0.20f));
        for (int i = 0; i < 4; ++i) D.push_back(at(crew.dummyGun[i].pos, 0.16f, -0.11f, 0.11f));              // turned to face their ports: long across the ship
        D.push_back(at(crew.galleyPos, 0.5f * crew.galleySize.x + 0.03f, -0.5f * crew.galleySize.z - 0.03f, 0.5f * crew.galleySize.z + 0.03f));
        D.push_back(at(crew.cauldronPos, 0.09f, -0.09f, 0.09f));
        for (int i = 0; i < 2; ++i) {
            D.push_back(at(crew.barrelStack[i], 0.17f, -0.09f, 0.09f));
            D.push_back(at(crew.crateStack[i], 0.09f, -0.09f, 0.265f));
        }
        D.push_back(at(crew.workbench, 0.09f, -0.18f, 0.18f));
        D.push_back(at(crew.shotRack, 0.075f, -0.075f, 0.075f));
        std::vector<Box2>& Q = L.colliders[LVL_QUARTER];
        // The captain\'s cabin on the quarterdeck: a front wall, a port wall, and an inboard wall with a doorway in it.
        const float x0 = crew.cabinMin.x, x1 = crew.cabinMax.x, z0 = crew.cabinMin.y, z1 = crew.cabinMax.y;
        const float gap0 = z0 + 0.5f * (z1 - z0) - 0.07f - 0.04f, gap1 = z0 + 0.5f * (z1 - z0) + 0.07f + 0.04f;
        Q.push_back({ x0, z1 - 0.015f, x1, z1 + 0.015f });
        Q.push_back({ x0 - 0.015f, z0, x0 + 0.015f, z1 });
        Q.push_back({ x1 - 0.015f, z0, x1 + 0.015f, gap0 });
        Q.push_back({ x1 - 0.015f, gap1, x1 + 0.015f, z1 });
        Q.push_back({ crew.tablePos.x - 0.5f * crew.tableSize.x, crew.tablePos.z - 0.5f * crew.tableSize.y, crew.tablePos.x + 0.5f * crew.tableSize.x, crew.tablePos.z + 0.5f * crew.tableSize.y });
        std::vector<Box2>& P = L.colliders[LVL_POOP];
        P.push_back({ crew.wheelHub.x - 0.08f, crew.wheelHub.z - 0.06f, crew.wheelHub.x + 0.08f, crew.wheelHub.z + 0.06f });
    }
    // The foot of every ladder, hatch and stair: beside it, but on ground a person can stand on.
    for (Connector& k : L.connectors) {
        k.lowerFoot = snapToStandable(L, k.lower, k.lowerFoot);
        k.upperFoot = snapToStandable(L, k.upper, k.upperFoot);
    }
    return L;
}

// ---- the walker ----------------------------------------------------------------------------------------------------------------

struct Explorer {
    int level = LVL_DECK;
    glm::vec3 pos = glm::vec3(0.0f);         // feet, hull-local
    bool climbing = false;
    float climbT = 0.0f;
    glm::vec3 climbFrom = glm::vec3(0.0f), climbTo = glm::vec3(0.0f);
    int climbConnector = -1, climbToLevel = LVL_DECK;
    float walkPhase = 0.0f;
    bool moving = false;
};

inline void explorerSpawn(Explorer& e, const InteriorLayout& L)
{
    e = Explorer();
    e.level = LVL_DECK;
    e.pos = snapToStandable(L, LVL_DECK, glm::vec3(0.0f, L.levelY[LVL_DECK], -0.20f));
}

// How far to either side the walker may go at z on `level`.
inline float explorerLimitX(const InteriorLayout& L, int level, float z)
{
    if (level == LVL_HOLD)
        return std::max(0.0f, L.halfWidthAt(z, L.levelY[LVL_HOLD] + 0.12f) - InteriorConfig::BODY_RADIUS - 0.02f);
    if (level == LVL_DECK)
        return hullGunwaleHalfBeamAt(L.profile, L.beam, L.length, z) - 0.10f;
    return L.xLimit[level];
}

// Which connector is within reach, and whether the walker stands at its lower end. -1 if none.
inline int explorerNearConnector(const Explorer& e, const InteriorLayout& L, bool& atLower)
{
    int best = -1;
    float bestD = InteriorConfig::USE_REACH;
    for (int i = 0; i < static_cast<int>(L.connectors.size()); ++i) {
        const Connector& c = L.connectors[i];
        if (e.level == c.lower) {
            const float d = glm::length(glm::vec2(e.pos.x - c.lowerFoot.x, e.pos.z - c.lowerFoot.z));
            if (d < bestD) { bestD = d; best = i; atLower = true; }
        }
        if (e.level == c.upper) {
            const float d = glm::length(glm::vec2(e.pos.x - c.upperFoot.x, e.pos.z - c.upperFoot.z));
            if (d < bestD) { bestD = d; best = i; atLower = false; }
        }
    }
    return best;
}

inline void explorerStartClimb(Explorer& e, const InteriorLayout& L, int connector, bool fromLower)
{
    const Connector& c = L.connectors[connector];
    e.climbing = true;
    e.climbT = 0.0f;
    e.climbConnector = connector;
    e.climbFrom = fromLower ? c.lowerFoot : c.upperFoot;
    e.climbTo = fromLower ? c.upperFoot : c.lowerFoot;
    e.climbToLevel = fromLower ? c.upper : c.lower;
}

// One step of walking: `wish` is the wanted velocity in the hull's x and z (already turned to where the walker is looking), `speed` its length per second.
inline void stepExplorer(Explorer& e, const InteriorLayout& L, const glm::vec2& wish, float dt)
{
    e.moving = false;
    if (e.climbing) {
        const float length = std::max(glm::length(e.climbTo - e.climbFrom), 0.05f);
        e.climbT += InteriorConfig::CLIMB_SPEED * dt / length;
        const float t = std::min(e.climbT, 1.0f);
        e.pos = glm::mix(e.climbFrom, e.climbTo, t);
        e.walkPhase += 5.0f * dt;
        e.moving = true;
        if (e.climbT >= 1.0f) {
            e.climbing = false;
            e.level = e.climbToLevel;
            e.pos = e.climbTo;
        }
        return;
    }
    if (glm::dot(wish, wish) < 1e-6f || dt <= 0.0f)
        return;
    const float r = InteriorConfig::BODY_RADIUS;
    glm::vec3 p = e.pos + glm::vec3(wish.x, 0.0f, wish.y) * dt;
    p.z = std::clamp(p.z, L.zMin[e.level], L.zMax[e.level]);
    const float lim = explorerLimitX(L, e.level, p.z);
    p.x = std::clamp(p.x, -lim, lim);
    for (int pass = 0; pass < 2; ++pass)
        for (const Box2& b : L.colliders[e.level]) {
            const float x0 = b.x0 - r, x1 = b.x1 + r, z0 = b.z0 - r, z1 = b.z1 + r;
            if (p.x > x0 && p.x < x1 && p.z > z0 && p.z < z1) {
                const float pushL = p.x - x0, pushR = x1 - p.x, pushB = p.z - z0, pushF = z1 - p.z;
                const float m = std::min(std::min(pushL, pushR), std::min(pushB, pushF));
                if (m == pushL) p.x = x0; else if (m == pushR) p.x = x1; else if (m == pushB) p.z = z0; else p.z = z1;
            }
        }
    p.z = std::clamp(p.z, L.zMin[e.level], L.zMax[e.level]);                      // and the walls last of all: an obstacle pushes you aside, never out of the ship
    {
        const float limit = explorerLimitX(L, e.level, p.z);
        p.x = std::clamp(p.x, -limit, limit);
    }
    e.pos = glm::vec3(p.x, L.levelY[e.level], p.z);
    e.walkPhase += glm::length(wish) * dt * 7.5f;
    e.moving = true;
}

// The walker's eye: hull-local, a head\'s height above the feet, with a small bob while walking and a lurch on a ladder.
inline glm::vec3 explorerEye(const Explorer& e)
{
    const float bob = e.moving ? 0.012f * std::sin(e.walkPhase) : 0.0f;
    return e.pos + glm::vec3(0.0f, InteriorConfig::EYE_HEIGHT + bob, 0.0f);
}

// True while the walker is in the hold: the camera is inside the hull, and the outside world is hidden (see renderInterior).
inline bool explorerInHold(const Explorer& e)
{
    if (!e.climbing)
        return e.level == LVL_HOLD;
    const bool fromHold = e.level == LVL_HOLD, toHold = e.climbToLevel == LVL_HOLD;
    if (fromHold && toHold)
        return true;
    if (fromHold)
        return e.climbT < 0.5f;                  // the outside takes over at the middle of the climb, where the picture is black
    return toHold && e.climbT >= 0.5f;
}
