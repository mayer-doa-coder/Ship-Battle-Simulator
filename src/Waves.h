#pragma once

// Environment build: the DYNAMIC OCEAN - a sea that moves, and a ship that moves with it.
//
// ONE TABLE, TWO PLACES IT RUNS. The sea's height is a sum of four travelling sine waves,
//
//     h(x, z, t) = S * sum_i  A_i * sin( k_i * (d_i . (x, z)) - w_i * t + phi_i )
//
// and it is needed twice: in the VERTEX SHADER, which lifts every vertex of the sea mesh to that height (and works out the surface's normal from the
// slope, so the light plays over the swell), and on the CPU, where the ship has to know how high the water is under its bow, its stern and its two
// sides so that it can ride it. If those were two copies of the formula, a ship could float above a trough or sink into a crest the moment someone
// edited one of them (the old Broadside project's "mismatched wave constants" bug). So there is ONE table here; waveHeight() reads it on the CPU,
// and waveGlslSource() prints the very same numbers into the GLSL text the shader program is built from. Change a wavelength, and both change.
//
// WHY THESE FOUR. Long and slow (24 units) is the swell that lifts the whole ship; 14 and 8 are the chop that rocks it; 4.6 is the ripple that breaks
// up the sun's reflection. Their directions differ so the surface never looks like ruled lines. Each wave's speed is DEEP-WATER DISPERSION,
// w = sqrt(g k): long waves travel faster than short ones, as they do at sea. g is not 9.81 - one world unit is about five metres of ship - but a
// value (WaveConfig::GRAVITY) chosen so the swell has a period of about seven seconds.
//
// S is the SEA STATE, a number the weather sets (Atmosphere::waveAmp): 0.45 on a fair day, 2.0 in a storm. The gallery's sea has S = 0 - flat - because
// Demo A depends on it being a flat grid.
//
// No OpenGL here: the checks that the CPU and the text agree run without a window.

#include <glm/glm.hpp>

#include <cmath>
#include <cstdio>
#include <string>

namespace WaveConfig {

constexpr int COUNT = 4;

// How fast waves of a given length travel. Chosen, not physical: see above.
constexpr float GRAVITY = 2.0f;

struct Component {
    float dirX, dirZ;        // the direction the wave travels, normalised below
    float wavelength;        // world units
    float amplitude;         // world units, at sea state 1
    float phase;             // radians: so the four do not all start at a crest at the origin
};

constexpr Component TABLE[COUNT] = {
    {  0.80f,  0.60f, 24.0f, 0.26f, 0.0f },     // the swell
    { -0.42f,  0.91f, 14.0f, 0.15f, 1.7f },
    {  0.95f, -0.31f,  8.0f, 0.075f, 3.1f },
    { -0.71f, -0.71f,  4.6f, 0.040f, 4.4f },    // the ripple
};

constexpr float TWO_PI = 6.28318530718f;

} // namespace WaveConfig

// The wave number k = 2 pi / wavelength, and the angular speed w = sqrt(g k).
inline float waveNumber(int i) { return WaveConfig::TWO_PI / WaveConfig::TABLE[i].wavelength; }
inline float waveSpeedW(int i) { return std::sqrt(WaveConfig::GRAVITY * waveNumber(i)); }

inline glm::vec2 waveDirection(int i)
{
    return glm::normalize(glm::vec2(WaveConfig::TABLE[i].dirX, WaveConfig::TABLE[i].dirZ));
}

// The sum of the amplitudes: the highest a crest can be at sea state 1. (The shader uses it to know how close to a crest a vertex is, for foam.)
inline float waveAmplitudeSum()
{
    float s = 0.0f;
    for (int i = 0; i < WaveConfig::COUNT; ++i)
        s += WaveConfig::TABLE[i].amplitude;
    return s;
}

// The height of the sea above its mean level at (x, z) at time t, for sea state `scale`.
inline float waveHeight(float x, float z, float t, float scale)
{
    float h = 0.0f;
    for (int i = 0; i < WaveConfig::COUNT; ++i) {
        const glm::vec2 d = waveDirection(i);
        h += WaveConfig::TABLE[i].amplitude * std::sin(waveNumber(i) * (d.x * x + d.y * z) - waveSpeedW(i) * t + WaveConfig::TABLE[i].phase);
    }
    return h * scale;
}

// The slope of the surface: (dh/dx, dh/dz). The surface normal is normalize(-dh/dx, 1, -dh/dz).
inline glm::vec2 waveSlope(float x, float z, float t, float scale)
{
    glm::vec2 g(0.0f);
    for (int i = 0; i < WaveConfig::COUNT; ++i) {
        const glm::vec2 d = waveDirection(i);
        const float k = waveNumber(i);
        const float c = WaveConfig::TABLE[i].amplitude * k * std::cos(k * (d.x * x + d.y * z) - waveSpeedW(i) * t + WaveConfig::TABLE[i].phase);
        g += d * c;
    }
    return g * scale;
}

// ---- the ship riding the sea -----------------------------------------------------------------------------------------------------

