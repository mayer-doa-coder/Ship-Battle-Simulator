#pragma once

// Phase 29: named materials, straight from L8 slide 60.
//
// Until this phase, an object's appearance came from its per-vertex colour and one
// set of reflectance values shared by the whole scene. That was a stand-in. A real
// illumination model says a surface is described by FOUR numbers:
//
//     k_a   how much ambient light it reflects
//     k_d   how much diffuse light it reflects   - its apparent colour
//     k_s   how much specular light it reflects  - its shine
//     n_s   the shininess exponent               - how TIGHT that shine is
//
// Those four together are a material, and this file is the table of them.
//
// THE FIRST THREE ARE VERBATIM FROM L8 SLIDE 60 AND MUST NOT BE "IMPROVED".
// They are the lecture's own numbers, which is exactly what makes them evidence in
// the report rather than something tuned until it looked nice. The remaining three
// are tuned for this project's own surfaces and are labelled as such.
//
// The n_s range across the table is deliberately enormous - 4 for sailcloth against
// 160 for the ocean, a FORTY-FOLD spread visible in a single frame. That spread
// reproduces L8 slide 46 with real objects instead of test spheres, and narrowing
// it to make the scene prettier would throw away the demonstration.

#include <glm/glm.hpp>

// A plain struct, like everything else in this project. No constructors, no
// inheritance, no "material system" - four numbers and a name.
struct Material {
    glm::vec3 ka;      // ambient reflectance
    glm::vec3 kd;      // diffuse reflectance
    glm::vec3 ks;      // specular reflectance
    float ns;          // shininess exponent
    const char* name;  // for the console report, so a material can be identified

    // Phase 44: EMISSION - light the surface gives out by itself, whatever is shining on it.
    //
    // The other four numbers all describe how a surface treats light that ARRIVES. This one
    // is different in kind: a lantern's glass, the sun's disc and a lit window are bright
    // because they are sources, and no amount of lighting them explains it. It is added to the
    // result after the lights and is not scaled by them, so it ignores the sun, the point light,
    // the 'L' key and the 'K' term mask alike.
    //
    // It is NOT a light source. It illuminates nothing else (that would be global illumination,
    // which this local model cannot do - L8 s10-11), and the project still has exactly two lights.
    //
    // It has a default of zero and sits last, so every material written above this line - the
    // L8 slide 60 values included - is unchanged and glows not at all. Setting it is the Phase 44
    // viva exercise.
    glm::vec3 ke = glm::vec3(0.0f);

    // Phase 45: when true, the emission is multiplied by the VERTEX COLOUR before it is added,
    // so one material can glow a different colour at every point of a mesh. The sky dome is the
    // only user: its vertices carry a gradient from the horizon's colour to the zenith's, and the
    // material's ke is white so the vertex colour comes through unchanged. False for everything
    // else, which leaves their pictures exactly as they were.
    bool keFromVertexColor = false;

    // Phase 48: when true, the diffuse and ambient reflectances are multiplied by the VERTEX COLOUR, so a
    // single material can be a different colour at every point of a mesh. False for every material written
    // above this line. The ship's hull is the user, and Phase 49 paints its planks this way.
    bool vertexAlbedo = false;
};

// ---- verbatim from L8 slide 60 --------------------------------------------

// The cannon barrel's material, from Stage G onward. Warm and strongly coloured in
// its diffuse term, with an almost-white specular - which is what metal does:
// it tints the light it absorbs but barely tints the light it bounces straight off.
const Material BRASS = {
    { 0.329f, 0.224f, 0.027f },
    { 0.780f, 0.569f, 0.114f },
    { 0.992f, 0.941f, 0.808f },
    27.9f,
    "brass"
};

// The cannon's fittings. Note how close its k_a, k_d and k_s are to being grey:
// silver has almost no colour of its own, it just reflects strongly. Its n_s of
// 89.6 is over three times brass's, so its highlight is far tighter.
const Material POLISHED_SILVER = {
    { 0.231f, 0.231f, 0.231f },
    { 0.278f, 0.278f, 0.278f },
    { 0.774f, 0.774f, 0.774f },
    89.6f,
    "polished silver"
};

