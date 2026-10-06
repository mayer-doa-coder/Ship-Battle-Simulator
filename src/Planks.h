// Ship Battle Simulator - Phase 51: deck planks - the plank strips, as data.
//
// Nothing in this file touches OpenGL. It says WHERE each strip of a deck lies across the beam and WHAT COLOUR it is;
// src/Mesh.h turns that into triangles, and the tests read the same numbers.
//
// A deck is laid in planks that run along the ship. Looked down on, it is a row of strips across the beam: a plank,
// a thin dark SEAM (the caulked gap between two planks), the next plank, and so on. The mesh is a UNIT mesh - one
// flat sheet, 1 x 1, lying in the xz plane at y = 0 with its normal up - and its real size is the draw-time scale
// (CLAUDE.md: scale only in the final drawMesh), so every deck is the same mesh stretched to fit.
//
//   x = -0.5                                                        x = +0.5
//    |s| plank 0 |S| plank 1 |S| plank 2 |S| ...  |S| plank 8 |s|        S = a full seam, s = half a seam
//
// There are PLANKS planks and PLANKS + 1 seams. The two OUTER seams are half-width, clipped to the sheet, so every
// plank has the same width and the pattern is symmetric about the centreline - which is why PLANKS is odd: with an
// odd count the light-dark alternation reads the same from either side.
//
// Colours are VERTEX COLOURS read as a multiplier on the material's diffuse and ambient colours (Phase 48), the same
// mechanism as the hull's planking (Phase 49): light and dark planks alternate so the eye can count them, and a seam is
// much darker than either.
#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace DeckPlanking {

// How many planks lie across the beam. Odd, so the pattern is symmetric. The viva exercise changes this one number.
constexpr int PLANKS = 9;
constexpr int SEAMS = PLANKS + 1;

// A seam's width as a fraction of one plank's PITCH (the distance from one plank's centre to the next's).
constexpr float SEAM_FRACTION = 0.14f;

// How the vertex colour multiplies the material.
const glm::vec3 LIGHT_PLANK(1.10f, 1.06f, 1.00f);
const glm::vec3 DARK_PLANK(0.88f, 0.84f, 0.80f);
const glm::vec3 SEAM_COLOR(0.30f, 0.27f, 0.24f);

// How far above a slab's top surface the plank sheet is laid, as a fraction of the slab's thickness. A sheet exactly on
// the surface would fight it for the depth buffer; this is small enough to be invisible and large enough not to fight.
constexpr float LIFT_FRACTION = 0.02f;

}  // namespace DeckPlanking

// One strip of the sheet across its width: from x0 to x1 (unit-mesh coordinates, -0.5 to 0.5).
struct DeckStrip {
    float x0;
    float x1;
    glm::vec3 color;
    bool isSeam;
    int plank;      // for a plank, its index 0..planks-1 from the port edge; for a seam, the seam's index 0..planks
};

// The colour of plank i: light, dark, light, ... starting light.
inline glm::vec3 deckPlankColor(int i)
{
    return (i % 2 == 0) ? DeckPlanking::LIGHT_PLANK : DeckPlanking::DARK_PLANK;
}

// The strips left to right: seam 0, plank 0, seam 1, plank 1, ..., plank planks-1, seam planks. Together they cover
// -0.5 .. +0.5 exactly once, with no gap and no overlap.
inline std::vector<DeckStrip> deckStrips(int planks, float seamFraction)
{
    std::vector<DeckStrip> strips;
    const float pitch = 1.0f / static_cast<float>(planks);
    const float half = 0.5f * seamFraction * pitch;                        // half a seam
    for (int i = 0; i <= planks; ++i) {
        const float line = -0.5f + static_cast<float>(i) * pitch;          // the seam's centre line
        const float lo = (i == 0) ? line : line - half;                    // the outer seams are clipped to the sheet
        const float hi = (i == planks) ? line : line + half;
        strips.push_back({ lo, hi, DeckPlanking::SEAM_COLOR, true, i });
        if (i < planks) {
            const float next = -0.5f + static_cast<float>(i + 1) * pitch;
            strips.push_back({ hi, next - half, deckPlankColor(i), false, i });     // ends where the next seam begins
        }
    }
    return strips;
}
