#pragma once
// The ship, built the same way every object in this project is built: a
// small chain of named local positions, each measured from the ship's own
// root, drawn with the project's reusable unit meshes tinted a different
// colour per part with 'uTint' (Phase 3).
//
// No lighting and no motion yet. Every part below is one flat colour, and
// nothing in this file reads the clock. This file only answers one
// question: where does each named part of the ship sit, relative to the
// ship's own root?

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Shader.h"

#include <cmath>

// Every size and colour the ship is built from, gathered in one place so a
// teacher can ask for "a taller mast" or "a bigger cannon" and get it with
// one changed number.
namespace ShipShape {

// Hull: the ship's main body. Its bottom sits exactly at y = 0, the water
// surface, so HULL_HEIGHT alone decides how "deep" the ship sits.
// HULL_BOTTOM_SCALE narrows the hull toward the keel (see
// makeUnitShipHull(), src/Mesh.h) - the hull is not just a tapered box any
// more, its WIDTH also changes along its length: pinched at the bow, full
// width at its widest point, and a flat transom at the stern.
constexpr float HULL_WIDTH = 2.0f;
constexpr float HULL_HEIGHT = 1.0f;
constexpr float HULL_LENGTH = 6.2f;
constexpr float HULL_BOTTOM_SCALE = 0.5f;

// Deck: a thin slab sitting on top of the hull, a little shorter than the
// hull itself so the pointed bow and the stern both show past its edges,
// the way a real deck sits inboard of the hull's own outline.
constexpr float DECK_WIDTH = 1.85f;
constexpr float DECK_HEIGHT = 0.10f;
constexpr float DECK_LENGTH = 5.6f;

// The height of the deck's own top surface above the hull's bottom (y = 0).
// Named once here so anything that needs to sit "on deck" - the cannon, the
// wheel, the crew, and any deck prop - measures from this instead of each
// repeating the same HULL_HEIGHT + DECK_HEIGHT sum.
constexpr float DECK_TOP_Y = HULL_HEIGHT + DECK_HEIGHT;

// The quarterdeck: a shorter, raised deck section at the stern, the way
// real sailing ships gave the helmsman and the officers a raised platform
// rather than standing at the same level as the rest of the crew. The
// wheel stands on this, not on the main deck.
constexpr float QUARTERDECK_LENGTH = 1.0f;
constexpr float QUARTERDECK_WIDTH = DECK_WIDTH * 0.9f;
constexpr float QUARTERDECK_HEIGHT = 0.35f;
constexpr float QUARTERDECK_Z = 2.3f;
constexpr float QUARTERDECK_TOP_Y = DECK_TOP_Y + QUARTERDECK_HEIGHT;

// Three masts, standing on the deck at these three positions along its
// length - fore, main, and mizzen, the same arrangement a real three-masted
// sailing ship uses, instead of the two-mast layout this project started
// with.
constexpr float MAST_RADIUS = 0.055f;
constexpr float MAST_HEIGHT = 2.6f;
constexpr float FORE_MAST_Z = -1.9f;
constexpr float MAIN_MAST_Z = 0.15f;
constexpr float MIZZEN_MAST_Z = 2.1f;

// One sail per mast, hanging at roughly the mast's own centre height.
constexpr float SAIL_WIDTH = 1.3f;
constexpr float SAIL_HEIGHT = 1.5f;
constexpr float SAIL_THICKNESS = 0.05f;

// A flag on top of the mizzen mast - the back-most of the three, where a
// ship's flag traditionally flies.
constexpr float FLAG_WIDTH = 0.46f;
constexpr float FLAG_HEIGHT = 0.28f;
constexpr float FLAG_THICKNESS = 0.02f;

// The bowsprit: a pole angled up and forward from the bow, the single most
// recognisable "this is a sailing ship" silhouette detail after the sails
// themselves.
constexpr float BOWSPRIT_RADIUS = 0.055f;
constexpr float BOWSPRIT_LENGTH = 1.3f;
constexpr float BOWSPRIT_PITCH_DEGREES = -75.0f;   // tilt up from pointing straight out

// The cannon: mount, then yoke, then barrel, each a child of the one before
// it, on the right-hand (+X) side of the deck.
constexpr float MOUNT_SIZE_X = 0.38f;
constexpr float MOUNT_SIZE_Y = 0.19f;
constexpr float MOUNT_SIZE_Z = 0.38f;
constexpr float MOUNT_X = 0.90f;
constexpr float MOUNT_Z = -0.9f;

constexpr float YOKE_SIZE = 0.27f;

constexpr float BARREL_RADIUS = 0.07f;
constexpr float BARREL_LENGTH = 1.05f;

// The wheel (the helm): a post with a spoked wheel on top, standing on the
// quarterdeck near the stern, opposite the bow.
constexpr float WHEEL_POST_RADIUS = 0.045f;
constexpr float WHEEL_POST_HEIGHT = 0.45f;
constexpr float WHEEL_RADIUS = 0.24f;
constexpr float WHEEL_THICKNESS = 0.05f;
constexpr float WHEEL_Z = 2.55f;

// Portholes: small round windows along both sides of the hull. Their colour
// is fixed rather than part of a ship's palette below - a dark opening looks
// the same regardless of whose ship it is.
constexpr float PORTHOLE_RADIUS = 0.08f;
constexpr float PORTHOLE_ZS[4] = { -1.6f, -0.5f, 0.6f, 1.7f };
const glm::vec3 PORTHOLE_COLOR(0.05f, 0.05f, 0.07f);

// All the colours ONE ship needs. Two ships can now be told apart just by
// using two different ShipPalette values - the same idea Stage I's planned
// fleet already relies on ("the SAME drawShip() ... a different hull
// material," docs/PHASE_PLAN.md) - rather than by building different
// geometry.
struct ShipPalette {
    glm::vec3 hull;
    glm::vec3 deck;
    glm::vec3 mast;
    glm::vec3 sail;
    glm::vec3 flag;
    glm::vec3 cannonMetal;
    glm::vec3 barrel;
    glm::vec3 wheel;
};

// The player's ship: warm wood tones, a pale sail, a red flag.
const ShipPalette PLAYER_PALETTE{
    /* hull */        glm::vec3(0.42f, 0.26f, 0.13f),
    /* deck */        glm::vec3(0.76f, 0.64f, 0.42f),
    /* mast */        glm::vec3(0.25f, 0.16f, 0.08f),
    /* sail */        glm::vec3(0.93f, 0.93f, 0.90f),
    /* flag */        glm::vec3(0.75f, 0.12f, 0.12f),
    /* cannonMetal */ glm::vec3(0.35f, 0.35f, 0.38f),
    /* barrel */      glm::vec3(0.55f, 0.42f, 0.18f),
    /* wheel */       glm::vec3(0.25f, 0.16f, 0.08f),
};

// The enemy's ship: the same shape, built from the same meshes, told apart
// on sight by a darker hull, a dark sail, and a dark flag instead of a red
// one.
const ShipPalette ENEMY_PALETTE{
    /* hull */        glm::vec3(0.16f, 0.16f, 0.18f),
    /* deck */        glm::vec3(0.30f, 0.28f, 0.27f),
    /* mast */        glm::vec3(0.10f, 0.09f, 0.09f),
    /* sail */        glm::vec3(0.20f, 0.19f, 0.20f),
    /* flag */        glm::vec3(0.10f, 0.10f, 0.11f),
    /* cannonMetal */ glm::vec3(0.30f, 0.28f, 0.29f),
    /* barrel */      glm::vec3(0.30f, 0.22f, 0.14f),
    /* wheel */       glm::vec3(0.10f, 0.09f, 0.09f),
};

} // namespace ShipShape