// The cannonball. The interesting one in the table, and the best single argument
// for having materials at all: its k_a is pure ZERO and its k_d is 0.01 - it
// reflects essentially no diffuse light, so it is genuinely black - and yet its
// k_s is 0.50, so it is clearly shiny.
//
// Colour and shine are independent properties. No single "colour" value could
// express "black but polished", which is precisely why four numbers are needed.
const Material BLACK_PLASTIC = {
    { 0.000f, 0.000f, 0.000f },
    { 0.010f, 0.010f, 0.010f },
    { 0.500f, 0.500f, 0.500f },
    32.0f,
    "black plastic"
};

// ---- tuned for this project, not from the slide ---------------------------

// The sea (the grid mesh, from Phase 81). Dark and barely coloured in diffuse, but
// with an almost-white specular at n_s = 160 - the highest in the table. That is
// what makes a sun streak glitter on water: water is nearly black when you look
// straight down into it and blindingly bright where it mirrors the sky.
//
// This is the high end of the 40x n_s spread, and it is the material Demo A uses.
const Material OCEAN = {
    { 0.02f, 0.05f, 0.08f },
    { 0.06f, 0.14f, 0.20f },
    { 0.90f, 0.94f, 0.98f },
    160.0f,
    "ocean"
};

// The ship's hull. Mid-brown, and almost matt - k_s of 0.15 at n_s = 8 gives a
// broad weak sheen rather than a spot, which is what unvarnished wood looks like.
const Material HULL_WOOD = {
    { 0.12f, 0.08f, 0.05f },
    { 0.38f, 0.25f, 0.15f },
    { 0.15f, 0.12f, 0.10f },
    8.0f,
    "hull wood"
};

// The sails. The matt end of the table: a strong pale diffuse and almost no
// specular at all, n_s = 4. Cloth scatters light in every direction, so it looks
// the same from anywhere - the opposite of the ocean.
//
// This is the low end of the 40x spread. Ocean 160 against sailcloth 4, both on
// screen at once, is the comparison L8 slide 46 is about.
const Material SAILCLOTH = {
    { 0.20f, 0.19f, 0.17f },
    { 0.75f, 0.73f, 0.68f },
    { 0.04f, 0.04f, 0.04f },
    4.0f,
    "sailcloth"
};

// The ship's deck (Phase 34). Tuned, not from the slide. A lighter, drier wood than
// HULL_WOOD, so that where the deck meets the hull the two read as separate parts -
// without it the deck would be a brown slab on a brown slab and the Phase 34
// checkpoint, "the deck is not stretched like the hull", would be hard to see.
// Matt, like the hull: k_s is small and n_s is low.
const Material DECK_WOOD = {
    { 0.16f, 0.12f, 0.08f },
    { 0.58f, 0.45f, 0.29f },
    { 0.10f, 0.09f, 0.07f },
    6.0f,
    "deck wood"
};

// The ship's flag (Phase 35). Tuned, not from the slide. A strong red, so the one small
// scrap of colour on the ship is unmistakable from a distance and tells you which way
// the masthead points. Cloth, so like SAILCLOTH it is matt: almost no specular, low n_s.
const Material FLAG_CLOTH = {
    { 0.22f, 0.02f, 0.02f },
    { 0.82f, 0.08f, 0.08f },
    { 0.04f, 0.04f, 0.04f },
    4.0f,
    "flag cloth"
};

