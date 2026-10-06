#pragma once

// Environment build: DRAWING THE SOLID PARTS of the war at sea (src/Fx.h has the state): holes shot in a hull, torn sails, the anchor and its chain, the rudder, flying and floating
// debris, powder kegs, and the ships far away. Included by main.cpp after WorldDraw.h; everything goes through drawMesh() like the rest of the scene, from the cube, sphere and
// cylinder, so the draw counters, the mirror pass (the sea shows all this upside down) and the lighting apply with no special case.

#include "Fx.h"
#include "WorldDraw.h"

// ---- materials for the ships far away --------------------------------------------------------------------------------------------------
inline const Material SAIL_BLACK = CHARCOAL;
inline const Material SAIL_CREAM = AGED_BEIGE;
inline const Material SAIL_WHITE = { { 0.30f, 0.31f, 0.33f }, { 0.88f, 0.89f, 0.92f }, { 0.05f, 0.05f, 0.05f }, 4.0f, "naval sail" };
inline const Material CHAR_BLACK = { { 0.01f, 0.01f, 0.01f }, { 0.03f, 0.025f, 0.02f }, { 0.02f, 0.02f, 0.02f }, 4.0f, "char" };

// ---- holes ---------------------------------------------------------------------------------------------------------------------------------

// Each hole: a charred ring, a black opening, and four splintered planks bent outwards round the rim. `age` lets a fresh hole glow at its edge for a moment.
inline void drawHoles(const CrewDrawContext& c, const glm::mat4& hull, const ShipFx& fx, float burning)
{
    for (int i = 0; i < fx.holeCount; ++i) {
        const Hole& h = fx.holes[i];
        const glm::mat4 F = hull * crewMove(h.local.x, h.local.y, h.local.z) * crewRotY(h.side > 0.0f ? 0.0f : 3.14159f);
        const float s = h.size;
        Material rim = CHAR_BLACK;
        rim.ke = glm::vec3(0.55f, 0.18f, 0.03f) * (burning * 0.5f);
        drawMesh(c.shader, c.stats, c.sphere, F * crewMove(0.0f, 0.0f, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.035f, 1.35f * s, 1.25f * s)), rim);
        drawMesh(c.shader, c.stats, c.sphere, F * crewMove(0.012f, 0.0f, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.03f, 0.95f * s, 0.85f * s)), GUNPORT_DARK);
        for (int k = 0; k < 4; ++k) {
            const float a = 0.78f + 1.57f * static_cast<float>(k) + 0.4f * static_cast<float>((i + k) % 3);
            crewBox(c, F * crewRotX(a) * crewMove(0.0f, 0.0f, 0.0f), glm::vec3(0.035f + 0.04f * s, 0.0f, 0.55f * s), glm::vec3(0.035f, 0.06f * s + 0.012f, 0.50f * s), (k % 2) ? CASTLE_WOOD : HULL_WOOD);
        }
    }
}

// ---- torn sails ----------------------------------------------------------------------------------------------------------------------------

// Tears in a sail: dark ragged patches (a rectangle and a turned one together) at places fixed by the mast and the tear's number, on both faces. `frame` is the sail's frame
// (hanging from the yard), `size` the sail's drawn size, `torn` how many tears.
inline void drawSailTears(const CrewDrawContext& c, const glm::mat4& frame, const glm::vec2& size, int torn, int mast, bool back)
{
    for (int k = 0; k < torn; ++k) {
        const unsigned int h = fleetHash(mast * 17 + k, k * 5 + 3);
        const float ux = (fleetU(h, 1) - 0.5f) * 0.72f * size.x, uy = (0.14f + 0.62f * fleetU(h, 2)) * size.y;
        const float w = (0.13f + 0.10f * fleetU(h, 3)) * size.x, hh = (0.10f + 0.12f * fleetU(h, 4)) * size.y;
        const float z = back ? -0.012f : 0.012f;
        crewBox(c, frame * crewMove(ux, -uy, z) * crewRotZ(0.6f * fleetU(h, 5) - 0.3f), glm::vec3(0.0f), glm::vec3(w, hh, 0.006f), CHAR_BLACK);
        crewBox(c, frame * crewMove(ux + 0.3f * w, -uy - 0.25f * hh, z * 1.2f) * crewRotZ(0.8f + fleetU(h, 6)), glm::vec3(0.0f), glm::vec3(0.55f * w, 0.55f * hh, 0.006f), CHAR_BLACK);
    }
}

// ---- the anchor ------------------------------------------------------------------------------------------------------------------------------

// Where the anchor hangs when stowed, hull-local: at the starboard bow, tucked against the hull below the hawse.
inline glm::vec3 anchorStowedLocal(const ShipDimensions& d) { return glm::vec3(0.34f * d.hullSize.x, 0.04f * d.hullSize.y, 0.5f * d.hullSize.z - 0.50f); }
inline glm::vec3 anchorHawseLocal(const ShipDimensions& d) { return glm::vec3(0.30f * d.hullSize.x, 0.26f * d.hullSize.y, 0.5f * d.hullSize.z - 0.42f); }

