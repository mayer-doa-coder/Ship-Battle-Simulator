#pragma once

// Environment build: THE LIVING WAR AT SEA - fire, smoke, explosions, damage, sails, the anchor, the helm, and the ships far away. No OpenGL in this file (like Crew.h and
// Wildlife.h): the state of each system, how it is stepped by a frame's duration, and what it asks the particle pool to draw. src/FxDraw.h draws the solid parts.
//
// THE SYSTEMS (each a small struct with static functions or a step(), no hierarchy):
//
//   FireSystem        hull-local fire sources on a ship: they grow, spread by degrees to neighbours, are fought by the crew and by rain, and each throws flames, sparks and embers
//   SmokeSystem       smoke that rises, widens, leans with the wind and fades (the particle pool does the ageing; the wind is set on the pool, see Particles.h)
//   ExplosionSystem   one reusable blast - flash, fireball, sparks, smoke, debris - for kegs, misfires and anything else
//   DamageSystem      holes shot in a hull, broken rails, torn sails and flying/floating debris, all tied to the hull's health (damageLevel)
//   SailSystem        the ordered state of the sails (open / partial / closed) and what it does to the ship's speed
//   AnchorSystem      lowering and raising the anchor, and how it holds the ship
//   SteeringSystem    the helm eased towards the keys, the rudder and wheel that follow it
//   ShipAI            reusable heading and ballistic-leading decisions for computer-controlled ships
//   FleetSystem       the other ships on the horizon (pirate, merchant, naval): their paths, their poses, their occasional broadsides
//
// Everything advances by dt, and every pool has a fixed size: nothing grows without bound, and expired things are removed.

#include "Particles.h"
#include "Ship.h"
#include "Waves.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace FxConfig {
constexpr int MAX_FIRES = 10;
constexpr int MAX_HOLES = 14;
constexpr int MAX_BROKEN_RAILS = 24;
constexpr int MAX_DEBRIS = 96;
constexpr int MAX_KEGS = 24;
constexpr float ANCHOR_SECONDS = 3.6f;      // to lower or raise it
constexpr float ANCHOR_DEPTH = 6.0f;        // how far below the sea it hangs when down
constexpr float FLEET_CELL = 150.0f;        // one distant ship to a cell of the sea this big
constexpr int FLEET_RADIUS = 1;             // the 3 x 3 cells round the player
constexpr float FLEET_SHOW = 96.0f;
}

// ---- the wind -------------------------------------------------------------------------------------------------------------------------

// The way the wind blows TOWARDS, as a bearing like a ship's heading (0 = +z, turning towards +x). It wanders slowly, as a closed form of the clock.
inline float windAngleAt(float t) { return 0.9f + 0.45f * std::sin(0.021f * t) + 0.20f * std::sin(0.057f * t + 1.3f); }

// The wind as a velocity in the world, for smoke and spray: 0.35 units a second in a calm, over 3 in a gale.
inline glm::vec3 windVectorAt(float t, float strength)
{
    const float a = windAngleAt(t), speed = 0.35f + 3.0f * strength;
    return glm::vec3(std::sin(a) * speed, 0.0f, std::cos(a) * speed);
}

inline float fxWrap(float a)
{
    while (a > SHIP_PI) a -= 2.0f * SHIP_PI;
    while (a < -SHIP_PI) a += 2.0f * SHIP_PI;
    return a;
}

// ---- the damage a ship shows --------------------------------------------------------------------------------------------------------

// 0 whole .. 4 critical, from the hull's health as a fraction.
inline int damageLevel(float health01)
{
    if (health01 > 0.80f) return 0;
    if (health01 > 0.55f) return 1;
    if (health01 > 0.35f) return 2;
    if (health01 > 0.15f) return 3;
    return 4;
}

struct Hole {
    glm::vec3 local = glm::vec3(0.0f);      // hull-local, on the skin of the hull
    float side = 1.0f;                      // +1 starboard (+x), -1 port: the way the hole faces
    float size = 0.2f;
    float age = 0.0f;
};

