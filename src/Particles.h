#pragma once

// Environment build: PARTICLES - muzzle flash and smoke, water splashes, wakes, impacts, bubbles. No OpenGL here (like Crew.h and Waves.h): a pool of
// particles, the rules that move them, and the emitters that create them. main.cpp turns the live particles into camera-facing quads and draws them with the
// effects shader (mode 4), in two batches: ADDITIVE (flash, sparks, light rays: they add light) and ALPHA (smoke, spray, foam, bubbles: they cover).
//
// EVERYTHING IS A CLOSED FORM OF AGE. A particle stores where it started, how fast, and how old it is; its colour and size are a blend from `c0` to `c1`
// and `size0` to `size1` by age / life. Nothing is recorded per frame, nothing is pre-computed, and all motion uses the frame's delta time, so an effect looks
// the same at 30 and 144 frames a second. A paused game has dt = 0 and everything freezes.
//
// BOUNDED. The pool has a fixed capacity (ParticleConfig::CAPACITY). Particles are removed the moment they expire (swap with the last, O(1)); when the pool
// is full new ones are simply not made, so a long fight can never grow memory or slow the frame.
//
// Reusable systems named in the brief: ParticleSystem (the pool), CannonSystem (muzzle blast, ball trail, ship and land impact), WaterSplash (a ball
// hitting the sea), ShipWake (foam and spray from a moving hull), UnderwaterRenderer (the camera's depth, bubbles, motes, light rays).

#include "Waves.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ParticleConfig {
constexpr int CAPACITY = 3200;           // live particles at once
constexpr float WATER_LIFT = 0.03f;      // foam floats this far above the surface so it never z-fights with the sea
}

enum class PKind : std::uint8_t { PUFF = 0, RING = 1, SPARK = 2, FOAM = 3, RAY = 4, BUBBLE = 5 };

// What a particle does about the sea.
enum class PWater : std::uint8_t {
    NONE,          // free flight
    STICK,         // rides the surface (foam, rings)
    DIE_ABOVE,     // removed when it rises through the surface (a bubble)
    DIE_BELOW      // removed when it falls into the sea (a droplet)
};

struct Particle {
    glm::vec3 pos = glm::vec3(0.0f), vel = glm::vec3(0.0f);
    glm::vec4 c0 = glm::vec4(1.0f), c1 = glm::vec4(1.0f);       // colour and alpha at birth and at death
    float size0 = 0.1f, size1 = 0.1f;                           // radius, world units
    float age = 0.0f, life = 1.0f;                              // age < 0 is a delay: not yet born
    float gravity = 0.0f, drag = 0.0f;
    PKind kind = PKind::PUFF;
    PWater water = PWater::NONE;
    bool additive = false;
};

// The sea, as the particles need to know it.
struct WaterContext {
    float seaY = 0.0f;
    float waveAmp = 0.45f;
    float time = 0.0f;
};

inline float waterSurfaceY(const WaterContext& w, float x, float z)
{
    return w.seaY + waveHeight(x, z, w.time, w.waveAmp);
}

struct ParticleSystem {
    std::vector<Particle> live;
    std::uint32_t rng = 2463534242u;

    // The light that falls on water and spray: what colours splashes, foam and bubbles. White water is not white, it is whatever colour the light on it is - gold at
    // sunset, bright white-blue at noon, grey under a storm, dim blue by moonlight. main.cpp sets it every frame from the atmosphere (ambient + sun), so the spray always
    // matches the weather and the time of day.
    glm::vec3 waterTint = glm::vec3(1.0f);

    // The wind, as a velocity in the world (set every frame by main.cpp from the weather). Smoke - a pale or dark puff that rises (negative gravity) without glowing - takes it on:
    // its sideways velocity eases towards the wind's, so a column leans over, drifts downwind as it widens and fades, and a gale tears it flat.
    glm::vec3 wind = glm::vec3(0.0f);
    glm::vec4 water(float r, float g, float b, float a) const { return glm::vec4(glm::vec3(r, g, b) * waterTint, a); }

    ParticleSystem() { live.reserve(ParticleConfig::CAPACITY); }

