#pragma once

// The ship hierarchy: Phase 34 built the root, hull and deck; Phase 35 adds the rigging.
//
// THE RULE THIS WHOLE FILE EXISTS TO ENFORCE
//
//     A stored frame holds a TRANSLATION and a ROTATION and never a scale.
//     Scale is applied once, at the very last moment, in the matrix handed to
//     drawMesh().
//
// Phase 16 introduced that rule for single objects. It becomes the most important
// rule in the project here, because a ship is a TREE of objects and a tree inherits.
//
// Suppose the hull were stored as `root * scale(0.42, 0.20, 1.40)` and the deck were
// built as that matrix times a translation. The deck would inherit the hull's
// squashing: 0.20 tall on y, 1.40 long on z - so a deck meant to be a thin flat slab
// would come out as the hull's own proportions again, and a mast built on the deck
// would be stretched 1.40x along z and squashed to a fifth of its height. Every
// child, grandchild and great-grandchild would carry the error with it.
//
// The fix is to keep two different things apart:
//
//     FRAME   where a node is and how it is turned - stored, inherited by children
//     SIZE    how big it is                        - applied only when drawing
//
// This file builds frames. It contains NO OpenGL at all, which is deliberate: it
// means the frames can be tested exactly on the CPU, and checked for the property
// that matters - that every one of them is a pure rigid motion.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Hull.h"
#include "Planks.h"

constexpr float SHIP_PI = 3.14159265358979f;

// Phase 59: a galleon rig - fore, main and mizzen, each carrying a course,
// topsail and topgallant yard.  The original course-yard fields remain the
// source dimensions, while shipYardDimensions() derives the two upper sails;
// one edit therefore keeps the whole tapered stack in proportion.
constexpr int SHIP_MAST_COUNT = 3;
constexpr int SHIP_YARD_COUNT = 3;
constexpr int FORE_MAST = 0;
constexpr int MAIN_MAST = 1;
constexpr int MIZZEN_MAST = 2;

// One mast and everything hung from it.
struct MastDimensions {
    // Where on the deck the mast stands, along the ship's length. +z is the bow.
    float z;

    // The mast is a cylinder standing on the deck.
    float height;
    float diameter;

    // The yard is the horizontal spar the sail hangs from. It crosses the mast at this
    // FRACTION of the mast's height, so making a mast taller moves its yard up with it.
    float yardHeightFraction;
    float yardLength;
    float yardDiameter;

    // The sail hangs below the yard: width across, height down.
    glm::vec2 sailSize;
};

struct YardDimensions {
    float heightFraction;
    float length;
    float diameter;
    glm::vec2 sailSize;
};

inline YardDimensions shipYardDimensions(const MastDimensions& m, int yard)
{
    static const float HEIGHT_ADD[SHIP_YARD_COUNT] = { 0.0f, 0.105f, 0.185f };
    static const float LENGTH_SCALE[SHIP_YARD_COUNT] = { 1.0f, 0.76f, 0.54f };
    static const float SAIL_WIDTH[SHIP_YARD_COUNT] = { 1.0f, 0.74f, 0.51f };
    static const float SAIL_HEIGHT[SHIP_YARD_COUNT] = { 1.0f, 0.58f, 0.38f };
    const int k = glm::clamp(yard, 0, SHIP_YARD_COUNT - 1);
    return {
        glm::min(0.965f, m.yardHeightFraction + HEIGHT_ADD[k]),
        m.yardLength * LENGTH_SCALE[k],
        m.yardDiameter * (1.0f - 0.16f * static_cast<float>(k)),
        glm::vec2(m.sailSize.x * SAIL_WIDTH[k], m.sailSize.y * SAIL_HEIGHT[k])
    };
}

// Phase 36: the cannon. A block on the deck (the mount), a pivot that turns about the
// vertical (the yoke), and a barrel that tilts about a horizontal axis.
struct CannonDimensions {
    // Where on the deck the mount stands: x across the ship, z along it. Hull-local, so
    // it moves with the ship.
    glm::vec2 mountXZ;

    // The mount is a plain block resting on the deck.
    glm::vec3 mountSize;

    // How far above the mount's top the barrel's pivot (its trunnion) sits. The barrel
    // is a tube with a radius, so a pivot ON the mount's top would bury half the
    // barrel in it.
    float trunnionHeight;

    // MUZZLE_Z: how far along the barrel, from its pivot, the muzzle is. This is also
    // the barrel's length, because the barrel runs from the pivot to the muzzle. It is
    // the named number the projectile will be launched from in Phase 86.
    float muzzleZ;
    float barrelDiameter;
};

// Where the cannon is currently AIMED. State, not dimensions: Phase 85 puts these on the
// arrow keys and Phase 87 lets a target drive them. For now they are fixed numbers that
// can be edited by hand.
struct CannonPose {
    // Radians. Positive turns the barrel from the bow (+z) towards +x. About the
    // vertical, on the yoke.
    float azimuth;

    // Radians. Positive RAISES the muzzle. About a horizontal axis, on the barrel.
    float elevation;
};

// ---- Phase 50: deck levels -----------------------------------------------------------------
//
// A galleon is not a flat deck. Its stern is a tall stepped castle - the QUARTERDECK, with the shorter
// POOP deck on top of it - and its bow carries a lower FORECASTLE; between them the deck sinks into the
// WAIST. Seen from the side that stepped profile is the silhouette of the type.
//
// Each level is a BLOCK (a box of timber from the level below up to this level's floor) with a thin
// planked SLAB on top, and a FRAME at the middle of its floor. The frame is a pure translation of its parent:
// the quarterdeck's and forecastle's parent is the hull, the poop's is the quarterdeck, because the poop
// stands on it. Everything that will stand on a level - railings, the helm, a mast - becomes a child of
// that frame, and the level's size is applied only when it is drawn.
constexpr int DECK_LEVEL_COUNT = 3;
constexpr int DECK_QUARTERDECK = 0;
constexpr int DECK_POOP = 1;
constexpr int DECK_FORECASTLE = 2;

struct DeckLevelDimensions {
    // Where the level lies along the hull, measured from the hull's CENTRE (+z is the bow): its aft and its
    // forward end. Its WIDTH is not stored: a level is as wide as the hull allows at its narrowest end, less
    // `inset` on each side, so it follows the hull's table (src/Hull.h) when that is edited.
    float zFrom;
    float zTo;

    // The height of the walking surface above the KEEL LINE. (The main deck's is the waist's gunwale plus
    // the deck slab's thickness.)
    float floorHeight;

    // How far inside the hull's gunwale the block stands, each side.
    float inset;
};