struct FireSource {
    bool alive = false;
    glm::vec3 local = glm::vec3(0.0f);      // hull-local
    float intensity = 0.0f;                 // 0 embers .. 1 a blaze
    float age = 0.0f;
    float spreadIn = 6.0f;                  // seconds (at full blaze) until it may light a neighbour
    float carry[4] = {};                    // fractions of a particle owed: flames, smoke, sparks, embers
};

// Everything one ship shows of being hurt and burning.
struct ShipFx {
    Hole holes[FxConfig::MAX_HOLES];
    int holeCount = 0;
    int brokenRails[FxConfig::MAX_BROKEN_RAILS];
    int brokenCount = 0;
    int ripped[SHIP_MAST_COUNT] = {};       // tears in each sail
    int mastDamage[SHIP_MAST_COUNT] = {};   // 0 sound .. 3 broken
    bool mastBroken[SHIP_MAST_COUNT] = {};
    float mastFall[SHIP_MAST_COUNT] = {};   // 0 upright .. 1 lying over
    FireSource fires[FxConfig::MAX_FIRES];
    int fireCount = 0;                      // alive sources
    float rudder = 0.0f;                    // radians, the blade's angle
    float helm = 0.0f;                      // -1..1, the eased steering input
    float burnedFor = 0.0f;
    float burnDamageCarry = 0.0f;           // fractional hull damage accumulated by an unchecked blaze
    int lastDamageLevel = 0;                // highest progressive visual state already applied

    void clear() { *this = ShipFx(); }
};

inline int railKey(int run, int side, int k) { return run * 4096 + (side > 0 ? 2048 : 0) + k; }
inline bool railBroken(const ShipFx& fx, int run, int side, int k)
{
    const int key = railKey(run, side, k);
    for (int i = 0; i < fx.brokenCount; ++i)
        if (fx.brokenRails[i] == key) return true;
    return false;
}
inline void breakRail(ShipFx& fx, int run, int side, int k)
{
    if (railBroken(fx, run, side, k) || fx.brokenCount >= FxConfig::MAX_BROKEN_RAILS) return;
    fx.brokenRails[fx.brokenCount++] = railKey(run, side, k);
}

struct DamageSystem {
    // A hole where a ball went in: on the hull's skin at the impact's height and length, on the side it came from. The oldest is replaced when all are used.
    static void addHole(ShipFx& fx, const glm::vec3& local, float halfBeamThere, float size)
    {
        Hole h;
        h.side = (local.x >= 0.0f) ? 1.0f : -1.0f;
        h.local = glm::vec3(h.side * halfBeamThere, local.y, local.z);
        h.size = size;
        if (fx.holeCount < FxConfig::MAX_HOLES) {
            fx.holes[fx.holeCount++] = h;
        } else {
            for (int i = 1; i < FxConfig::MAX_HOLES; ++i) fx.holes[i - 1] = fx.holes[i];
            fx.holes[FxConfig::MAX_HOLES - 1] = h;
        }
    }

    // A tear in the sail on the mast nearest the impact (along the ship), more of them the worse the damage.
    static void tearSail(ShipFx& fx, int mast) { fx.ripped[mast] = std::min(fx.ripped[mast] + 1, 5); }

    static bool hitMast(ShipFx& fx, int mast, int force)
    {
        mast = std::max(0, std::min(mast, SHIP_MAST_COUNT - 1));
        if (fx.mastBroken[mast]) return false;
        fx.mastDamage[mast] = std::min(3, fx.mastDamage[mast] + std::max(1, force));
        tearSail(fx, mast);
        if (fx.mastDamage[mast] >= 3) {
            fx.mastBroken[mast] = true;
            return true;
        }
        return false;
    }

    static void stepMasts(ShipFx& fx, float dt)
    {
        for (int i = 0; i < SHIP_MAST_COUNT; ++i)
            if (fx.mastBroken[i])
                fx.mastFall[i] = std::min(1.0f, fx.mastFall[i] + 0.32f * dt);
    }
};

// ---- FireSystem and SmokeSystem ----------------------------------------------------------------------------------------------------

struct FireEnv {
    float rain = 0.0f;                      // 0..1
    float wind = 0.0f;                      // 0..1
    glm::vec3 windVec = glm::vec3(0.0f);
    float light = 1.0f;                     // how bright the world is (smoke is darker at night)
    float crewHelp = 0.0f;                  // the rate at which the crew beat out each fire, per second
    bool sinking = false;
};