    float rand01()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return static_cast<float>(rng & 0xFFFFFFu) / 16777216.0f;
    }
    float range(float a, float b) { return a + (b - a) * rand01(); }
    glm::vec3 unitSphere()
    {
        const float z = range(-1.0f, 1.0f), a = range(0.0f, 6.2831853f), r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        return glm::vec3(r * std::cos(a), z, r * std::sin(a));
    }

    bool full() const { return static_cast<int>(live.size()) >= ParticleConfig::CAPACITY; }
    bool add(const Particle& p)
    {
        if (full())
            return false;
        live.push_back(p);
        return true;
    }
    void clear() { live.clear(); }
};

// Advances every particle by dt and removes the expired ones. (The brief's updateParticles().)
inline void updateParticles(ParticleSystem& ps, float dt, const WaterContext& water)
{
    if (dt <= 0.0f)
        return;
    std::vector<Particle>& v = ps.live;
    for (std::size_t i = 0; i < v.size();) {
        Particle& p = v[i];
        p.age += dt;
        bool dead = p.age >= p.life;
        if (!dead && p.age > 0.0f) {
            p.vel.y -= p.gravity * dt;
            if (p.kind == PKind::PUFF && !p.additive && p.gravity < 0.0f) {
                const float k = 1.0f - std::exp(-0.7f * dt);
                p.vel.x += (ps.wind.x - p.vel.x) * k;
                p.vel.z += (ps.wind.z - p.vel.z) * k;
            }
            if (p.drag > 0.0f)
                p.vel *= std::exp(-p.drag * dt);
            p.pos += p.vel * dt;
            switch (p.water) {
            case PWater::STICK:
                p.pos.y = waterSurfaceY(water, p.pos.x, p.pos.z) + ParticleConfig::WATER_LIFT;
                break;
            case PWater::DIE_ABOVE:
                dead = p.pos.y > waterSurfaceY(water, p.pos.x, p.pos.z);
                break;
            case PWater::DIE_BELOW:
                dead = p.vel.y < 0.0f && p.pos.y < waterSurfaceY(water, p.pos.x, p.pos.z);
                break;
            default: break;
            }
        }
        if (dead) {
            p = v.back();
            v.pop_back();
        } else {
            ++i;
        }
    }
}

// ---- shared helpers for the emitters ---------------------------------------------------------------------------------------------

inline Particle makeParticle(PKind kind, const glm::vec3& pos, const glm::vec3& vel, float life, float size0, float size1,
                             const glm::vec4& c0, const glm::vec4& c1, bool additive, float gravity = 0.0f, float drag = 0.0f, PWater water = PWater::NONE)
{
    Particle p;
    p.kind = kind; p.pos = pos; p.vel = vel; p.life = life; p.size0 = size0; p.size1 = size1;
    p.c0 = c0; p.c1 = c1; p.additive = additive; p.gravity = gravity; p.drag = drag; p.water = water;
    return p;
}

// ---- CannonSystem -----------------------------------------------------------------------------------------------------------------