// ---- Phase 52: railings ---------------------------------------------------------------------
//
// A rail is a row of posts with a rail along their tops. Posts stand on whatever deck lies under them and follow the
// outline of the hull; they are placed at EQUAL spacing along a RUN - a stretch of one deck's edge - and a run's ends come
// from the deck-level table, not from numbers of their own, so editing a level moves its railings with it.
//
//   waist        between the quarterdeck and the forecastle, on the hull's own cap, along the gunwale
//   quarterdeck  the part of its edge the poop does not cover
//   poop         the whole of its edge
//   forecastle   the whole of its edge
//
// Each run is built on BOTH sides, a mirror image of itself. Posts and rails are the unit cube at draw time; nothing here is a
// frame that carries a size.
constexpr int RAIL_RUN_COUNT = 4;
constexpr int RAIL_WAIST = 0;
constexpr int RAIL_QUARTERDECK = 1;
constexpr int RAIL_POOP = 2;
constexpr int RAIL_FORECASTLE = 3;

struct RailDimensions {
    // The spacing the posts are placed at, aimed for. A run's ends are fixed, so the actual spacing is the run's length shared
    // out equally among a whole number of gaps, which is the nearest to this.
    float spacing;

    float postHeight;   // above the surface the post stands on
    float postSize;     // each post is a square column this wide
    float railSize;     // the rail along the tops is a square bar this thick
    float embed;        // how far the post's foot sinks into the surface, so there is never a gap under it
    float sideGap;      // on the waist: how far inside the gunwale's edge the post's outer face stands
};

// ---- Phase 53: gunports ---------------------------------------------------------------------
//
// A gunport is the opening in a ship's side a cannon fires through. Here it is a dark rectangle on the planking inside a slightly
// lower frame, so it reads as a recess: the opening stands a little further off the hull than the frame round it, and the frame is
// one board darker than the planks. Nothing is cut out of the hull - the mesh is closed and stays closed.
//
// Where: a row of GUNPORT_COUNT on each side, at the z stations of a table. How high: centred on the line between strakes 10 and
// 11 (row 10.0 of the loft, the middle of the topsides' upper half), which keeps every port, frame included, between the deck-line
// belt (strake 9) and the gunwale rail (strake 12). Because the row is a fraction of the hull's own height, a port follows the
// sheer: it rises towards the ends of the ship with the planking it sits on.
constexpr int GUNPORT_COUNT = 8;

namespace GunportShape {
constexpr float ROW = 10.0f;     // the loft row the middle of every port lies on (a row is 1 strake; 9 .. 11 is strakes 10 and 11)
}

struct GunportDimensions {
    float z[GUNPORT_COUNT];  // the stations, along the ship from the hull's centre (+z is the bow)
    float width;             // along the ship, the opening
    float height;            // up the side, the opening
    float frame;             // the frame's width round the opening, all four sides
    float proud;             // how far the opening's outer face stands off the planking at the port's centre
    float frameProud;        // the same for the frame: less than `proud`, so the opening is the nearer to the eye
    float embed;             // how far both sink into the hull, so no gap can show where the planking curves away
};
// How big the parts of the ship are. A plain struct passed by value into the frame
// builder, rather than constants read from inside it, so that the same code can be
// run with a different ship and the result compared. "Change the hull dimensions and
// show the deck is unaffected" is then a call with a different argument, not an edit.
struct ShipDimensions {
    // Full extents along the ship's own axes: x across the beam, y depth, z along its
    // length with the bow towards +z.
    glm::vec3 hullSize;

    // Phase 47: the hull's SHAPE - ten cross-sections from stern to stem (src/Hull.h). Every number in
    // it is a fraction, so it describes the hull at any size: hullSize says how big, this says what shape.
    HullProfile hullProfile;

    // The deck is a thin slab laid on top of the hull. Its size is its OWN, and is
    // independent of the hull's - that independence is the Phase 34 checkpoint.
    glm::vec3 deckSize;

    // Phase 47: where along the hull the deck slab is centred, measured from the hull's centre (+z is the
    // bow). It was 0 while the hull was a box. A galleon's hull is pointed at the bow and narrow aft, so a
    // rectangular slab can only lie where the hull is wide - amidships, a little aft of centre.
    float deckCenterZ;

    // How far the hull sits below the waterline. The root is at the waterline, so a
    // hull centred on the root would float half out of the water on a ship that
    // should sit IN it.
    float hullDraft;

    // Phase 35: the rigging.
    MastDimensions masts[SHIP_MAST_COUNT];

    // The flag flown from the main masthead: how far it streams astern, and how deep.
    glm::vec2 flagSize;

    // Phase 36: the cannon.
    CannonDimensions cannon;

    // Phase 50: the quarterdeck, the poop deck above it, and the forecastle (the DECK_* indices).
    DeckLevelDimensions deckLevels[DECK_LEVEL_COUNT];

    // Phase 52: the railings.
    RailDimensions rails;

    // Phase 53: the gunports.
    GunportDimensions gunports;
};

