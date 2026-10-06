#pragma once

// Environment build: THE CREW. Thirteen kinds of sailor, each with a permanent post on the ship, a job, and a small state machine that decides what
// they are doing right now - hauling a rope, climbing a shroud, loading the cannon, stirring the pot - and how they react to the rain, a storm, the
// dark, a fog, an enemy and a shot that lands.
//
// This file is the BRAIN and the SKELETON, and contains no OpenGL (like src/Ship.h and src/Scenery.h), so every claim about it can be checked on the
// CPU: that every post is on a deck and inside the hull, that a pose never bends a joint past what a person can, that a storm sends the sailors to the
// ropes. The drawing (the primitives each person is built from) is src/CrewDraw.h.
//
// THREE LAYERS.
//   1. CrewLayout   Where things ARE, worked out from the ship's own dimensions (ShipDimensions): the helm, the crow's nest, the cabin, the galley, the
//                   guns, the ladders, the patrol routes. All in the HULL's frame, so a person standing at (x, y, z) in it goes wherever the hull goes -
//                   through the ship's roll and pitch on the waves, with no separate bookkeeping. That is the hierarchy rule of Stage D at work again.
//   2. CrewMember   One person's STATE: where they are, what task they are on, for how long. updateShipCrew() steps every member by the frame's length;
//                   it is the "simple AI". The state is small and discrete (a task and a timer), not a recording: nothing is stored that could be played back.
//   3. CrewPose     What that state looks like: the angle of every joint, as a closed form of the member's own animation clock. crewPoseOf() is pure.
//
// THE FIGURE. A person is 0.44 of a world unit tall (the hull is 5.6 long, so a real man would be about 0.32; these are a little larger so they read
// from the chase camera). Proportions are fractions of that height, so a tall sailor and a short one are the same code with a different number.

#include "Ship.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

// ---- roles and tasks --------------------------------------------------------------------------------------------------------------

enum class CrewRole {
    CAPTAIN, FIRST_MATE, HELMSMAN, LOOKOUT, SAILOR, RIGGER, CANNON_CREW, DECK_CREW, CARPENTER, COOK, NAVIGATOR, MARINE, CABIN_CREW
};

inline const char* crewRoleName(CrewRole r)
{
    static const char* const NAMES[] = { "captain", "first mate", "helmsman", "lookout", "sailor", "rigger", "cannon crew", "deck crew", "carpenter",
                                         "cook", "navigator", "marine", "cabin crew" };
    return NAMES[static_cast<int>(r)];
}

enum class CrewTask {
    IDLE,          // standing easy: breathing, glancing about
    WALK,          // going somewhere
    STEER,         // both hands on the wheel
    COMMAND,       // pointing, giving orders
    TALK,          // talking with a hand raised
    LOOK_SEA,      // a hand to the brow, scanning the sea
    SCAN,          // binoculars to the eyes
    PULL_ROPE,     // hand over hand on a line
    CLIMB,         // up or down a shroud or a ladder
    RIG_WORK,      // aloft, arms up at the yard
    LOAD_GUN,      // the gun's reload: swab, ball, ram
    FIRE_GUN,      // standing back from the gun as it fires
    CARRY,         // a barrel, a crate, a cannonball, a tray
    HAMMER,        // repairs
    READ_MAP,      // bent over the chart
    COOK,          // stirring the pot
    SECURE,        // in a storm: lashing things down
    READY_ARMS,    // musket raised, facing the enemy
    PATROL_STAND,  // a guard at a halt, musket grounded
    REST,          // sitting down, off duty
    BRACE,         // flinching from an impact
    INJURED,       // hurt, kneeling
    PANIC          // the ship is going down
};

inline const char* crewTaskName(CrewTask t)
{
    static const char* const NAMES[] = { "idle", "walk", "steer", "command", "talk", "look at the sea", "scan with binoculars", "pull a rope", "climb",
                                         "work aloft", "load the gun", "stand back from the gun", "carry", "hammer", "read the chart", "cook", "secure",
                                         "ready arms", "stand guard", "rest", "brace", "injured", "panic" };
    return NAMES[static_cast<int>(t)];
}

namespace CrewConfig {

constexpr float HEIGHT = 0.44f;           // world units: the standard person (individuals vary by +-10%)
constexpr float WALK_SPEED = 0.42f;       // units per second
constexpr float RUN_SPEED = 0.90f;        // a storm sends sailors running
constexpr float STRIDE = 0.20f;           // distance walked in one whole gait cycle (two steps)
constexpr float CLIMB_SPEED = 0.34f;
constexpr float COMBAT_RANGE = 80.0f;     // an enemy this close puts the ship at action stations

constexpr int MAX_MEMBERS = 24;

// How long things last (seconds).
constexpr float BRACE_SECONDS = 0.9f;
constexpr float INJURED_SECONDS = 6.0f;
constexpr float REPAIR_SECONDS = 11.0f;
constexpr float SAIL_RATE = 0.10f;        // fraction of the sail's height raised or lowered per second

} // namespace CrewConfig

// ---- the layout: where everything is -----------------------------------------------------------------------------------------------
//
// Every position is in the HULL's frame: x across (the ship's left is +x), y up (0 is the hull's centre), z along (+z is the bow). A "station" is a
// place a person stands and the way they face (radians about y, 0 = towards the bow, positive turns towards +x).

struct Station {
    glm::vec3 pos = glm::vec3(0.0f);
    float facing = 0.0f;
};

inline float facingToward(const glm::vec3& from, const glm::vec3& to)
{
    return std::atan2(to.x - from.x, to.z - from.z);
}

struct Route {
    static constexpr int MAX = 12;
    int count = 0;
    glm::vec3 node[MAX] = {};
    bool climbToNext[MAX] = {};      // is the leg from this node to the next a ladder
    float pause[MAX] = {};           // how long a guard stands at this node
};

struct DummyGun {
    glm::vec3 pos = glm::vec3(0.0f);     // the middle of the carriage's foot, on the deck
    float facing = 0.0f;                 // the way the muzzle points (out through a gunport)
};

struct CrewLayout {
    // The walking surfaces (hull-local y).
    float waistY = 0.0f, quarterY = 0.0f, poopY = 0.0f, foreY = 0.0f;

    // The helm.
    glm::vec3 wheelHub = glm::vec3(0.0f);
    float wheelRadius = 0.15f;
    Station helmsman, captain, firstMate;
    glm::vec3 helmLantern = glm::vec3(0.0f);       // a lantern on a post beside the wheel (the point light is lent to it at night)

    // The crow's nest.
    glm::vec3 nestFloor = glm::vec3(0.0f);
    float nestRadius = 0.27f;
    Station lookout;

    // The captain's cabin, an open-topped deckhouse on the quarterdeck.
    glm::vec2 cabinMin = glm::vec2(0.0f), cabinMax = glm::vec2(0.0f);
    float cabinWallHeight = 0.24f;
    glm::vec3 tablePos = glm::vec3(0.0f);
    glm::vec2 tableSize = glm::vec2(0.24f, 0.17f);
    Station navigator, steward;
    glm::vec3 shelfPos = glm::vec3(0.0f);          // where the steward fetches from
    glm::vec3 cabinLantern = glm::vec3(0.0f);

    // The galley.
    glm::vec3 galleyPos = glm::vec3(0.0f);         // the hut's footprint centre
    glm::vec3 galleySize = glm::vec3(0.36f, 0.34f, 0.32f);
    glm::vec3 cauldronPos = glm::vec3(0.0f);
    Station cook;
    glm::vec3 galleyLantern = glm::vec3(0.0f);

    // The guns. gunCrew[0..1] stand at the aimable cannon, gunCrew[2..3] at the two other guns.
    glm::vec3 cannonPos = glm::vec3(0.0f);
    Station gunStation[4];
    DummyGun dummyGun[4];
    glm::vec3 shotRack = glm::vec3(0.0f);

    // Cargo.
    glm::vec3 barrelStack[2] = {};                  // A (aft, port) and B (forward, starboard)
    glm::vec3 crateStack[2] = {};
    glm::vec3 ropeCoil[3] = {};
    glm::vec3 workbench = glm::vec3(0.0f);
    Station carpenter;

    // Ladders. Each is a pair of points, foot and head; a ladder leans, so the two are not above one another.
    glm::vec3 quarterLadder[2] = {};                // waist -> quarterdeck
    glm::vec3 poopLadder[2] = {};                   // quarterdeck -> poop
    glm::vec3 foreLadder[2] = {};                   // waist -> forecastle
    glm::vec3 shroud[SHIP_MAST_COUNT][2] = {};      // foot on the starboard rail, head at the mast: the riggers' way up
    glm::vec3 mastFoot[SHIP_MAST_COUNT] = {};       // the mast's base
    float yardY[SHIP_MAST_COUNT] = {};              // the yard's height (hull-local)
    float yardHalfLength[SHIP_MAST_COUNT] = {};

    // The belaying points where sailors haul: two to each mast, one each side.
    Station belay[4];

    // Patrol routes for the marines.
    Route patrol[2];

    // Where the crew first stands, one entry per member (see rosterFor).
    float dockY = 0.0f;
};