// Draws one complete ship - hull, deck, quarterdeck, three masts with
// sails, a bowsprit, a flag, a cannon, and a wheel - all placed relative to
// 'shipRoot' and coloured from 'palette'. Every part is one of the
// project's reusable meshes, scaled to size and tinted its own colour.
//
// The four functions below each draw one group of parts - the hull and its
// rigging, the cannon, the flag, and the wheel - so any one of them can be
// shown on its own (a teacher asking to see just the cannon, for instance)
// as well as together. drawShip() at the bottom is the convenience that
// draws all four, which is what "the whole ship" has always meant in this
// project.

// A tiny local helper, shared by every function below, so each part is one
// readable line: set where it is, set what colour it is, draw it.
inline void drawTintedPart(
    ShaderProgram& shader, const Mesh& mesh, const glm::mat4& model, const glm::vec3& color)
{
    shader.setMat4("uModel", model);
    shader.setVec3("uTint", color);
    mesh.draw();
}

// The hull, the deck, the quarterdeck, three masts and sails, the
// bowsprit, and the portholes along both sides of the hull - everything
// about the ship except the cannon, the flag, and the wheel, which are
// their own functions below.
inline void drawHullAndRigging(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitCylinder,
    const Mesh& unitSphere,
    const Mesh& hullShape,
    const glm::mat4& shipRoot,
    const ShipShape::ShipPalette& palette)
{
    using namespace ShipShape;

    // Hull: centred at HULL_HEIGHT / 2 so its bottom face sits at y = 0.
    // 'hullShape' is makeUnitShipHull() (src/Mesh.h) - full width at its
    // widest point, pinched to a point at the bow, and a flat transom at
    // the stern, with the bottom pulled in toward the keel throughout.
    const glm::mat4 hullFrame =
        glm::translate(shipRoot, glm::vec3(0.0f, HULL_HEIGHT * 0.5f, 0.0f));
    drawTintedPart(
        shader, hullShape,
        glm::scale(hullFrame, glm::vec3(HULL_WIDTH, HULL_HEIGHT, HULL_LENGTH)),
        palette.hull);

    // Deck: sits right on top of the hull.
    const float deckCenterY = HULL_HEIGHT + DECK_HEIGHT * 0.5f;
    const glm::mat4 deckFrame = glm::translate(shipRoot, glm::vec3(0.0f, deckCenterY, 0.0f));
    drawTintedPart(
        shader, unitCube,
        glm::scale(deckFrame, glm::vec3(DECK_WIDTH, DECK_HEIGHT, DECK_LENGTH)),
        palette.deck);

    // Quarterdeck: a second, shorter deck slab, raised above the main deck,
    // near the stern - where the wheel stands.
    const glm::mat4 quarterdeckFrame = glm::translate(
        shipRoot, glm::vec3(0.0f, DECK_TOP_Y + QUARTERDECK_HEIGHT * 0.5f, QUARTERDECK_Z));
    drawTintedPart(
        shader, unitCube,
        glm::scale(quarterdeckFrame, glm::vec3(QUARTERDECK_WIDTH, QUARTERDECK_HEIGHT, QUARTERDECK_LENGTH)),
        palette.deck);

    // Masts and sails: fore, main, and mizzen, each with its own sail
    // attached at the same position (a stylised "the sail hangs here"
    // rather than a fully separate rigging chain, which is Stage D's
    // later, more detailed job).
    const float mastZs[3] = { FORE_MAST_Z, MAIN_MAST_Z, MIZZEN_MAST_Z };
    for (float mastZ : mastZs) {
        const float mastCenterY = DECK_TOP_Y + MAST_HEIGHT * 0.5f;
        const glm::mat4 mastFrame =
            glm::translate(shipRoot, glm::vec3(0.0f, mastCenterY, mastZ));

        drawTintedPart(
            shader, unitCylinder,
            glm::scale(mastFrame, glm::vec3(MAST_RADIUS * 2.0f, MAST_HEIGHT, MAST_RADIUS * 2.0f)),
            palette.mast);

        drawTintedPart(
            shader, unitCube,
            glm::scale(mastFrame, glm::vec3(SAIL_WIDTH, SAIL_HEIGHT, SAIL_THICKNESS)),
            palette.sail);
    }

    // Bowsprit: a pole reaching out past the bow, tilted up from
    // horizontal by BOWSPRIT_PITCH_DEGREES. The unit cylinder's own axis is
    // Y; rotating it about X by that pitch turns that axis to point mostly
    // forward (-Z) and a little upward (+Y) - 'dir' below is exactly that
    // direction, used both to aim the pole and to slide it out from its
    // attachment point at the bow by half its own length, the same
    // "translate along the real target direction, rotate the mesh to
    // match" pairing the barrel and the wheel's spokes already use.
    const float bowspritPitch = glm::radians(BOWSPRIT_PITCH_DEGREES);
    const glm::vec3 bowspritDir(0.0f, std::cos(bowspritPitch), std::sin(bowspritPitch));
    const glm::mat4 bowspritBase = glm::translate(
        shipRoot, glm::vec3(0.0f, HULL_HEIGHT * 0.62f, -HULL_LENGTH * 0.5f));
    const glm::mat4 bowspritFrame = bowspritBase
        * glm::translate(glm::mat4(1.0f), bowspritDir * (BOWSPRIT_LENGTH * 0.5f))
        * glm::rotate(glm::mat4(1.0f), bowspritPitch, glm::vec3(1.0f, 0.0f, 0.0f));
    drawTintedPart(
        shader, unitCylinder,
        glm::scale(bowspritFrame, glm::vec3(BOWSPRIT_RADIUS * 2.0f, BOWSPRIT_LENGTH, BOWSPRIT_RADIUS * 2.0f)),
        palette.mast);

    // Portholes: small round windows, four down each side of the hull, at
    // the hull's own mid-height. Their centre sits exactly on the hull's
    // outer surface, so half of each sphere reads as a round window flush
    // with the wood.
    const float portholeY = HULL_HEIGHT * 0.5f;
    const float sides[2] = { -1.0f, 1.0f };
    for (float side : sides) {
        for (float z : PORTHOLE_ZS) {
            const glm::mat4 portholeFrame = glm::translate(
                shipRoot, glm::vec3(side * HULL_WIDTH * 0.5f, portholeY, z));
            drawTintedPart(
                shader, unitSphere,
                glm::scale(portholeFrame, glm::vec3(PORTHOLE_RADIUS * 2.0f)),
                PORTHOLE_COLOR);
        }
    }
}