namespace ShipConfig {

// Sized for this project's scene rather than copied from the reference guide's
// 1.2 x 0.6 x 4.0. The sea is only 4 x 4 units, so a ship that long would cover all
// of it. The proportions are what matter: about 3.3 times as long as it is wide.
const ShipDimensions DEFAULT_DIMENSIONS = {
    glm::vec3(0.36f, 0.26f, 1.40f),   // hull: beam, depth (keel to the HIGHEST gunwale), length
    {{
        // The galleon's stations, stern (t = 0) to stem (t = 1). See src/Hull.h for what each column means.
        // Phase 47's viva exercise: sharpen the bow or widen the beam by editing THIS table.
        //   t     halfBeam keelRise sheer
        { 0.00f,  0.62f,   0.16f,   1.00f },   // the transom: narrower than the beam, keel lifted, gunwale at its highest
        { 0.10f,  0.82f,   0.06f,   0.92f },
        { 0.22f,  0.96f,   0.02f,   0.80f },
        { 0.38f,  1.00f,   0.00f,   0.66f },   // the widest part, keel on the keel line
        { 0.55f,  1.00f,   0.00f,   0.64f },   // the waist: the LOWEST gunwale - the main deck's level
        { 0.70f,  0.95f,   0.03f,   0.68f },
        { 0.82f,  0.74f,   0.12f,   0.76f },
        { 0.92f,  0.42f,   0.30f,   0.84f },
        { 0.97f,  0.20f,   0.50f,   0.90f },
        { 1.00f,  0.00f,   0.72f,   0.94f },   // the stem: width zero, forefoot raked up out of the water
    }},
    glm::vec3(0.26f, 0.03f, 0.62f),   // deck: a slab laid in the waist, where the hull is wide enough to hold it
    -0.04f,                           // the slab's centre, a little aft of the hull's
    0.06f,                            // draft
    {
        // z      height diam   yard@  yardL  yardD  sail (w, h)
        {  0.24f, 1.24f, 0.030f, 0.68f, 0.40f, 0.020f, glm::vec2(0.36f, 0.31f) },   // fore: 0.85 of main
        { -0.10f, 1.46f, 0.034f, 0.68f, 0.46f, 0.022f, glm::vec2(0.42f, 0.38f) },   // main: 1.04 hull lengths
        { -0.43f, 0.95f, 0.026f, 0.68f, 0.32f, 0.017f, glm::vec2(0.29f, 0.24f) },   // mizzen: 0.65 of main
    },
    glm::vec2(0.20f, 0.11f),          // flag: 0.20 astern, 0.11 deep
    {
        // The cannon stands on the +x side, between the masts, so its barrel can look
        // out over the side. Clear of the fore mast's sail, whose lower edge is well
        // above the deck, and of the main mast.
        glm::vec2(0.07f, 0.0f),       // mount position on the deck (x, z)
        glm::vec3(0.10f, 0.07f, 0.10f), // mount block
        0.045f,                       // trunnion height above the mount's top
        0.34f,                        // MUZZLE_Z: the barrel is 0.34 long
        0.050f                        // barrel diameter
    },
    {
        // Phase 50. Heights are above the keel line; the waist's walking surface is 0.1964 (gunwale 0.1664 + the 0.03 slab).
        //   zFrom   zTo    floor  inset
        { -0.66f, -0.36f, 0.30f, 0.012f },   // quarterdeck: the long raised deck aft
        { -0.66f, -0.50f, 0.38f, 0.012f },   // poop: the short, highest deck at the very stern, on top of the quarterdeck
        {  0.32f,  0.54f, 0.28f, 0.012f },   // forecastle: lower than the sterncastle, in the bows
    },
    {
        // Phase 52.  spacing postH  postW  railW  embed  sideGap
        0.075f, 0.050f, 0.010f, 0.012f, 0.004f, 0.004f
    },
    {
        // Phase 53. The stations along the ship, then the sizes.
        { -0.34f, -0.25f, -0.16f, -0.07f, 0.02f, 0.11f, 0.20f, 0.29f },
        //  width   height  frame   proud   fProud  embed
        0.040f, 0.016f, 0.002f, 0.004f, 0.0025f, 0.006f
    }
};

// Phase 37: the rotation applied to the root to PROVE the hierarchy. Forty-five degrees of
// roll, about the ship's own long axis.
//
// It is a roll rather than a turn because roll is what the sea will apply in Stage F, and
// what the `H` key will keep clearing. It is large on purpose: a gentle tilt could hide a
// part that failed to follow, and 45 degrees cannot.
//
// Once Phase 83 sets the roll from the waves this constant goes away; until then it is the
// "root rotation angle" the viva asks you to change.
const float PROOF_ROLL = 45.0f * SHIP_PI / 180.0f;

// The default aim. Both are DELIBERATELY not zero: a barrel pointing straight ahead and
// perfectly level would look correct under almost any mistake in the two rotations, and
// the checkpoint is that editing these swings the gun correctly.
//
// 30 degrees off the bow towards +x, so it looks out over the side, and 12 degrees up.
// The later limits (Phase 85) are -5 to 45 degrees of elevation; 12 is well inside.
const CannonPose DEFAULT_CANNON_POSE = {
    30.0f * SHIP_PI / 180.0f,
    12.0f * SHIP_PI / 180.0f
};

} // namespace ShipConfig

// ---- Phase 38: the same ship at another size ---------------------------------------------
//
// Multiplies EVERY LENGTH in a ShipDimensions by k and returns the result.
//
// THIS SCALES DATA, NOT A MATRIX. That distinction is the whole phase. A matrix scale put on
// the root would be inherited by every child - the mast would be stretched along with the
// hull, and every frame below it would stop being a rigid motion (the Phase 34 rule). Here
// the root is untouched. What changes is the NUMBER each builder reads: a hull that is 5.6
// long instead of 1.4, a deck that is 4 times as thick, a mast that stands 4 times as high.
// buildShipFrames() then turns those numbers into translations as usual, and every frame it
// produces is still a rotation plus a translation.
//
// Which fields are lengths and which are not matters, and it is why this is a function and
// not a loop over the struct's floats:
//   - every size, offset, diameter and height is a length and is multiplied by k;
//   - yardHeightFraction is a FRACTION of the mast's height. It is already relative, so the
//     yard rises with its mast by itself, and multiplying it would put the yard through the
//     masthead at any k above 1.
//
// Any phase that adds a length to ShipDimensions must add it here too. The Phase 38 test
// reads the struct as an array of floats and requires that exactly the fractions are left
// alone, so a forgotten length fails loudly instead of silently staying small.
inline ShipDimensions scaleShipDimensions(const ShipDimensions& d, float k)
{
    ShipDimensions s = d;

    s.hullSize *= k;
    // s.hullProfile is all FRACTIONS (src/Hull.h) - the shape is the same at every size - so it is
    // deliberately NOT scaled, exactly like yardHeightFraction below.
    s.deckSize *= k;
    s.deckCenterZ *= k;
    s.hullDraft *= k;

    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        MastDimensions& m = s.masts[i];
        m.z *= k;
        m.height *= k;
        m.diameter *= k;
        // m.yardHeightFraction is a fraction of the height - deliberately NOT scaled.
        m.yardLength *= k;
        m.yardDiameter *= k;
        m.sailSize *= k;
    }

    s.flagSize *= k;

    for (int i = 0; i < DECK_LEVEL_COUNT; ++i) {
        DeckLevelDimensions& L = s.deckLevels[i];
        L.zFrom *= k;
        L.zTo *= k;
        L.floorHeight *= k;
        L.inset *= k;
    }

    s.rails.spacing *= k;
    s.rails.postHeight *= k;
    s.rails.postSize *= k;
    s.rails.railSize *= k;
    s.rails.embed *= k;
    s.rails.sideGap *= k;

    for (int i = 0; i < GUNPORT_COUNT; ++i) s.gunports.z[i] *= k;
    s.gunports.width *= k;
    s.gunports.height *= k;
    s.gunports.frame *= k;
    s.gunports.proud *= k;
    s.gunports.frameProud *= k;
    s.gunports.embed *= k;

    CannonDimensions& c = s.cannon;
    c.mountXZ *= k;
    c.mountSize *= k;
    c.trunnionHeight *= k;
    c.muzzleZ *= k;
    c.barrelDiameter *= k;

    return s;
}

// The frames of one mast's chain: mast -> yard -> sail, with a back-facing copy of the
// sail because a quad is drawn from one side only.
struct MastFrames {
    // The mast's BASE, standing on the deck. A child of the deck frame.
    glm::mat4 mast = glm::mat4(1.0f);

    // The yard's CENTRE, where it crosses the mast. A child of the mast frame.
    glm::mat4 yard = glm::mat4(1.0f);

    // The sail's HANGING EDGE: the line along the yard it is slung from. A child of the
    // yard. Its origin is at the yard, with the sail hanging DOWN from it - so that
    // when the sail is later swung (Phase 84) it pivots about the yard, where a real
    // one does, and not about its own middle.
    glm::mat4 sail = glm::mat4(1.0f);

    // The same place turned half a revolution about the vertical, so that the sail's
    // other face is the one a viewer behind it sees. Still a child of the sail.
    glm::mat4 sailBack = glm::mat4(1.0f);

