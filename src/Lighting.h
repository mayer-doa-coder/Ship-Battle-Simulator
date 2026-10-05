#pragma once

// Phase 31: the illumination model, written ONCE, as GLSL text in a C++ string.
//
// WHY IT LIVES IN A STRING RATHER THAN IN A FILE
//
// Gouraud shading evaluates the lighting once per VERTEX and lets the rasteriser
// blend the resulting colours across the triangle. Phong shading blends the NORMALS
// across the triangle and evaluates the lighting once per FRAGMENT. The whole value
// of comparing them is that this is the ONLY difference between them.
//
// If the vertex shader and the fragment shader each had their own copy of the
// formula, that claim would be unverifiable: a typo in one copy, or an improvement
// made to one and forgotten in the other, would make the comparison a lie - and the
// comparison is the single biggest piece of evidence in the report (L9).
//
// So this text is handed to ShaderProgram::loadFromFiles(), which splices it into
// both stages immediately after their #version lines. One source of truth, two
// places it runs.
//
// It deliberately contains no main(), no attribute and no varying - only the
// uniforms it needs and two pure functions. Everything stage-specific stays in the
// .vert and .frag files.

#include <string>

// The three shading modes, as plain ints shared between C++ and GLSL. GLSL 330 has
// no enum, so the numbers are written out in the string below too and have to agree.
namespace ShadingMode {
constexpr int FLAT = 0;
constexpr int GOURAUD = 1;
constexpr int PHONG = 2;
constexpr int COUNT = 3;

inline const char* name(int mode)
{
    switch (mode) {
    case FLAT:    return "Flat";
    case GOURAUD: return "Gouraud";
    case PHONG:   return "Phong";
    default:      return "?";
    }
}
} // namespace ShadingMode