struct SmokeSystem {
    // One puff of smoke from a point: it rises (negative gravity), widens as it goes, takes on the wind and fades. `density` 0..1 thickens and darkens it; rain turns it pale (steam).
    static void puff(ParticleSystem& ps, const glm::vec3& at, float density, const FireEnv& env, float sizeScale = 1.0f)
    {
        const float shade = (0.10f + 0.20f * ps.rand01()) * (0.35f + 0.65f * env.light);
        const float steam = std::clamp(env.rain * 1.4f, 0.0f, 0.7f);
        const float g = glm::mix(shade, 0.62f * (0.4f + 0.6f * env.light), steam);
        ps.add(makeParticle(PKind::PUFF, at, glm::vec3(ps.range(-0.2f, 0.2f), ps.range(0.8f, 1.5f), ps.range(-0.2f, 0.2f)) + env.windVec * 0.25f,
                            ps.range(4.0f, 6.5f), (0.22f + 0.2f * density) * sizeScale, (1.5f + 1.4f * density) * sizeScale * ps.range(0.8f, 1.2f),
                            glm::vec4(g, g, g * 1.03f, 0.18f + 0.40f * density), glm::vec4(g, g, g, 0.0f), false, -0.30f, 0.55f));
    }
};

struct FireSystem {
    static bool start(ShipFx& fx, const glm::vec3& local, float intensity = 0.25f)
    {
        for (int i = 0; i < FxConfig::MAX_FIRES; ++i)
            if (!fx.fires[i].alive) {
                FireSource f;
                f.alive = true;
                f.local = local;
                f.intensity = intensity;
                f.spreadIn = 5.0f + 0.01f * static_cast<float>(i * 37 % 100);
                fx.fires[i] = f;
                ++fx.fireCount;
                return true;
            }
        return false;
    }