// The anchor and its chain. Stowed, it hangs at the bow against the hull with a little chain to the hawse. Lowered, it drops straight down from there through the sea
// (`depth01` of its full ANCHOR_DEPTH) with the chain paying out behind it in a sagging line; it follows the ship because both ends are measured from the ship's own frame.
inline void drawAnchor(const CrewDrawContext& c, const glm::mat4& hull, const ShipDimensions& d, float depth01, float heading)
{
    const glm::vec3 hawse = glm::vec3(hull * glm::vec4(anchorHawseLocal(d), 1.0f));
    const glm::vec3 stowed = glm::vec3(hull * glm::vec4(anchorStowedLocal(d), 1.0f));
    const glm::vec3 pos = stowed + glm::vec3(0.0f, -depth01 * FxConfig::ANCHOR_DEPTH, 0.0f);
    const glm::mat4 A = glm::translate(glm::mat4(1.0f), pos) * crewRotY(heading + 1.5708f);
    // the anchor: a shank, a stock across its top, two curved flukes (angled blocks) and a ring
    drawMesh(c.shader, c.stats, c.cylinder, A * glm::scale(glm::mat4(1.0f), glm::vec3(0.034f, 0.44f, 0.034f)), IRON_DARK);
    drawMesh(c.shader, c.stats, c.cylinder, A * crewMove(0.0f, 0.17f, 0.0f) * crewRotX(1.5708f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.028f, 0.24f, 0.028f)), IRON_DARK);
    for (int s = -1; s <= 1; s += 2)
        crewBox(c, A * crewMove(0.0f, -0.20f, 0.075f * static_cast<float>(s)) * crewRotX(0.7f * static_cast<float>(s)), glm::vec3(0.0f), glm::vec3(0.035f, 0.05f, 0.14f), IRON_DARK);
    crewBall(c, A, glm::vec3(0.0f, -0.205f, 0.0f), glm::vec3(0.07f), IRON_DARK);
    crewBall(c, A, glm::vec3(0.0f, 0.225f, 0.0f), glm::vec3(0.055f), IRON_DARK);
    // the chain: 22 short links from the hawse to the ring, sagging between
    const glm::vec3 ring = pos + glm::vec3(0.0f, 0.225f, 0.0f);
    constexpr int LINKS = 22;
    glm::vec3 prev = hawse;
    for (int i = 1; i <= LINKS; ++i) {
        const float u = static_cast<float>(i) / LINKS;
        glm::vec3 p = glm::mix(hawse, ring, u);
        p += glm::vec3(0.0f, 0.0f, 0.0f) + glm::vec3(0.12f * std::sin(3.1416f * u) * (1.0f - depth01 * 0.5f), 0.0f, 0.0f);
        if (i < LINKS || depth01 > 0.0f)
            drawMesh(c.shader, c.stats, c.cylinder, sceneryBetween(prev, glm::mix(prev, p, 0.82f), 0.020f), IRON_DARK);
        prev = p;
    }
}

// ---- the rudder -------------------------------------------------------------------------------------------------------------------------------

// A blade hung on the stern post below the waterline, turning about its forward edge by `angle` (positive turns the stern to starboard).
inline void drawRudder(const CrewDrawContext& c, const glm::mat4& hull, const ShipDimensions& d, float angle)
{
    const glm::mat4 R = hull * crewMove(0.0f, -0.36f * d.hullSize.y, -0.5f * d.hullSize.z + 0.04f) * crewRotY(angle);
    crewBox(c, R, glm::vec3(0.0f, 0.0f, -0.13f), glm::vec3(0.035f, 0.40f * d.hullSize.y, 0.27f), HULL_WOOD);
    drawMesh(c.shader, c.stats, c.cylinder, R * glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.42f * d.hullSize.y, 0.04f)), IRON_DARK);       // the pintles
}

// ---- debris ------------------------------------------------------------------------------------------------------------------------------------

inline void drawDebris(const CrewDrawContext& c, const DebrisPool& pool)
{
    for (const DebrisPiece& d : pool.piece) {
        if (!d.alive) continue;
        const glm::mat4 M = glm::translate(glm::mat4(1.0f), d.pos) * glm::rotate(glm::mat4(1.0f), d.angle, d.axis) * glm::scale(glm::mat4(1.0f), d.size);
        drawMesh(c.shader, c.stats, c.cube, M, d.charred ? CHAR_BLACK : (((&d - pool.piece) & 1) ? CASTLE_WOOD : HULL_WOOD));
    }
}

// ---- powder kegs ----------------------------------------------------------------------------------------------------------------------------------