// The cannon: mount, then yoke, then barrel, each a child of the one before
// it, on the right-hand (+X) side of 'shipRoot'.
inline void drawCannon(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitCylinder,
    const glm::mat4& shipRoot,
    const ShipShape::ShipPalette& palette = ShipShape::PLAYER_PALETTE)
{
    using namespace ShipShape;

    // Mount: sits on the deck, off to one side.
    const glm::mat4 mountFrame = glm::translate(
        shipRoot, glm::vec3(MOUNT_X, DECK_TOP_Y + MOUNT_SIZE_Y * 0.5f, MOUNT_Z));
    drawTintedPart(
        shader, unitCube,
        glm::scale(mountFrame, glm::vec3(MOUNT_SIZE_X, MOUNT_SIZE_Y, MOUNT_SIZE_Z)),
        palette.cannonMetal);

    // Yoke: a child of the mount, stacked directly on top of it.
    const glm::mat4 yokeFrame = glm::translate(
        mountFrame, glm::vec3(0.0f, MOUNT_SIZE_Y * 0.5f + YOKE_SIZE * 0.5f, 0.0f));
    drawTintedPart(shader, unitCube, glm::scale(yokeFrame, glm::vec3(YOKE_SIZE)), palette.cannonMetal);

    // Barrel: a child of the yoke. The unit cylinder's own axis is Y, so it
    // is rotated -90 degrees around Z first, which turns that axis to point
    // along +X - outward, over the side of the ship - THEN slid out from the
    // yoke's centre by half its own length, so its base stays at the yoke
    // and only its length extends outward. Reading right to left (the same
    // rule Phase 6 established): scale sizes the unit cylinder, rotate turns
    // it on the spot, translate carries the already-turned barrel outward.
    const glm::mat4 barrelFrame = yokeFrame
        * glm::translate(glm::mat4(1.0f), glm::vec3(BARREL_LENGTH * 0.5f, 0.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    drawTintedPart(
        shader, unitCylinder,
        glm::scale(barrelFrame, glm::vec3(BARREL_RADIUS * 2.0f, BARREL_LENGTH, BARREL_RADIUS * 2.0f)),
        palette.barrel);
}

// The flag, on top of the mizzen mast.
inline void drawFlag(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const glm::mat4& shipRoot,
    const ShipShape::ShipPalette& palette = ShipShape::PLAYER_PALETTE)
{
    using namespace ShipShape;

    const float mizzenTopY = DECK_TOP_Y + MAST_HEIGHT;
    const glm::mat4 flagFrame = glm::translate(
        shipRoot, glm::vec3(0.0f, mizzenTopY + FLAG_HEIGHT * 0.5f, MIZZEN_MAST_Z));
    drawTintedPart(
        shader, unitCube,
        glm::scale(flagFrame, glm::vec3(FLAG_WIDTH, FLAG_HEIGHT, FLAG_THICKNESS)),
        palette.flag);
}

// The wheel: a post standing on the quarterdeck, with a spoked wheel turned
// to face along the ship's own length - the same way a real helm is
// something the crew look ALONG the ship to use, not across it.
//
// A real ship's wheel reads as a wheel because of its SPOKES, not because
// of its outline - a plain solid disc is just as round but has no line
// dividing it into anything, so it looks like a puck rather than a wheel.
// This is built from a dark backing disc (the same round silhouette as
// before) with a lighter hub and lighter spokes sitting slightly in front
// of it - the colour difference between the two is what makes each spoke
// readable as its own line, since this project has no lighting yet to
// shade a groove or a raised edge into visibility.
inline void drawWheel(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitCylinder,
    const glm::mat4& shipRoot,
    const ShipShape::ShipPalette& palette)
{
    using namespace ShipShape;

    const float baseY = QUARTERDECK_TOP_Y;

    const glm::mat4 postFrame = glm::translate(
        shipRoot, glm::vec3(0.0f, baseY + WHEEL_POST_HEIGHT * 0.5f, WHEEL_Z));
    drawTintedPart(
        shader, unitCube,
        glm::scale(postFrame, glm::vec3(WHEEL_POST_RADIUS * 2.0f, WHEEL_POST_HEIGHT, WHEEL_POST_RADIUS * 2.0f)),
        palette.wheel);

    // Every part of the wheel shares this ONE rotation: the unit cylinder's
    // own axis is Y (it would stand up like a second post); rotating it 90
    // degrees around X turns that axis to point along Z instead, laying
    // every disc and spoke flat in the X-Y plane so the whole wheel's round
    // face looks straight along the ship's length, exactly where a
    // helmsman standing behind it would see it.
    const glm::mat4 wheelCenter = glm::translate(
        shipRoot, glm::vec3(0.0f, baseY + WHEEL_POST_HEIGHT, WHEEL_Z));
    const glm::mat4 facing = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // The dark backing disc - the full wheel's silhouette, in a darkened
    // version of the wheel colour so the lighter spokes stand out against it.
    const glm::vec3 darkWood = palette.wheel * 0.45f;
    drawTintedPart(
        shader, unitCylinder,
        glm::scale(wheelCenter * facing, glm::vec3(WHEEL_RADIUS * 2.0f, WHEEL_THICKNESS, WHEEL_RADIUS * 2.0f)),
        darkWood);

    // The hub and spokes sit slightly forward of the backing disc (towards
    // whoever is standing at the wheel), so they never fight the backing
    // disc for the same pixels.
    const glm::mat4 raised = glm::translate(wheelCenter, glm::vec3(0.0f, 0.0f, WHEEL_THICKNESS)) * facing;

    constexpr float hubRadius = WHEEL_RADIUS * 0.24f;
    drawTintedPart(
        shader, unitCylinder,
        glm::scale(raised, glm::vec3(hubRadius * 2.0f, WHEEL_THICKNESS, hubRadius * 2.0f)),
        palette.wheel);

    // Spokes: 'translate' first extends a thin cylinder from the centre
    // outward along its OWN local Y axis, from y = 0 to y = WHEEL_RADIUS.
    // 'rotate(angle, Z)', applied AFTER that (so further left, read right
    // to left), then sweeps that whole radiating spoke around the centre by
    // 'angle' - one end stays pinned at the centre while the other end
    // traces the wheel's rim - which is what makes each spoke point
    // radially outward instead of every spoke pointing the same direction.
    constexpr int spokeCount = 8;
    constexpr float spokeRadius = WHEEL_RADIUS * 0.05f;
    for (int i = 0; i < spokeCount; ++i) {
        const float angle = 360.0f * static_cast<float>(i) / static_cast<float>(spokeCount);
        const glm::mat4 spokeFrame = raised
            * glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, WHEEL_RADIUS * 0.5f, 0.0f));
        drawTintedPart(
            shader, unitCylinder,
            glm::scale(spokeFrame, glm::vec3(spokeRadius * 2.0f, WHEEL_RADIUS, spokeRadius * 2.0f)),
            palette.wheel);
    }
}

// The whole ship: hull, rigging, cannon, flag, and wheel together - what
// "the ship" has always meant in the combined scene.
inline void drawShip(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitCylinder,
    const Mesh& unitSphere,
    const Mesh& hullShape,
    const glm::mat4& shipRoot,
    const ShipShape::ShipPalette& palette = ShipShape::PLAYER_PALETTE)
{
    drawHullAndRigging(shader, unitCube, unitCylinder, unitSphere, hullShape, shipRoot, palette);
    drawCannon(shader, unitCube, unitCylinder, shipRoot, palette);
    drawFlag(shader, unitCube, shipRoot, palette);
    drawWheel(shader, unitCube, unitCylinder, shipRoot, palette);
}