    // One frame: every fire grows, is fought, may spread, and throws flames, smoke and sparks into the pool. `hull` is the ship's hull frame (the sources are hull-local, so
    // the flames come out where the ship now is). Returns the number of fires alive.
    static int step(ShipFx& fx, ParticleSystem& ps, const glm::mat4& hull, float dt, const FireEnv& env)
    {
        if (dt <= 0.0f) return fx.fireCount;
        const bool room = static_cast<int>(ps.live.size()) < ParticleConfig::CAPACITY - 400;
        int alive = 0;
        for (int i = 0; i < FxConfig::MAX_FIRES; ++i) {
            FireSource& f = fx.fires[i];
            if (!f.alive) continue;
            f.age += dt;
            // Growth: fire feeds on wood and wind; rain, the crew's buckets and the sea (a sinking deck going under) fight it.
            const float grow = 0.075f * (1.0f + 0.7f * env.wind);
            const float fight = 0.09f * env.rain + env.crewHelp + (env.sinking ? 0.55f : 0.0f);
            f.intensity = std::clamp(f.intensity + (grow - fight) * dt, 0.0f, 1.0f);
            if (f.intensity <= 0.015f && f.age > 2.0f) {
                f.alive = false;
                continue;
            }
            ++alive;
            // Spread: only a fire that has taken hold lights a neighbour, and only after a while, so a blaze grows over a minute and not in a second.
            f.spreadIn -= dt * f.intensity;
            if (f.spreadIn <= 0.0f && f.intensity > 0.5f) {
                f.spreadIn = 7.0f + 5.0f * ps.rand01();
                const glm::vec3 next = f.local + glm::vec3(ps.range(-0.45f, 0.45f), 0.0f, ps.range(-0.7f, 0.7f));
                if (std::fabs(next.x) < 0.62f && std::fabs(next.z) < 2.3f)
                    start(fx, next, 0.12f);
            }
            if (!room) continue;
            const glm::vec3 at = glm::vec3(hull * glm::vec4(f.local, 1.0f));
            const float I = f.intensity;
            const auto count = [&](int slot, float perSecond) {
                f.carry[slot] += perSecond * dt;
                const int n = std::min(static_cast<int>(f.carry[slot]), 12);
                f.carry[slot] -= std::floor(f.carry[slot]);
                return n;
            };
            for (int n = count(0, 14.0f + 26.0f * I); n > 0; --n) {          // flames: bright, quick, additive, tongues that lean with the wind
                const float s = 0.10f + 0.16f * I;
                ps.add(makeParticle(PKind::PUFF, at + glm::vec3(ps.range(-0.12f, 0.12f) * (0.4f + I), ps.range(0.0f, 0.1f), ps.range(-0.12f, 0.12f) * (0.4f + I)),
                                    glm::vec3(ps.range(-0.25f, 0.25f), ps.range(0.9f, 1.9f) * (0.55f + I), ps.range(-0.25f, 0.25f)) + env.windVec * 0.22f,
                                    ps.range(0.40f, 0.85f), s * ps.range(0.8f, 1.3f), 0.03f, glm::vec4(1.0f, ps.range(0.55f, 0.85f), 0.18f, 0.92f), glm::vec4(0.9f, 0.18f, 0.04f, 0.0f), true, -0.4f, 0.8f));
            }
            for (int n = count(0 + 1, 3.5f + 8.0f * I); n > 0; --n)
                SmokeSystem::puff(ps, at + glm::vec3(ps.range(-0.1f, 0.1f), 0.25f + 0.35f * I, ps.range(-0.1f, 0.1f)), I, env);
            for (int n = count(2, (2.0f + 9.0f * I) * (1.0f + env.wind)); n > 0; --n)    // sparks
                ps.add(makeParticle(PKind::SPARK, at + glm::vec3(0.0f, 0.1f, 0.0f), glm::vec3(ps.range(-0.7f, 0.7f), ps.range(1.5f, 3.6f), ps.range(-0.7f, 0.7f)) + env.windVec * 0.5f,
                                    ps.range(0.6f, 1.5f), 0.035f, 0.02f, glm::vec4(1.0f, 0.78f, 0.35f, 1.0f), glm::vec4(1.0f, 0.3f, 0.05f, 0.0f), true, 4.5f, 0.3f));
            if (I > 0.55f)
                for (int n = count(3, 2.5f * I); n > 0; --n)                                // embers drifting down and downwind
                    ps.add(makeParticle(PKind::SPARK, at + glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(ps.range(-0.4f, 0.4f), ps.range(0.3f, 1.2f), ps.range(-0.4f, 0.4f)) + env.windVec * 0.9f,
                                        ps.range(1.8f, 3.2f), 0.03f, 0.015f, glm::vec4(1.0f, 0.5f, 0.12f, 0.9f), glm::vec4(0.6f, 0.1f, 0.02f, 0.0f), true, 0.6f, 0.4f));
        }
        fx.fireCount = alive;
        if (alive > 0) fx.burnedFor += dt;
        return alive;
    }

    // The sum of fires' brightness, for the halos and the burning-deck glow.
    static float totalIntensity(const ShipFx& fx)
    {
        float s = 0.0f;
        for (const FireSource& f : fx.fires) if (f.alive) s += f.intensity;
        return s;
    }
};

// ---- ExplosionSystem ----------------------------------------------------------------------------------------------------------------