struct CannonSystem {
    // The muzzle blast: a flash that lasts a tenth of a second, a fireball, sparks thrown out along the barrel, and smoke that billows, drifts and thins over
    // several seconds. `light` (0..1) is how bright the scene is, so smoke is pale grey in daylight and nearly black at night.
    static void emitMuzzleBlast(ParticleSystem& ps, const glm::vec3& muzzle, const glm::vec3& dir, float light)
    {
        const glm::vec3 d = glm::normalize(dir);
        const float L = 0.25f + 0.75f * light;
        ps.add(makeParticle(PKind::PUFF, muzzle + d * 0.3f, d * 1.0f, 0.12f, 0.8f, 1.7f, glm::vec4(1.0f, 0.95f, 0.75f, 1.0f), glm::vec4(1.0f, 0.7f, 0.3f, 0.0f), true));
        ps.add(makeParticle(PKind::PUFF, muzzle + d * 0.1f, d * 0.5f, 0.2f, 0.55f, 1.2f, glm::vec4(1.0f, 0.6f, 0.2f, 0.9f), glm::vec4(0.8f, 0.2f, 0.05f, 0.0f), true));
        for (int i = 0; i < 6; ++i)                                                   // the fireball
            ps.add(makeParticle(PKind::PUFF, muzzle + d * ps.range(0.1f, 0.6f), d * ps.range(2.0f, 5.0f) + ps.unitSphere() * 0.8f, ps.range(0.18f, 0.32f),
                                0.3f, 0.75f, glm::vec4(1.0f, 0.7f, 0.25f, 0.8f), glm::vec4(0.9f, 0.25f, 0.05f, 0.0f), true, 0.0f, 3.0f));
        for (int i = 0; i < 16; ++i)                                                  // sparks
            ps.add(makeParticle(PKind::SPARK, muzzle + d * 0.2f, d * ps.range(4.0f, 10.0f) + ps.unitSphere() * 2.2f, ps.range(0.45f, 1.0f), 0.06f, 0.03f,
                                glm::vec4(1.0f, 0.85f, 0.45f, 1.0f), glm::vec4(1.0f, 0.35f, 0.05f, 0.0f), true, 7.0f, 0.6f));
        for (int i = 0; i < 14; ++i) {                                                // smoke: starts fast and thick at the muzzle, slows, spreads and fades
            const float shade = ps.range(0.55f, 0.85f) * L;
            ps.add(makeParticle(PKind::PUFF, muzzle + d * ps.range(0.2f, 0.9f), d * ps.range(1.0f, 3.6f) + glm::vec3(0.0f, ps.range(0.2f, 0.8f), 0.0f) + ps.unitSphere() * 0.5f,
                                ps.range(3.0f, 5.5f), ps.range(0.35f, 0.55f), ps.range(2.0f, 3.4f),
                                glm::vec4(shade, shade, shade * 1.02f, 0.6f), glm::vec4(shade * 0.8f, shade * 0.8f, shade * 0.82f, 0.0f), false, -0.12f, 1.1f));
        }
        for (int i = 0; i < 4; ++i)                                                   // wisps left hanging on the ship's side
            ps.add(makeParticle(PKind::PUFF, muzzle - d * ps.range(0.0f, 0.6f), ps.unitSphere() * 0.25f + glm::vec3(0.0f, 0.25f, 0.0f), ps.range(4.0f, 6.0f), 0.3f, 1.8f,
                                glm::vec4(0.7f * L, 0.7f * L, 0.7f * L, 0.35f), glm::vec4(0.6f * L, 0.6f * L, 0.6f * L, 0.0f), false, -0.05f, 0.6f));
    }

    // One puff of the thin smoke a cannonball leaves along its flight, so the whole arc can be followed by eye.
    static void emitBallTrail(ParticleSystem& ps, const glm::vec3& at, float light)
    {
        const float s = (0.5f + 0.4f * ps.rand01()) * (0.3f + 0.7f * light);
        ps.add(makeParticle(PKind::PUFF, at, ps.unitSphere() * 0.15f, 0.9f, 0.10f, 0.42f, glm::vec4(s, s, s, 0.38f), glm::vec4(s, s, s, 0.0f), false, 0.0f, 1.5f));
    }