// A keg of powder: a red barrel with two iron hoops and a black fuse hole on top. `fuse` >= 0 means it is burning towards a blast: it glows and flickers.
inline void drawKeg(const CrewDrawContext& c, const glm::mat4& frame, float fuse, float time)
{
    Material keg = KEG_RED;
    if (fuse >= 0.0f)
        keg.ke = glm::vec3(0.7f, 0.2f, 0.04f) * (0.6f + 0.4f * std::sin(40.0f * time));
    crewDrum(c, frame, glm::vec3(0.0f, 0.11f, 0.0f), 0.19f, 0.22f, keg);
    crewDrum(c, frame, glm::vec3(0.0f, 0.05f, 0.0f), 0.205f, 0.02f, IRON_DARK);
    crewDrum(c, frame, glm::vec3(0.0f, 0.17f, 0.0f), 0.205f, 0.02f, IRON_DARK);
    crewDrum(c, frame, glm::vec3(0.0f, 0.225f, 0.0f), 0.09f, 0.012f, GUNPORT_DARK);
}

// ---- the ships far away ------------------------------------------------------------------------------------------------------------------------

// A ship of the fleet, at level of detail `lod` (0 near, 1 middle, 2 far). It is built from the same hull mesh and the same frame functions as the player's, at a different
// scale and with a few proportions changed, so the three kinds have different silhouettes: the pirate low and dark with black sails, the merchant fat-sailed and lighter,
// the naval ship taller with white canvas. The far level is a hull, two masts and their sails; the middle adds the yards, the deck and the flag; the near one the castles.
inline void drawFleetShip(ShaderProgram& shader, RenderStats& stats, const Mesh& hullMesh, const Mesh& cubeMesh, const Mesh& cylinderMesh, const Mesh& quadMesh, const Mesh& sailMesh,
                          const glm::mat4& root, const ShipDimensions& baseDims, FleetKind kind, float scale, int lod, float time, float windRelative, float wind, float sailSet)
{
    ShipDimensions d = scaleShipDimensions(baseDims, scale);
    const Material* sail = &SAIL_BLACK;
    Material hullMat = HULL_TIMBER;
    if (kind == FleetKind::MERCHANT) {
        sail = &SAIL_CREAM;
        hullMat.ka *= glm::vec3(0.8f, 0.95f, 0.7f); hullMat.kd *= glm::vec3(0.8f, 0.95f, 0.7f);
        for (int i = 0; i < SHIP_MAST_COUNT; ++i) { d.masts[i].height *= 0.86f; d.masts[i].sailSize.x *= 1.22f; d.masts[i].yardLength *= 1.22f; }
    } else if (kind == FleetKind::NAVAL) {
        sail = &SAIL_WHITE;
        hullMat.ka *= glm::vec3(0.45f, 0.50f, 0.62f); hullMat.kd *= glm::vec3(0.45f, 0.50f, 0.62f);
        for (int i = 0; i < SHIP_MAST_COUNT; ++i) { d.masts[i].height *= 1.12f; d.masts[i].sailSize.y *= 1.12f; }
    } else {
        hullMat.ka *= 0.7f; hullMat.kd *= 0.7f;
    }
    const ShipFrames f = buildShipFrames(root, d);
    drawMesh(shader, stats, hullMesh, shipHullModel(f, d), hullMat);
    if (lod <= 1)
        drawMesh(shader, stats, cubeMesh, shipDeckModel(f, d), DECK_WOOD);
    if (lod == 0)
        for (int i = 0; i < DECK_LEVEL_COUNT; ++i) {
            drawMesh(shader, stats, cubeMesh, shipDeckBlockModel(f, d, i), CASTLE_WOOD);
            drawMesh(shader, stats, cubeMesh, shipDeckSlabModel(f, d, i), DECK_WOOD);
        }
    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        const MastFrames& r = f.rig[i];
        const MastDimensions& m = d.masts[i];
        drawMesh(shader, stats, cylinderMesh, shipMastModel(r, m), HULL_WOOD);
        // the yards are braced round to catch the wind, and the canvas belly a little, both as closed forms of the clock
        const float brace = 0.35f * std::sin(windRelative);
        const glm::mat4 turn = r.mast * glm::rotate(glm::mat4(1.0f), brace, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::inverse(r.mast);
        const glm::mat4 belly = glm::rotate(glm::mat4(1.0f), wind * 0.07f * std::sin(1.6f * time + 1.7f * static_cast<float>(i)), glm::vec3(1.0f, 0.0f, 0.0f));
        if (lod <= 1) {
            MastFrames braced = r;
            braced.yard = turn * r.yard;
            drawMesh(shader, stats, cylinderMesh, shipYardModel(braced, m), HULL_WOOD);
        }
        MastDimensions set = m;
        set.sailSize.y *= std::clamp(sailSet, 0.05f, 1.0f);
        drawMesh(shader, stats, sailMesh, shipSailModel(turn * r.sail * belly, set), *sail);
    }
    if (lod <= 1) {
        drawMesh(shader, stats, quadMesh, shipFlagModel(f.flag * glm::rotate(glm::mat4(1.0f), 0.3f * wind * std::sin(7.0f * time) + 0.7f * std::sin(windRelative), glm::vec3(0.0f, 1.0f, 0.0f)), d),
                 (kind == FleetKind::NAVAL) ? SAIL_WHITE : FLAG_CLOTH);
    }
}