struct ExplosionSystem {
    // The particles of a blast at `at`. `power` 0.5 is a misfire, 1 a powder keg, 2 a magazine. Debris and damage are the caller's (they need the ships).
    static void emit(ParticleSystem& ps, const glm::vec3& at, float power, float light, const glm::vec3& windVec)
    {
        const float p = std::max(power, 0.2f);
        ps.add(makeParticle(PKind::PUFF, at, glm::vec3(0.0f), 0.14f, 1.3f * p, 3.2f * p, glm::vec4(1.0f, 0.97f, 0.8f, 1.0f), glm::vec4(1.0f, 0.7f, 0.3f, 0.0f), true));         // the flash
        for (int i = 0; i < static_cast<int>(10 + 10 * p); ++i)                                                                                                     // the fireball
            ps.add(makeParticle(PKind::PUFF, at, ps.unitSphere() * ps.range(1.5f, 5.0f) * p + glm::vec3(0.0f, 1.0f, 0.0f), ps.range(0.4f, 0.9f), 0.35f * p, ps.range(1.0f, 1.8f) * p,
                                glm::vec4(1.0f, ps.range(0.5f, 0.8f), 0.15f, 0.9f), glm::vec4(0.8f, 0.15f, 0.03f, 0.0f), true, -0.5f, 2.2f));
        for (int i = 0; i < static_cast<int>(18 + 28 * p); ++i)                                                                                                     // sparks
            ps.add(makeParticle(PKind::SPARK, at, ps.unitSphere() * ps.range(4.0f, 11.0f) * p + glm::vec3(0.0f, 2.0f, 0.0f), ps.range(0.6f, 1.6f), 0.06f, 0.03f,
                                glm::vec4(1.0f, 0.85f, 0.4f, 1.0f), glm::vec4(1.0f, 0.3f, 0.05f, 0.0f), true, 9.0f, 0.4f));
        FireEnv env;
        env.light = light;
        env.windVec = windVec;
        for (int i = 0; i < static_cast<int>(8 + 10 * p); ++i)                                                                                                      // the column of smoke
            SmokeSystem::puff(ps, at + ps.unitSphere() * 0.4f * p, 0.9f, env, 1.0f + 0.8f * p);
    }
};

// ---- debris -----------------------------------------------------------------------------------------------------------------------------

// A chunk of timber thrown from a hit. It flies, falls, floats on the swell for a while (riding the real waves, drifting downwind) and sinks. Drawn as a small turned cube.
struct DebrisPiece {
    bool alive = false;
    glm::vec3 pos = glm::vec3(0.0f), vel = glm::vec3(0.0f);
    glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
    float angle = 0.0f, spin = 0.0f;
    glm::vec3 size = glm::vec3(0.1f);
    float age = 0.0f, life = 20.0f;
    bool floating = false;
    bool charred = false;
};

struct DebrisPool {
    DebrisPiece piece[FxConfig::MAX_DEBRIS];
    int next = 0;

    // Throws one in. The oldest slot is reused when the pool is full, so the pool never grows.
    DebrisPiece& spawn()
    {
        DebrisPiece& d = piece[next];
        next = (next + 1) % FxConfig::MAX_DEBRIS;
        d = DebrisPiece();
        d.alive = true;
        return d;
    }

    void step(float dt, float seaY, float waveAmp, float time, const glm::vec3& windVec)
    {
        if (dt <= 0.0f) return;
        for (DebrisPiece& d : piece) {
            if (!d.alive) continue;
            d.age += dt;
            d.angle += d.spin * dt;
            if (!d.floating) {
                d.vel.y -= 9.8f * dt;
                d.pos += d.vel * dt;
                const float surface = seaY + waveHeight(d.pos.x, d.pos.z, time, waveAmp);
                if (d.pos.y <= surface && d.vel.y < 0.0f) {
                    d.floating = true;
                    d.vel = glm::vec3(d.vel.x * 0.15f, 0.0f, d.vel.z * 0.15f);
                    d.spin *= 0.3f;
                }
            } else {
                d.vel += (windVec * 0.12f - d.vel) * (1.0f - std::exp(-0.8f * dt));
                d.pos.x += d.vel.x * dt;
                d.pos.z += d.vel.z * dt;
                const float sink = std::clamp((d.age - 0.7f * d.life) / (0.3f * d.life), 0.0f, 1.0f);
                d.pos.y = seaY + waveHeight(d.pos.x, d.pos.z, time, waveAmp) + 0.03f - 0.25f * sink;
            }
            if (d.age >= d.life) d.alive = false;
        }
    }

    int count() const { int n = 0; for (const DebrisPiece& d : piece) n += d.alive ? 1 : 0; return n; }
};

// ---- powder kegs -----------------------------------------------------------------------------------------------------------------------

struct PowderKeg {
    bool alive = false;
    int owner = -1;                         // -1 floating in the sea, 0 on the player's ship, 1 on the enemy's
    glm::vec3 pos = glm::vec3(0.0f);        // world (owner -1) or hull-local
    float fuse = -1.0f;                     // >= 0: counting down to a chain-reaction blast
};

// ---- SailSystem ------------------------------------------------------------------------------------------------------------------------

