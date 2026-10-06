#pragma once

// Environment build: WILDLIFE - birds in the air, fish in the sea, dolphins jumping. No OpenGL here (like Crew.h and Waves.h): where each animal is and how it holds its
// body, as closed forms of the game clock, plus the little state that lets them react to the world.
//
// EVERYTHING FOLLOWS A PATH THAT IS A FORMULA OF TIME. A gull is on a circle round a centre at a height; a school of fish drifts on a slow ellipse and each fish wanders round
// the school s middle on its own Lissajous curve; a dolphin circles the ship and, once every cycle, leaves the water along a parabola. No path is stored or recorded, so the
// animals are continuous at any frame rate and cost almost nothing to move. The same functions give the heading (from the path s own slope) and the banking, the tail s wag and
// the wings  beat.
//
// REACTIONS ARE OFFSETS THAT DIE AWAY. When a cannon fires or a ball lands, startle() gives every animal near it a push away from the noise: an OFFSET added to its path
// position, which then decays (exponentially) back to zero. So a flock scatters outward from a broadside, flaps hard while it does, and drifts home to its circle a few seconds
// later; fish and dolphins dart away under water and return the same way. Nothing else in the path changes.

#include "Particles.h"
#include "Waves.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace WildlifeConfig {

constexpr int BIRDS = 28;
constexpr int SCHOOLS = 5;
constexpr int FISH_PER_SCHOOL = 9;
constexpr int DOLPHINS = 3;

constexpr float BIRD_STARTLE_RANGE = 70.0f;      // a gun this near scatters the gulls
constexpr float FISH_STARTLE_RANGE = 32.0f;
constexpr float DOLPHIN_STARTLE_RANGE = 45.0f;
constexpr float RETURN_RATE = 0.45f;             // per second: how quickly an offset dies away (about 2 s to lose 60% of it)

} // namespace WildlifeConfig

struct Wildlife {
    glm::vec3 birdOffset[WildlifeConfig::BIRDS] = {};
    float birdPanic[WildlifeConfig::BIRDS] = {};                   // > 0 while a bird is frightened: it flaps hard and flies away from the noise
    glm::vec3 birdAway[WildlifeConfig::BIRDS] = {};
    glm::vec3 schoolOffset[WildlifeConfig::SCHOOLS] = {};
    glm::vec3 dolphinOffset[WildlifeConfig::DOLPHINS] = {};
    glm::vec3 dolphinVelocity[WildlifeConfig::DOLPHINS] = {};       // startle impulse integrated over time: no one-frame teleport
    float dolphinPanic[WildlifeConfig::DOLPHINS] = {};
    float dolphinPrevU[WildlifeConfig::DOLPHINS] = { -1.0f, -1.0f, -1.0f };      // the last frame s place in the jump cycle, to notice take-off and splash-down
    glm::vec3 centreBird[3] = {};                                  // the three flocks  circle centres of the last frame (for the splash events and the drawing)
};

// ---- the paths --------------------------------------------------------------------------------------------------------------------

// What the animals circle round: the ship, the nearest islands, and the sky high above.
struct WildlifeAnchors {
    glm::vec3 ship = glm::vec3(0.0f);          // the player s ship, at the waterline
    float shipHeading = 0.0f;
    glm::vec3 island[3] = {};                  // up to three islands (their centres), nearest first
    float islandRadius[3] = {};
    int islands = 0;
    float seaY = 0.0f;
    float waveAmp = 0.0f;                      // the sea's wave scale, so a dolphin rides the real swell
};

struct BirdPose {
    glm::vec3 pos = glm::vec3(0.0f);
    float heading = 0.0f;                      // radians about y, 0 = +z
    float bank = 0.0f;                         // roll into the turn
    float pitch = 0.0f;
    float flap = 0.0f;                         // wing angle: -1 .. 1
    float size = 1.0f;
    int type = 0;                              // 0 gull, 1 tern
    bool visible = true;
};