inline CrewLayout buildCrewLayout(const ShipDimensions& d)
{
    CrewLayout L;
    const float lift = shipDeckPlankLift(d);
    const float halfDepth = d.hullSize.y * 0.5f;
    L.waistY = d.hullSize.y * hullLowestSheer(d.hullProfile) - halfDepth + d.deckSize.y + lift;
    L.quarterY = d.deckLevels[DECK_QUARTERDECK].floorHeight - halfDepth + lift;
    L.poopY = d.deckLevels[DECK_POOP].floorHeight - halfDepth + lift;
    L.foreY = d.deckLevels[DECK_FORECASTLE].floorHeight - halfDepth + lift;

    const DeckLevelDimensions& poop = d.deckLevels[DECK_POOP];
    const DeckLevelDimensions& quarter = d.deckLevels[DECK_QUARTERDECK];
    const float poopMid = 0.5f * (poop.zFrom + poop.zTo);

    // ---- the helm, on the poop, at the very stern
    L.wheelRadius = 0.15f;
    L.wheelHub = glm::vec3(0.0f, L.poopY + 0.25f, poopMid + 0.12f);
    L.helmsman = { glm::vec3(0.0f, L.poopY, L.wheelHub.z - 0.19f), 0.0f };
    L.captain = { glm::vec3(0.30f, L.poopY, poopMid - 0.02f), 0.35f };
    L.helmLantern = glm::vec3(-0.30f, L.poopY + 0.50f, poopMid - 0.05f);

    // ---- the first mate, on the open part of the quarterdeck, near the foot of the poop ladder
    const float qOpenFrom = poop.zTo, qOpenTo = quarter.zTo;       // the part of the quarterdeck the poop does not cover
    L.firstMate = { glm::vec3(0.27f, L.quarterY, qOpenFrom + 0.26f), 3.14159f };

    // ---- ladders
    L.poopLadder[0] = glm::vec3(0.42f, L.quarterY, qOpenFrom + 0.24f);
    L.poopLadder[1] = glm::vec3(0.42f, L.poopY, qOpenFrom + 0.02f);
    L.quarterLadder[0] = glm::vec3(0.42f, L.waistY, qOpenTo + 0.20f);
    L.quarterLadder[1] = glm::vec3(0.42f, L.quarterY, qOpenTo - 0.03f);
    const DeckLevelDimensions& fore = d.deckLevels[DECK_FORECASTLE];
    L.foreLadder[0] = glm::vec3(0.40f, L.waistY, fore.zFrom - 0.18f);
    L.foreLadder[1] = glm::vec3(0.40f, L.foreY, fore.zFrom + 0.03f);

    // ---- the cabin: a low-walled room, open to the sky, on the PORT side of the quarterdeck's open part
    L.cabinMin = glm::vec2(-0.50f, qOpenFrom + 0.06f);
    L.cabinMax = glm::vec2(-0.08f, qOpenTo - 0.10f);
    L.tablePos = glm::vec3(-0.34f, L.quarterY, qOpenFrom + 0.20f);
    L.navigator = { glm::vec3(-0.34f, L.quarterY, qOpenFrom + 0.38f), 3.14159f };
    L.steward = { glm::vec3(-0.17f, L.quarterY, qOpenFrom + 0.22f), -1.57f };
    L.shelfPos = glm::vec3(-0.43f, L.quarterY, qOpenFrom + 0.32f);
    L.cabinLantern = glm::vec3(-0.34f, L.quarterY + 0.28f, qOpenFrom + 0.20f);

    // ---- the masts, the shrouds and the crow's nest
    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        const MastDimensions& m = d.masts[i];
        const float z = d.deckCenterZ + m.z;
        L.mastFoot[i] = glm::vec3(0.0f, L.waistY, z);
        L.yardY[i] = L.waistY + m.height * m.yardHeightFraction;
        L.yardHalfLength[i] = 0.5f * m.yardLength;
        L.shroud[i][0] = glm::vec3(0.52f, L.waistY, z);
        L.shroud[i][1] = glm::vec3(0.10f, L.yardY[i] - 0.10f, z);
    }
    const MastDimensions& mainMast = d.masts[MAIN_MAST];
    L.nestFloor = glm::vec3(0.0f, L.waistY + 0.90f * mainMast.height, L.mastFoot[MAIN_MAST].z);
    L.lookout = { L.nestFloor + glm::vec3(0.0f, 0.02f, 0.0f), 0.0f };

    // ---- belaying points at the rail, two to each mast
    L.belay[0] = { glm::vec3(0.47f, L.waistY, L.mastFoot[MAIN_MAST].z - 0.18f), 1.57f };
    L.belay[1] = { glm::vec3(-0.47f, L.waistY, L.mastFoot[MAIN_MAST].z - 0.18f), -1.57f };
    L.belay[2] = { glm::vec3(0.47f, L.waistY, L.mastFoot[FORE_MAST].z - 0.12f), 1.57f };
    L.belay[3] = { glm::vec3(-0.47f, L.waistY, L.mastFoot[FORE_MAST].z - 0.12f), -1.57f };

    // ---- the guns
    L.cannonPos = glm::vec3(d.cannon.mountXZ.x, L.waistY, d.deckCenterZ + d.cannon.mountXZ.y);
    L.gunStation[0] = { L.cannonPos + glm::vec3(-0.24f, 0.0f, -0.20f), 1.57f };        // the loader, inboard and aft of the breech
    L.gunStation[1] = { L.cannonPos + glm::vec3(-0.24f, 0.0f, 0.20f), 1.57f };         // the gunner with the linstock
    L.dummyGun[0] = { glm::vec3(0.36f, L.waistY, -1.00f), 1.57f };
    L.dummyGun[1] = { glm::vec3(0.36f, L.waistY, 0.44f), 1.57f };
    L.dummyGun[2] = { glm::vec3(-0.36f, L.waistY, -1.00f), -1.57f };
    L.dummyGun[3] = { glm::vec3(-0.36f, L.waistY, 0.44f), -1.57f };
    L.gunStation[2] = { L.dummyGun[0].pos + glm::vec3(-0.26f, 0.0f, 0.0f), 1.57f };
    L.gunStation[3] = { L.dummyGun[3].pos + glm::vec3(0.26f, 0.0f, 0.0f), -1.57f };
    L.shotRack = glm::vec3(0.16f, L.waistY, L.cannonPos.z - 0.52f);

    // ---- the galley, on the port side by the foremast, with its fire door facing the centre of the deck
    L.galleyPos = glm::vec3(-0.30f, L.waistY, fore.zFrom - 0.20f);
    L.cauldronPos = glm::vec3(-0.02f, L.waistY, L.galleyPos.z);
    L.cook = { glm::vec3(0.14f, L.waistY, L.galleyPos.z), -1.57f };
    L.galleyLantern = glm::vec3(-0.12f, L.waistY + 0.40f, L.galleyPos.z + 0.19f);

    // ---- cargo, rope and the carpenter's bench
    L.barrelStack[0] = glm::vec3(-0.38f, L.waistY, -0.62f);
    L.barrelStack[1] = glm::vec3(0.34f, L.waistY, 0.40f);
    L.crateStack[0] = glm::vec3(0.34f, L.waistY, -0.56f);
    L.crateStack[1] = glm::vec3(-0.36f, L.waistY, 0.40f);
    L.ropeCoil[0] = glm::vec3(0.18f, L.waistY, L.mastFoot[MAIN_MAST].z + 0.14f);
    L.ropeCoil[1] = glm::vec3(-0.14f, L.waistY, L.mastFoot[FORE_MAST].z - 0.16f);
    L.ropeCoil[2] = glm::vec3(0.28f, L.foreY, fore.zFrom + 0.32f);
    L.workbench = glm::vec3(-0.40f, L.waistY, -1.18f);
    L.carpenter = { L.workbench + glm::vec3(0.26f, 0.0f, 0.0f), -1.57f };
    L.dockY = L.waistY;

    // ---- the marines' routes. Route 0 walks the waist round and climbs to the quarterdeck and back; route 1 keeps the forecastle.
    Route& r0 = L.patrol[0];
    const float zA = L.waistY < 0.0f ? -1.12f : -1.12f;
    const glm::vec3 w1(-0.36f, L.waistY, zA), w2(-0.36f, L.waistY, 0.90f), w3(0.36f, L.waistY, 0.90f), w4(0.36f, L.waistY, zA);
    r0.count = 0;
    const auto add = [](Route& r, const glm::vec3& p, bool climb, float pause) {
        r.node[r.count] = p; r.climbToNext[r.count] = climb; r.pause[r.count] = pause; ++r.count;
    };
    add(r0, w1, false, 1.5f);
    add(r0, w2, false, 2.5f);
    add(r0, w3, false, 1.5f);
    add(r0, w4, false, 0.5f);
    add(r0, L.quarterLadder[0], true, 0.0f);
    add(r0, L.quarterLadder[1], false, 0.0f);
    add(r0, glm::vec3(0.18f, L.quarterY, qOpenFrom + 0.36f), false, 3.0f);
    add(r0, L.quarterLadder[1], true, 0.0f);          // the way back down: this leg goes from the head to the foot
    add(r0, L.quarterLadder[0], false, 0.5f);

    Route& r1 = L.patrol[1];
    const float fz0 = fore.zFrom + 0.16f, fz1 = fore.zTo - 0.14f;
    add(r1, glm::vec3(-0.22f, L.foreY, fz0), false, 3.0f);
    add(r1, glm::vec3(0.22f, L.foreY, fz0), false, 1.0f);
    add(r1, glm::vec3(0.22f, L.foreY, fz1), false, 3.5f);
    add(r1, glm::vec3(-0.22f, L.foreY, fz1), false, 1.0f);
    return L;
}

// ---- the roster: who is aboard ----------------------------------------------------------------------------------------------------

struct RosterEntry {
    CrewRole role;
    int site;            // which of the role's posts: which belaying point, which gun, which route
};

// The player's ship: twenty-one souls. The enemy's has a smaller company (the same posts, fewer hands).
inline std::vector<RosterEntry> crewRoster(bool fullCompany)
{
    std::vector<RosterEntry> r = {
        { CrewRole::CAPTAIN, 0 }, { CrewRole::FIRST_MATE, 0 }, { CrewRole::HELMSMAN, 0 }, { CrewRole::LOOKOUT, 0 },
        { CrewRole::SAILOR, 0 }, { CrewRole::SAILOR, 1 }, { CrewRole::SAILOR, 2 },
        { CrewRole::RIGGER, 0 }, { CrewRole::RIGGER, 1 },
        { CrewRole::CANNON_CREW, 0 }, { CrewRole::CANNON_CREW, 1 }, { CrewRole::CANNON_CREW, 2 }, { CrewRole::CANNON_CREW, 3 },
        { CrewRole::DECK_CREW, 0 }, { CrewRole::DECK_CREW, 1 },
        { CrewRole::CARPENTER, 0 }, { CrewRole::COOK, 0 }, { CrewRole::NAVIGATOR, 0 },
        { CrewRole::MARINE, 0 }, { CrewRole::MARINE, 1 }, { CrewRole::CABIN_CREW, 0 },
    };
    if (!fullCompany) {
        // The enemy: drop a sailor, a rigger, two of the gun crews, a deck hand, the cook, the navigator and the steward.
        const CrewRole drop[] = { CrewRole::NAVIGATOR, CrewRole::CABIN_CREW, CrewRole::COOK };
        for (CrewRole d : drop)
            r.erase(std::remove_if(r.begin(), r.end(), [d](const RosterEntry& e) { return e.role == d; }), r.end());
        r.erase(std::remove_if(r.begin(), r.end(), [](const RosterEntry& e) {
            return (e.role == CrewRole::SAILOR && e.site == 2) || (e.role == CrewRole::RIGGER && e.site == 1)
                || (e.role == CrewRole::CANNON_CREW && e.site >= 2) || (e.role == CrewRole::DECK_CREW && e.site == 1);
        }), r.end());
    }
    return r;
}

// ---- one person's state --------------------------------------------------------------------------------------------------------

struct CrewMember {
    CrewRole role = CrewRole::SAILOR;
    int site = 0;
    int variant = 0;                       // picks height, build and colours (see crewLookFor)

    glm::vec3 pos = glm::vec3(0.0f);       // hull-local, at the feet
    float facing = 0.0f;

    CrewTask task = CrewTask::IDLE;
    float taskTime = 0.0f;                 // seconds in this task
    float taskLength = 3.0f;               // how long this spell lasts
    float anim = 0.0f;                     // animation clock: runs at the member's work rate, so a hurry looks like a hurry
    float phase = 0.0f;                    // a personal offset, so no two move in step
    float walkPhase = 0.0f;                // radians of gait, advanced by distance walked
    float climbPhase = 0.0f;

