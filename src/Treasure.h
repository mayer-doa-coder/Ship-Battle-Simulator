#pragma once

// Environment build: COLLECTABLE TREASURE. No OpenGL here: what is collected, when it was opened, how near you must be, and how bright a chest looks from where you are.
//
// WHAT IS STATE. Only two facts per chest persist: whether it has been collected, and the clock time it was opened at. The opening itself (the lid swinging up, the gold
// settling, the glow dying) is a closed form of `now - openedAt`, like every other animation in the project (no frame table). A chest is named by an id: the world's chests by
// location and place in that location's list (so a collected chest stays collected if you leave a location and come back), the ship's own by their place in the interior.
//
// HOW YOU REACH ONE. A chest is collected with `F` when you are close enough, by simple distance:
//   - on a beach, in a cave, on a wreck, on a pier: sail within SHORE_REACH of it (the ship is the player; the chest is within a boat-hook of the bow);
//   - on a summit or a stack of rock: sail within SUMMIT_REACH, a longer way - the climb is not played - but the chest does not even glow until you are near, so you must go and look;
//   - on the seabed: be UNDER the sea within DIVE_REACH (the free camera can dive: F6, then Q to sink);
//   - in the ship: walk to it in the below-deck view (TAB) and be within SHIP_REACH.
// A chest the player cannot be expected to see from the usual approach (`hidden`) only shows its glow close up, so the sea has to be explored.

#include "Scenery.h"

#include <algorithm>
#include <cmath>

namespace TreasureConfig {
constexpr int WORLD_SLOTS_PER_LOCATION = 8;
constexpr int LOCATIONS = 5;
constexpr int SHIP_BASE = WORLD_SLOTS_PER_LOCATION * LOCATIONS;        // the ship's chests follow the world's
constexpr int SHIP_SLOTS = 8;
constexpr int SLOTS = SHIP_BASE + SHIP_SLOTS;

constexpr float OPEN_SECONDS = 1.25f;          // the lid swings up in this long
constexpr float GOLD_SECONDS = 0.9f;           // then the gold takes this long to be gathered
constexpr float WORLD_SCALE = 2.6f;            // a chest on an island is drawn this many times the size of one in the hold, so it can be seen from the sea

constexpr float SHORE_REACH = 11.0f;
constexpr float SUMMIT_REACH = 17.0f;
constexpr float DIVE_REACH = 4.0f;
constexpr float SHIP_REACH = 0.55f;

constexpr float SEE_RANGE = 60.0f;             // a chest in plain sight glows from this far
constexpr float HIDDEN_SEE_RANGE = 24.0f;      // a hidden one only from this far
constexpr float DRAW_RANGE = 120.0f;
}

struct TreasureState {
    bool collected[TreasureConfig::SLOTS] = {};
    float openedAt[TreasureConfig::SLOTS] = {};
    int gold = 0;                              // the total collected
    int found = 0;                             // how many chests
    float lastCollectAt = -100.0f;             // for the counter's flash on the screen
    int lastCollectGold = 0;
};

inline int treasureWorldId(int location, int index) { return location * TreasureConfig::WORLD_SLOTS_PER_LOCATION + std::min(index, TreasureConfig::WORLD_SLOTS_PER_LOCATION - 1); }
inline int treasureShipId(int index) { return TreasureConfig::SHIP_BASE + std::min(index, TreasureConfig::SHIP_SLOTS - 1); }

inline float treasureReach(TreasureKind kind)
{
    switch (kind) {
    case TreasureKind::SUMMIT:     return TreasureConfig::SUMMIT_REACH;
    case TreasureKind::UNDERWATER: return TreasureConfig::DIVE_REACH;
    case TreasureKind::SHIP:       return TreasureConfig::SHIP_REACH;
    default:                       return TreasureConfig::SHORE_REACH;
    }
}

inline float treasureScale(TreasureKind kind) { return kind == TreasureKind::SHIP ? 1.0f : TreasureConfig::WORLD_SCALE; }

// 0 closed .. 1 fully open, from the time since the chest was opened (-1 if it is not open). An ease-out: the lid leaps up and settles.
inline float treasureLidOpen(float age)
{
    if (age < 0.0f)
        return 0.0f;
    const float t = std::clamp(age / TreasureConfig::OPEN_SECONDS, 0.0f, 1.0f);
    const float o = 1.0f - t;
    return 1.0f - o * o * o;
}

// 1 while the gold is still in the chest, falling to 0 as it is gathered (after the lid is up).
inline float treasureGoldLeft(float age)
{
    if (age < 0.0f)
        return 1.0f;
    const float t = std::clamp((age - TreasureConfig::OPEN_SECONDS * 0.7f) / TreasureConfig::GOLD_SECONDS, 0.0f, 1.0f);
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

// How much a chest glows from `distance` away: 0 out of sight, rising to 1 as you come within its reach, with a slow pulse so it catches the eye. A hidden chest is only
// seen close up.
inline float treasureGlow(float distance, bool hidden, float reach, float time, float phase)
{
    const float range = hidden ? TreasureConfig::HIDDEN_SEE_RANGE : TreasureConfig::SEE_RANGE;
    const float near = std::clamp((range - distance) / range, 0.0f, 1.0f);
    const float close = std::clamp(1.4f - distance / std::max(reach, 0.1f), 0.0f, 1.0f);
    const float pulse = 0.75f + 0.25f * std::sin(2.6f * time + phase);
    return std::clamp((0.35f * near + 0.65f * close) * pulse, 0.0f, 1.0f);
}