// The bird s path BEFORE any scatter. Birds 0-9 circle the ship at 6 to 22 units and 5 to 15 high, 10-19 circle an island (or, when there is none, wheel high over the sea),
// 20-27 sweep a lazy figure of eight 35 to 60 units up. Directions of travel alternate; speeds differ; every bird has its own phase.
inline glm::vec3 birdPathPosition(int i, float t, const WildlifeAnchors& a)
{
    const float fi = static_cast<float>(i);
    const float dir = (i % 2 == 0) ? 1.0f : -1.0f;
    if (i < 10) {
        const float R = 6.0f + 16.0f * std::fabs(std::sin(fi * 2.31f + 0.4f)), H = 5.0f + 10.0f * std::fabs(std::sin(fi * 1.7f + 1.1f));
        const float w = dir * (0.20f + 0.05f * std::fmod(fi * 0.37f, 1.0f)), ang = fi * 1.1f + w * t;
        return glm::vec3(a.ship.x + R * std::cos(ang), a.seaY + H + 0.9f * std::sin(0.9f * t + fi), a.ship.z + R * std::sin(ang));
    }
    if (i < 20) {
        glm::vec3 centre = a.ship + glm::vec3(0.0f, 0.0f, 25.0f);
        float R = 14.0f;
        if (a.islands > 0) {
            const int k = (i - 10) % a.islands;
            centre = a.island[k];
            R = a.islandRadius[k] * 0.8f + 3.0f + 5.0f * std::fabs(std::sin(fi));
        }
        const float H = 4.0f + 9.0f * std::fabs(std::sin(fi * 1.3f + 0.2f));
        const float w = dir * (0.16f + 0.05f * std::fmod(fi * 0.41f, 1.0f)), ang = fi * 0.9f + w * t;
        return glm::vec3(centre.x + R * std::cos(ang), a.seaY + H + 0.7f * std::sin(0.8f * t + fi), centre.z + R * std::sin(ang));
    }
    const float R = 38.0f + 12.0f * std::fabs(std::sin(fi * 1.9f)), H = 35.0f + 22.0f * std::fabs(std::sin(fi * 2.7f));
    const float ang = fi * 0.8f + dir * 0.07f * t;
    return glm::vec3(a.ship.x * 0.9f + R * std::sin(ang), a.seaY + H + 2.0f * std::sin(0.4f * t + fi), a.ship.z * 0.9f + 0.5f * R * std::sin(2.0f * ang));
}

inline BirdPose birdPoseOf(int i, float t, const WildlifeAnchors& a, const Wildlife& w)
{
    BirdPose b;
    const glm::vec3 p0 = birdPathPosition(i, t, a), p1 = birdPathPosition(i, t + 0.1f, a), p2 = birdPathPosition(i, t - 0.1f, a);
    b.pos = p0 + w.birdOffset[i];
    glm::vec3 v = (p1 - p0) * 10.0f;
    glm::vec3 turn = (p1 - 2.0f * p0 + p2) * 100.0f;
    float heading = std::atan2(v.x, v.z);
    const float panic = std::clamp(w.birdPanic[i], 0.0f, 1.0f);
    if (panic > 0.05f) {                                        // a frightened bird turns and flees along its push, not along its circle
        const glm::vec3 away = w.birdAway[i];
        heading = std::atan2(glm::mix(v.x, away.x * 6.0f, panic), glm::mix(v.z, away.z * 6.0f, panic));
        v.y += away.y * 3.0f * panic;
    }
    b.heading = heading;
    b.bank = std::clamp(-(turn.x * std::cos(heading) - turn.z * std::sin(heading)) * 0.07f, -0.7f, 0.7f);
    b.pitch = std::clamp(std::atan2(v.y, std::sqrt(v.x * v.x + v.z * v.z) + 1e-3f), -0.5f, 0.5f);
    const float rate = (5.0f + 0.7f * std::fmod(static_cast<float>(i) * 1.7f, 3.0f)) * (1.0f + 1.4f * panic);
    // Gulls beat, then glide: a beat of 1.4 s of wing then 0.8 s of glide, unless frightened.
    const float cycle = std::fmod(t * 0.45f + static_cast<float>(i) * 0.37f, 1.0f);
    const float gliding = (panic > 0.1f) ? 0.0f : (cycle > 0.62f ? 1.0f : 0.0f);
    b.flap = std::sin(t * rate + static_cast<float>(i)) * (1.0f - 0.85f * gliding);
    b.size = (i < 20) ? 1.0f : 1.5f;
    b.type = (i % 3 == 0) ? 1 : 0;
    return b;
}