    // Phase 59's full stack. Index 0 is mirrored into the legacy named fields
    // above so the Stage-C demonstrations and crew layout remain compatible.
    glm::mat4 yards[SHIP_YARD_COUNT] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
    glm::mat4 sails[SHIP_YARD_COUNT] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
    glm::mat4 sailBacks[SHIP_YARD_COUNT] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
};

// Phase 37: where the ship is and how it is oriented, as four plain numbers.
//
// The root frame is built from these, and ONLY the root. Everything else on the ship is a
// child of the root, so it follows without being told - that is what the proof in this
// phase is about.
//
// The four are kept separate rather than baked into one matrix because they have
// different OWNERS. Position and heading belong to the player: Phase 79 steers with them.
// Roll and pitch belong to the sea: Phase 83 sets them from the waves under the hull. The
// `H` key clears the sea's contribution and must leave the player's alone - and it can only
// do that if the two were never mixed together.
struct ShipPose {
    glm::vec3 position;   // world position of the root, at the waterline
    float heading;        // radians about the vertical: where the bow points
    float roll;           // radians about the ship's own long axis (z)
    float pitch;          // radians about the ship's own cross axis (x)
};

// The root matrix: T * R_y(heading) * R_z(roll) * R_x(pitch).
//
// Read right to left, it applies pitch first, then roll, then heading, then moves the
// result into place. So pitch and roll are about the SHIP's own axes - a ship pointing
// east still rolls about its own length, not about the world's - and only then is the
// whole thing turned to its heading. That is the order a vessel actually behaves in.
inline glm::mat4 shipRootMatrix(const ShipPose& p)
{
    return glm::translate(glm::mat4(1.0f), p.position)
         * glm::rotate(glm::mat4(1.0f), p.heading, glm::vec3(0.0f, 1.0f, 0.0f))
         * glm::rotate(glm::mat4(1.0f), p.roll, glm::vec3(0.0f, 0.0f, 1.0f))
         * glm::rotate(glm::mat4(1.0f), p.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
}

// The `H` key's effect: the same pose with the SEA's rotation removed. Roll and pitch go to
// zero. Position and heading - which belong to the player - are returned untouched.
inline ShipPose shipPoseWithoutSeaRotation(ShipPose p)
{
    p.roll = 0.0f;
    p.pitch = 0.0f;
    return p;
}

// Every stored matrix of the ship. A frame is a rigid motion: an orthonormal basis
// and a position, nothing else.
struct ShipFrames {
    // The root. Everything on the ship is a descendant of this one matrix, which is
    // why moving, turning or rolling the ship later is a single change.
    glm::mat4 root = glm::mat4(1.0f);

    // The hull's centre. A child of the root.
    glm::mat4 hull = glm::mat4(1.0f);

    // The deck's centre. A child of the HULL FRAME - not of the hull's scaled
    // matrix, which does not exist anywhere in this struct.
    glm::mat4 deck = glm::mat4(1.0f);

    // Phase 35: the rigging, two chains.
    MastFrames rig[SHIP_MAST_COUNT];

    // The flag hoist: a point at the main MASTHEAD that the flag is flown from.
    //
    // A child of the main YARD, NOT of the main sail. The distinction matters from
    // Phase 84: the sail will swing about its yard, and a flag parented to the sail
    // would swing with it. The flag has its own motion and must not inherit the
    // sail's.
    //
    // It is a separate frame from the flag itself because they have different jobs.
    // The hoist is the PIVOT - Phase 84 turns the flag about the masthead, and this is
    // the frame it will turn. The flag below is a child of it, sitting where the cloth
    // is.
    glm::mat4 flagHoist = glm::mat4(1.0f);

    // The flag's own CENTRE: half its length astern of the pole and half its depth
    // below the masthead. A child of the hoist.
    glm::mat4 flag = glm::mat4(1.0f);

    // The same place turned half a revolution about the vertical, IN PLACE - so it is
    // the same rectangle seen from its other side. Because `flag` is at the cloth's
    // centre, turning about it leaves the cloth where it was. (An earlier version
    // turned the HOIST instead, which swung the back copy round the pole and put a
    // second flag on the other side of the mast.)
    glm::mat4 flagBack = glm::mat4(1.0f);

    // Phase 36: the cannon chain, mount -> yoke -> barrel -> muzzle.

    // The mount block's CENTRE, standing on the deck. A child of the deck.
    glm::mat4 mount = glm::mat4(1.0f);

    // The yoke: a pivot on top of the mount that turns about the VERTICAL. This is where
    // azimuth is applied. It has no mesh of its own - it is a frame, and its whole job is
    // to carry a rotation to its child.
    glm::mat4 yoke = glm::mat4(1.0f);

    // The barrel's PIVOT (its trunnion) - the breech end - turned about a HORIZONTAL axis
    // by the elevation. A child of the yoke, so it inherits the azimuth for free. The
    // barrel runs from here along +z to the muzzle.
    glm::mat4 barrel = glm::mat4(1.0f);

    // The muzzle: the barrel's tip. An empty frame, with no mesh, existing only to be read
    // - it is where a cannonball will be born in Phase 86.
    glm::mat4 muzzle = glm::mat4(1.0f);

    // Phase 50: the centre of each deck level's FLOOR (indexed DECK_QUARTERDECK, DECK_POOP, DECK_FORECASTLE).
    // Pure translations of their parents: the hull for the quarterdeck and forecastle, the quarterdeck for the poop.
    glm::mat4 level[DECK_LEVEL_COUNT] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
};

// Builds the ship's frames from its root matrix.
//
// The root arrives as an argument because what it IS belongs to other phases: a
// fixed placement now, a steered and wave-driven pose in Stages E and F. This
// function does not care, and that is the point of a hierarchy.
inline ShipFrames buildShipFrames(const glm::mat4& root, const ShipDimensions& d,
                                  const CannonPose& aim = ShipConfig::DEFAULT_CANNON_POSE)
{
    ShipFrames f;
    f.root = root;

    // The hull's centre sits half its depth above its own underside, and its
    // underside is `hullDraft` below the waterline (the root's origin).
    f.hull = f.root * glm::translate(glm::mat4(1.0f),
                                     glm::vec3(0.0f, d.hullSize.y * 0.5f - d.hullDraft, 0.0f));

    // The deck rests on top of the hull: its centre is half the hull's depth plus
    // half its own thickness above the hull's centre.
    //
    // READ THIS ONE CAREFULLY. The hull's SIZE appears here, but only as a NUMBER
    // used to work out how high to lift the deck. The hull's SCALE MATRIX does not
    // appear, and cannot, because there is none to multiply by. If the deck were
    // built from a scaled hull matrix its own dimensions would be distorted. Built
    // from a frame, only its position depends on the hull - which is exactly the
    // right amount of dependence, since a deck must sit on whatever hull it is on.
    //
    // Phase 47: "the top of the hull" is not one height any more - the gunwale rises towards both ends.
    // The slab rests at the LOWEST point of that sheer line, the waist (hullLowestSheer, a fraction of the
    // depth). For a hull whose every sheer is 1 - Phase 34's box - that is the depth, so the formula is the
    // old one exactly. It is also centred at deckCenterZ along the hull instead of at its middle.
    const float mainDeckHeight = d.hullSize.y * hullLowestSheer(d.hullProfile);
    f.deck = f.hull * glm::translate(glm::mat4(1.0f),
                                     glm::vec3(0.0f,
                                               mainDeckHeight - d.hullSize.y * 0.5f + d.deckSize.y * 0.5f,
                                               d.deckCenterZ));

    // Phase 35: each mast is a child of the deck, and its yard a child of the mast.
    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        const MastDimensions& m = d.masts[i];
        MastFrames& r = f.rig[i];

        // The mast's base stands on the deck's TOP surface, half the deck's thickness
        // above the deck frame's centre.
        r.mast = f.deck * glm::translate(glm::mat4(1.0f),
                                         glm::vec3(0.0f, d.deckSize.y * 0.5f, m.z));

        for (int y = 0; y < SHIP_YARD_COUNT; ++y) {
            const YardDimensions yd = shipYardDimensions(m, y);
            r.yards[y] = r.mast * glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(0.0f, m.height * yd.heightFraction, 0.0f));
            r.sails[y] = r.yards[y];
            r.sailBacks[y] = r.sails[y] * glm::rotate(glm::mat4(1.0f), SHIP_PI, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        r.yard = r.yards[0];
        r.sail = r.sails[0];
        r.sailBack = r.sailBacks[0];
    }

    // Phase 35: the flag flies from the TOP of the main mast.
    {
        const MastDimensions& m = d.masts[MAIN_MAST];

        // The hoist: a child of the main yard, lifted by however far the masthead is
        // above the yard.
        f.flagHoist = f.rig[MAIN_MAST].yard * glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, m.height * (1.0f - m.yardHeightFraction), 0.0f));

        // The cloth's centre: half its length astern (-z) and half its depth down, so
        // that its top edge is level with the masthead and its leading edge is on the
        // mast.
        f.flag = f.flagHoist * glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, -d.flagSize.y * 0.5f, -d.flagSize.x * 0.5f));

        // Turned in place about its own centre, so it is the same rectangle.
        f.flagBack = f.flag * glm::rotate(glm::mat4(1.0f), SHIP_PI, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Phase 36: the cannon. A chain of four frames, each one local step from its parent.
    {
        const CannonDimensions& c = d.cannon;

        // The mount's centre: standing on the deck's top surface, so half the mount's
        // own height above it.
        f.mount = f.deck * glm::translate(glm::mat4(1.0f),
                                          glm::vec3(c.mountXZ.x,
                                                    d.deckSize.y * 0.5f + c.mountSize.y * 0.5f,
                                                    c.mountXZ.y));

        // The yoke sits on top of the mount and turns about the vertical by the azimuth.
        // The translation comes FIRST (it is the outer matrix), so the turn happens at
        // the yoke's own position and not about the ship's origin.
        f.yoke = f.mount
               * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, c.mountSize.y * 0.5f, 0.0f))
               * glm::rotate(glm::mat4(1.0f), aim.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));

        // The barrel's pivot is a little above the yoke, and tilts about the yoke's own x
        // axis. THE SIGN MATTERS: a rotation about +x by a positive angle carries a
        // vector along +z DOWNWARDS (z goes to -y), so a positive elevation needs the
        // angle negated to raise the muzzle. Without the minus sign the cannon depresses
        // when told to elevate - the sort of mistake that only shows when the angle is
        // non-zero.
        f.barrel = f.yoke
                 * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, c.trunnionHeight, 0.0f))
                 * glm::rotate(glm::mat4(1.0f), -aim.elevation, glm::vec3(1.0f, 0.0f, 0.0f));

        // The muzzle: MUZZLE_Z along the barrel's own axis from the pivot.
        f.muzzle = f.barrel * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, c.muzzleZ));
    }

    // Phase 50: the deck levels. Each frame is where the middle of that level's floor is.
    {
        const auto centreZ = [&d](int i) { return 0.5f * (d.deckLevels[i].zFrom + d.deckLevels[i].zTo); };
        for (int i = 0; i < DECK_LEVEL_COUNT; ++i) {
            const DeckLevelDimensions& L = d.deckLevels[i];
            if (i == DECK_POOP) {
                // The poop stands ON the quarterdeck, so it is the quarterdeck's child: a lift to its own floor and a
                // slide along the ship to its own middle.
                f.level[i] = f.level[DECK_QUARTERDECK]
                           * glm::translate(glm::mat4(1.0f),
                                            glm::vec3(0.0f,
                                                      L.floorHeight - d.deckLevels[DECK_QUARTERDECK].floorHeight,
                                                      centreZ(i) - centreZ(DECK_QUARTERDECK)));
            } else {
                f.level[i] = f.hull
                           * glm::translate(glm::mat4(1.0f),
                                            glm::vec3(0.0f, L.floorHeight - d.hullSize.y * 0.5f, centreZ(i)));
            }
        }
    }

    return f;
}

