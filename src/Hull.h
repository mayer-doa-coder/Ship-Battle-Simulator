#pragma once

// Phase 47: the lofted hull - the mathematics, with no OpenGL.
//
// WHY A HULL CANNOT BE A SCALED CUBE
//
// A cube scaled to (0.42, 0.20, 1.40) is a barge. A galleon is narrow at the stem, full amidships,
// rounded under the water, higher at the stern and bow than in the waist, and it tapers again
// aft. None of that is a size - it is a SHAPE - and a scale matrix cannot make a shape. So this is
// the first of the three generators the plan allows beyond the five original meshes (the others
// are the plank strip, Phase 51, and the sail, Phase 60).
//
// LOFTING
//
// A hull is described the way a shipwright's plan describes it: by cross-sections. Each STATION is
// the outline of the hull cut across at one point along its length. Draw a few of them - stern to
// bow - and fair a surface through them, and that is the hull ("lofting" is exactly that).
//
// Here a station is four numbers, all FRACTIONS, so the table says nothing about how big the ship
// is. Size is applied at draw time like everything else (Phase 16):
//
//     t          where along the hull: 0 at the stern, 1 at the stem
//     halfBeam   half the width at that station, as a fraction of the widest point (1 = widest)
//     keelRise   how far the bottom of the section is lifted off the keel line, as a fraction of
//                the hull's depth (0 = the keel; the forefoot and the run aft rise out of the water)
//     sheer      the height of the gunwale (the top edge of the side), as a fraction of the depth;
//                it is lowest in the waist and rises towards both ends - the "sheer line"
//
// The mesh built from the table is a UNIT mesh: it spans [-0.5, 0.5] on each axis, so scaling it by
// (beam, depth, length) gives the real hull.

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

constexpr int HULL_STATION_COUNT = 10;

struct HullStation {
    float t;
    float halfBeam;
    float keelRise;
    float sheer;
};

// All floats on purpose (see the Phase 38 test that reads ShipDimensions as an array of floats): the
// whole table is fractions, so scaleShipDimensions() must leave every one of them alone.
struct HullProfile {
    HullStation stations[HULL_STATION_COUNT];
};

namespace HullShape {

// How a cross-section is drawn between the keel and the gunwale. These are the same for every
// station; the station table only changes how wide, how high and how raised each one is.
//
//   Keel ... bilge   a rounded quarter-ellipse. Its rows are spaced EQUALLY IN ANGLE round the arc, so the
//                    curve has the same resolution all the way up it.
//   Bilge ... gunwale   the topsides: straight, rising, and leaning IN by TUMBLEHOME. A real galleon's
//                    sides lean inward above the waterline, which both looks right and shrinks the top
//                    deck. Rows here are spaced equally in HEIGHT.
//
// BILGE_HEIGHT is the fraction of the section's height taken by the rounded part.
constexpr float BILGE_HEIGHT = 0.55f;
constexpr float TUMBLEHOME = 0.12f;       // the gunwale's half-width is (1 - this) of the widest point

// How many strakes (rows of planking) the side has in each part, counted from the keel up. A strake
// is one row of the loft; Phase 49 colours them in alternating shades.
constexpr int BILGE_STRAKES = 6;
constexpr int TOPSIDE_STRAKES = 6;
constexpr int STRAKES = BILGE_STRAKES + TOPSIDE_STRAKES;

// How many rings (cross-sections) the finished mesh is cut into along its length. More than the
// ten stations in the table, because the surface is faired BETWEEN them (see hullStationAt).
constexpr int RINGS = 41;

} // namespace HullShape