    int stage = 0;                         // where in a multi-step job
    int routeNode = 0;
    float routePause = 0.0f;
    glm::vec3 goal = glm::vec3(0.0f);      // where a WALK or CLIMB is heading
    bool goalClimb = false;
    glm::vec3 climbFrom = glm::vec3(0.0f);
    float climbT = 0.0f;
    bool carrying = false;
    bool moving = false;                   // moved this frame (the pose adds a walking gait under whatever the hands are doing)

    float brace = 0.0f;                    // seconds of flinch left
    float injured = 0.0f;                  // seconds of injury left
    float aimYaw = 0.0f;                   // the way a look or a point is turned (hull-local bearing)
    bool offDuty = false;                  // resting for the night
    unsigned int rng = 1u;
};

// What the world is doing, as far as the crew is concerned. Filled in by the game from the atmosphere and the battle each frame.
struct CrewContext {
    float rain = 0.0f;                     // 0..1
    float wind = 0.0f;                     // 0..1
    float hazeDensity = 0.03f;
    float lanterns = 0.0f;                 // 0 day .. 1 night
    bool stormy = false;                   // a gale: ropes to secure, guns to lash
    bool rainy = false;
    bool foggy = false;
    bool dark = false;

    bool combat = false;                   // an enemy is in range and both ships are afloat
    float enemyBearing = 0.0f;             // hull-local bearing of the enemy (0 = bow, + towards +x)
    bool enemyOnPositiveSide = true;

    bool ownGunFired = false;              // the cannon fired this frame
    float reloadFraction = 1.0f;           // 0 just fired .. 1 ready
    int newHits = 0;                       // hits taken this frame
    glm::vec3 hitLocal = glm::vec3(0.0f);  // where (hull-local)
    float healthFraction = 1.0f;

    bool sinking = false;
    float turnInput = 0.0f;                // -1 (right) .. 1 (left)
    float throttle = 0.0f;
    float sailOrder = -1.0f;               // the captain's order for the sails (1 full, 0.55 half, 0 furled); -1 = no order, the weather decides
};

struct ShipCrew {
    std::vector<CrewMember> members;
    float sailSet = 1.0f;                  // how much of the sail is set: 1 full, 0.4 close-reefed
    float sailTarget = 1.0f;
    bool hoisting = false;                 // the sails are being raised or lowered
    float wheelAngle = 0.0f;
    bool burning = false;                  // the ship is on fire: the hands go to the flames as they go to a hole
    float repairTimer = 0.0f;              // > 0 while there is damage to mend
    glm::vec3 repairSpot = glm::vec3(0.0f);
    bool gear = false;                     // oilskins on
    float cannonRecoil = 0.0f;             // 1 at the shot, dying to 0: the gun crew stands back
    float time = 0.0f;                     // the crew's own clock
    bool built = false;
    CrewContext ctx;                       // what the world asked of them in the last update (the poses are drawn from it)
};

// ---- appearance ---------------------------------------------------------------------------------------------------------------------

enum class HatKind { NONE, TRICORN, BICORN, BANDANA, CAP, SOUWESTER, COOK_CAP, FEATHERED };

struct CrewLook {
    float height = CrewConfig::HEIGHT;
    float build = 1.0f;                    // 1 average, > 1 broad
    glm::vec3 skin = glm::vec3(0.86f, 0.66f, 0.52f);
    glm::vec3 coat = glm::vec3(0.35f, 0.25f, 0.18f);
    glm::vec3 trousers = glm::vec3(0.25f, 0.22f, 0.20f);
    glm::vec3 boots = glm::vec3(0.10f, 0.07f, 0.05f);
    glm::vec3 belt = glm::vec3(0.12f, 0.08f, 0.05f);
    glm::vec3 hat = glm::vec3(0.10f, 0.08f, 0.08f);
    glm::vec3 trim = glm::vec3(0.80f, 0.62f, 0.20f);     // buttons, braid, the feather's quill
    HatKind hatKind = HatKind::TRICORN;
    bool beard = false;
    bool coatTails = false;                // a long coat
    bool sword = false;
    bool pistol = false;
    bool apron = false;
    bool epaulettes = false;
    bool eyepatch = false;
    bool sash = false;
};

inline float crewHash01(unsigned int x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return static_cast<float>(x & 0xFFFFFFu) / 16777216.0f;
}

// The look of a role. `variant` varies height, build and colours within it (no two sailors alike); `gear` puts on oilskins; `enemy` shifts the whole
// company to darker, redder colours so the two ships' crews can be told apart.
inline CrewLook crewLookFor(CrewRole role, int variant, bool gear, bool enemy)
{
    CrewLook l;
    const float v1 = crewHash01(static_cast<unsigned int>(variant) * 7919u + static_cast<unsigned int>(role) * 104729u + 11u);
    const float v2 = crewHash01(static_cast<unsigned int>(variant) * 6151u + static_cast<unsigned int>(role) * 15485863u + 3u);
    const float v3 = crewHash01(static_cast<unsigned int>(variant) * 3571u + static_cast<unsigned int>(role) * 1299709u + 29u);
    l.height = CrewConfig::HEIGHT * (0.92f + 0.18f * v1);
    l.build = 0.92f + 0.22f * v2;
    static const glm::vec3 SKINS[4] = { glm::vec3(0.86f, 0.66f, 0.52f), glm::vec3(0.72f, 0.52f, 0.38f), glm::vec3(0.54f, 0.38f, 0.28f), glm::vec3(0.90f, 0.74f, 0.62f) };
    l.skin = SKINS[static_cast<int>(v3 * 3.99f)];
    l.beard = v2 > 0.55f;

    switch (role) {
    case CrewRole::CAPTAIN:
        l.height = CrewConfig::HEIGHT * 1.12f; l.build = 1.12f;
        l.coat = glm::vec3(0.55f, 0.07f, 0.09f); l.trousers = glm::vec3(0.88f, 0.84f, 0.74f); l.boots = glm::vec3(0.05f, 0.04f, 0.04f);
        l.hat = glm::vec3(0.07f, 0.06f, 0.08f); l.hatKind = HatKind::FEATHERED; l.coatTails = true; l.sword = true; l.pistol = true;
        l.epaulettes = true; l.beard = true; l.belt = glm::vec3(0.35f, 0.20f, 0.08f); l.trim = glm::vec3(0.95f, 0.78f, 0.25f);
        break;
    case CrewRole::FIRST_MATE:
        l.height = CrewConfig::HEIGHT * 1.04f;
        l.coat = glm::vec3(0.12f, 0.20f, 0.38f); l.trousers = glm::vec3(0.30f, 0.27f, 0.24f); l.hat = glm::vec3(0.08f, 0.09f, 0.14f);
        l.hatKind = HatKind::BICORN; l.coatTails = true; l.sword = true; l.epaulettes = true; l.trim = glm::vec3(0.80f, 0.70f, 0.30f);
        break;
    case CrewRole::HELMSMAN:
        l.coat = glm::vec3(0.30f, 0.34f, 0.40f); l.trousers = glm::vec3(0.22f, 0.20f, 0.19f); l.hatKind = HatKind::CAP; l.hat = glm::vec3(0.12f, 0.14f, 0.18f);
        l.build = 1.18f; l.beard = true;
        break;
    case CrewRole::LOOKOUT:
        l.height = CrewConfig::HEIGHT * 0.94f; l.build = 0.92f;
        l.coat = glm::vec3(0.58f, 0.52f, 0.40f); l.trousers = glm::vec3(0.28f, 0.24f, 0.18f); l.hatKind = HatKind::BANDANA; l.hat = glm::vec3(0.70f, 0.10f, 0.10f);
        break;
    case CrewRole::SAILOR:
        l.coat = (variant % 3 == 0) ? glm::vec3(0.78f, 0.74f, 0.64f) : ((variant % 3 == 1) ? glm::vec3(0.30f, 0.38f, 0.52f) : glm::vec3(0.62f, 0.52f, 0.38f));
        l.trousers = glm::vec3(0.82f, 0.78f, 0.68f); l.hatKind = HatKind::BANDANA; l.hat = (variant % 2) ? glm::vec3(0.18f, 0.30f, 0.62f) : glm::vec3(0.72f, 0.16f, 0.12f);
        l.sash = true;
        break;
    case CrewRole::RIGGER:
        l.height = CrewConfig::HEIGHT * 0.96f; l.build = 0.90f;
        l.coat = glm::vec3(0.50f, 0.42f, 0.30f); l.trousers = glm::vec3(0.36f, 0.30f, 0.22f); l.hatKind = HatKind::BANDANA; l.hat = glm::vec3(0.88f, 0.84f, 0.74f);
        break;
    case CrewRole::CANNON_CREW:
        l.build = 1.20f; l.coat = glm::vec3(0.60f, 0.20f, 0.14f); l.trousers = glm::vec3(0.20f, 0.19f, 0.18f);
        l.hatKind = HatKind::BANDANA; l.hat = glm::vec3(0.10f, 0.10f, 0.12f);
        break;
    case CrewRole::DECK_CREW:
        l.build = 1.25f; l.coat = (variant % 2) ? glm::vec3(0.40f, 0.48f, 0.30f) : glm::vec3(0.52f, 0.40f, 0.26f);
        l.trousers = glm::vec3(0.26f, 0.22f, 0.18f); l.hatKind = HatKind::CAP; l.hat = glm::vec3(0.34f, 0.26f, 0.16f);
        break;
    case CrewRole::CARPENTER:
        l.coat = glm::vec3(0.50f, 0.36f, 0.20f); l.trousers = glm::vec3(0.30f, 0.26f, 0.20f); l.hatKind = HatKind::CAP; l.hat = glm::vec3(0.52f, 0.44f, 0.30f);
        l.apron = true; l.beard = true;
        break;
    case CrewRole::COOK:
        l.build = 1.30f; l.coat = glm::vec3(0.92f, 0.90f, 0.84f); l.trousers = glm::vec3(0.34f, 0.32f, 0.30f);
        l.hatKind = HatKind::COOK_CAP; l.hat = glm::vec3(0.96f, 0.95f, 0.92f); l.apron = true; l.beard = false;
        break;
    case CrewRole::NAVIGATOR:
        l.coat = glm::vec3(0.22f, 0.30f, 0.22f); l.trousers = glm::vec3(0.30f, 0.28f, 0.24f); l.hatKind = HatKind::BICORN; l.hat = glm::vec3(0.10f, 0.12f, 0.10f);
        l.coatTails = true; l.build = 0.95f; l.trim = glm::vec3(0.80f, 0.70f, 0.30f);
        break;
    case CrewRole::MARINE:
        l.coat = glm::vec3(0.72f, 0.12f, 0.12f); l.trousers = glm::vec3(0.88f, 0.86f, 0.80f); l.boots = glm::vec3(0.07f, 0.06f, 0.05f);
        l.hatKind = HatKind::TRICORN; l.hat = glm::vec3(0.06f, 0.06f, 0.07f); l.sword = true; l.pistol = (variant % 2) == 0; l.trim = glm::vec3(0.92f, 0.90f, 0.82f);
        l.build = 1.10f;
        break;
    case CrewRole::CABIN_CREW:
        l.height = CrewConfig::HEIGHT * 0.88f; l.build = 0.86f;
        l.coat = glm::vec3(0.86f, 0.84f, 0.78f); l.trousers = glm::vec3(0.22f, 0.20f, 0.20f); l.hatKind = HatKind::NONE; l.apron = true; l.beard = false;
        break;
    }

    if (enemy) {
        // The enemy's company: darker and redder, black hats.
        l.coat = glm::mix(l.coat, glm::vec3(0.18f, 0.06f, 0.06f), 0.45f);
        if (l.hatKind != HatKind::NONE && l.hatKind != HatKind::COOK_CAP)
            l.hat = glm::mix(l.hat, glm::vec3(0.04f, 0.04f, 0.05f), 0.6f);
    }

    if (gear) {
        // OILSKINS for rain: officers in dark navy, the rest in yellow, and a sou'wester (a brimmed hat) for everyone who wears one.
        const bool officer = role == CrewRole::CAPTAIN || role == CrewRole::FIRST_MATE || role == CrewRole::NAVIGATOR;
        l.coat = officer ? glm::vec3(0.08f, 0.12f, 0.20f) : glm::vec3(0.88f, 0.70f, 0.10f);
        if (role != CrewRole::COOK && role != CrewRole::CABIN_CREW) {
            l.hat = officer ? glm::vec3(0.05f, 0.08f, 0.14f) : glm::vec3(0.90f, 0.72f, 0.10f);
            l.hatKind = HatKind::SOUWESTER;
        }
        l.apron = false;
    }
    return l;
}