    // A ball striking a ship. `nearWater` (the hit is within a metre or so of the sea) adds a splash of water to the wood: sparks and splinters fly,
    // dark smoke rises, and a flash of light goes off. `normalHint` is roughly the way out of the hull (the direction the ball came from, reversed).
    static void emitShipImpact(ParticleSystem& ps, const glm::vec3& at, const glm::vec3& normalHint, bool nearWater, float light)
    {
        const glm::vec3 n = glm::length(normalHint) > 1e-4f ? glm::normalize(normalHint) : glm::vec3(0.0f, 1.0f, 0.0f);
        ps.add(makeParticle(PKind::PUFF, at, glm::vec3(0.0f), 0.14f, 0.45f, 1.1f, glm::vec4(1.0f, 0.85f, 0.5f, 1.0f), glm::vec4(1.0f, 0.4f, 0.1f, 0.0f), true));
        for (int i = 0; i < 20; ++i)                                                  // sparks
            ps.add(makeParticle(PKind::SPARK, at, n * ps.range(1.5f, 5.0f) + ps.unitSphere() * 3.0f + glm::vec3(0.0f, 1.0f, 0.0f), ps.range(0.4f, 0.9f), 0.06f, 0.03f,
                                glm::vec4(1.0f, 0.8f, 0.4f, 1.0f), glm::vec4(1.0f, 0.3f, 0.05f, 0.0f), true, 9.0f, 0.4f));
        for (int i = 0; i < 16; ++i) {                                                // splinters and wood chips, pale and dark
            const float t = ps.range(0.0f, 1.0f);
            const glm::vec3 wood = glm::mix(glm::vec3(0.28f, 0.17f, 0.08f), glm::vec3(0.62f, 0.45f, 0.24f), t) * (0.45f + 0.55f * light);
            ps.add(makeParticle(PKind::SPARK, at, n * ps.range(1.0f, 4.0f) + ps.unitSphere() * 2.6f + glm::vec3(0.0f, 1.6f, 0.0f), ps.range(1.0f, 1.8f),
                                ps.range(0.07f, 0.15f), ps.range(0.07f, 0.15f), glm::vec4(wood, 1.0f), glm::vec4(wood, 0.0f), false, 9.8f, 0.2f));
        }
        for (int i = 0; i < 7; ++i) {                                                 // smoke
            const float g = ps.range(0.18f, 0.4f) * (0.3f + 0.7f * light);
            ps.add(makeParticle(PKind::PUFF, at + ps.unitSphere() * 0.2f, n * 0.6f + glm::vec3(0.0f, ps.range(0.5f, 1.4f), 0.0f) + ps.unitSphere() * 0.4f, ps.range(2.0f, 3.6f),
                                0.3f, ps.range(1.5f, 2.4f), glm::vec4(g, g, g, 0.65f), glm::vec4(g, g, g, 0.0f), false, -0.1f, 0.9f));
        }
        if (nearWater) {                                                              // the ball went in at the waterline: water as well as wood
            for (int i = 0; i < 14; ++i)
                ps.add(makeParticle(PKind::SPARK, at, glm::vec3(ps.range(-1.4f, 1.4f), ps.range(2.5f, 5.5f), ps.range(-1.4f, 1.4f)) + n * 1.2f, ps.range(0.9f, 1.4f), 0.08f, 0.05f,
                                    ps.water(0.85f, 0.93f, 1.0f, 0.9f), ps.water(0.85f, 0.93f, 1.0f, 0.0f), false, 9.8f, 0.2f, PWater::DIE_BELOW));
            for (int i = 0; i < 4; ++i)
                ps.add(makeParticle(PKind::PUFF, at, n * 0.5f + glm::vec3(0.0f, ps.range(0.6f, 1.4f), 0.0f), ps.range(1.0f, 1.8f), 0.35f, 1.7f,
                                    ps.water(0.92f, 0.96f, 1.0f, 0.4f), ps.water(0.92f, 0.96f, 1.0f, 0.0f), false, 0.0f, 1.2f));
        }
    }

    // A ball striking land: a puff of dust and a spray of stones.
    static void emitLandImpact(ParticleSystem& ps, const glm::vec3& at, float light)
    {
        for (int i = 0; i < 8; ++i) {
            const float g = ps.range(0.3f, 0.5f) * (0.3f + 0.7f * light);
            ps.add(makeParticle(PKind::PUFF, at + ps.unitSphere() * 0.3f, glm::vec3(ps.range(-1.0f, 1.0f), ps.range(0.5f, 2.0f), ps.range(-1.0f, 1.0f)), ps.range(1.8f, 3.0f),
                                0.4f, ps.range(1.6f, 2.8f), glm::vec4(g * 1.1f, g, g * 0.85f, 0.7f), glm::vec4(g, g, g * 0.85f, 0.0f), false, -0.05f, 1.0f));
        }
        for (int i = 0; i < 12; ++i)
            ps.add(makeParticle(PKind::SPARK, at, glm::vec3(ps.range(-2.5f, 2.5f), ps.range(2.0f, 6.0f), ps.range(-2.5f, 2.5f)), ps.range(0.8f, 1.5f), 0.1f, 0.1f,
                                glm::vec4(0.35f, 0.32f, 0.28f, 1.0f), glm::vec4(0.35f, 0.32f, 0.28f, 0.0f), false, 9.8f, 0.2f));
    }
};