// ---- Reading the muzzle out of the hierarchy (Phase 36, used from Phase 86) --------
//
// A position and a direction are DIFFERENT THINGS and a matrix treats them differently.
// A position needs the translation in the matrix to apply to it, so w = 1. A direction is
// only an orientation - it has no location - so the translation must NOT touch it, and
// w = 0. Getting this backwards sends a cannonball towards the world origin instead of
// along the barrel.

inline glm::vec3 shipMuzzlePosition(const ShipFrames& f)
{
    return glm::vec3(f.muzzle * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));   // w = 1: a POSITION
}

inline glm::vec3 shipMuzzleDirection(const ShipFrames& f)
{
    // The barrel runs along its own +z. Taken from the BARREL's frame rather than the
    // muzzle's - they share an orientation, since the muzzle is a pure translation of the
    // barrel - and normalised, because a rotation matrix keeps length 1 but a product of
    // many of them accumulates a little float error.
    return glm::normalize(glm::vec3(f.barrel * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));   // w = 0: a DIRECTION
}

// ---- The matrices actually handed to drawMesh() ----------------------------
//
// THE ONLY PLACE a ship part's scale is applied. They live here, rather than inline in
// main.cpp, so that a test calls the very same functions the renderer does and cannot
// drift from them.

inline glm::mat4 shipHullModel(const ShipFrames& f, const ShipDimensions& d)
{
    return f.hull * glm::scale(glm::mat4(1.0f), d.hullSize);
}

inline glm::mat4 shipDeckModel(const ShipFrames& f, const ShipDimensions& d)
{
    return f.deck * glm::scale(glm::mat4(1.0f), d.deckSize);
}