// ---- shape-preserving interpolation -------------------------------------------------------
//
// To fair a surface through ten stations the table is interpolated along the hull. A plain
// Catmull-Rom spline would overshoot: through half-beams 0.72, 0.40, 0.18, 0.00 it would swing
// BELOW zero near the stem and give the bow a negative width. PCHIP (piecewise cubic Hermite
// interpolation, Fritsch-Carlson) never overshoots its data: between two stations the value stays
// between the two stations' values, and at a local minimum or maximum - the waist's low sheer - the
// curve has exactly that value. Both properties are tested.
inline float hullPchip(const float* x, const float* y, int n, float xq)
{
    if (xq <= x[0]) return y[0];
    if (xq >= x[n - 1]) return y[n - 1];

    // Which interval is xq in?
    int i = 0;
    while (i < n - 2 && xq > x[i + 1]) ++i;

    // Secant slopes and interval widths.
    float h[HULL_STATION_COUNT - 1], d[HULL_STATION_COUNT - 1];
    for (int k = 0; k < n - 1; ++k) {
        h[k] = x[k + 1] - x[k];
        d[k] = (y[k + 1] - y[k]) / h[k];
    }

    // The derivative at each station: zero where the data turns round (so no overshoot), otherwise
    // the weighted harmonic mean of the neighbouring secants.
    float m[HULL_STATION_COUNT];
    for (int k = 1; k < n - 1; ++k) {
        if (d[k - 1] * d[k] <= 0.0f) {
            m[k] = 0.0f;
        } else {
            const float w1 = 2.0f * h[k] + h[k - 1];
            const float w2 = h[k] + 2.0f * h[k - 1];
            m[k] = (w1 + w2) / (w1 / d[k - 1] + w2 / d[k]);
        }
    }
    // The ends: a one-sided three-point estimate, limited so it cannot overshoot either.
    auto endSlope = [](float h0, float h1, float d0, float d1) {
        float s = ((2.0f * h0 + h1) * d0 - h0 * d1) / (h0 + h1);
        if (s * d0 <= 0.0f) s = 0.0f;
        else if (d0 * d1 <= 0.0f && std::fabs(s) > 3.0f * std::fabs(d0)) s = 3.0f * d0;
        return s;
    };
    m[0] = endSlope(h[0], h[1], d[0], d[1]);
    m[n - 1] = endSlope(h[n - 2], h[n - 3], d[n - 2], d[n - 3]);

    // The cubic Hermite polynomial on [x[i], x[i+1]].
    const float s = (xq - x[i]) / h[i];
    const float s2 = s * s, s3 = s2 * s;
    const float h00 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    const float h10 = s3 - 2.0f * s2 + s;
    const float h01 = -2.0f * s3 + 3.0f * s2;
    const float h11 = s3 - s2;
    return h00 * y[i] + h10 * h[i] * m[i] + h01 * y[i + 1] + h11 * h[i] * m[i + 1];
}

// The section at ANY t between 0 and 1, faired through the table. At a station's own t this
// returns that station's numbers exactly.
inline HullStation hullStationAt(const HullProfile& p, float t)
{
    float x[HULL_STATION_COUNT], b[HULL_STATION_COUNT], r[HULL_STATION_COUNT], s[HULL_STATION_COUNT];
    for (int k = 0; k < HULL_STATION_COUNT; ++k) {
        x[k] = p.stations[k].t;
        b[k] = p.stations[k].halfBeam;
        r[k] = p.stations[k].keelRise;
        s[k] = p.stations[k].sheer;
    }
    t = std::min(1.0f, std::max(0.0f, t));
    return { t,
             hullPchip(x, b, HULL_STATION_COUNT, t),
             hullPchip(x, r, HULL_STATION_COUNT, t),
             hullPchip(x, s, HULL_STATION_COUNT, t) };
}

// One cross-section of the finished mesh, in the mesh's own UNIT space.
struct HullRing {
    float z;          // along the hull, -0.5 (stern) to +0.5 (stem)
    float halfWidth;  // half the section's widest width: 0.5 * halfBeam
    float bottom;     // y of the section's lowest point: -0.5 + keelRise
    float top;        // y of the gunwale:                  -0.5 + sheer
};

// The rings the mesh is built from: RINGS stations, evenly spaced, each read off the faired table.
inline std::vector<HullRing> hullRings(const HullProfile& p, int ringCount = HullShape::RINGS)
{
    std::vector<HullRing> rings;
    rings.reserve(static_cast<std::size_t>(ringCount));
    for (int i = 0; i < ringCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(ringCount - 1);
        const HullStation s = hullStationAt(p, t);
        rings.push_back({ t - 0.5f, 0.5f * s.halfBeam, -0.5f + s.keelRise, -0.5f + s.sheer });
    }
    return rings;
}