// ---- Phase 75: Stage-D pirate palette -----------------------------------------------------
// Kept as named, measured materials rather than ad-hoc RGB values in drawing code.
const Material MAHOGANY = { {0.10f,0.045f,0.025f}, {0.31f,0.115f,0.055f}, {0.12f,0.07f,0.04f}, 8.0f, "dark mahogany" };
const Material WEATHERED_BROWN = { {0.10f,0.070f,0.040f}, {0.32f,0.23f,0.13f}, {0.09f,0.07f,0.05f}, 7.0f, "weathered brown" };
const Material CHARCOAL = { {0.018f,0.019f,0.021f}, {0.070f,0.075f,0.082f}, {0.035f,0.035f,0.040f}, 5.0f, "charcoal sailcloth" };
const Material AGED_BEIGE = { {0.19f,0.165f,0.12f}, {0.62f,0.56f,0.42f}, {0.035f,0.035f,0.03f}, 4.0f, "aged beige canvas" };
const Material MUTED_GOLD = { {0.20f,0.145f,0.035f}, {0.58f,0.40f,0.075f}, {0.62f,0.52f,0.24f}, 38.0f, "muted gold" };
const Material AGED_IRON = { {0.045f,0.045f,0.050f}, {0.16f,0.165f,0.18f}, {0.36f,0.37f,0.40f}, 26.0f, "aged iron" };
const Material HEMP = { {0.16f,0.12f,0.065f}, {0.52f,0.42f,0.22f}, {0.035f,0.030f,0.020f}, 4.0f, "hemp rope" };

// ---- Phase 45: the sky -------------------------------------------------------------------
//
// Both are PURELY EMISSIVE: k_a, k_d and k_s are zero, so no light changes them and they change
// no light. A sky is not a surface that the sun shines on; it is where the sun is.

// The dome. ke is white and keFromVertexColor is true, so what appears on screen is exactly the
// vertex colour: a horizon-to-zenith gradient painted into the vertices of the existing sphere.
// n_s is 1 only because the shader divides nothing by it and 0 would be an invalid exponent.
const Material SKY_DOME = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    1.0f,
    "sky dome",
    { 1.0f, 1.0f, 1.0f },
    true
};

// The sun's disc: a small sphere placed along the direction the sun's light comes FROM. Warm
// white, brighter than the sky around it. Its ke is above 1 in no channel on purpose - the
// framebuffer clamps at 1 anyway, and a disc that is simply the brightest, warmest thing in the
// picture is all that is needed.
const Material SUN_DISC = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    1.0f,
    "sun disc",
    { 1.0f, 0.90f, 0.62f },
    false
};

// ---- Phase 46: distant islands ----------------------------------------------------------
//
// Dark, cool and matt. It is never seen at full strength: the islands are 38 to 56 units from
// the camera, where the Phase 45 haze is 76% to 93% complete, so only a few percent of this
// colour survives. It is dark on purpose - the faint silhouette against the horizon comes from the
// island being DARKER than the haze it dissolves into - and matt (k_s 0.03, n_s 6) so that the low
// sun does not put a glint on a hill that is a quarter of a mile away.
const Material ISLAND_ROCK = {
    { 0.030f, 0.035f, 0.040f },
    { 0.100f, 0.110f, 0.110f },
    { 0.030f, 0.030f, 0.030f },
    6.0f,
    "island rock"
};

// ---- Phase 48: the hull's timber -----------------------------------------------------------
//
// EXACTLY HULL_WOOD's four numbers, with the vertex-colour switch turned on. The hull mesh's vertices
// carry a multiplier (white = "as the material says"), and from Phase 49 the planks are darker or
// lighter shades of it. It is a separate material from HULL_WOOD because the masts and yards also use
// HULL_WOOD, and their mesh - the cylinder - still carries Phase 20's silver gradient in its vertices,
// which must NOT start tinting them.
const Material HULL_TIMBER = {
    { 0.12f, 0.08f, 0.05f },
    { 0.38f, 0.25f, 0.15f },
    { 0.15f, 0.12f, 0.10f },
    8.0f,
    "hull timber",
    { 0.0f, 0.0f, 0.0f },
    false,
    true
};
// ---- Phase 50: the castles --------------------------------------------------------------------
//
// HULL_WOOD's four numbers under a name of their own. The masts and yards also use HULL_WOOD, so the
// sterncastle and forecastle blocks need a separate name for the picture tests to be able to draw them
// - and, from Phase 56, the gilded trim along their edges to be able to tell them apart.
const Material CASTLE_WOOD = {
    { 0.12f, 0.08f, 0.05f },
    { 0.38f, 0.25f, 0.15f },
    { 0.15f, 0.12f, 0.10f },
    8.0f,
    "castle wood"
};