// The cylinder mesh is a unit tube standing along y and centred on its origin. The
// mast's frame is at its BASE, so the tube is lifted half its height to stand on it.
inline glm::mat4 shipMastModel(const MastFrames& r, const MastDimensions& m)
{
    return r.mast
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, m.height * 0.5f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(m.diameter, m.height, m.diameter));
}

// A yard lies ACROSS the ship, along x. The tube stands along y, so it is turned a
// quarter revolution about z first. Its length is the cylinder's y scale.
inline glm::mat4 shipYardModel(const MastFrames& r, const MastDimensions& m)
{
    return r.yard
         * glm::rotate(glm::mat4(1.0f), SHIP_PI * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(m.yardDiameter, m.yardLength, m.yardDiameter));
}

inline glm::mat4 shipYardModel(const glm::mat4& frame, const YardDimensions& y)
{
    return frame
         * glm::rotate(glm::mat4(1.0f), SHIP_PI * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(y.diameter, y.length, y.diameter));
}

// The quad mesh is a unit square in the xy plane, facing +z, centred on its origin.
// The sail's frame is at its hanging edge, so the quad is lowered half its height to
// hang BELOW it. `frame` is r.sail for the front face and r.sailBack for the back.
inline glm::mat4 shipSailModel(const glm::mat4& frame, const MastDimensions& m)
{
    return frame
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -m.sailSize.y * 0.5f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(m.sailSize.x, m.sailSize.y, 1.0f));
}

inline glm::mat4 shipSailModel(const glm::mat4& frame, const YardDimensions& y)
{
    return frame
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -y.sailSize.y * 0.5f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(y.sailSize.x, y.sailSize.y, glm::max(y.sailSize.x, 0.001f)));
}

// The flag lies in the plane containing the vertical and the ship's length. The quad
// faces +z, so it is turned a quarter revolution about y to lie in that plane. Its
// frame is already at the cloth's centre, so nothing else is needed.
// `frame` is f.flag for one face and f.flagBack for the other.
inline glm::mat4 shipFlagModel(const glm::mat4& frame, const ShipDimensions& d)
{
    return frame
         * glm::rotate(glm::mat4(1.0f), SHIP_PI * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(d.flagSize.x, d.flagSize.y, 1.0f));
}

// Phase 36: the mount is a plain block, centred on its frame.
inline glm::mat4 shipMountModel(const ShipFrames& f, const ShipDimensions& d)
{
    return f.mount * glm::scale(glm::mat4(1.0f), d.cannon.mountSize);
}

// The barrel. The cylinder mesh is a unit tube standing along y and centred on its
// origin; the barrel must run along +z from its pivot to the muzzle. So the tube is
//   1. scaled to its diameter and its length (the scale is applied FIRST, innermost);
//   2. turned a quarter revolution about x, which carries its y axis onto +z;
//   3. moved half its length along +z, so that one end is at the pivot and the other at
//      the muzzle instead of the pivot being at its middle.
inline glm::mat4 shipBarrelModel(const ShipFrames& f, const ShipDimensions& d)
{
    const CannonDimensions& c = d.cannon;
    return f.barrel
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, c.muzzleZ * 0.5f))
         * glm::rotate(glm::mat4(1.0f), SHIP_PI * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(c.barrelDiameter, c.muzzleZ, c.barrelDiameter));
}

// ---- Phase 50: the deck levels' models ---------------------------------------------------
//
// THE ONLY PLACE a level's scale is applied. A level is a block of timber with a slab on top.

// The height above the keel line a level's block stands on: the level below it. The poop stands on the
// quarterdeck; the quarterdeck and the forecastle stand on the main deck (the waist's gunwale), which they
// are embedded in where the hull rises beside them.
inline float shipDeckLevelBaseHeight(const ShipDimensions& d, int i)
{
    if (i == DECK_POOP)
        return d.deckLevels[DECK_QUARTERDECK].floorHeight;
    return d.hullSize.y * hullLowestSheer(d.hullProfile);
}

// How wide a level is: the hull's gunwale width at the narrowest point along its length, less `inset` each
// side. The hull is pointed at the bow and narrow aft, so a box that has to fit must be as wide as the
// hull's narrowest part under it - sampled, because the narrowest part is wherever the table puts it.
inline float shipDeckLevelWidth(const ShipDimensions& d, int i)
{
    const DeckLevelDimensions& L = d.deckLevels[i];
    float half = 1e9f;
    for (int s = 0; s <= 64; ++s) {
        const float z = L.zFrom + (L.zTo - L.zFrom) * static_cast<float>(s) / 64.0f;
        half = std::min(half, hullGunwaleHalfBeamAt(d.hullProfile, d.hullSize.x, d.hullSize.z, z));
    }
    return 2.0f * (half - L.inset);
}

// The timber block: from the level below up to the underside of this level's slab.
inline glm::mat4 shipDeckBlockModel(const ShipFrames& f, const ShipDimensions& d, int i)
{
    const DeckLevelDimensions& L = d.deckLevels[i];
    const float slab = d.deckSize.y;
    const float top = L.floorHeight - slab;                       // the block's top, above the keel line
    const float height = top - shipDeckLevelBaseHeight(d, i);
    return f.level[i]
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -slab - height * 0.5f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(shipDeckLevelWidth(d, i), height, L.zTo - L.zFrom));
}

// How far a slab overhangs its block all round: a fifth of its thickness.
inline float shipDeckSlabLip(const ShipDimensions& d)
{
    return d.deckSize.y * 0.2f;
}

// The planked slab laid on the block, a little proud of it all round so its edge is a visible lip.
inline glm::mat4 shipDeckSlabModel(const ShipFrames& f, const ShipDimensions& d, int i)
{
    const DeckLevelDimensions& L = d.deckLevels[i];
    const float slab = d.deckSize.y;
    const float lip = shipDeckSlabLip(d);
    return f.level[i]
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -slab * 0.5f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(shipDeckLevelWidth(d, i) + 2.0f * lip, slab, L.zTo - L.zFrom + 2.0f * lip));
}

// ---- Phase 51: the deck planks' models --------------------------------------------------------------
//
// The plank sheet (a 1 x 1 unit mesh, src/Planks.h) is laid over the top of each deck slab, a hair above it. Its size is
// the slab's footprint, applied here and nowhere else.

// How far above a slab's top the sheet floats: a fraction of the slab's thickness.
inline float shipDeckPlankLift(const ShipDimensions& d)
{
    return d.deckSize.y * DeckPlanking::LIFT_FRACTION;
}

// The main deck: over the whole of the deck cube's top face.
inline glm::mat4 shipDeckPlankModel(const ShipFrames& f, const ShipDimensions& d)
{
    return f.deck
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, d.deckSize.y * 0.5f + shipDeckPlankLift(d), 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(d.deckSize.x, 1.0f, d.deckSize.z));
}