// One point of a half-section: k = 0 is the keel (on the centreline), k = STRAKES is the gunwale.
// Returns (x, y) in unit space for the RIGHT (+x) side; mirror x for the left.
//
//     k = 0 .. BILGE_STRAKES      round the bilge:  x = w sin(phi),  height = BILGE_HEIGHT (1 - cos phi)
//                                  with phi stepping equally from 0 to a quarter turn
//     k > BILGE_STRAKES           up the topsides:  height rises linearly; x leans in by TUMBLEHOME
//
// `height` is the fraction of the section's own height (bottom to top), so y = bottom + height * (top - bottom).
inline glm::vec2 hullSectionPointAt(const HullRing& r, float k)
{
    constexpr float HALF_PI = 1.57079632679489662f;
    float xFrac, height;
    if (k <= static_cast<float>(HullShape::BILGE_STRAKES)) {
        const float phi = HALF_PI * k / static_cast<float>(HullShape::BILGE_STRAKES);
        xFrac = std::sin(phi);
        height = HullShape::BILGE_HEIGHT * (1.0f - std::cos(phi));
    } else {
        const float tt = (k - static_cast<float>(HullShape::BILGE_STRAKES)) / static_cast<float>(HullShape::TOPSIDE_STRAKES);
        xFrac = 1.0f - HullShape::TUMBLEHOME * tt;
        height = HullShape::BILGE_HEIGHT + (1.0f - HullShape::BILGE_HEIGHT) * tt;
    }
    return glm::vec2(r.halfWidth * xFrac, r.bottom + height * (r.top - r.bottom));
}

// The mesh's own ring-points are at whole k; the same curve can be read between them (k = 4.5 is the middle of the
// fifth strake, ON the surface - the straight line between two rows is a chord, and for the rounded bilge it lies inside
// the hull). Tests and anything that wants a point on a plank use this.
inline glm::vec2 hullSectionPoint(const HullRing& r, int k)
{
    return hullSectionPointAt(r, static_cast<float>(k));
}

// ---- reading the hull's size at a point, in real units ------------------------------------
//
// These take the profile and the three dimensions of the hull's box, and answer questions the rest
// of the ship needs: how wide is the hull here, how high is the deck here? `z` is measured along the
// hull from its CENTRE (so the stern is -length/2), in the same units as `length`.

// Half the hull's width at its widest point of the section at z.
inline float hullHalfBeamAt(const HullProfile& p, float beam, float length, float z)
{
    return beam * hullStationAt(p, z / length + 0.5f).halfBeam * 0.5f;
}

// Half the width at the GUNWALE at z (narrower than the above, by the tumblehome).
inline float hullGunwaleHalfBeamAt(const HullProfile& p, float beam, float length, float z)
{
    return hullHalfBeamAt(p, beam, length, z) * (1.0f - HullShape::TUMBLEHOME);
}

// Height of the gunwale above the KEEL LINE at z.
inline float hullGunwaleHeightAt(const HullProfile& p, float depth, float length, float z)
{
    return depth * hullStationAt(p, z / length + 0.5f).sheer;
}

// Height of the underside of the section above the keel line at z (the keel rises at both ends).
inline float hullKeelHeightAt(const HullProfile& p, float depth, float length, float z)
{
    return depth * hullStationAt(p, z / length + 0.5f).keelRise;
}

// The lowest the gunwale ever gets - the level of the waist. Pchip's extremum is exactly at a
// station, so the minimum over the stations is the minimum over the whole curve. This is the level
// the main deck sits at (Ship.h), and for a box-shaped hull whose every sheer is 1 it is the depth,
// which is where Phase 34 put the deck.
inline float hullLowestSheer(const HullProfile& p)
{
    float lo = p.stations[0].sheer;
    for (int k = 1; k < HULL_STATION_COUNT; ++k) lo = std::min(lo, p.stations[k].sheer);
    return lo;
}