// ---- Phase 51: the deck's planks ---------------------------------------------------------------
//
// DECK_WOOD's four numbers with the vertex-colour switch on, for the plank sheet (src/Planks.h): each strip of the
// sheet carries a light or dark plank shade or a dark seam as a multiplier. DECK_WOOD itself stays plain, because the
// slabs underneath, and the lip around them, are the unplanked cube.
const Material DECK_PLANK = {
    { 0.16f, 0.12f, 0.08f },
    { 0.58f, 0.45f, 0.29f },
    { 0.10f, 0.09f, 0.07f },
    6.0f,
    "deck plank",
    { 0.0f, 0.0f, 0.0f },
    false,
    true
};

// ---- Phase 52: the railings --------------------------------------------------------------------
//
// HULL_WOOD's four numbers under a name of its own, like CASTLE_WOOD: the masts and yards use HULL_WOOD too, so a picture
// test needs a separate name to draw the railings alone.
const Material RAIL_WOOD = {
    { 0.12f, 0.08f, 0.05f },
    { 0.38f, 0.25f, 0.15f },
    { 0.15f, 0.12f, 0.10f },
    8.0f,
    "rail wood"
};

// ---- Phase 53: the gunports --------------------------------------------------------------------
//
// Tuned, not from the slide. The opening is nearly black: it reads as a hole in the side. The frame is the hull's timber a shade
// darker, so a border of it shows round the opening and the opening looks recessed.
const Material GUNPORT_DARK = {
    { 0.010f, 0.008f, 0.006f },
    { 0.030f, 0.022f, 0.015f },
    { 0.020f, 0.020f, 0.020f },
    4.0f,
    "gunport"
};

const Material PORT_FRAME = {
    { 0.070f, 0.046f, 0.028f },
    { 0.210f, 0.140f, 0.085f },
    { 0.100f, 0.080f, 0.060f },
    8.0f,
    "port frame"
};

// ---- Playable build: health bars and clouds -----------------------------------------------------
//
// Pure emission: ka = kd = ks = 0, so the lights add nothing and the colour on screen is `ke` alone, the same at noon or in shadow. A health
// bar is a flat coloured rectangle that must read at a glance, not a lit object. Clouds are emissive too, so they hold their warm colour
// against the golden-hour sky instead of going dark on the shadowed side.
const Material BAR_BACK  = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "bar back",  { 0.05f, 0.04f, 0.04f } };
const Material BAR_GREEN = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "bar green", { 0.15f, 0.85f, 0.20f } };
const Material BAR_AMBER = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "bar amber", { 0.95f, 0.70f, 0.10f } };
const Material BAR_RED   = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "bar red",   { 0.90f, 0.12f, 0.10f } };
const Material CLOUD     = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "cloud",     { 0.98f, 0.80f, 0.62f } };

// ---- Environment build: terrain, vegetation and night lights -------------------------------------------------------------------------
//
// Tuned, not from a slide - like the hull timber and the sails. The values are chosen to read at the distances the scenery is seen from
// (20 to 70 units, under the haze) and to keep a clear lit side and shaded side under every light the environment can produce.
// Matt, mostly: a leaf, a rock and sand have little specular (k_s 0.03 - 0.08), so the sun's streak stays a property of the SEA.