struct SchoolInfo {
    glm::vec3 centre = glm::vec3(0.0f);
    bool near = false;
};

// A school s centre: schools 0-2 swim round the ship at 6 to 20 units, schools 3-4 patrol the sea round the nearest island (or further out). Depth 2.5 to 7 below the surface.
inline glm::vec3 schoolCentre(int s, float t, const WildlifeAnchors& a)
{
    const float fs = static_cast<float>(s);
    const float depth = 2.5f + 4.5f * std::fabs(std::sin(fs * 1.3f + 0.6f));
    if (s < 3) {
        const float R = 7.0f + 6.0f * fs, ang = fs * 2.1f + (s % 2 ? -1.0f : 1.0f) * (0.10f + 0.03f * fs) * t;
        return glm::vec3(a.ship.x + R * std::cos(ang), a.seaY - depth, a.ship.z + R * std::sin(ang));
    }
    glm::vec3 c = a.ship + glm::vec3(-20.0f + 40.0f * (s - 3), 0.0f, 22.0f);
    float R = 9.0f;
    if (a.islands > 0) {
        c = a.island[(s - 3) % a.islands];
        R = a.islandRadius[(s - 3) % a.islands] + 4.0f;
    }
    const float ang = fs * 1.7f + 0.09f * t;
    return glm::vec3(c.x + R * std::cos(ang), a.seaY - depth, c.z + R * std::sin(1.3f * ang) * 0.8f);
}

struct FishPose {
    glm::vec3 pos = glm::vec3(0.0f);
    float heading = 0.0f;
    float pitch = 0.0f;
    float wag = 0.0f;                        // tail angle, -1..1
    float size = 1.0f;
    int type = 0;                            // 0 silver, 1 orange, 2 blue
};

inline glm::vec3 fishPath(int s, int j, float t, const WildlifeAnchors& a)
{
    const float fj = static_cast<float>(j), ph = fj * 1.37f + static_cast<float>(s) * 0.9f;
    const glm::vec3 c = schoolCentre(s, t, a);
    // Each fish wanders round the school s middle on its own Lissajous curve, 1 to 2.2 units out; the school as a whole keeps together because they share the centre.
    const float r = 1.0f + 1.2f * std::fabs(std::sin(fj * 2.3f + static_cast<float>(s)));
    return c + glm::vec3(r * std::sin(0.55f * t + ph), 0.35f * std::sin(0.8f * t + 2.0f * ph), r * std::cos(0.43f * t + 1.7f * ph));
}

inline FishPose fishPoseOf(int s, int j, float t, const WildlifeAnchors& a, const Wildlife& w)
{
    FishPose f;
    const glm::vec3 p0 = fishPath(s, j, t, a), p1 = fishPath(s, j, t + 0.1f, a);
    const glm::vec3 v = (p1 - p0) * 10.0f;
    f.pos = p0 + w.schoolOffset[s];
    f.heading = std::atan2(v.x, v.z);
    f.pitch = std::atan2(v.y, std::sqrt(v.x * v.x + v.z * v.z) + 1e-3f);
    f.wag = std::sin(t * 9.0f + static_cast<float>(j) * 1.9f);
    f.size = (0.8f + 0.5f * std::fabs(std::sin(static_cast<float>(j * 3 + s)))) * 1.6f;
    f.type = (s + j) % 3;
    return f;
}