// A level: over the whole of its slab's top face, lip included. The frame is at the middle of the floor, which IS the
// slab's top, so the sheet is the frame lifted by the hair.
inline glm::mat4 shipDeckLevelPlankModel(const ShipFrames& f, const ShipDimensions& d, int i)
{
    const DeckLevelDimensions& L = d.deckLevels[i];
    const float lip = shipDeckSlabLip(d);
    return f.level[i]
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, shipDeckPlankLift(d), 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(shipDeckLevelWidth(d, i) + 2.0f * lip, 1.0f, L.zTo - L.zFrom + 2.0f * lip));
}

// ---- Phase 52: the railings' placement ---------------------------------------------------------------------------
//
// Everything is worked out in the LOCAL space of the frame a run stands on: the hull's frame for the waist, a level's frame for
// the rest. A post's "base" is the centre of its foot on the surface, a rail segment joins the tops of two neighbouring posts.

// One run: where it lies along the ship (from the hull's centre) and what it stands on (-1 = the hull's cap, else a DECK_* level).
struct RailRun {
    float zFrom;
    float zTo;
    int level;
};

inline RailRun shipRailRun(const ShipDimensions& d, int run)
{
    const DeckLevelDimensions& Q = d.deckLevels[DECK_QUARTERDECK];
    const DeckLevelDimensions& P = d.deckLevels[DECK_POOP];
    const DeckLevelDimensions& F = d.deckLevels[DECK_FORECASTLE];
    switch (run) {
    case RAIL_WAIST:       return { Q.zTo, F.zFrom, -1 };                  // between the castles
    case RAIL_QUARTERDECK: return { P.zTo, Q.zTo, DECK_QUARTERDECK };      // the part the poop does not stand on
    case RAIL_POOP:        return { P.zFrom, P.zTo, DECK_POOP };
    default:               return { F.zFrom, F.zTo, DECK_FORECASTLE };
    }
}

// The frame a run's posts stand on.
inline const glm::mat4& shipRailFrame(const ShipFrames& f, const ShipDimensions& d, int run)
{
    const RailRun r = shipRailRun(d, run);
    return (r.level < 0) ? f.hull : f.level[r.level];
}

// The end posts stand half a post inside the run, so no post pokes past the end of the deck it stands on: the outer face of the
// last post is flush with the end of the run, where a higher deck's wall (the poop's, the forecastle's) begins.
// Posts in a run: the nearest whole number of gaps to (length - one post) / spacing, and at least one gap.
inline int shipRailPostCount(const ShipDimensions& d, int run)
{
    const RailRun r = shipRailRun(d, run);
    const int gaps = std::max(1, static_cast<int>(std::lround((r.zTo - r.zFrom - d.rails.postSize) / d.rails.spacing)));
    return gaps + 1;
}

// Post k's z, from the hull's centre: equal steps from the first post to the last.
inline float shipRailPostZ(const ShipDimensions& d, int run, int k)
{
    const RailRun r = shipRailRun(d, run);
    const int n = shipRailPostCount(d, run);
    const float first = r.zFrom + 0.5f * d.rails.postSize, last = r.zTo - 0.5f * d.rails.postSize;
    return first + (last - first) * static_cast<float>(k) / static_cast<float>(n - 1);
}

// The centre of post k's foot, on the surface it stands on, in its frame's local space. side is -1 (port) or +1 (starboard).
inline glm::vec3 shipRailPostBase(const ShipDimensions& d, int run, int side, int k)
{
    const RailRun r = shipRailRun(d, run);
    const float z = shipRailPostZ(d, run, k);
    const float sign = static_cast<float>(side);
    if (r.level < 0) {
        // The waist: on the hull's cap, just inside the gunwale. The hull frame's origin is half the hull's depth above the keel line.
        const float surface = hullGunwaleHeightAt(d.hullProfile, d.hullSize.y, d.hullSize.z, z) - d.hullSize.y * 0.5f;
        const float edge = hullGunwaleHalfBeamAt(d.hullProfile, d.hullSize.x, d.hullSize.z, z);
        return glm::vec3(sign * (edge - d.rails.sideGap - d.rails.postSize * 0.5f), surface, z);
    }
    // A level: on its slab, with the post's outer face flush with the block's side. The frame is the middle of the floor.
    const DeckLevelDimensions& L = d.deckLevels[r.level];
    return glm::vec3(sign * (shipDeckLevelWidth(d, r.level) * 0.5f - d.rails.postSize * 0.5f), 0.0f, z - 0.5f * (L.zFrom + L.zTo));
}

// The post: a square column from `embed` below its foot to `postHeight` above it.
inline glm::mat4 shipRailPostModel(const ShipFrames& f, const ShipDimensions& d, int run, int side, int k)
{
    const RailDimensions& R = d.rails;
    const glm::vec3 base = shipRailPostBase(d, run, side, k);
    return shipRailFrame(f, d, run)
         * glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 0.5f * (R.postHeight - R.embed), 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(R.postSize, R.postHeight + R.embed, R.postSize));
}

// The top of post k, where the rail meets it, in the frame's local space.
inline glm::vec3 shipRailPostTop(const ShipDimensions& d, int run, int side, int k)
{
    return shipRailPostBase(d, run, side, k) + glm::vec3(0.0f, d.rails.postHeight, 0.0f);
}

// The rail between posts k and k + 1: a square bar whose own z axis runs from one post top to the next.
inline glm::mat4 shipRailSegmentModel(const ShipFrames& f, const ShipDimensions& d, int run, int side, int k)
{
    const glm::vec3 a = shipRailPostTop(d, run, side, k), b = shipRailPostTop(d, run, side, k + 1);
    const glm::vec3 along = b - a;
    const float length = glm::length(along);
    const glm::vec3 zAxis = along / length;
    const glm::vec3 xAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zAxis));
    const glm::vec3 yAxis = glm::cross(zAxis, xAxis);
    glm::mat4 frame(1.0f);
    frame[0] = glm::vec4(xAxis, 0.0f);
    frame[1] = glm::vec4(yAxis, 0.0f);
    frame[2] = glm::vec4(zAxis, 0.0f);
    frame[3] = glm::vec4(0.5f * (a + b), 1.0f);
    return shipRailFrame(f, d, run) * frame * glm::scale(glm::mat4(1.0f), glm::vec3(d.rails.railSize, d.rails.railSize, length));
}

// ---- Phase 53: the gunports' placement ---------------------------------------------------------------------------
//
// A point ON the planking, in the hull's local space: side is -1 (port) or +1 (starboard), z along the ship from the hull's
// centre, row the loft row (6 is where the rounded bilge meets the topsides, 12 the gunwale). It is read off the same station
// table, and the same section curve, that builds the mesh, so it lies on the hull - except between two of the mesh's 41 rings,
// where the mesh is straight and the curve is not; the tests measure how far that is.
inline glm::vec3 shipHullSurfacePoint(const ShipDimensions& d, int side, float z, float row)
{
    const float t = z / d.hullSize.z + 0.5f;
    const HullStation s = hullStationAt(d.hullProfile, t);
    const HullRing ring = { t - 0.5f, 0.5f * s.halfBeam, -0.5f + s.keelRise, -0.5f + s.sheer };
    const glm::vec2 p = hullSectionPointAt(ring, row);
    return glm::vec3(static_cast<float>(side) * p.x * d.hullSize.x, p.y * d.hullSize.y, z);
}