const Material SAND = {
    { 0.20f, 0.17f, 0.11f }, { 0.78f, 0.68f, 0.46f }, { 0.05f, 0.05f, 0.04f }, 6.0f, "sand"
};
const Material DUNE_GRASS = {
    { 0.09f, 0.10f, 0.05f }, { 0.34f, 0.40f, 0.19f }, { 0.03f, 0.03f, 0.02f }, 5.0f, "dune grass"
};
const Material JUNGLE_DARK = {
    { 0.025f, 0.090f, 0.030f }, { 0.07f, 0.26f, 0.08f }, { 0.04f, 0.06f, 0.04f }, 10.0f, "jungle dark"
};
const Material JUNGLE_MID = {
    { 0.040f, 0.130f, 0.035f }, { 0.11f, 0.38f, 0.10f }, { 0.05f, 0.07f, 0.04f }, 10.0f, "jungle mid"
};
const Material JUNGLE_LIGHT = {
    { 0.060f, 0.170f, 0.040f }, { 0.17f, 0.48f, 0.11f }, { 0.06f, 0.08f, 0.04f }, 10.0f, "jungle light"
};
const Material PALM_FROND = {
    { 0.060f, 0.160f, 0.045f }, { 0.20f, 0.52f, 0.15f }, { 0.10f, 0.12f, 0.08f }, 14.0f, "palm frond"
};
const Material TREE_TRUNK = {
    { 0.100f, 0.070f, 0.040f }, { 0.34f, 0.23f, 0.14f }, { 0.04f, 0.04f, 0.03f }, 5.0f, "tree trunk"
};
const Material PINE_GREEN = {
    { 0.016f, 0.065f, 0.035f }, { 0.05f, 0.21f, 0.11f }, { 0.04f, 0.06f, 0.05f }, 9.0f, "pine green"
};
const Material CLIFF_ROCK = {
    { 0.115f, 0.105f, 0.095f }, { 0.38f, 0.34f, 0.30f }, { 0.07f, 0.07f, 0.07f }, 9.0f, "cliff rock"
};
const Material ROCK_DARK = {
    { 0.100f, 0.095f, 0.100f }, { 0.26f, 0.25f, 0.26f }, { 0.08f, 0.08f, 0.09f }, 12.0f, "dark rock"
};
const Material HILL_GREEN = {
    { 0.07f, 0.10f, 0.05f }, { 0.24f, 0.36f, 0.16f }, { 0.03f, 0.04f, 0.03f }, 6.0f, "hill green"
};
const Material SNOW = {
    { 0.30f, 0.31f, 0.34f }, { 0.86f, 0.89f, 0.94f }, { 0.40f, 0.42f, 0.46f }, 24.0f, "snow"
};
const Material GULL_WHITE = {
    { 0.30f, 0.30f, 0.30f }, { 0.88f, 0.88f, 0.86f }, { 0.05f, 0.05f, 0.05f }, 6.0f, "gull", { 0.17f, 0.18f, 0.19f }
};

// Emissive things. Pure emission (ka = kd = ks = 0) so the colour on screen is `ke` alone: a flame is as bright in shadow as in light.
// LANTERN_GLASS's ke is the LIT value; the renderer scales it by how lit the lanterns are (Atmosphere::lanterns), so by day the glass is
// a dark amber and at night it glows.
const Material LANTERN_GLASS = {
    { 0.10f, 0.06f, 0.02f }, { 0.30f, 0.18f, 0.06f }, { 0.60f, 0.50f, 0.30f }, 40.0f, "lantern glass", { 1.00f, 0.72f, 0.32f }
};
const Material LANTERN_GLOW  = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "lantern glow", { 1.00f, 0.68f, 0.28f } };
const Material MOON_DISC     = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "moon disc",    { 0.88f, 0.92f, 1.00f } };
const Material LIGHTNING_BOLT = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "lightning",   { 0.88f, 0.93f, 1.00f } };
const Material LIGHTHOUSE_LAMP = { {0,0,0}, {0,0,0}, {0,0,0}, 1.0f, "lamp",       { 1.00f, 0.90f, 0.55f } };