struct DolphinPose {
    glm::vec3 pos = glm::vec3(0.0f);
    float heading = 0.0f;
    float pitch = 0.0f;
    float wag = 0.0f;                        // the tail s up-and-down beat
    float u = 0.0f;                          // 0..1 through the jump cycle; < JUMP_FRACTION means airborne
    bool airborne = false;
};

constexpr float DOLPHIN_JUMP_FRACTION = 0.22f;

// Dolphin d swims a circle round a point ahead of the ship, going in the same sense as the ship, at 12 to 20 units out. Every cycle (7 to 11 s) it leaves the water along an arc
// 2.4 units high, and the rest of the time it rolls through the swell with only its back showing.
inline DolphinPose dolphinPoseOf(int d, float t, const WildlifeAnchors& a, const Wildlife& w)
{
    const float fd = static_cast<float>(d);
    const glm::vec3 fwd(std::sin(a.shipHeading), 0.0f, std::cos(a.shipHeading));
    const glm::vec3 centre = a.ship + fwd * 9.0f;
    const float R = 12.0f + 4.0f * fd;
    const float omega = 0.22f - 0.03f * fd;
    const float period = 7.0f + 2.0f * fd;
    const auto cycleAt = [&](float tt) {
        const float cycle = tt / period + fd * 0.31f;
        return cycle - std::floor(cycle);                         // unlike fmod, remains in 0..1 for a small negative sample time
    };
    // A sixth-order bell has zero velocity and acceleration at both ends.  The old
    // airborne sine met an unrelated underwater sine at the waterline, so position
    // and pitch snapped twice per cycle.  These two bells meet at exactly the same
    // height with the same slope, including where the cycle wraps from 1 back to 0.
    const auto bell = [](float x) {
        const float a = std::clamp(x, 0.0f, 1.0f), b = a * (1.0f - a);
        return 64.0f * b * b * b;                                // 0 at ends, 1 in the middle
    };
    const auto pathAt = [&](float tt) {
        const float ang = fd * 2.1f + omega * tt;
        const float x = centre.x + R * std::cos(ang), z = centre.z + R * std::sin(ang);
        const float u = cycleAt(tt);
        const float vertical = u < DOLPHIN_JUMP_FRACTION
            ? 2.35f * bell(u / DOLPHIN_JUMP_FRACTION)
            : -0.48f * bell((u - DOLPHIN_JUMP_FRACTION) / (1.0f - DOLPHIN_JUMP_FRACTION));
        const float surface = waterSurfaceY(WaterContext{a.seaY, a.waveAmp, tt}, x, z);
        return glm::vec3(x, surface - 0.20f + vertical, z);
    };

    DolphinPose p;
    p.u = cycleAt(t);
    p.airborne = p.u < DOLPHIN_JUMP_FRACTION;
    const float sample = 0.035f;
    const glm::vec3 p0 = pathAt(t), before = pathAt(t - sample), after = pathAt(t + sample);
    const glm::vec3 v = (after - before) / (2.0f * sample);           // tangent of the actual 3-D path, including the moving waves
    p.pos = p0 + w.dolphinOffset[d];
    p.heading = std::atan2(v.x, v.z);
    p.pitch = std::clamp(std::atan2(v.y, std::sqrt(v.x*v.x + v.z*v.z) + 1e-3f), -1.05f, 1.05f);
    const float panic = std::clamp(w.dolphinPanic[d], 0.0f, 1.0f);
    const float jumpShape = p.airborne ? bell(p.u / DOLPHIN_JUMP_FRACTION) : 0.0f;
    const int beats = 7 + d;                                        // integer beats make the tail phase continuous at u 1 -> 0
    p.wag = std::sin(6.2831853f * (static_cast<float>(beats) * p.u + 0.13f * fd) + 0.35f * panic)
          * (1.0f - 0.35f * jumpShape);                             // less frantic in the air, but never a switched frequency
    return p;
}