// ---- the pose: what the state looks like --------------------------------------------------------------------------------------------

enum class CrewHold {
    NONE, BINOCULARS, MUSKET_SHOULDER, MUSKET_AIMED, MUSKET_GROUNDED, SWORD_DRAWN, HAMMER, LADLE, ROPE, BARREL, CRATE, TRAY, MAP_PEN,
    LINSTOCK, RAMMER, SPONGE, BALL, LANTERN
};

struct Arm  { float pitch = 0.0f, roll = 0.0f, bend = 0.0f; };       // shoulder forward, shoulder outward, elbow
struct Leg  { float pitch = 0.0f, bend = 0.0f; };                    // hip forward, knee

struct CrewPose {
    float lift = 0.0f;               // in heights: the whole body up (a bounce) or down
    float lean = 0.0f;               // forward lean of the torso, radians
    float sway = 0.0f;               // side lean, positive towards +x
    float twist = 0.0f;              // torso turned relative to the hips
    float headYaw = 0.0f, headPitch = 0.0f;
    Arm armL, armR;                  // left is the +x side
    Leg legL, legR;
    CrewHold hold = CrewHold::NONE;  // what is in the right hand (or both)
    float holdParam = 0.0f;
    bool lampInHand = false;         // an officer's lantern, in the left
    float crouch = 0.0f;             // for the information of the drawer (0..1)
};

inline float crewMix(float a, float b, float t) { return a + (b - a) * t; }

namespace CrewBody {
// Fractions of the figure's height.
constexpr float THIGH = 0.205f, SHIN = 0.215f;
constexpr float TORSO = 0.300f, NECK = 0.020f, HEAD = 0.190f;
constexpr float UPPER_ARM = 0.150f, FOREARM = 0.140f;
constexpr float HIP_X = 0.055f, SHOULDER_X = 0.135f, SHOULDER_DOWN = 0.025f;
} // namespace CrewBody

// How far a leg reaches down from the hip when bent: the pelvis rides at this height above the feet.
inline float crewLegReach(const Leg& g)
{
    return CrewBody::THIGH * std::cos(g.pitch) + CrewBody::SHIN * std::cos(g.pitch - g.bend);
}