struct ShipRide {
    float heave = 0.0f;     // how far the waterline is above the mean sea level under the ship
    float roll = 0.0f;      // radians about the ship's long axis (ShipPose::roll's sign: positive lifts the ship's +x side)
    float pitch = 0.0f;     // radians about its cross axis (ShipPose::pitch's sign: positive LOWERS the bow)
};

// Where a ship of this heading, standing at (x, z), sits on the sea: the height of the water under its bow, stern and two sides, averaged for the
// heave, and the slope between bow and stern and between the sides for the pitch and roll. This is what makes the ship rise to a swell and lean into a
// trough - the wave under the hull sets the ship's rotation, as Phase 83 planned.
//
// The sample points are the ends of the hull (2.0 ahead and astern, a little inside the bow and stern) and a little outside the beam; a ship is
// longer than the ripples, so it spans them and only the long waves tilt it, as a real hull does.
inline ShipRide shipRideOnWaves(float x, float z, float heading, float t, float scale)
{
    constexpr float HALF_LENGTH = 2.0f;     // bow and stern sample points
    constexpr float HALF_BEAM = 0.6f;       // port and starboard sample points
    constexpr float MAX_TILT = 0.30f;       // radians: about 17 degrees, which is plenty even in a storm
    const glm::vec2 forward(std::sin(heading), std::cos(heading));
    const glm::vec2 side(std::cos(heading), -std::sin(heading));         // the ship's +x

    const float bow = waveHeight(x + forward.x * HALF_LENGTH, z + forward.y * HALF_LENGTH, t, scale);
    const float stern = waveHeight(x - forward.x * HALF_LENGTH, z - forward.y * HALF_LENGTH, t, scale);
    const float left = waveHeight(x + side.x * HALF_BEAM, z + side.y * HALF_BEAM, t, scale);
    const float right = waveHeight(x - side.x * HALF_BEAM, z - side.y * HALF_BEAM, t, scale);

    ShipRide r;
    r.heave = 0.25f * (bow + stern + left + right);
    r.roll = std::atan2(left - right, 2.0f * HALF_BEAM);
    r.pitch = -std::atan2(bow - stern, 2.0f * HALF_LENGTH);
    const auto clampTilt = [&](float a) { return a < -MAX_TILT ? -MAX_TILT : (a > MAX_TILT ? MAX_TILT : a); };
    r.roll = clampTilt(r.roll);
    r.pitch = clampTilt(r.pitch);
    return r;
}

// ---- the GLSL text, generated from the same table -----------------------------------------------------------------------------

// Spliced into BOTH shader stages after the shared lighting block (src/Lighting.h), exactly as that block is. The vertex shader uses
// seaWaveHeight() and seaWaveNormal(); the fragment shader only needs the uniforms.
inline const std::string& waveGlslSource()
{
    static const std::string source = [] {
        char line[256];
        std::string s;
        s += "\n// ===========================================================================\n";
        s += "// THE SEA'S WAVES - generated by src/Waves.h from the SAME table the CPU reads.\n";
        s += "// ===========================================================================\n";
        s += "uniform int uSeaMode;        // 1 while the sea is being drawn: its vertices are lifted by the waves and its normal comes from their slope\n";
        s += "uniform float uWaveTime;     // the game clock\n";
        s += "uniform float uWaveScale;    // the sea state S\n";
        s += "uniform float uFoam;         // 0..1: how much of the crests is white water\n";
        s += "uniform float uReflect;      // 0..1: how mirror-like the sea is (the planar reflection is blended in by the Fresnel term)\n";
        s += "float seaWaveHeight(vec2 p)\n{\n    float h = 0.0;\n";
        for (int i = 0; i < WaveConfig::COUNT; ++i) {
            const glm::vec2 d = waveDirection(i);
            std::snprintf(line, sizeof(line), "    h += %.7f * sin(%.7f * (%.7f * p.x + %.7f * p.y) - %.7f * uWaveTime + %.7f);\n",
                          WaveConfig::TABLE[i].amplitude, waveNumber(i), d.x, d.y, waveSpeedW(i), WaveConfig::TABLE[i].phase);
            s += line;
        }
        s += "    return h * uWaveScale;\n}\n";
        s += "vec3 seaWaveNormal(vec2 p)\n{\n    vec2 g = vec2(0.0);\n";
        for (int i = 0; i < WaveConfig::COUNT; ++i) {
            const glm::vec2 d = waveDirection(i);
            std::snprintf(line, sizeof(line), "    g += vec2(%.7f, %.7f) * (%.7f * cos(%.7f * (%.7f * p.x + %.7f * p.y) - %.7f * uWaveTime + %.7f));\n",
                          d.x, d.y, WaveConfig::TABLE[i].amplitude * waveNumber(i), waveNumber(i), d.x, d.y, waveSpeedW(i), WaveConfig::TABLE[i].phase);
            s += line;
        }
        s += "    g *= uWaveScale;\n    return normalize(vec3(-g.x, 1.0, -g.y));\n}\n";
        std::snprintf(line, sizeof(line), "const float SEA_AMPLITUDE_SUM = %.7f;\n", waveAmplitudeSum());
        s += line;
        s += "// ============================ end of the sea's waves ==========================\n";
        return s;
    }();
    return source;
}