struct SailSystem {
    int state = 0;                          // 0 fully open, 1 partly open, 2 closed
    static float order(int state) { return state == 0 ? 1.0f : (state == 1 ? 0.55f : 0.0f); }
    static const char* name(int state) { return state == 0 ? "FULL" : (state == 1 ? "HALF" : "FURLED"); }
    void cycle(int direction) { state = (state + direction + 3) % 3; }

    // How fast the ship can go with this much sail set (0..1) and the wind: a little way even under bare poles (the hull and the current), the rest from the cloth, and the
    // wind behind or abeam helps more than the wind in the teeth. `relative` is the wind's bearing minus the ship's heading (0 = from astern).
    static float speedFactor(float sailSet, float relative, float wind)
    {
        const float push = 0.5f * (1.0f + std::cos(relative));                                // 1 wind from astern .. 0 on the nose
        const float cloth = 0.10f + 0.90f * std::clamp(sailSet, 0.0f, 1.0f);
        return cloth * (0.90f + 0.20f * wind * (push - 0.5f) * 2.0f + 0.04f * (push - 0.5f));
    }
};

// ---- AnchorSystem ----------------------------------------------------------------------------------------------------------------------

struct AnchorSystem {
    bool down = false;                      // what the captain wants
    float depth01 = 0.0f;                   // 0 stowed .. 1 hanging at the bottom of its chain
    bool splashed = false;                  // the splash of going into the sea has been made this descent

    void toggle() { down = !down; }

    // Returns true on the frame the anchor first touches the water (the caller makes the splash).
    bool step(float dt)
    {
        const float before = depth01;
        const float rate = dt / FxConfig::ANCHOR_SECONDS;
        depth01 = std::clamp(depth01 + (down ? rate : -rate), 0.0f, 1.0f);
        const bool entered = down && before < 0.18f && depth01 >= 0.18f;
        if (depth01 < 0.1f) splashed = false;
        return entered;
    }

    // How hard it holds: 0 while it is still coming down, rising to 1 as it takes the bottom.
    float hold() const { return std::clamp((depth01 - 0.35f) / 0.5f, 0.0f, 1.0f); }
    // The helm and the throttle are the captain's only when the anchor is fully up.
    bool free() const { return depth01 <= 0.0f && !down; }
};

// ---- SteeringSystem --------------------------------------------------------------------------------------------------------------------

struct SteeringSystem {
    // The helm is eased towards what the keys ask (3.2 per second: a half-second to full), so the ship's turn builds up and dies away instead of switching.
    static float easeHelm(float helm, float want, float dt) { return helm + std::clamp(want - helm, -3.2f * dt, 3.2f * dt); }
    // The rudder follows the helm a little behind it (a quarter of a second), through 0.55 rad (about 31 degrees) either way.
    static float easeRudder(float rudder, float helm, float dt) { return rudder + (0.55f * helm - rudder) * (1.0f - std::exp(-6.0f * dt)); }
    // A ship turns more readily when she is moving: not at all with no way on, fully at a third of her top speed.
    static float turnAuthority(float speedFraction) { return 0.35f + 0.65f * std::clamp(std::fabs(speedFraction) * 3.0f, 0.0f, 1.0f); }
};

// ---- ShipAI ---------------------------------------------------------------------------------------------------------------------------

struct ShipAI {
    static float steering(float currentHeading, float wantedHeading, float deadZone = 0.08f)
    {
        const float error = fxWrap(wantedHeading - currentHeading);
        return error > deadZone ? 1.0f : (error < -deadZone ? -1.0f : 0.0f);
    }

    static glm::vec3 lead(const glm::vec3& target, const glm::vec3& targetVelocity, float distance, float projectileSpeed)
    {
        return target + targetVelocity * (distance / std::max(projectileSpeed, 0.001f));
    }
};

// ---- FleetSystem: the other ships --------------------------------------------------------------------------------------------------------

enum class FleetKind { PIRATE, MERCHANT, NAVAL };

struct FleetShip {
    bool present = false;
    FleetKind kind = FleetKind::PIRATE;
    float cx = 0.0f, cz = 0.0f;              // centre of its loop
    float ax = 30.0f, az = 20.0f;            // radii of the loop
    float tilt = 0.0f;                       // the loop's own turn
    float omega = 0.02f;                     // angular speed of its progress round the loop (radians a second); sign is the direction
    float phase = 0.0f;
    float scale = 1.0f;
    float shotPeriod = 24.0f, shotPhase = 0.0f;
    unsigned int hash = 0;
};