// ---- Phase 49: planking - the colours the vertices carry -----------------------------------
//
// The hull's side is STRAKES rows of planks, from the keel (strake 1) to the gunwale (strake STRAKES).
// A real plank is one colour; so each strake gets one colour, and the colours are VERTEX COLOURS - the
// switch Phase 48 turned on - read as a MULTIPLIER on HULL_TIMBER's diffuse colour (1 = as the material
// says, below 1 = darker timber).
//
//   light plank   a little brighter than the material
//   dark plank    a little darker; light and dark ALTERNATE up the side, so the eye can count the planks
//   belt          dark, heavy timber: the WALES that run the length of a real hull, thicker than the planking
//                 and nearly black with tar. There are three: at the waterline, at the deck line (just under
//                 where the gunports will cut the side), and the gunwale rail along the top.
//
// All of these are numbers a teacher can change; none is computed from a picture.
namespace HullPlanking {

constexpr int BELT_COUNT = 3;

// Where the belts are, as a fraction of the way up the side (0 = keel, 1 = gunwale). A fraction, not a
// strake number, so that changing the number of strakes moves the belts with it instead of leaving one
// off the end. 0.40 puts the first belt on the strake that holds the waterline amidships.
constexpr float BELT_FRACTIONS[BELT_COUNT] = { 0.40f, 0.75f, 1.00f };

const glm::vec3 LIGHT_PLANK(1.12f, 1.08f, 1.02f);
const glm::vec3 DARK_PLANK(0.86f, 0.82f, 0.78f);
const glm::vec3 BELT(0.45f, 0.40f, 0.36f);

// The cap is the deck seen between the gunwales: a lighter, drier wood than the sides. 1.45-ish times
// HULL_TIMBER's brown lands close to DECK_WOOD, so the cap and the deck slab laid on it read as one deck.
const glm::vec3 CAP(1.45f, 1.50f, 1.50f);
const glm::vec3 TRANSOM(0.80f, 0.76f, 0.72f);

} // namespace HullPlanking

// The strake (1 .. strakes) a belt sits on: the belt's fraction of the way up, rounded to the nearest
// strake and kept inside the side.
inline int hullBeltStrake(int belt, int strakes)
{
    const int k = static_cast<int>(std::lround(HullPlanking::BELT_FRACTIONS[belt] * static_cast<float>(strakes)));
    return std::max(1, std::min(strakes, k));
}

inline bool hullIsBeltStrake(int k, int strakes)
{
    for (int b = 0; b < HullPlanking::BELT_COUNT; ++b)
        if (hullBeltStrake(b, strakes) == k) return true;
    return false;
}

// The colour (a multiplier) of strake k, counting from the keel at 1: a belt if it is one, otherwise
// light and dark planks alternating.
inline glm::vec3 hullStrakeColor(int k, int strakes)
{
    if (hullIsBeltStrake(k, strakes)) return HullPlanking::BELT;
    return (k % 2 == 1) ? HullPlanking::LIGHT_PLANK : HullPlanking::DARK_PLANK;
}

// Everything the hull builder needs to paint a hull.
struct HullColors {
    std::vector<glm::vec3> strake;   // index 0 unused; 1 .. STRAKES
    glm::vec3 cap;
    glm::vec3 transom;
};

inline HullColors hullPlankColors(int strakes = HullShape::STRAKES)
{
    HullColors c;
    c.strake.assign(static_cast<std::size_t>(strakes) + 1u, glm::vec3(1.0f));
    for (int k = 1; k <= strakes; ++k) c.strake[static_cast<std::size_t>(k)] = hullStrakeColor(k, strakes);
    c.cap = HullPlanking::CAP;
    c.transom = HullPlanking::TRANSOM;
    return c;
}

// A hull painted one colour everywhere: Phase 47's look, kept for tests that do not care about planks.
inline HullColors hullPlainColors(const glm::vec3& color, int strakes = HullShape::STRAKES)
{
    HullColors c;
    c.strake.assign(static_cast<std::size_t>(strakes) + 1u, color);
    c.cap = color;
    c.transom = color;
    return c;
}

// The strake a side triangle belongs to, from the positions of its vertices IN THE RING (j = 0 is the left gunwale,
// j = STRAKES the keel, j = 2 * STRAKES the right gunwale). A quad between point j and point j + 1 on the left is
// the strake whose upper row is STRAKES - j; on the right it is j - STRAKES + 1.
inline int hullStrakeOfQuad(int jLow, int strakes)
{
    return (jLow < strakes) ? (strakes - jLow) : (jLow - strakes + 1);
}