// The frame of a port: its origin on the planking at the port's centre, x pointing straight out of the hull, z ALONG THE PLANKS
// (the tangent of the loft row, which rises with the sheer towards the ends of the ship), y up the side perpendicular to them. So
// a port's top and bottom edges run parallel to the planks above and below it and cannot cross a belt however steep the sheer
// is there. Pure rotation and translation (no scale), built from two tangents of the surface by finite differences. On the
// port side z is turned round, so y still points up and the frame is a true reflection of the starboard one. The hull's own
// frame carries it, so it rocks with the ship.
inline glm::mat4 shipGunportFrame(const ShipFrames& f, const ShipDimensions& d, int side, int i)
{
    const float z = d.gunports.z[i];
    const float dz = 0.01f * d.hullSize.z, dr = 0.25f;
    const glm::vec3 p = shipHullSurfacePoint(d, side, z, GunportShape::ROW);
    const glm::vec3 along = shipHullSurfacePoint(d, side, z + dz, GunportShape::ROW) - shipHullSurfacePoint(d, side, z - dz, GunportShape::ROW);
    const glm::vec3 up = shipHullSurfacePoint(d, side, z, GunportShape::ROW + dr) - shipHullSurfacePoint(d, side, z, GunportShape::ROW - dr);
    glm::vec3 n = glm::normalize(side > 0 ? glm::cross(up, along) : glm::cross(along, up));      // outward
    glm::vec3 zAxis = glm::normalize(along - n * glm::dot(n, along));
    if (side < 0) zAxis = -zAxis;
    const glm::vec3 yAxis = glm::cross(zAxis, n);
    glm::mat4 frame(1.0f);
    frame[0] = glm::vec4(n, 0.0f);
    frame[1] = glm::vec4(yAxis, 0.0f);
    frame[2] = glm::vec4(zAxis, 0.0f);
    frame[3] = glm::vec4(p, 1.0f);
    return f.hull * frame;
}

// The dark opening: a box from `embed` inside the planking to `proud` outside it.
inline glm::mat4 shipGunportOpeningModel(const ShipFrames& f, const ShipDimensions& d, int side, int i)
{
    const GunportDimensions& G = d.gunports;
    return shipGunportFrame(f, d, side, i)
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * (G.proud - G.embed), 0.0f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(G.proud + G.embed, G.height, G.width));
}

// The frame: a larger, lower box behind it, so a border of it shows all round the opening.
inline glm::mat4 shipGunportFrameModel(const ShipFrames& f, const ShipDimensions& d, int side, int i)
{
    const GunportDimensions& G = d.gunports;
    return shipGunportFrame(f, d, side, i)
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * (G.frameProud - G.embed), 0.0f, 0.0f))
         * glm::scale(glm::mat4(1.0f), glm::vec3(G.frameProud + G.embed, G.height + 2.0f * G.frame, G.width + 2.0f * G.frame));
}

// ---- Phase 54: one complete cannon chain behind every port --------------------------------
struct BroadsideCannonFrames {
    glm::mat4 port = glm::mat4(1.0f);       // port surface; +x points outboard
    glm::mat4 carriage = glm::mat4(1.0f);   // on the gun deck, behind the opening
    glm::mat4 barrel = glm::mat4(1.0f);     // breech pivot; +x points toward the muzzle
    glm::mat4 muzzle = glm::mat4(1.0f);
    glm::mat4 wheel[4] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
};

inline BroadsideCannonFrames buildBroadsideCannonFrames(const ShipFrames& f, const ShipDimensions& d,
                                                         int side, int port, float recoil01 = 0.0f)
{
    BroadsideCannonFrames b;
    b.port = shipGunportFrame(f, d, side, port);
    const float length = 0.095f * d.hullSize.z;
    const float retreat = 0.18f * length * glm::clamp(recoil01, 0.0f, 1.0f);
    b.carriage = b.port * glm::translate(glm::mat4(1.0f), glm::vec3(-0.64f * length - retreat, -0.55f * d.gunports.height, 0.0f));
    // Keep the muzzle inside the port thickness (rather than visibly floating
    // beyond the plank skin); a projectile starts a hair outward from this tip.
    b.barrel = b.port * glm::translate(glm::mat4(1.0f), glm::vec3(-length + 0.25f * d.gunports.proud - retreat, 0.0f, 0.0f));
    b.muzzle = b.barrel * glm::translate(glm::mat4(1.0f), glm::vec3(length, 0.0f, 0.0f));
    const float wx[2] = { -0.30f * length, 0.30f * length };
    const float wz[2] = { -0.42f * d.gunports.width, 0.42f * d.gunports.width };
    int k = 0;
    for (int a = 0; a < 2; ++a) for (int z = 0; z < 2; ++z)
        b.wheel[k++] = b.carriage * glm::translate(glm::mat4(1.0f), glm::vec3(wx[a], -0.22f * d.gunports.height, wz[z]));
    return b;
}

inline glm::vec3 shipBroadsideMuzzlePosition(const BroadsideCannonFrames& b)
{
    return glm::vec3(b.muzzle * glm::vec4(0, 0, 0, 1));
}

inline glm::vec3 shipBroadsideMuzzleDirection(const BroadsideCannonFrames& b)
{
    return glm::normalize(glm::vec3(b.barrel * glm::vec4(1, 0, 0, 0)));
}

inline glm::mat4 shipBroadsideCarriageModel(const BroadsideCannonFrames& b, const ShipDimensions& d)
{
    const float length = 0.095f * d.hullSize.z;
    return b.carriage * glm::scale(glm::mat4(1.0f), glm::vec3(0.72f * length, 0.80f * d.gunports.height, 1.65f * d.gunports.width));
}

inline glm::mat4 shipBroadsideBarrelModel(const BroadsideCannonFrames& b, const ShipDimensions& d)
{
    const float length = 0.095f * d.hullSize.z;
    const float diameter = 0.62f * d.gunports.height;
    return b.barrel * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * length, 0.0f, 0.0f))
                    * glm::rotate(glm::mat4(1.0f), -0.5f * SHIP_PI, glm::vec3(0, 0, 1))
                    * glm::scale(glm::mat4(1.0f), glm::vec3(diameter, length, diameter));
}

inline glm::mat4 shipBroadsideWheelModel(const BroadsideCannonFrames& b, const ShipDimensions& d, int wheel)
{
    const float diameter = 0.70f * d.gunports.height;
    return b.wheel[wheel] * glm::rotate(glm::mat4(1.0f), 0.5f * SHIP_PI, glm::vec3(1, 0, 0))
                          * glm::scale(glm::mat4(1.0f), glm::vec3(diameter, 0.20f * d.gunports.width, diameter));
}