// ---- reactions ---------------------------------------------------------------------------------------------------------------------

// Something loud happened at `at` (a cannon fired, a ball landed, a hull was struck). `strength` 1 is a gun; a ball landing is 0.6. Animals within range are pushed away, the
// nearer the harder; birds are also frightened (they flap hard and flee along the push until it has died away).
inline void startle(Wildlife& w, const glm::vec3& at, float strength, const WildlifeAnchors& a, float t)
{
    using namespace WildlifeConfig;
    for (int i = 0; i < BIRDS; ++i) {
        const glm::vec3 p = birdPathPosition(i, t, a) + w.birdOffset[i];
        glm::vec3 away = p - at;
        const float d = glm::length(away);
        if (d > BIRD_STARTLE_RANGE)
            continue;
        away = (d > 1e-3f) ? away / d : glm::vec3(0.0f, 1.0f, 0.0f);
        const float k = strength * (1.0f - d / BIRD_STARTLE_RANGE);
        away.y = std::max(away.y, 0.0f) + 0.5f;                      // up as well as away
        w.birdOffset[i] += glm::normalize(away) * (14.0f * k);
        w.birdAway[i] = glm::normalize(away);
        w.birdPanic[i] = std::min(1.5f, w.birdPanic[i] + 1.2f * k + 0.2f);
    }
    for (int s = 0; s < SCHOOLS; ++s) {
        glm::vec3 away = schoolCentre(s, t, a) + w.schoolOffset[s] - at;
        const float d = glm::length(away);
        if (d > FISH_STARTLE_RANGE || d < 1e-3f)
            continue;
        away.y = 0.0f;
        const float k = strength * (1.0f - d / FISH_STARTLE_RANGE);
        if (glm::dot(away, away) > 1e-6f)
            w.schoolOffset[s] += glm::normalize(away) * (9.0f * k);
    }
    for (int d = 0; d < DOLPHINS; ++d) {
        const DolphinPose p = dolphinPoseOf(d, t, a, w);
        glm::vec3 away = p.pos - at;
        const float dist = glm::length(away);
        if (dist > DOLPHIN_STARTLE_RANGE || dist < 1e-3f)
            continue;
        away.y = 0.0f;
        const float k = strength * (1.0f - dist / DOLPHIN_STARTLE_RANGE);
        if (glm::dot(away, away) > 1e-6f)
            w.dolphinVelocity[d] += glm::normalize(away) * (7.0f * k);
        w.dolphinPanic[d] = std::min(1.5f, w.dolphinPanic[d] + k + 0.1f);
    }
}

// Lets the reactions die away (delta time, so the same at any frame rate).
inline void updateWildlife(Wildlife& w, float dt)
{
    using namespace WildlifeConfig;
    if (dt <= 0.0f)
        return;
    const float keep = std::exp(-RETURN_RATE * dt);
    for (int i = 0; i < BIRDS; ++i) { w.birdOffset[i] *= keep; w.birdPanic[i] = std::max(0.0f, w.birdPanic[i] - 0.45f * dt); }
    for (int s = 0; s < SCHOOLS; ++s) w.schoolOffset[s] *= keep;
    const float dolphinDrag = std::exp(-2.2f * dt);
    for (int d = 0; d < DOLPHINS; ++d) {
        w.dolphinOffset[d] += w.dolphinVelocity[d] * dt;
        w.dolphinVelocity[d] *= dolphinDrag;
        w.dolphinOffset[d] *= keep;
        w.dolphinPanic[d] = std::max(0.0f, w.dolphinPanic[d] - 0.4f * dt);
    }
}