// The pose of a member NOW: a pure function of its task, its animation clock and the world. `ctx` supplies what the world asks of it (an enemy to
// point at, rain to hunch against).
inline CrewPose crewPoseOf(const CrewMember& m, const CrewContext& ctx, const ShipCrew& crew)
{
    CrewPose p;
    const float t = m.anim + m.phase;
    const float breathe = std::sin(1.9f * t);
    const float w = m.walkPhase;
    const float hunch = ctx.rainy ? 0.08f : 0.0f;

    // Standing easy: the base every task starts from.
    p.lift = 0.004f * breathe;
    p.lean = hunch + 0.02f * breathe;
    p.sway = 0.035f * std::sin(0.55f * t);
    p.headYaw = 0.45f * std::sin(0.37f * t) + 0.2f * std::sin(0.93f * t + 1.0f);
    p.headPitch = 0.05f * std::sin(0.6f * t);
    p.armL = { 0.05f * std::sin(0.8f * t), 0.10f, 0.15f };
    p.armR = { -0.05f * std::sin(0.8f * t), 0.10f, 0.15f };
    p.legL = { 0.03f, 0.04f };
    p.legR = { -0.02f, 0.03f };

    // A flinch or an injury overrides everything else.
    if (m.injured > 0.0f) {
        const float groan = std::sin(2.2f * t);
        p.crouch = 1.0f;
        p.lean = 0.55f; p.headPitch = 0.45f; p.headYaw = 0.2f * groan;
        p.legL = { 1.15f, 2.20f }; p.legR = { 0.35f, 1.95f };      // down on one knee
        p.armL = { 0.55f, 0.05f, 0.9f };                            // hand to the wound
        p.armR = { 0.55f, 0.5f, 0.8f };
        return p;
    }
    if (m.brace > 0.0f) {
        const float k = std::min(1.0f, m.brace / 0.25f);
        p.crouch = 0.7f * k;
        p.lean = 0.35f * k + hunch; p.headPitch = 0.4f * k;
        p.legL = { 0.80f * k, 1.55f * k }; p.legR = { 0.65f * k, 1.35f * k };
        p.armL = { 1.15f * k, 0.25f, 1.55f * k };                   // arms up over the head
        p.armR = { 1.15f * k, 0.25f, 1.55f * k };
        return p;
    }

    switch (m.task) {
    case CrewTask::IDLE:
        break;

    case CrewTask::WALK: {
        const float swing = std::sin(w);
        const float run = ctx.stormy ? 1.35f : 1.0f;
        p.legL = { 0.62f * run * swing, std::max(0.0f, 1.00f * run * std::sin(w + 1.57f)) * 0.9f };
        p.legR = { -0.62f * run * swing, std::max(0.0f, -1.00f * run * std::sin(w + 1.57f)) * 0.9f };
        p.armL = { -0.55f * run * swing, 0.08f, 0.25f + 0.2f * (swing > 0 ? 0.0f : 1.0f) };
        p.armR = { 0.55f * run * swing, 0.08f, 0.25f + 0.2f * (swing > 0 ? 1.0f : 0.0f) };
        p.lift = 0.014f * std::fabs(std::cos(w)) * run;
        p.lean = 0.06f + hunch + (ctx.stormy ? 0.10f : 0.0f);
        p.twist = 0.10f * swing;
        p.headYaw = 0.12f * std::sin(0.5f * t);
        if (m.carrying) {
            p.armL = { 0.95f, 0.18f, 1.00f }; p.armR = { 0.95f, 0.18f, 1.00f };
            p.lean = 0.12f; p.twist = 0.0f;
        }
        break;
    }

    case CrewTask::STEER: {
        // Both hands on the rim of the wheel, which the helmsman turns: his arms follow the wheel through the spokes' angle.
        const float a = crew.wheelAngle;
        p.armL = { 1.12f + 0.20f * std::sin(a + 1.2f), 0.30f, 0.45f - 0.2f * std::sin(a + 1.2f) };
        p.armR = { 1.12f + 0.20f * std::sin(a + 4.3f), 0.30f, 0.45f - 0.2f * std::sin(a + 4.3f) };
        p.lean = 0.12f + hunch + (ctx.stormy ? 0.12f : 0.0f);
        p.sway = 0.05f * std::sin(a);
        p.headYaw = 0.15f * std::sin(0.4f * t); p.headPitch = -0.05f;
        p.legL = { 0.12f, 0.20f }; p.legR = { -0.10f, 0.18f };
        break;
    }

    case CrewTask::COMMAND: {
        // Pointing at something with a straight arm, the other hand on the sword: turned to the bearing the order is about.
        const float point = 0.6f + 0.4f * std::sin(1.4f * t);
        p.twist = std::clamp(m.aimYaw, -1.0f, 1.0f);
        p.armL = { 1.45f + 0.10f * point, 0.20f, 0.05f };
        p.armR = { 0.05f, 0.55f, 0.55f };
        p.headYaw = 0.5f * p.twist; p.lean = 0.04f + hunch;
        p.lift = 0.006f * std::sin(2.8f * t);
        break;
    }

    case CrewTask::TALK: {
        const float g = std::sin(2.6f * t), h = std::sin(1.7f * t + 1.0f);
        p.armL = { 0.55f + 0.45f * g, 0.35f, 0.95f + 0.3f * h };
        p.armR = { 0.20f + 0.2f * h, 0.20f, 0.6f };
        p.headYaw = 0.5f * std::sin(0.8f * t); p.headPitch = 0.08f * g;
        p.lean = 0.05f + hunch;
        break;
    }

    case CrewTask::LOOK_SEA: {
        // A hand flat against the brow, the head turning slowly along the horizon.
        p.armL = { 1.55f, 0.10f, 1.50f };
        p.armR = { 0.05f, 0.25f, 0.4f };
        p.headYaw = m.aimYaw + 0.5f * std::sin(0.55f * t); p.headPitch = -0.06f;
        p.lean = 0.04f + hunch;
        break;
    }

    case CrewTask::SCAN: {
        // Binoculars at the eyes, both elbows out, the whole body turning to sweep the horizon. A fog makes the lookout twice as eager.
        const float eager = ctx.foggy ? 1.9f : 1.0f;
        const float sweep = std::sin(0.55f * eager * t) * (ctx.foggy ? 1.15f : 0.95f);
        const float yaw = ctx.combat ? m.aimYaw + 0.12f * std::sin(1.2f * t) : sweep;
        p.twist = 0.55f * yaw;
        p.headYaw = 0.65f * yaw; p.headPitch = -0.02f + (ctx.foggy ? 0.06f : 0.0f);
        p.armL = { 1.72f, 0.30f, 1.80f }; p.armR = { 1.72f, 0.30f, 1.80f };
        p.lean = 0.06f + (ctx.foggy ? 0.13f : 0.0f) + hunch;
        p.hold = CrewHold::BINOCULARS;
        break;
    }

    case CrewTask::PULL_ROPE: {
        // Hand over hand: the arms haul alternately, the whole body rocking back with each pull.
        const float c = std::sin(3.0f * t);
        const float d = std::sin(3.0f * t + 3.14159f);
        p.armL = { 0.95f + 0.50f * c, 0.20f, 0.85f - 0.55f * c };
        p.armR = { 0.95f + 0.50f * d, 0.20f, 0.85f - 0.55f * d };
        p.lean = -0.10f + 0.28f * (0.5f + 0.5f * std::sin(3.0f * t - 0.8f)) + hunch;
        p.crouch = 0.25f;
        p.legL = { 0.35f, 0.65f }; p.legR = { -0.18f, 0.50f };
        p.headPitch = -0.15f; p.headYaw = 0.0f;
        p.hold = CrewHold::ROPE;
        p.holdParam = 0.5f + 0.5f * c;
        break;
    }

    case CrewTask::CLIMB: {
        // Hand over hand and foot over foot up a ladder: opposite arm and leg together, the body hugging the rungs.
        const float c = std::sin(m.climbPhase);
        p.lean = -0.18f;
        p.armL = { 2.15f + 0.30f * c, 0.15f, 0.55f - 0.3f * c };
        p.armR = { 2.15f - 0.30f * c, 0.15f, 0.55f + 0.3f * c };
        p.legL = { 0.95f - 0.45f * c, 1.45f - 0.75f * c };
        p.legR = { 0.95f + 0.45f * c, 1.45f + 0.75f * c };
        p.headPitch = -0.35f; p.headYaw = 0.0f;
        p.lift = 0.0f;
        break;
    }

    case CrewTask::RIG_WORK: {
        // Aloft: one arm clasped round the mast, the other reaching up to the yard, working a knot.
        const float k = std::sin(2.4f * t);
        p.armL = { 1.35f, 0.65f, 0.40f };
        p.armR = { 2.35f + 0.25f * k, 0.10f, 0.45f + 0.35f * k };
        p.lean = -0.08f; p.sway = 0.12f;
        p.headYaw = 0.0f; p.headPitch = -0.4f + 0.05f * k;
        p.legL = { 0.90f, 1.35f }; p.legR = { 0.65f, 1.15f };
        break;
    }

    case CrewTask::LOAD_GUN: {
        // The reload in three parts, driven by how far the reload has got (stage, 0..2): swab out, ball in, ram home.
        const float s = std::sin(3.4f * t);
        if (m.stage == 0) {                      // sponge: both hands on the staff, pushing in and out
            p.armL = { 1.0f + 0.40f * s, 0.15f, 0.45f }; p.armR = { 1.1f + 0.40f * s, 0.15f, 0.40f };
            p.lean = 0.18f + 0.10f * s; p.hold = CrewHold::SPONGE; p.holdParam = 0.5f + 0.5f * s;
        } else if (m.stage == 1) {               // ball: lifted from the chest to the muzzle
            p.armL = { 0.95f, 0.30f, 1.20f }; p.armR = { 0.95f, 0.30f, 1.20f };
            p.lean = 0.20f; p.hold = CrewHold::BALL;
        } else {                                 // rammer: short hard strokes
            p.armL = { 1.0f + 0.45f * s, 0.12f, 0.5f }; p.armR = { 1.1f + 0.45f * s, 0.12f, 0.45f };
            p.lean = 0.22f + 0.14f * s; p.hold = CrewHold::RAMMER; p.holdParam = 0.5f + 0.5f * s;
        }
        p.crouch = 0.2f; p.legL = { 0.35f, 0.55f }; p.legR = { -0.2f, 0.45f };
        p.headPitch = 0.18f;
        break;
    }

    case CrewTask::FIRE_GUN: {
        // Standing clear: the match held out at arm's length, the head turned away, the other hand over the ear.
        const float recoil = std::clamp(crew.cannonRecoil, 0.0f, 1.0f);
        p.armL = { 1.20f, 0.40f, 0.30f }; p.armR = { 1.45f - 0.4f * recoil, 0.35f, 0.9f };
        p.lean = -0.15f * recoil; p.headYaw = 0.9f; p.headPitch = 0.15f;
        p.crouch = 0.3f * recoil; p.legL = { 0.2f + 0.3f * recoil, 0.5f * recoil }; p.legR = { -0.15f, 0.3f };
        p.hold = CrewHold::LINSTOCK;
        break;
    }

    case CrewTask::CARRY: {
        p.armL = { 1.05f, 0.18f, 1.0f }; p.armR = { 1.05f, 0.18f, 1.0f };
        p.lean = 0.14f + hunch;
        // What is in the hands only while it is being carried (not while reaching for it).
        if (m.carrying)
            p.hold = (m.role == CrewRole::CABIN_CREW) ? CrewHold::TRAY : (m.stage >= 100 ? CrewHold::BALL : (m.site == 0 ? CrewHold::BARREL : CrewHold::CRATE));
        if (m.role == CrewRole::DECK_CREW && (m.stage == 3 || m.stage == 1)) {   // picking up or setting down: a deep crouch
            p.crouch = 0.8f; p.lean = 0.65f; p.legL = { 1.0f, 1.9f }; p.legR = { 0.9f, 1.8f };
            p.armL = { 0.7f, 0.2f, 0.4f }; p.armR = { 0.7f, 0.2f, 0.4f };
        }
        break;
    }

    case CrewTask::HAMMER: {
        const float swing = 0.5f + 0.5f * std::sin(5.2f * t);
        p.armR = { 0.6f + 1.55f * swing, 0.12f, 0.35f + 1.0f * (1.0f - swing) };
        p.armL = { 0.95f, 0.30f, 1.10f };
        p.lean = 0.22f + 0.12f * (1.0f - swing); p.crouch = 0.35f;
        p.legL = { 0.55f, 0.95f }; p.legR = { 0.10f, 0.65f };
        p.headPitch = 0.35f; p.hold = CrewHold::HAMMER; p.holdParam = swing;
        break;
    }

    case CrewTask::READ_MAP: {
        // Bent over the table: one hand moving a pair of dividers over the chart, the head down, now and then looking up.
        const float look = std::max(0.0f, std::sin(0.35f * t));
        p.lean = 0.42f - 0.2f * look; p.headPitch = 0.5f - 0.5f * look;
        p.armL = { 1.15f, 0.20f, 0.35f };
        p.armR = { 1.05f + 0.15f * std::sin(1.3f * t), 0.15f + 0.12f * std::sin(0.9f * t), 0.5f };
        p.legL = { 0.15f, 0.2f }; p.legR = { -0.05f, 0.18f };
        p.hold = CrewHold::MAP_PEN;
        break;
    }

    case CrewTask::COOK: {
        const float s = std::sin(2.2f * t);
        p.armR = { 1.05f, 0.30f + 0.18f * s, 0.55f + 0.18f * std::cos(2.2f * t) };
        p.armL = { 0.80f, 0.35f, 0.9f };
        p.lean = 0.25f; p.headPitch = 0.35f;
        p.hold = CrewHold::LADLE; p.holdParam = s;
        p.legL = { 0.18f, 0.2f }; p.legR = { -0.1f, 0.2f };
        break;
    }

    case CrewTask::SECURE: {
        // In a storm: crouched, both hands working a lashing, swaying with the ship, head down against the rain.
        const float s = std::sin(5.0f * t);
        p.crouch = 0.55f;
        p.armL = { 1.0f + 0.30f * s, 0.15f, 0.8f }; p.armR = { 1.0f - 0.30f * s, 0.15f, 0.8f };
        p.lean = 0.42f; p.headPitch = 0.35f; p.sway = 0.10f * std::sin(1.4f * t);
        p.legL = { 0.85f, 1.55f }; p.legR = { 0.65f, 1.30f };
        p.hold = CrewHold::ROPE; p.holdParam = 0.5f;
        break;
    }

    case CrewTask::READY_ARMS: {
        // A musket to the shoulder, aimed at the enemy, one eye along the barrel.
        p.twist = std::clamp(m.aimYaw, -1.2f, 1.2f);
        p.armR = { 1.55f, 0.15f, 0.55f }; p.armL = { 1.40f, 0.42f, 0.30f };
        p.lean = -0.04f; p.headYaw = 0.5f * p.twist + 0.2f; p.headPitch = 0.12f;
        p.legL = { 0.15f, 0.2f }; p.legR = { -0.30f, 0.25f };
        p.hold = CrewHold::MUSKET_AIMED;
        break;
    }

    case CrewTask::PATROL_STAND:
        p.armR = { 0.25f, 0.18f, 0.35f }; p.armL = { 0.10f, 0.15f, 0.2f };
        p.hold = CrewHold::MUSKET_GROUNDED;
        p.headYaw = 0.7f * std::sin(0.5f * t);
        break;

    case CrewTask::REST: {
        // Off duty: sitting, knees up, arms on the knees, head sunk.
        p.crouch = 1.0f;
        p.legL = { 1.45f, 1.55f }; p.legR = { 1.40f, 1.50f };
        p.armL = { 0.55f, 0.20f, 1.20f }; p.armR = { 0.55f, 0.20f, 1.20f };
        p.lean = 0.35f + 0.03f * breathe; p.headPitch = 0.45f; p.headYaw = 0.1f * std::sin(0.3f * t);
        break;
    }

    case CrewTask::PANIC: {
        const float f = std::sin(9.0f * t);
        p.armL = { 2.4f + 0.5f * f, 0.5f, 0.3f }; p.armR = { 2.4f - 0.5f * f, 0.5f, 0.3f };
        p.lean = 0.1f * f; p.lift = 0.02f * std::fabs(f);
        p.legL = { 0.4f * f, 0.4f }; p.legR = { -0.4f * f, 0.4f };
        p.headYaw = 0.6f * std::sin(5.0f * t);
        break;
    }

    default:
        break;
    }

    // Under whatever the hands are doing, the legs walk if the person is moving (carrying a barrel across the deck, hauling a rope while stepping).
    if (m.moving && m.task != CrewTask::WALK && m.task != CrewTask::CLIMB) {
        const float swing = std::sin(w);
        p.legL = { 0.55f * swing, std::max(0.0f, 0.9f * std::sin(w + 1.57f)) };
        p.legR = { -0.55f * swing, std::max(0.0f, -0.9f * std::sin(w + 1.57f)) };
        p.lift = std::max(p.lift, 0.012f * std::fabs(std::cos(w)));
        p.crouch = 0.0f;
    }

    // An officer carries a lantern in the dark.
    if (ctx.dark && (m.role == CrewRole::FIRST_MATE || m.role == CrewRole::CAPTAIN) && m.task != CrewTask::SCAN) {
        p.lampInHand = true;
        p.armL = { 0.55f, 0.18f, 1.05f };
    }
    return p;
}

// ---- the brain ------------------------------------------------------------------------------------------------------------------------

inline float crewRand(CrewMember& m)
{
    m.rng ^= m.rng << 13; m.rng ^= m.rng >> 17; m.rng ^= m.rng << 5;
    return static_cast<float>(m.rng & 0xFFFFFFu) / 16777216.0f;
}