// R"GLSL( ... )GLSL" is a C++11 raw string: everything between the delimiters is
// taken literally, so the GLSL can contain quotes and backslashes and keep its own
// line breaks. Without it this would be an unreadable wall of "\n" escapes.
inline const std::string& sharedLightingSource()
{
    static const std::string source = R"GLSL(
// ===========================================================================
// SHARED LIGHTING - injected into BOTH the vertex and the fragment shader.
// Edited in src/Lighting.h. There is exactly one copy of this code.
// ===========================================================================

// Phase 31: which shading model to use. 0 Flat, 1 Gouraud, 2 Phong.
//
// ONE uniform and ONE branch, in ONE program - not three programs. Three programs
// would mean three copies of this file to keep in step, and would mean a
// glUseProgram call every time the mode changed. With a single program, switching
// mode costs one integer upload and no state change at all.
uniform int uShadingMode;

// ---- the material (Phase 29) ----------------------------------------------
uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uShininess;

// ---- the lights (Phases 27 and 30) ----------------------------------------
uniform vec3 uGlobalAmbient;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform vec3 uPointPosition;
uniform vec3 uPointColor;
uniform float uPointIntensity;
uniform vec3 uAttenuation;
uniform int uLightMask;

// Where the camera is, in world space. Must be refreshed every frame.
uniform vec3 uViewPos;

// Phase 32: WHICH TERMS of the illumination model are switched on, as bit flags.
//
//   bit 0 (1) ambient
//   bit 1 (2) diffuse
//   bit 2 (4) specular
//
// This is L8 slide 54 made interactive. That slide shows the three terms separately
// and then added together, and the 'K' key walks through exactly that sequence in
// the project's own scene rather than on the slide's test spheres.
//
// It is worth having as a key rather than a screenshot because the terms are only
// separable BEFORE they are added. Once three numbers have been summed into one
// pixel, no amount of looking can take them apart again.
uniform int uTermMask;

// Phase 32: 0 uses Phong's (R . V)^n, 1 uses Blinn-Phong's (N . H)^n (L8 s49-50).
uniform int uUseBlinn;


// Everything one light does to one surface point (Phase 30).
//
//   N            the surface normal, unit length
//   V            from the surface toward the viewer, unit length
//   L            from the surface toward the light, unit length
//   lightColor   the light's colour, already scaled by its intensity
//   attenuation  1.0 for a directional light, 1/(a0+a1d+a2d^2) for a point light
vec3 lightContribution(vec3 N, vec3 V, vec3 L, vec3 lightColor, float attenuation)
{
    // Lambert's cosine law. N . L IS the cosine, because both are unit vectors.
    // max(..., 0.0) stops a surface facing away from SUBTRACTING light.
    float lambert = max(dot(N, L), 0.0);

    // Phase 32: two ways to measure "how close are you to the mirror bounce?".
    //
    // PHONG       (R . V)^n   R is the light reflected off the surface.
    // BLINN-PHONG (N . H)^n   H is the HALFWAY vector between L and V.
    //
    // They answer the same question differently (L8 s49-50). H is the direction a
    // surface would have to face for you to be looking straight down the mirror
    // bounce - so N . H asks "how far is this surface from the orientation that
    // would send the light at me?", while R . V asks "how far am I from where the
    // light actually went?".
    //
    // Blinn is cheaper: H is one add and one normalize, where R needs a reflect().
    // And it never produces the artefact Phong can - when the light and the viewer
    // are on nearly opposite sides, R . V can go negative and cut the highlight off
    // abruptly at a glancing angle, where N . H stays smooth.
    //
    // The two are NOT the same number. For the same exponent Blinn's highlight is
    // noticeably BROADER, because the angle between N and H is roughly half the angle
    // between R and V. Matching them needs Blinn's exponent to be about 2-4x Phong's,
    // which is exactly why 'B' is a comparison key and not a drop-in replacement.
    float base;
    if (uUseBlinn != 0) {
        vec3 H = normalize(L + V);
        base = max(dot(N, H), 0.0);
    } else {
        // reflect() wants the INCOMING direction, the way the light travels toward the
        // surface, which is -L.
        base = max(dot(reflect(-L, N), V), 0.0);
    }

    // Gating on lambert matters: the specular base can be large on a surface facing
    // AWAY from the light, which would make a shiny object glint on its own dark side.
    float specular = (lambert > 0.0) ? pow(base, uShininess) : 0.0;

    // Phase 32: each term can be switched off independently, which is what makes
    // L8 slide 54 demonstrable rather than just quotable.
    //
    // The specular term is NOT multiplied by uKd. A highlight is the colour of the
    // LIGHT, not of the object, which is why a shiny red ball has a white highlight.
    vec3 result = vec3(0.0);
    if ((uTermMask & 2) != 0) result += uKd * lambert;
    if ((uTermMask & 4) != 0) result += uKs * specular;

    return attenuation * lightColor * result;
}

// The whole illumination model for one point on a surface.
//
// THIS is the function Gouraud calls per vertex and Phong calls per fragment. It
// does not know or care which - it is handed a normal and a world position and
// returns a colour. That is what makes the two shading modes genuinely comparable.
vec3 computeLighting(vec3 N, vec3 worldPos)
{
    // From the surface toward the camera. Worked out once and shared by both lights,
    // because it depends on where you are standing and not on which light it is.
    vec3 V = normalize(uViewPos - worldPos);

    // The ambient term belongs to the SCENE, not to a light, so it is added once
    // rather than inside the per-light work - otherwise two lights would double it.
    // Phase 32: and it is bit 0 of the term mask.
    vec3 total = ((uTermMask & 1) != 0) ? (uGlobalAmbient * uKa) : vec3(0.0);

    // The sun: directional, so attenuation is 1.0. uSunDirection stores the direction
    // the light TRAVELS, and L has to point from the surface toward the light, so it
    // is negated.
    if ((uLightMask & 1) != 0) {
        total += lightContribution(N, V, -normalize(uSunDirection), uSunColor, 1.0);
    }

    // The point light: it HAS a position, so it has a distance, so it falls off.
    if ((uLightMask & 2) != 0) {
        vec3 toLight = uPointPosition - worldPos;
        float d = length(toLight);
        float attenuation = 1.0 / (uAttenuation.x
                                 + uAttenuation.y * d
                                 + uAttenuation.z * d * d);
        total += lightContribution(N, V, toLight / d, uPointColor * uPointIntensity,
                                   attenuation);
    }

    return total;
}
// ===================== end of shared lighting ==============================
)GLSL";
    return source;
}
