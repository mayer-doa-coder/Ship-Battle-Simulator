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

// The sea (the grid mesh, from Phase 40). Dark and barely coloured in diffuse, but
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