// ---- WaterSplash --------------------------------------------------------------------------------------------------------------------

struct WaterSplash {
    // A body striking the sea at `at` (x, z used; the height is the surface's) with `power` (0.3 a pebble, 1 a cannonball, 1.6 a big impact): ripples that
    // spread out and fade (two or three, one after another), a crown of droplets thrown up that fall back and vanish into the sea, a column of mist, and a
    // patch of foam that lingers on the surface.
    static void emit(ParticleSystem& ps, const WaterContext& water, const glm::vec3& at, float power, float light)
    {
        const float y = waterSurfaceY(water, at.x, at.z) + ParticleConfig::WATER_LIFT;
        const glm::vec3 base(at.x, y, at.z);
        for (int i = 0; i < 3; ++i) {
            Particle ring = makeParticle(PKind::RING, base, glm::vec3(0.0f), 1.5f + 0.4f * static_cast<float>(i), 0.15f, (1.5f + 0.9f * static_cast<float>(i)) * power + 0.6f,
                                         ps.water(0.92f, 0.97f, 1.0f, 0.85f - 0.2f * static_cast<float>(i)), ps.water(0.92f, 0.97f, 1.0f, 0.0f), false, 0.0f, 0.0f, PWater::STICK);
            ring.age = -0.18f * static_cast<float>(i);
            ps.add(ring);
        }
        const int drops = static_cast<int>(10.0f + 16.0f * power);
        for (int i = 0; i < drops; ++i) {
            const float a = ps.range(0.0f, 6.2831853f), out = ps.range(0.2f, 2.0f) * power;
            const float s = ps.range(0.07f, 0.16f);
            ps.add(makeParticle(PKind::SPARK, base + glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(std::cos(a) * out, ps.range(3.5f, 8.5f) * (0.5f + 0.5f * power), std::sin(a) * out), 2.5f, s, s * 0.8f,
                                ps.water(0.88f, 0.95f, 1.0f, 0.9f), ps.water(0.88f, 0.95f, 1.0f, 0.5f), false, 9.8f, 0.1f, PWater::DIE_BELOW));
        }
        const float v = 0.55f + 0.45f * light;
        for (int i = 0; i < 7; ++i)
            ps.add(makeParticle(PKind::PUFF, base + glm::vec3(ps.range(-0.3f, 0.3f), 0.2f, ps.range(-0.3f, 0.3f)), glm::vec3(ps.range(-0.4f, 0.4f), ps.range(1.0f, 2.6f) * power, ps.range(-0.4f, 0.4f)),
                                ps.range(1.2f, 2.0f), 0.3f, ps.range(1.2f, 2.0f) * power + 0.5f, ps.water(0.9f * v, 0.95f * v, v, 0.42f), ps.water(0.9f * v, 0.95f * v, v, 0.0f), false, -0.2f, 1.4f));
        ps.add(makeParticle(PKind::FOAM, base, glm::vec3(0.0f), 3.2f + 1.5f * power, 0.4f, 1.1f * power + 0.6f, ps.water(0.95f, 0.98f, 1.0f, 0.7f), ps.water(0.95f, 0.98f, 1.0f, 0.0f),
                            false, 0.0f, 0.0f, PWater::STICK));
    }
};

// ---- ShipWake -------------------------------------------------------------------------------------------------------------------------

// The emitter of one ship's wake. It keeps only fractional-particle accumulators (so the rate is the same at any frame rate); the foam itself lives in the pool.
struct ShipWake {
    float sternAcc = 0.0f, sideAcc = 0.0f, bowAcc = 0.0f, bubbleAcc = 0.0f, roughAcc = 0.0f;