inline unsigned int fleetHash(int i, int j)
{
    unsigned int h = static_cast<unsigned int>(i) * 2246822519u + static_cast<unsigned int>(j) * 3266489917u + 374761393u;
    h = (h ^ (h >> 15)) * 2654435761u;
    return h ^ (h >> 13);
}
inline float fleetU(unsigned int h, int slot) { return static_cast<float>((h >> (slot * 4)) & 255u) / 255.0f; }

// The ship that lives in cell (i, j) of the sea - the same one every time the cell is asked about. About three cells in four have one.
inline FleetShip fleetShipAt(int i, int j)
{
    FleetShip s;
    const unsigned int h = fleetHash(i, j);
    s.hash = h;
    if (h % 100u >= 78u) return s;
    s.present = true;
    const float cell = FxConfig::FLEET_CELL;
    s.cx = (static_cast<float>(i) + 0.30f + 0.40f * fleetU(h, 1)) * cell;
    s.cz = (static_cast<float>(j) + 0.30f + 0.40f * fleetU(h, 2)) * cell;
    s.ax = 22.0f + 30.0f * fleetU(h, 3);
    s.az = 12.0f + 22.0f * fleetU(h, 4);
    s.tilt = 6.2832f * fleetU(h, 5);
    const float seconds = 380.0f + 360.0f * fleetU(h, 6);                      // six to twelve minutes to go round: slow
    s.omega = ((h >> 20) & 1u ? 1.0f : -1.0f) * 6.2832f / seconds;
    s.phase = 6.2832f * fleetU(h, 7);
    const unsigned int k = (h >> 24) % 10u;
    s.kind = (k < 4u) ? FleetKind::PIRATE : (k < 7u ? FleetKind::MERCHANT : FleetKind::NAVAL);
    s.scale = (s.kind == FleetKind::PIRATE) ? 0.9f : (s.kind == FleetKind::MERCHANT ? 1.05f : 1.2f);
    s.shotPeriod = 20.0f + 18.0f * fleetU(h, 0);
    s.shotPhase = 40.0f * fleetU(h, 1);
    return s;
}

struct FleetPose {
    glm::vec3 pos = glm::vec3(0.0f);        // at the waterline, before the waves
    float heading = 0.0f;
};

// Where the ship is at time t: an ellipse, turned, traversed at its own slow rate. The heading is along the path.
inline FleetPose fleetPoseAt(const FleetShip& s, float t, float seaY)
{
    const float a = s.phase + s.omega * t;
    const float ct = std::cos(s.tilt), st = std::sin(s.tilt);
    const float ex = s.ax * std::cos(a), ez = s.az * std::sin(a);
    const float dx = -s.ax * std::sin(a) * s.omega, dz = s.az * std::cos(a) * s.omega;
    FleetPose p;
    p.pos = glm::vec3(s.cx + ex * ct - ez * st, seaY, s.cz + ex * st + ez * ct);
    const float vx = dx * ct - dz * st, vz = dx * st + dz * ct;
    p.heading = std::atan2(vx, vz);
    return p;
}

// Is the whole loop clear of land (so the ship never sails over an island)? `obstacles` as in src/Scenery.h. Sampled at 32 points with a margin.
template <class ObstacleList>
inline bool fleetLoopClear(const FleetShip& s, const ObstacleList& obstacles)
{
    for (int k = 0; k < 32; ++k) {
        const float a = 6.2832f * static_cast<float>(k) / 32.0f;
        const float ct = std::cos(s.tilt), st = std::sin(s.tilt);
        const float ex = s.ax * std::cos(a), ez = s.az * std::sin(a);
        const float x = s.cx + ex * ct - ez * st, z = s.cz + ex * st + ez * ct;
        for (const auto& o : obstacles) {
            const float dx = x - o.x, dz = z - o.z, r = o.radius + 7.0f;
            if (dx * dx + dz * dz < r * r) return false;
        }
    }
    return true;
}