inline float crewRange(CrewMember& m, float a, float b) { return a + (b - a) * crewRand(m); }

inline void crewSetTask(CrewMember& m, CrewTask t, float length)
{
    if (m.task != t) {
        m.task = t;
        m.taskTime = 0.0f;
    }
    m.taskLength = length;
}

inline float crewWrap(float a) { return std::atan2(std::sin(a), std::cos(a)); }

// Turn smoothly towards an angle (the short way round), at most `rate` radians a second.
inline float crewTurnTowards(float from, float to, float rate, float dt)
{
    float d = std::atan2(std::sin(to - from), std::cos(to - from));
    const float step = rate * dt;
    d = std::clamp(d, -step, step);
    return from + d;
}

// Walks a member towards `goal` at `speed`. Returns true on arrival. Faces the way it is going; its gait advances with the distance covered, so feet do not
// slide whatever the speed.
inline bool crewWalkTo(CrewMember& m, const glm::vec3& goal, float speed, float dt)
{
    glm::vec3 d = goal - m.pos;
    const float flat = std::sqrt(d.x * d.x + d.z * d.z);
    const float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (dist < 1e-5f) {
        m.pos = goal;
        return true;
    }
    const float step = std::min(speed * dt, dist);
    m.moving = true;
    m.pos += d * (step / dist);
    if (flat > 0.03f)
        m.facing = crewTurnTowards(m.facing, std::atan2(d.x, d.z), 7.0f, dt);
    m.walkPhase += step / CrewConfig::STRIDE * 6.2831853f;
    return step >= dist - 1e-5f;
}

// Moves along a ladder from `from` to `goal` (t from 0 to 1), facing the ladder, with the climbing gait running. Returns true at the top.
inline bool crewClimb(CrewMember& m, float speed, float dt)
{
    const float length = glm::length(m.goal - m.climbFrom);
    const float step = speed * dt;
    m.moving = true;
    m.climbT = std::min(1.0f, m.climbT + (length > 1e-4f ? step / length : 1.0f));
    m.pos = m.climbFrom + (m.goal - m.climbFrom) * m.climbT;
    m.climbPhase += step / 0.075f;
    return m.climbT >= 1.0f;
}

inline void crewStartClimb(CrewMember& m, const glm::vec3& to)
{
    m.climbFrom = m.pos;
    m.goal = to;
    m.climbT = 0.0f;
    // Face the ladder. A ladder is climbed facing the way it leans, and descended facing it too, which is the opposite of the way you are moving.
    const glm::vec3 d = to - m.pos;
    if (d.x * d.x + d.z * d.z > 1e-4f)
        m.facing = std::atan2(d.x, d.z) + (d.y < 0.0f ? 3.14159265f : 0.0f);
    crewSetTask(m, CrewTask::CLIMB, 1000.0f);
}

// The member in the roster at `index` is created at its post.
inline CrewMember makeCrewMember(const CrewLayout& L, const RosterEntry& e, int index, unsigned int seed)
{
    CrewMember m;
    m.role = e.role;
    m.site = e.site;
    m.variant = index + static_cast<int>(seed % 7u);
    m.rng = seed * 2654435761u + static_cast<unsigned int>(index) * 40503u + 17u;
    m.phase = 6.28f * crewHash01(m.rng);
    Station s;
    switch (e.role) {
    case CrewRole::CAPTAIN:     s = L.captain; break;
    case CrewRole::FIRST_MATE:  s = L.firstMate; break;
    case CrewRole::HELMSMAN:    s = L.helmsman; break;
    case CrewRole::LOOKOUT:     s = L.lookout; break;
    case CrewRole::SAILOR:      s = L.belay[std::min(e.site, 3)]; break;
    case CrewRole::RIGGER:      s = { L.shroud[std::min(e.site, SHIP_MAST_COUNT - 1)][0] + glm::vec3(-0.12f, 0.0f, 0.0f), 1.57f }; break;
    case CrewRole::CANNON_CREW: s = L.gunStation[std::min(e.site, 3)]; break;
    case CrewRole::DECK_CREW:   s = { L.barrelStack[e.site % 2] + glm::vec3(0.0f, 0.0f, 0.22f), 0.0f }; break;
    case CrewRole::CARPENTER:   s = L.carpenter; break;
    case CrewRole::COOK:        s = L.cook; break;
    case CrewRole::NAVIGATOR:   s = L.navigator; break;
    case CrewRole::MARINE:      s = { L.patrol[e.site % 2].node[0], 0.0f }; break;
    case CrewRole::CABIN_CREW:  s = L.steward; break;
    }
    m.pos = s.pos;
    m.facing = s.facing;
    m.goal = s.pos;
    m.taskLength = 1.0f + 3.0f * crewHash01(m.rng + 5u);
    return m;
}

inline ShipCrew makeShipCrew(const CrewLayout& L, bool fullCompany, unsigned int seed)
{
    ShipCrew c;
    const std::vector<RosterEntry> roster = crewRoster(fullCompany);
    int index = 0;
    for (const RosterEntry& e : roster)
        c.members.push_back(makeCrewMember(L, e, index++, seed));
    c.built = true;
    return c;
}

// Which jobs stop for the night: a quiet watch needs the helm, the lookout, the officers, a guard and one cannon crew; everyone else goes off duty.
inline bool crewWorksAtNight(const CrewMember& m)
{
    switch (m.role) {
    case CrewRole::CAPTAIN: case CrewRole::FIRST_MATE: case CrewRole::HELMSMAN: case CrewRole::LOOKOUT: case CrewRole::NAVIGATOR:
        return true;
    case CrewRole::MARINE:      return m.site == 0;
    case CrewRole::CANNON_CREW: return m.site == 0;
    case CrewRole::SAILOR:      return m.site == 0;
    case CrewRole::CARPENTER:   return false;
    default:                    return false;
    }
}