    // `speed01` is |speed| / top speed (0..1); `rough` the sea state (0 flat .. 1.5 storm); the hull is `halfLength` long from its position fore and aft.
    void emit(ParticleSystem& ps, const WaterContext& water, const glm::vec3& pos, float heading, float speed01, float rough, float halfLength, float halfBeam, float dt, float light)
    {
        if (dt <= 0.0f)
            return;
        const glm::vec3 fwd(std::sin(heading), 0.0f, std::cos(heading));
        const glm::vec3 side(std::cos(heading), 0.0f, -std::sin(heading));
        const float s = std::clamp(speed01, 0.0f, 1.0f);
        const float wild = 1.0f + 1.6f * rough;                                // a rough sea throws more of everything
        const auto surface = [&](const glm::vec3& p) { return glm::vec3(p.x, waterSurfaceY(water, p.x, p.z) + ParticleConfig::WATER_LIFT, p.z); };
        const glm::vec3 white = ps.waterTint * (0.82f + 0.18f * light);

        if (s > 0.04f) {
            // The stern wash: foam spawned in the ship's path, spreading outwards in a V and then standing still on the sea as the ship leaves it behind.
            sternAcc += dt * (8.0f + 42.0f * s);
            while (sternAcc >= 1.0f) {
                sternAcc -= 1.0f;
                const float lat = ps.range(-1.0f, 1.0f) * halfBeam * 0.8f;
                const glm::vec3 p = surface(pos - fwd * (halfLength * 0.95f) + side * lat);
                const glm::vec3 v = -fwd * (0.1f + 0.3f * s) + side * (lat >= 0.0f ? 1.0f : -1.0f) * (0.15f + 0.45f * s);
                ps.add(makeParticle(PKind::FOAM, p, v, 4.5f + 3.5f * s, 0.45f + 0.3f * s, 1.5f + 1.1f * s, glm::vec4(white, 0.45f + 0.3f * s), glm::vec4(white, 0.0f),
                                    false, 0.0f, 0.5f, PWater::STICK));
            }
            // Foam along the sides, a thin white line where the hull cuts the water.
            sideAcc += dt * (6.0f + 26.0f * s) * wild * 0.6f;
            while (sideAcc >= 1.0f) {
                sideAcc -= 1.0f;
                const float sgn = (ps.rand01() < 0.5f) ? -1.0f : 1.0f;
                const glm::vec3 p = surface(pos + fwd * ps.range(-halfLength * 0.5f, halfLength * 0.8f) + side * sgn * halfBeam * 1.05f);
                ps.add(makeParticle(PKind::FOAM, p, side * sgn * 0.25f - fwd * (0.2f * s), 2.2f, 0.28f, 0.8f, glm::vec4(white, 0.5f * s + 0.15f), glm::vec4(white, 0.0f),
                                    false, 0.0f, 0.6f, PWater::STICK));
            }
            // The bow wave and its spray: droplets thrown up and out, more the faster the ship (square) and the rougher the sea.
            bowAcc += dt * (3.0f + 55.0f * s * s) * wild;
            while (bowAcc >= 1.0f) {
                bowAcc -= 1.0f;
                const float sgn = (ps.rand01() < 0.5f) ? -1.0f : 1.0f;
                const glm::vec3 p = surface(pos + fwd * (halfLength * 0.98f) + side * sgn * ps.range(0.0f, halfBeam * 0.7f));
                const float q = ps.range(0.07f, 0.15f);
                ps.add(makeParticle(PKind::SPARK, p + glm::vec3(0.0f, 0.1f, 0.0f), side * sgn * ps.range(0.5f, 1.6f + s) + fwd * (0.8f * s * 3.0f) + glm::vec3(0.0f, ps.range(1.3f, 3.0f) * (0.6f + s), 0.0f),
                                    1.4f, q, q * 0.8f, ps.water(0.9f, 0.96f, 1.0f, 0.85f), ps.water(0.9f, 0.96f, 1.0f, 0.2f), false, 9.8f, 0.2f, PWater::DIE_BELOW));
                if (ps.rand01() < 0.35f)
                    ps.add(makeParticle(PKind::FOAM, p, side * sgn * (0.5f + 0.8f * s), 2.6f, 0.3f, 1.0f, glm::vec4(white, 0.6f), glm::vec4(white, 0.0f),
                                        false, 0.0f, 0.5f, PWater::STICK));
            }
            // Bubbles churned down by the hull and the rudder: they rise and vanish at the surface (seen from below).
            bubbleAcc += dt * (4.0f + 18.0f * s);
            while (bubbleAcc >= 1.0f) {
                bubbleAcc -= 1.0f;
                const glm::vec3 p = pos - fwd * ps.range(0.0f, halfLength) + side * ps.range(-halfBeam, halfBeam) + glm::vec3(0.0f, -ps.range(0.3f, 1.2f), 0.0f);
                const float q = ps.range(0.04f, 0.13f);
                ps.add(makeParticle(PKind::BUBBLE, p, glm::vec3(ps.range(-0.1f, 0.1f), ps.range(0.5f, 1.1f), ps.range(-0.1f, 0.1f)), 12.0f, q, q, ps.water(0.85f, 0.97f, 1.0f, 0.7f),
                                    ps.water(0.85f, 0.97f, 1.0f, 0.7f), false, 0.0f, 0.2f, PWater::DIE_ABOVE));
            }
        }
        // A rough sea slaps the hull even when it lies still: bursts of spray along both sides.
        if (rough > 0.4f) {
            roughAcc += dt * 7.0f * (rough - 0.3f);
            while (roughAcc >= 1.0f) {
                roughAcc -= 1.0f;
                const float sgn = (ps.rand01() < 0.5f) ? -1.0f : 1.0f;
                const glm::vec3 p = surface(pos + fwd * ps.range(-halfLength, halfLength) + side * sgn * halfBeam);
                for (int k = 0; k < 4; ++k) {
                    const float q = ps.range(0.06f, 0.13f);
                    ps.add(makeParticle(PKind::SPARK, p + glm::vec3(0.0f, 0.1f, 0.0f), side * sgn * ps.range(0.2f, 1.2f) + glm::vec3(0.0f, ps.range(1.5f, 3.5f), 0.0f), 1.2f, q, q * 0.8f,
                                        ps.water(0.9f, 0.96f, 1.0f, 0.85f), ps.water(0.9f, 0.96f, 1.0f, 0.2f), false, 9.8f, 0.2f, PWater::DIE_BELOW));
                }
                ps.add(makeParticle(PKind::FOAM, p, side * sgn * 0.3f, 2.4f, 0.35f, 1.0f, glm::vec4(white, 0.55f), glm::vec4(white, 0.0f), false, 0.0f, 0.6f, PWater::STICK));
            }
        }
    }
};