// ---- Treasure, wildlife and the ship's interior --------------------------------------------------------------------------------------------
//
// Tuned, like the rest of the scenery. GOLD is the one bright, shiny material in the scene besides brass; a little emission (ke) makes it catch the eye in shadow
// and in the dark of the hold, where the chests are meant to be found.
const Material GOLD = {
    { 0.34f, 0.25f, 0.05f }, { 0.85f, 0.66f, 0.14f }, { 0.95f, 0.85f, 0.35f }, 70.0f, "gold", { 0.10f, 0.07f, 0.01f }
};
const Material CHEST_WOOD = {
    { 0.12f, 0.07f, 0.04f }, { 0.40f, 0.23f, 0.11f }, { 0.12f, 0.10f, 0.08f }, 10.0f, "chest wood"
};
const Material IRON_DARK = {
    { 0.05f, 0.05f, 0.055f }, { 0.20f, 0.20f, 0.22f }, { 0.60f, 0.60f, 0.65f }, 40.0f, "dark iron"
};
const Material LEATHER = {
    { 0.09f, 0.05f, 0.03f }, { 0.34f, 0.19f, 0.10f }, { 0.08f, 0.06f, 0.05f }, 8.0f, "leather"
};
const Material BOOK_RED   = { { 0.10f, 0.01f, 0.01f }, { 0.55f, 0.06f, 0.06f }, { 0.05f, 0.05f, 0.05f }, 8.0f, "red book" };
const Material BOOK_GREEN = { { 0.01f, 0.07f, 0.03f }, { 0.08f, 0.38f, 0.15f }, { 0.05f, 0.05f, 0.05f }, 8.0f, "green book" };
const Material BOOK_BLUE  = { { 0.01f, 0.03f, 0.10f }, { 0.07f, 0.16f, 0.55f }, { 0.05f, 0.05f, 0.05f }, 8.0f, "blue book" };
const Material PAPER = {
    { 0.34f, 0.31f, 0.22f }, { 0.88f, 0.82f, 0.62f }, { 0.04f, 0.04f, 0.04f }, 4.0f, "paper"
};
const Material KEG_RED = {
    { 0.09f, 0.02f, 0.02f }, { 0.34f, 0.07f, 0.06f }, { 0.08f, 0.06f, 0.06f }, 10.0f, "powder keg"
};
const Material SACK = {
    { 0.20f, 0.17f, 0.11f }, { 0.64f, 0.55f, 0.36f }, { 0.03f, 0.03f, 0.03f }, 3.0f, "sack"
};
const Material BLANKET = {
    { 0.05f, 0.08f, 0.16f }, { 0.16f, 0.26f, 0.52f }, { 0.03f, 0.03f, 0.03f }, 3.0f, "blanket"
};
const Material ROPE_MAT = {
    { 0.18f, 0.14f, 0.08f }, { 0.60f, 0.50f, 0.30f }, { 0.04f, 0.04f, 0.04f }, 4.0f, "rope"
};
const Material HOLD_PLANK = {
    { 0.10f, 0.07f, 0.045f }, { 0.34f, 0.23f, 0.14f }, { 0.07f, 0.06f, 0.05f }, 6.0f, "hold plank"
};
const Material GULL_GREY = {
    { 0.20f, 0.20f, 0.22f }, { 0.52f, 0.54f, 0.58f }, { 0.04f, 0.04f, 0.04f }, 6.0f, "tern", { 0.11f, 0.11f, 0.12f }
};
const Material FISH_SILVER = {
    { 0.22f, 0.26f, 0.30f }, { 0.60f, 0.70f, 0.78f }, { 0.70f, 0.75f, 0.80f }, 40.0f, "silver fish"
};
const Material FISH_ORANGE = {
    { 0.34f, 0.14f, 0.02f }, { 0.95f, 0.42f, 0.06f }, { 0.50f, 0.40f, 0.30f }, 30.0f, "orange fish"
};
const Material FISH_BLUE = {
    { 0.02f, 0.10f, 0.24f }, { 0.08f, 0.34f, 0.80f }, { 0.50f, 0.55f, 0.70f }, 30.0f, "blue fish"
};
// Water falling down a rock face: a pale blue-white that glows a little by itself, drawn translucent; the renderer scales the glow by the light of the hour.
const Material WATERFALL = {
    { 0.50f, 0.58f, 0.64f }, { 0.78f, 0.88f, 0.96f }, { 0.60f, 0.66f, 0.72f }, 40.0f, "waterfall", { 0.30f, 0.40f, 0.50f }
};
const Material DOLPHIN_BACK = {
    { 0.18f, 0.23f, 0.28f }, { 0.46f, 0.56f, 0.64f }, { 0.45f, 0.50f, 0.55f }, 50.0f, "dolphin", { 0.04f, 0.06f, 0.08f }
};
const Material DOLPHIN_BELLY = {
    { 0.30f, 0.32f, 0.34f }, { 0.82f, 0.86f, 0.88f }, { 0.35f, 0.38f, 0.40f }, 40.0f, "dolphin belly", { 0.10f, 0.11f, 0.12f }
};