// Steps every member by dt seconds. This is the simple AI: for each role, what to do given the world.
inline void updateShipCrew(ShipCrew& crew, const CrewLayout& L, const CrewContext& ctx, float dt, bool enemyShip)
{
    (void)enemyShip;
    crew.time += dt;
    crew.ctx = ctx;

    // ---- ship-wide state
    crew.gear = ctx.rain > 0.25f || (crew.gear && ctx.rain > 0.08f);
    crew.cannonRecoil = std::max(0.0f, crew.cannonRecoil - 2.2f * dt);
    if (ctx.ownGunFired)
        crew.cannonRecoil = 1.0f;

    // The sails: reefed in a storm, shortened in rain, fuller in a calm; the sailors haul while they move.
    float target = 1.0f;
    if (ctx.stormy) target = 0.42f;
    else if (ctx.rainy) target = 0.80f;
    else if (ctx.foggy) target = 0.85f;
    else if (ctx.dark) target = 0.88f;
    if (ctx.combat) target = std::min(target, 0.85f);
    if (ctx.sailOrder >= 0.0f) target = std::min(target, ctx.sailOrder);          // the captain may shorten sail, never set more than the weather allows
    crew.sailTarget = target;
    const float step = CrewConfig::SAIL_RATE * dt;
    crew.hoisting = std::fabs(crew.sailSet - crew.sailTarget) > 1e-3f;
    crew.sailSet += std::clamp(crew.sailTarget - crew.sailSet, -step, step);

    // ---- damage: a hit starts a repair job at the place it struck, and flinches everyone near it
    if (ctx.newHits > 0) {
        glm::vec3 spot = ctx.hitLocal;
        spot.x = std::clamp(spot.x, -0.44f, 0.44f);
        spot.z = std::clamp(spot.z, L.workbench.z, L.foreLadder[0].z - 0.2f);
        spot.y = L.waistY;
        crew.repairSpot = spot;
        crew.repairTimer = CrewConfig::REPAIR_SECONDS;
        for (CrewMember& m : crew.members) {
            const float d = glm::length(m.pos - ctx.hitLocal);
            if (d < 3.0f)
                m.brace = std::max(m.brace, d < 1.5f ? CrewConfig::BRACE_SECONDS : 0.4f);
            if (d < 0.9f && m.role != CrewRole::CAPTAIN && crewRand(m) < 0.45f)
                m.injured = CrewConfig::INJURED_SECONDS;
        }
    }
    if (ctx.ownGunFired) {
        for (CrewMember& m : crew.members)
            if (glm::length(m.pos - L.cannonPos) < 0.9f && m.role != CrewRole::CANNON_CREW)
                m.brace = std::max(m.brace, 0.3f);
    }
    crew.repairTimer = std::max(0.0f, crew.repairTimer - dt);

    // The steering wheel turns with the helm: towards the direction of the turn, plus the small corrections a helmsman always makes.
    const float wheelWant = ctx.turnInput * 1.7f + 0.10f * std::sin(0.9f * crew.time) + (ctx.stormy ? 0.30f * std::sin(2.3f * crew.time) : 0.0f);
    crew.wheelAngle += std::clamp(wheelWant - crew.wheelAngle, -2.5f * dt, 2.5f * dt);

    // ---- each member
    for (CrewMember& m : crew.members) {
        m.brace = std::max(0.0f, m.brace - dt);
        m.injured = std::max(0.0f, m.injured - dt);
        m.taskTime += dt;
        m.moving = false;

        // How fast this person works: a storm is a hurry, rain and the dark are slower.
        float rate = 1.0f;
        if (ctx.stormy) rate = 1.45f;
        else if (ctx.rainy) rate = 0.85f;
        if (ctx.dark) rate *= 0.8f;
        m.anim += dt * rate;

        if (ctx.sinking) {
            crewSetTask(m, CrewTask::PANIC, 1000.0f);
            m.anim += dt * 1.5f;
            continue;
        }
        if (m.injured > 0.0f || m.brace > 0.0f)
            continue;                       // stunned: not working (the pose shows it)

        const float walkSpeed = (ctx.stormy ? CrewConfig::RUN_SPEED : (ctx.rainy ? 0.8f * CrewConfig::WALK_SPEED : CrewConfig::WALK_SPEED));
        const bool sleeper = ctx.dark && !crewWorksAtNight(m) && !(m.role == CrewRole::CARPENTER && crew.repairTimer > 0.0f) && !ctx.combat;

        // Anyone off duty for the night goes to the rail (on the main deck) and sits down against it; riggers aloft come down first. On the quarterdeck, the
        // poop and the forecastle, and the cook by his fire, they sit where they stand.
        if (sleeper) {
            m.offDuty = true;
            const bool aloftRigger = m.role == CrewRole::RIGGER && m.pos.y > L.waistY + 0.08f;
            if (aloftRigger) {
                if (m.task != CrewTask::CLIMB)
                    crewStartClimb(m, L.shroud[m.site % SHIP_MAST_COUNT][0] + glm::vec3(-0.05f, 0.0f, 0.0f));
                // fall through to the rigger's own code, which finishes the climb
            } else if (m.task != CrewTask::CLIMB) {
                glm::vec3 spot = m.pos;
                if (std::fabs(m.pos.y - L.waistY) < 0.05f && m.role != CrewRole::COOK)
                    spot.x = (m.pos.x >= 0.0f) ? 0.44f : -0.44f;
                if (glm::length(spot - m.pos) > 0.04f) {
                    crewWalkTo(m, spot, walkSpeed, dt);
                    crewSetTask(m, CrewTask::WALK, 1000.0f);
                } else {
                    if (m.role != CrewRole::COOK)
                        m.facing = crewTurnTowards(m.facing, m.pos.x >= 0.0f ? -1.57f : 1.57f, 3.0f, dt);
                    crewSetTask(m, CrewTask::REST, 1000.0f);
                }
                continue;
            }
        } else if (m.offDuty) {
            m.offDuty = false;
            crewSetTask(m, CrewTask::IDLE, 0.5f);
        }

        // A job that is done: pick the next. Every role has its own idea of what that is.
        const bool spellOver = m.taskTime >= m.taskLength;
        switch (m.role) {

        case CrewRole::HELMSMAN:
            crewSetTask(m, CrewTask::STEER, 1000.0f);
            m.facing = crewTurnTowards(m.facing, L.helmsman.facing, 3.0f, dt);
            break;

        case CrewRole::LOOKOUT:
            crewSetTask(m, CrewTask::SCAN, 1000.0f);
            m.aimYaw = ctx.combat ? ctx.enemyBearing : 0.0f;
            m.facing = L.lookout.facing;
            break;

        case CrewRole::CAPTAIN: {
            m.aimYaw = ctx.combat ? crewWrap(ctx.enemyBearing - m.facing) : m.aimYaw;
            if (ctx.combat) {                          // directing the fight: pointing at the enemy, then at the guns, over and over
                if (m.task != CrewTask::COMMAND && m.task != CrewTask::LOOK_SEA) crewSetTask(m, CrewTask::COMMAND, 3.0f);
                if (spellOver) crewSetTask(m, m.task == CrewTask::COMMAND ? CrewTask::LOOK_SEA : CrewTask::COMMAND, crewRange(m, 2.0f, 3.5f));
                if (m.task == CrewTask::LOOK_SEA) m.aimYaw = crewWrap(ctx.enemyBearing - m.facing);
            } else if (spellOver) {
                const float r = crewRand(m);
                if (ctx.stormy) { crewSetTask(m, CrewTask::IDLE, crewRange(m, 3.0f, 5.0f)); }
                else if (r < 0.35f) { crewSetTask(m, CrewTask::LOOK_SEA, crewRange(m, 3.5f, 6.0f)); m.aimYaw = crewRange(m, -1.1f, 1.1f); }
                else if (r < 0.65f) { crewSetTask(m, CrewTask::COMMAND, crewRange(m, 3.0f, 5.0f)); m.aimYaw = crewRange(m, -0.8f, 0.8f); }
                else crewSetTask(m, CrewTask::IDLE, crewRange(m, 3.0f, 6.0f));
            }
            m.facing = crewTurnTowards(m.facing, L.captain.facing, 2.0f, dt);
            break;
        }

        case CrewRole::FIRST_MATE: {
            // Walks a short beat on the quarterdeck, stopping to speak towards the helm or to point out the fight.
            if (m.task == CrewTask::WALK) {
                if (crewWalkTo(m, m.goal, walkSpeed, dt)) crewSetTask(m, ctx.combat ? CrewTask::COMMAND : CrewTask::TALK, crewRange(m, 3.0f, 5.5f));
            } else if (spellOver || m.task == CrewTask::IDLE) {
                const float x = crewRange(m, -0.02f, 0.46f);
                m.goal = glm::vec3(x, L.quarterY, L.firstMate.pos.z + crewRange(m, -0.14f, 0.20f));
                crewSetTask(m, CrewTask::WALK, 20.0f);
            } else {
                // Facing the captain to talk (up and to port), or the enemy to point.
                const float toward = facingToward(m.pos, L.captain.pos);
                m.facing = crewTurnTowards(m.facing, toward, 3.0f, dt);
                m.aimYaw = ctx.combat ? crewWrap(ctx.enemyBearing - m.facing) : 0.0f;       // pointing at the enemy while facing his captain
                if (m.task == CrewTask::TALK && ctx.stormy) crewSetTask(m, CrewTask::IDLE, 2.0f);
            }
            break;
        }

        case CrewRole::SAILOR: {
            // A sailor works the lines at the belaying points, one after another. In a storm they run to the lines and lash them; while the sails are
            // being raised or lowered two of them haul; in a fight one goes to mend the damage.
            const bool hauling = crew.hoisting && m.site <= 1;
            const bool repair = (ctx.combat || crew.burning) && (m.site == 1 || (crew.burning && m.site == 0)) && crew.repairTimer > 0.0f;
            if (repair) {
                if (crewWalkTo(m, crew.repairSpot + glm::vec3(0.0f, 0.0f, 0.1f), walkSpeed, dt)) {
                    crewSetTask(m, CrewTask::HAMMER, 1000.0f);
                    m.facing = crewTurnTowards(m.facing, 1.57f, 4.0f, dt);
                } else
                    crewSetTask(m, CrewTask::WALK, 1000.0f);
                break;
            }
            const auto spotOf = [&](int stage) {
                const Station& s = L.belay[(m.site + stage) % 4];
                return s.pos + glm::vec3(s.pos.x > 0.0f ? -0.10f : 0.10f, 0.0f, 0.0f);       // stands a little inboard of the pin
            };
            const bool lash = ctx.stormy && !hauling;          // while the sails are being reefed the two haulers haul, even in the gale
            const auto startWork = [&](float length) {
                crewSetTask(m, lash ? CrewTask::SECURE : CrewTask::PULL_ROPE, length);
            };
            const Station& post = L.belay[(m.site + m.stage) % 4];
            const bool working = m.task == CrewTask::PULL_ROPE || m.task == CrewTask::SECURE;
            if (m.task == CrewTask::WALK) {
                if (crewWalkTo(m, m.goal, walkSpeed, dt)) {
                    m.facing = post.facing;
                    startWork(crewRange(m, 6.0f, 11.0f));
                }
            } else if (working && spellOver) {
                m.stage = (m.stage + 1) % 4;                    // on to the next line
                m.goal = spotOf(m.stage);
                crewSetTask(m, CrewTask::WALK, 1000.0f);
            } else if (!working) {
                // Idle (or just done with something else): go to the line.
                m.goal = spotOf(m.stage);
                if (glm::length(m.goal - m.pos) < 0.04f) { m.facing = post.facing; startWork(crewRange(m, 6.0f, 11.0f)); }
                else crewSetTask(m, CrewTask::WALK, 1000.0f);
            } else {
                // Working. The weather can change the job without changing the place: haul in fair weather, lash in a gale.
                if (lash && m.task == CrewTask::PULL_ROPE) crewSetTask(m, CrewTask::SECURE, crewRange(m, 6.0f, 9.0f));
                if (!lash && m.task == CrewTask::SECURE) crewSetTask(m, CrewTask::PULL_ROPE, crewRange(m, 6.0f, 9.0f));
                m.facing = crewTurnTowards(m.facing, post.facing, 4.0f, dt);
            }
            break;
        }

        case CrewRole::RIGGER: {
            const int mast = m.site % SHIP_MAST_COUNT;
            const glm::vec3 foot = L.shroud[mast][0] + glm::vec3(-0.05f, 0.0f, 0.0f);
            const glm::vec3 head = L.shroud[mast][1];
            const bool aloft = m.pos.y > L.waistY + 0.08f;
            if (crew.hoisting) {                              // a sail order sends the rigger to the shrouds and then to the yard
                if (m.task == CrewTask::CLIMB) {
                    if (crewClimb(m, CrewConfig::CLIMB_SPEED * (ctx.rainy ? 0.7f : 1.0f), dt)) crewSetTask(m, CrewTask::RIG_WORK, 1000.0f);
                } else if (aloft) {
                    crewSetTask(m, CrewTask::RIG_WORK, 1000.0f);
                } else if (m.task == CrewTask::WALK) {
                    if (crewWalkTo(m, foot, walkSpeed, dt)) crewStartClimb(m, head);
                } else {
                    m.goal = foot;
                    crewSetTask(m, CrewTask::WALK, 1000.0f);
                }
                break;
            }
            if (ctx.stormy || ctx.combat) {                      // everyone comes down in a gale or a fight
                if (aloft && m.task != CrewTask::CLIMB) {
                    crewStartClimb(m, foot);
                } else if (m.task == CrewTask::CLIMB) {
                    if (crewClimb(m, CrewConfig::CLIMB_SPEED * 1.5f, dt)) crewSetTask(m, CrewTask::SECURE, crewRange(m, 5.0f, 8.0f));
                } else if (spellOver || m.task != CrewTask::SECURE) {
                    crewSetTask(m, CrewTask::SECURE, crewRange(m, 5.0f, 8.0f));
                }
                break;
            }
            if (m.task == CrewTask::CLIMB) {
                if (crewClimb(m, CrewConfig::CLIMB_SPEED * (ctx.rainy ? 0.7f : 1.0f), dt)) {
                    if (m.goal.y > L.waistY + 0.5f) crewSetTask(m, CrewTask::RIG_WORK, crewRange(m, 7.0f, 12.0f));
                    else crewSetTask(m, CrewTask::IDLE, crewRange(m, 3.0f, 6.0f));
                }
            } else if (m.task == CrewTask::WALK) {
                if (crewWalkTo(m, foot, walkSpeed, dt))
                    crewStartClimb(m, head);
            } else if (spellOver || m.task == CrewTask::IDLE) {
                if (aloft) crewStartClimb(m, foot);
                else if (m.task == CrewTask::IDLE && m.taskTime < m.taskLength) { /* wait out the rest */ }
                else { m.goal = foot; crewSetTask(m, CrewTask::WALK, 30.0f); }
            }
            // Hauling the sails, the rigger aloft works the yard.
            break;
        }

        case CrewRole::CANNON_CREW: {
            const Station& st = L.gunStation[std::min(m.site, 3)];
            const bool mainGun = m.site <= 1;
            m.facing = crewTurnTowards(m.facing, st.facing, 4.0f, dt);
            if (glm::length(m.pos - st.pos) > 0.05f) {
                if (crewWalkTo(m, st.pos, walkSpeed, dt)) crewSetTask(m, CrewTask::IDLE, 1.0f);
                else crewSetTask(m, CrewTask::WALK, 30.0f);
                break;
            }
            if (ctx.stormy) {
                crewSetTask(m, CrewTask::SECURE, 1000.0f);                 // lashing the gun against the roll
            } else if (ctx.combat) {
                if (mainGun) {
                    if (crew.cannonRecoil > 0.05f) crewSetTask(m, CrewTask::FIRE_GUN, 1000.0f);
                    else if (ctx.reloadFraction < 0.999f) {
                        crewSetTask(m, CrewTask::LOAD_GUN, 1000.0f);
                        m.stage = ctx.reloadFraction < 0.34f ? 0 : (ctx.reloadFraction < 0.68f ? 1 : 2);
                        if (m.site == 1 && m.stage == 0) crewSetTask(m, CrewTask::IDLE, 1000.0f);        // the gunner waits with the match
                    } else crewSetTask(m, m.site == 1 ? CrewTask::FIRE_GUN : CrewTask::IDLE, 1000.0f);
                } else {
                    // The other guns run through the same cycle on their own clock, out of step with the main gun's.
                    const float cycle = std::fmod(crew.time * 0.17f + 0.3f * static_cast<float>(m.site), 1.0f);
                    crewSetTask(m, CrewTask::LOAD_GUN, 1000.0f);
                    m.stage = cycle < 0.34f ? 0 : (cycle < 0.68f ? 1 : 2);
                }
            } else {
                // Peacetime: leaning on the gun, chatting, now and then wiping it down.
                if (spellOver) {
                    const float r = crewRand(m);
                    crewSetTask(m, r < 0.45f ? CrewTask::TALK : (r < 0.7f ? CrewTask::LOOK_SEA : CrewTask::IDLE), crewRange(m, 3.0f, 7.0f));
                    m.aimYaw = crewRange(m, -0.9f, 0.9f);
                }
            }
            break;
        }

        case CrewRole::DECK_CREW: {
            // Carries a barrel or crate between its two stacks; in a storm lashes the cargo; in a fight carries shot to the gun.
            const glm::vec3 a = (m.site == 0 ? L.barrelStack[0] : L.crateStack[0]) + glm::vec3(0.0f, 0.0f, 0.22f);
            const glm::vec3 b = (m.site == 0 ? L.barrelStack[1] : L.crateStack[1]) + glm::vec3(0.0f, 0.0f, -0.22f);
            if (ctx.stormy) {
                if (crewWalkTo(m, a, walkSpeed, dt)) crewSetTask(m, CrewTask::SECURE, 1000.0f);
                else crewSetTask(m, CrewTask::WALK, 1000.0f);
                break;
            }
            if (ctx.combat && m.site == 0) {
                // Shot from the rack to the gun and back.
                const glm::vec3 rack = L.shotRack + glm::vec3(0.0f, 0.0f, -0.16f), gun = L.gunStation[0].pos + glm::vec3(0.0f, 0.0f, 0.12f);
                if (m.stage < 100) { m.stage = 100; m.carrying = false; crewSetTask(m, CrewTask::WALK, 1000.0f); }
                if (!m.carrying) {
                    if (crewWalkTo(m, rack, walkSpeed, dt)) { m.carrying = true; crewSetTask(m, CrewTask::CARRY, 1000.0f); }
                } else {
                    if (crewWalkTo(m, gun, walkSpeed, dt)) { m.carrying = false; crewSetTask(m, CrewTask::WALK, 1000.0f); }
                }
                if (m.carrying) crewSetTask(m, CrewTask::CARRY, 1000.0f);
                break;
            }
            if (m.stage >= 100) { m.stage = 0; m.carrying = false; }
            switch (m.stage) {
            case 0:     // go to the pickup
                crewSetTask(m, CrewTask::WALK, 1000.0f);
                if (crewWalkTo(m, a, walkSpeed, dt)) { m.stage = 1; m.taskTime = 0.0f; }
                break;
            case 1:     // lift it
                crewSetTask(m, CrewTask::CARRY, 1000.0f);
                if (m.taskTime > 1.1f) { m.stage = 2; m.carrying = true; }
                break;
            case 2:     // carry it across
                crewSetTask(m, CrewTask::CARRY, 1000.0f);
                if (crewWalkTo(m, b, walkSpeed * 0.8f, dt)) { m.stage = 3; m.taskTime = 0.0f; }
                break;
            case 3:     // set it down
                crewSetTask(m, CrewTask::CARRY, 1000.0f);
                if (m.taskTime > 1.1f) { m.stage = 4; m.carrying = false; m.taskTime = 0.0f; }
                break;
            default:    // a breather, then back for the next
                crewSetTask(m, m.site == 0 ? CrewTask::TALK : CrewTask::IDLE, 1000.0f);
                if (m.taskTime > 2.5f) { m.stage = 0; }
                break;
            }
                    break;
        }

        case CrewRole::CARPENTER: {
            const bool repairing = crew.repairTimer > 0.0f;
            const glm::vec3 spot = repairing ? crew.repairSpot + glm::vec3(0.0f, 0.0f, -0.1f) : L.carpenter.pos;
            if (glm::length(m.pos - spot) > 0.04f) {
                crewWalkTo(m, spot, walkSpeed, dt);
                crewSetTask(m, CrewTask::WALK, 1000.0f);
            } else {
                m.facing = crewTurnTowards(m.facing, repairing ? 1.57f : L.carpenter.facing, 4.0f, dt);
                if (repairing || spellOver) {
                    // At the bench he alternates sawing-measuring pauses with hammering.
                    if (repairing) crewSetTask(m, CrewTask::HAMMER, 1000.0f);
                    else crewSetTask(m, m.task == CrewTask::HAMMER ? CrewTask::IDLE : CrewTask::HAMMER, crewRange(m, 4.0f, 8.0f));
                }
            }
            break;
        }

        case CrewRole::COOK: {
            m.facing = crewTurnTowards(m.facing, L.cook.facing, 3.0f, dt);
            if (spellOver || m.task == CrewTask::IDLE) {
                const float r = crewRand(m);
                crewSetTask(m, r < 0.8f ? CrewTask::COOK : CrewTask::TALK, crewRange(m, 5.0f, 10.0f));
            }
            break;
        }

        case CrewRole::NAVIGATOR: {
            // Bent over the chart; every so often he straightens and looks out over the quarterdeck rail.
            m.facing = crewTurnTowards(m.facing, L.navigator.facing, 3.0f, dt);
            if (spellOver || m.task == CrewTask::IDLE) {
                const float r = crewRand(m);
                if (ctx.combat) crewSetTask(m, CrewTask::LOOK_SEA, crewRange(m, 2.0f, 4.0f));
                else crewSetTask(m, r < 0.8f ? CrewTask::READ_MAP : CrewTask::LOOK_SEA, crewRange(m, 6.0f, 12.0f));
                m.aimYaw = 0.0f;
            }
            break;
        }

        case CrewRole::MARINE: {
            const Route& route = L.patrol[m.site % 2];
            if (ctx.combat && m.task != CrewTask::CLIMB) {          // (a marine on a ladder finishes the ladder first)
                // Action stations: to the rail on the enemy's side OF THE DECK THEY ARE ON, muskets up, faces turned to the enemy.
                const float sideX = ctx.enemyOnPositiveSide ? 1.0f : -1.0f;
                const bool onFore = std::fabs(m.pos.y - L.foreY) < 0.12f;
                const bool onQuarter = std::fabs(m.pos.y - L.quarterY) < 0.12f;
                glm::vec3 post(0.44f * sideX, L.waistY, -0.82f);
                if (onFore) post = glm::vec3(0.30f * sideX, L.foreY, L.patrol[1].node[0].z + 0.35f);
                else if (onQuarter) post = glm::vec3(0.40f * sideX, L.quarterY, L.firstMate.pos.z + 0.20f);
                if (crewWalkTo(m, post, walkSpeed * 1.2f, dt)) {
                    m.facing = crewTurnTowards(m.facing, sideX > 0 ? 1.2f : -1.2f, 4.0f, dt);
                    m.aimYaw = crewWrap(ctx.enemyBearing - m.facing);
                    crewSetTask(m, CrewTask::READY_ARMS, 1000.0f);
                } else crewSetTask(m, CrewTask::WALK, 1000.0f);
                break;
            }
            if (m.task == CrewTask::PATROL_STAND) {
                if (m.taskTime >= m.routePause) {
                    m.routeNode = (m.routeNode + 1) % route.count;
                    m.goal = route.node[m.routeNode];
                    crewSetTask(m, CrewTask::WALK, 1000.0f);
                    // the leg just left may have been a ladder
                }
            } else if (m.task == CrewTask::CLIMB) {
                if (crewClimb(m, CrewConfig::CLIMB_SPEED, dt)) { crewSetTask(m, CrewTask::PATROL_STAND, 1.0f); m.routePause = route.pause[m.routeNode]; m.taskTime = 0.0f; }
            } else if (m.task == CrewTask::WALK) {
                if (crewWalkTo(m, m.goal, walkSpeed * (ctx.dark ? 0.8f : 1.0f), dt)) {
                    // Arrived at node routeNode. If the leg from here is a ladder, climb it.
                    if (route.climbToNext[m.routeNode]) {
                        const int next = (m.routeNode + 1) % route.count;
                        m.routeNode = next;
                        crewStartClimb(m, route.node[next]);
                    } else {
                        crewSetTask(m, CrewTask::PATROL_STAND, 1.0f);
                        m.routePause = route.pause[m.routeNode];
                        m.taskTime = 0.0f;
                    }
                }
            } else {
                // First instruction (or just out of action stations): go to the nearest node.
                int best = 0; float bd = 1e9f;
                for (int i = 0; i < route.count; ++i) {
                    const float dd = glm::length(route.node[i] - m.pos);
                    if (dd < bd) { bd = dd; best = i; }
                }
                m.routeNode = best; m.goal = route.node[best];
                crewSetTask(m, CrewTask::WALK, 1000.0f);
            }
            break;
        }

        case CrewRole::CABIN_CREW: {
            // Fetches a tray from the shelf to the table and back, with a pause at each end.
            const glm::vec3 shelf = L.shelfPos + glm::vec3(0.14f, 0.0f, 0.0f), table = L.tablePos + glm::vec3(0.0f, 0.0f, -0.20f);
            if (m.stage == 0) {                                   // at rest by the table
                crewSetTask(m, CrewTask::IDLE, 1000.0f);
                if (m.taskTime > 5.0f) { m.stage = 1; m.carrying = false; }
            } else if (m.stage == 1) {                           // to the shelf
                crewSetTask(m, CrewTask::WALK, 1000.0f);
                if (crewWalkTo(m, shelf, walkSpeed * 0.7f, dt)) { m.stage = 2; m.taskTime = 0.0f; m.carrying = true; }
            } else if (m.stage == 2) {
                crewSetTask(m, CrewTask::CARRY, 1000.0f); m.facing = crewTurnTowards(m.facing, -1.57f, 4.0f, dt);
                if (m.taskTime > 1.0f) m.stage = 3;
            } else if (m.stage == 3) {                           // to the table with it
                crewSetTask(m, CrewTask::CARRY, 1000.0f);
                if (crewWalkTo(m, table + glm::vec3(0.0f, 0.0f, 0.0f), walkSpeed * 0.7f, dt)) { m.stage = 0; m.taskTime = 0.0f; m.carrying = false; }
            }
            break;
        }
        }
    }
}