// ---- UnderwaterRenderer -----------------------------------------------------------------------------------------------------------------

// The camera and the water surface. `depth` is how far the eye is below the surface directly above it (negative above); `amount` is a smooth 0..1 version, 0
// clearly above, 1 clearly below, with a blend across the 0.6-unit band around the surface - a function of the eye's position alone, so it works however the camera
// or the ship moves, and whatever the weather or the time of day.
struct UnderwaterRenderer {
    static constexpr float BAND = 0.30f;

    static float depthOf(const WaterContext& water, const glm::vec3& eye)
    {
        return waterSurfaceY(water, eye.x, eye.z) - eye.y;
    }
    static float amountOf(float depth)
    {
        const float t = std::clamp((depth + BAND) / (2.0f * BAND), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Bubbles and drifting specks round the camera, so the water has depth and the player can see they are moving through it. Only when underwater.
    struct Ambience { float bubbleAcc = 0.0f, moteAcc = 0.0f; };

    static void emitAmbience(ParticleSystem& ps, Ambience& a, const WaterContext& water, const glm::vec3& eye, float amount, float dt, float light)
    {
        if (amount < 0.3f || dt <= 0.0f)
            return;
        const float surface = waterSurfaceY(water, eye.x, eye.z);
        const auto place = [&](float spread, float below, float above) {
            const float y = std::min(eye.y + ps.range(-below, above), surface - 0.3f);
            return glm::vec3(eye.x + ps.range(-spread, spread), y, eye.z + ps.range(-spread, spread));
        };
        a.bubbleAcc += dt * 16.0f;
        while (a.bubbleAcc >= 1.0f) {
            a.bubbleAcc -= 1.0f;
            const float q = ps.range(0.04f, 0.16f);
            ps.add(makeParticle(PKind::BUBBLE, place(11.0f, 7.0f, 1.0f), glm::vec3(ps.range(-0.08f, 0.08f), ps.range(0.5f, 1.2f), ps.range(-0.08f, 0.08f)), 25.0f, q, q,
                                ps.water(0.85f, 0.97f, 1.0f, 0.65f), ps.water(0.85f, 0.97f, 1.0f, 0.65f), false, 0.0f, 0.1f, PWater::DIE_ABOVE));
        }
        // Specks: plankton, silt and small bits of wreckage - pale, green and brown - turning slowly in the water.
        a.moteAcc += dt * 20.0f;
        while (a.moteAcc >= 1.0f) {
            a.moteAcc -= 1.0f;
            const float k = ps.rand01();
            const glm::vec3 col = (k < 0.5f) ? glm::vec3(0.75f, 0.9f, 0.9f) : ((k < 0.8f) ? glm::vec3(0.35f, 0.55f, 0.25f) : glm::vec3(0.45f, 0.32f, 0.2f));
            const float q = (k < 0.8f) ? ps.range(0.02f, 0.05f) : ps.range(0.05f, 0.11f);
            ps.add(makeParticle(PKind::SPARK, place(10.0f, 6.0f, 3.0f), ps.unitSphere() * 0.12f + glm::vec3(0.0f, -0.05f, 0.0f), 6.0f, q, q,
                                glm::vec4(col * (0.4f + 0.6f * light), 0.55f), glm::vec4(col * (0.4f + 0.6f * light), 0.0f), false, 0.0f, 0.3f));
        }
    }

    // Shafts of light: long, soft, slanting beams from the surface, swaying and breathing. Built as camera-facing strips along the sun's (or moon's) direction
    // and given to the additive batch. `out` receives the corner positions and (u, v) of each strip, six vertices per beam. `strength` 0..1.
    struct RayQuad { glm::vec3 p[4]; float strength; glm::vec3 color; };

    static void buildRays(std::vector<RayQuad>& out, const glm::vec3& eye, const glm::vec3& lightTravelDir, const WaterContext& water, float strength, float time)
    {
        out.clear();
        if (strength <= 0.01f)
            return;
        glm::vec3 d = lightTravelDir;
        d.y = std::min(d.y, -0.25f);                               // water bends the light towards the vertical
        d = glm::normalize(glm::mix(glm::normalize(d), glm::vec3(0.0f, -1.0f, 0.0f), 0.45f));
        const float surface = waterSurfaceY(water, eye.x, eye.z);
        constexpr int BEAMS = 11;
        for (int i = 0; i < BEAMS; ++i) {
            const float fi = static_cast<float>(i);
            const float ang = fi * 2.399963f;                      // the golden angle: spread round the eye without lining up
            const float rad = 3.0f + 14.0f * std::fabs(std::sin(fi * 1.7f + 0.3f));
            const glm::vec3 top(eye.x + rad * std::cos(ang) + 1.2f * std::sin(0.31f * time + fi), surface + 0.4f, eye.z + rad * std::sin(ang) + 1.2f * std::cos(0.27f * time + fi * 1.3f));
            const float length = 34.0f;
            const glm::vec3 bottom = top + d * (length / std::max(0.2f, -d.y));
            const glm::vec3 axis = glm::normalize(bottom - top);
            glm::vec3 side = glm::cross(axis, glm::normalize(top - eye));
            if (glm::dot(side, side) < 1e-6f)
                side = glm::vec3(1.0f, 0.0f, 0.0f);
            side = glm::normalize(side);
            const float w = 0.7f + 0.5f * std::fabs(std::sin(fi * 2.3f + 1.0f));
            RayQuad q;
            q.p[0] = top - side * w;  q.p[1] = top + side * w;
            q.p[2] = bottom + side * (w * 3.2f);  q.p[3] = bottom - side * (w * 3.2f);
            q.strength = strength * (0.55f + 0.45f * std::sin(0.8f * time + fi * 2.1f)) * 0.14f;
            q.color = glm::vec3(0.62f, 0.95f, 1.0f);
            out.push_back(q);
        }
    }
};
