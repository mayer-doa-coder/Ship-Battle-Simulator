#pragma once

// Environment build: the CAMERA VIEWS. The original orbit camera that follows the ship stays as the CHASE view; four more are added (Environment.h lists them):
//
//   FREE          a detached camera that flies anywhere - over the ship, round the island, down under the sea. The ship is not steered while it is in use.
//   FIRST PERSON  a sailor's eyes, standing on the forecastle and looking out over the bow. The eye is a point in the ship's own frame, so it pitches and rolls
//                 with the ship on the waves (the horizon tilts with the deck, but at only 60% of the ship's tilt: a head and a neck steady the picture a little).
//   CINEMATIC     a film camera that works by itself. Five shots (an orbit, a low bow shot, a crane shot, a tracking shot from the side, a chase from astern) each
//                 nine seconds long, each a closed form of the shot's own clock; when the two ships are close it cuts to a wide shot of the duel instead. The camera
//                 eases to each new shot instead of jumping to it.
//   CAPTAIN       through the captain's own eyes. The eye is at his head and looks where he looks - along the deck at the helm, at the sea when he scans it, at the
//                 enemy when he directs the fight (his head turn is read from the crew animation, src/Crew.h).
//
// In FIRST PERSON and CAPTAIN the mouse (drag) and the keys , . PgUp PgDn look round from there; in FREE they turn the camera. There is no OpenGL here: a view is
// just an eye, a target, an up vector and a field of view.

#include "Crew.h"
#include "Environment.h"
#include "Ship.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

struct ViewResult {
    glm::vec3 eye = glm::vec3(0.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 45.0f;
};

namespace ViewConfig {
constexpr float FREE_SPEED = 9.0f;               // units per second; Shift makes it 3x
constexpr float FREE_FAST = 3.0f;
constexpr float FREE_MAX_RANGE = 130.0f;         // how far from the player's ship the free camera may go
constexpr float FOV_FREE = 55.0f, FOV_FIRST = 64.0f, FOV_CAPTAIN = 62.0f, FOV_CINEMATIC = 36.0f;
constexpr float ROLL_KEPT = 0.6f;                // how much of the ship's roll the first-person horizon keeps
constexpr float SHOT_SECONDS = 9.0f;
constexpr float CINEMATIC_EASE = 1.3f;           // 1 / seconds: how quickly the film camera settles on a new shot
constexpr float CINEMATIC_EYE_SPEED = 5.5f;      // units a second: the film camera never moves, or swings its aim, faster than this
constexpr float CINEMATIC_TARGET_SPEED = 4.5f;
constexpr float CAPTAIN_TURN_RATE = 1.1f;        // radians a second: the fastest the captain's view may turn by itself
constexpr float DUEL_RANGE = 75.0f;              // ships closer than this get the wide duel shot
constexpr float MIN_HEIGHT_ABOVE_SEA = 0.9f;     // the film camera does not dip into the waves
}

// A direction from two look angles. Yaw turns RIGHT as it grows and pitch looks UP as it grows - the way a mouse drag feels - and both are zero looking along +z.
inline glm::vec3 viewLookDirection(float yaw, float pitch)
{
    return glm::vec3(-std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
}

inline void viewAnglesOf(const glm::vec3& dir, float& yaw, float& pitch)
{
    pitch = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
    yaw = std::atan2(-dir.x, dir.z);
}

// Moves the free camera. `forward`, `right`, `up` are -1..1 inputs; movement follows where the camera LOOKS (so flying forward while looking down dives).
inline glm::vec3 stepFreeCamera(const glm::vec3& pos, float yaw, float pitch, float forward, float right, float up, bool fast, float dt)
{
    const glm::vec3 dir = viewLookDirection(yaw, pitch);
    const glm::vec3 rightVec = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float speed = ViewConfig::FREE_SPEED * (fast ? ViewConfig::FREE_FAST : 1.0f);
    return pos + (dir * forward + rightVec * right + glm::vec3(0.0f, 1.0f, 0.0f) * up) * (speed * dt);
}

inline ViewResult freeView(const glm::vec3& pos, float yaw, float pitch)
{
    ViewResult v;
    v.eye = pos;
    v.target = pos + viewLookDirection(yaw, pitch);
    v.fovDegrees = ViewConfig::FOV_FREE;
    return v;
}

// A view from a point fixed in the ship. `hull` is the hull's frame (so the point rides the waves), `localEye` the point in it, `facing` the way the viewer faces in the
// ship (0 = towards the bow, positive towards +x), and `lookYaw` / `lookPitch` the extra turn from the mouse or the keys.
inline ViewResult shipBorneView(const glm::mat4& hull, const glm::vec3& localEye, float facing, float lookYaw, float lookPitch, float fov)
{
    const float theta = facing - lookYaw;                                          // a turn to the right reduces the ship-frame bearing
    const glm::vec3 local(std::sin(theta) * std::cos(lookPitch), std::sin(lookPitch), std::cos(theta) * std::cos(lookPitch));
    ViewResult v;
    v.eye = glm::vec3(hull * glm::vec4(localEye, 1.0f));
    const glm::vec3 dir = glm::normalize(glm::vec3(hull * glm::vec4(local, 0.0f)));
    v.target = v.eye + dir;
    const glm::vec3 shipUp = glm::normalize(glm::vec3(hull * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    v.up = glm::normalize(glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), shipUp, ViewConfig::ROLL_KEPT));
    v.fovDegrees = fov;
    return v;
}

// The first-person eye: on the main deck at the foot of the forecastle, on the centreline, at the height of a standing sailor - high enough to see over the
// forecastle's rail and out over the bow, and far enough from the lanterns on its foremost posts that they are small.
inline glm::vec3 firstPersonEye(const CrewLayout& L, const ShipDimensions& d)
{
    const DeckLevelDimensions& fore = d.deckLevels[DECK_FORECASTLE];
    return glm::vec3(-0.02f, L.waistY + 0.43f, fore.zFrom - 0.85f);
}

// The captain's eye and the way he is looking: from his head, a little ahead of his face (so the hat's brim and his own head are behind the camera), turned by his
// own body and head as the crew animation has them.
inline void captainEye(const CrewMember& captain, const CrewContext& ctx, const ShipCrew& crew, glm::vec3& eyeLocal, float& facing, float& pitch)
{
    const CrewPose pose = crewPoseOf(captain, ctx, crew);
    const CrewLook look = crewLookFor(CrewRole::CAPTAIN, captain.variant, crew.gear, false);
    facing = captain.facing - pose.twist * 0.5f - pose.headYaw * 0.33f;
    pitch = -pose.headPitch * 0.5f;
    const float eyeHeight = look.height * (CrewBody::TORSO + CrewBody::THIGH + CrewBody::SHIN + CrewBody::NECK + 0.6f * CrewBody::HEAD) + pose.lift * look.height;
    eyeLocal = captain.pos + glm::vec3(std::sin(captain.facing) * 0.06f, eyeHeight - 0.02f * pose.lean * 10.0f, std::cos(captain.facing) * 0.06f);
}

// ---- the film camera ----------------------------------------------------------------------------------------------------------------

struct CinematicState {
    bool started = false;
    glm::vec3 eye = glm::vec3(0.0f), target = glm::vec3(0.0f);
    float duelUntil = -1.0f;      // the wide duel shot is held until this time (a hysteresis: once a fight starts the camera does not flick in and out of it)
    float duelSide = 0.0f;        // which side of the line between the ships the wide shot stands on, chosen once when it begins
    bool inDuel = false;
};

// Where shot `shot` wants the camera `u` (0..1) of the way through it. `pos` and `heading` are the ship's; `y0` the sea level (shots are measured from it).
inline void cinematicShot(int shot, float u, const glm::vec3& pos, float heading, float y0, glm::vec3& eye, glm::vec3& target)
{
    const glm::vec3 fwd(std::sin(heading), 0.0f, std::cos(heading));
    const glm::vec3 left(std::cos(heading), 0.0f, -std::sin(heading));
    const auto bearing = [](float a) { return glm::vec3(std::sin(a), 0.0f, std::cos(a)); };
    const glm::vec3 base(pos.x, y0, pos.z);
    switch (shot % 5) {
    case 0: {   // a slow orbit from astern round to the quarter
        const float a = heading + 3.14159f * 0.8f + 1.2f * u;
        eye = base + bearing(a) * 13.0f + glm::vec3(0.0f, 5.5f - 1.5f * u, 0.0f);
        target = base + glm::vec3(0.0f, 2.6f, 0.0f);
        break;
    }
    case 1:     // low over the water off the bow, looking back along the ship
        eye = base + fwd * (11.0f - 3.0f * u) + left * (5.0f + 3.0f * u) + glm::vec3(0.0f, 1.6f, 0.0f);
        target = base - fwd * 0.5f + glm::vec3(0.0f, 2.6f, 0.0f);
        break;
    case 2: {   // a crane shot, high on the port side and coming down
        const float a = heading + 1.5708f + 0.5f * u;
        eye = base + bearing(a) * (18.0f - 4.0f * u) + glm::vec3(0.0f, 16.0f - 7.0f * u, 0.0f);
        target = base + glm::vec3(0.0f, 2.0f, 0.0f);
        break;
    }
    case 3:     // tracking alongside on the starboard side, sliding from astern to ahead
        eye = base - left * 16.0f + fwd * (-6.0f + 12.0f * u) + glm::vec3(0.0f, 3.2f, 0.0f);
        target = base + fwd * 1.5f + glm::vec3(0.0f, 2.2f, 0.0f);
        break;
    default:    // a chase from astern, closing in, looking past the ship at where it is going
        eye = base - fwd * (11.0f - 3.0f * u) + left * (2.0f - 2.0f * u) + glm::vec3(0.0f, 4.5f - 1.2f * u, 0.0f);
        target = base + fwd * 7.0f + glm::vec3(0.0f, 3.0f, 0.0f);
        break;
    }
}

// The wide shot of a duel: the camera stands off to one side of the line between the ships (`side` is +1 or -1, fixed for the whole fight), high, looking at the middle.
inline void cinematicDuel(float side, const glm::vec3& a, const glm::vec3& b, float y0, glm::vec3& eye, glm::vec3& target)
{
    const glm::vec3 mid = 0.5f * (a + b);
    const glm::vec3 line = glm::vec3(b.x - a.x, 0.0f, b.z - a.z);
    const float len = std::max(glm::length(line), 1.0f);
    const glm::vec3 across = glm::vec3(-line.z, 0.0f, line.x) / len;
    eye = glm::vec3(mid.x, y0, mid.z) + across * side * (22.0f + 0.35f * len) + glm::vec3(0.0f, 9.0f + 0.18f * len, 0.0f);
    target = glm::vec3(mid.x, y0 + 1.5f, mid.z);
}

// Moves `cur` towards `want`: a smooth exponential approach (the same at any frame rate), but never faster than `maxSpeed` units a second. The cap is what stops a
// far-away new shot from whipping the camera across the sea in a few frames.
inline void cinematicApproach(glm::vec3& cur, const glm::vec3& want, float maxSpeed, float dt)
{
    const glm::vec3 d = want - cur;
    const float len = glm::length(d);
    if (len < 1e-5f)
        return;
    const float step = std::min(len * (1.0f - std::exp(-ViewConfig::CINEMATIC_EASE * dt)), maxSpeed * dt);
    cur += d * (step / len);
}

// One frame of the film camera. The wanted eye and target come from the current shot; the camera EASES towards them with a speed limit, which turns a change of shot
// into a smooth swing. `duelWanted` says a fight is on; the wide shot is entered on the first such frame, held for at least seven seconds after the last, and always seen
// from the same side.
inline ViewResult stepCinematic(CinematicState& st, float time, float dt, const glm::vec3& shipPos, float heading, const glm::vec3& otherPos, bool duelWanted,
                                float y0, float minEyeY)
{
    if (duelWanted)
        st.duelUntil = time + 7.0f;
    const bool duel = time < st.duelUntil;
    const int shot = static_cast<int>(std::floor(time / ViewConfig::SHOT_SECONDS));
    const float u = time / ViewConfig::SHOT_SECONDS - static_cast<float>(shot);
    glm::vec3 eye, target;
    if (duel) {
        if (!st.inDuel) {                                         // a fight has just begun: take the side the camera is already nearer to
            const glm::vec3 mid = 0.5f * (shipPos + otherPos);
            const glm::vec3 line = glm::vec3(otherPos.x - shipPos.x, 0.0f, otherPos.z - shipPos.z);
            const glm::vec3 across = glm::vec3(-line.z, 0.0f, line.x);
            st.duelSide = (glm::dot(across, st.eye - mid) >= 0.0f) ? 1.0f : -1.0f;
        }
        cinematicDuel(st.duelSide, shipPos, otherPos, y0, eye, target);
    } else {
        cinematicShot(shot, u, shipPos, heading, y0, eye, target);
    }
    st.inDuel = duel;
    eye.y = std::max(eye.y, minEyeY);
    if (!st.started) {
        st.eye = eye;
        st.target = target;
        st.started = true;
    } else {
        cinematicApproach(st.eye, eye, ViewConfig::CINEMATIC_EYE_SPEED, dt);
        cinematicApproach(st.target, target, ViewConfig::CINEMATIC_TARGET_SPEED, dt);
    }
    ViewResult v;
    v.eye = st.eye;
    v.target = st.target;
    v.fovDegrees = ViewConfig::FOV_CINEMATIC;
    return v;
}

// ---- the captain's view, smoothed -------------------------------------------------------------------------------------------------------
//
// The captain's body and head change their pose the instant his task changes (from looking at the sea to pointing at an order), which turns a camera in his head by
// up to a radian in a single frame. What the player wants is the captain's VIEW, not his twitches: the direction is eased towards his, at no more than 1.1 radians a
// second, and only half of his body twist and a third of his head turn are followed.
struct CaptainViewState {
    bool started = false;
    float facing = 0.0f, pitch = 0.0f;
    glm::vec3 eye = glm::vec3(0.0f);
};

inline void stepCaptainView(CaptainViewState& st, const glm::vec3& eyeLocal, float rawFacing, float rawPitch, float dt)
{
    if (!st.started) {
        st.facing = rawFacing; st.pitch = rawPitch; st.eye = eyeLocal;
        st.started = true;
        return;
    }
    st.facing = crewTurnTowards(st.facing, rawFacing, ViewConfig::CAPTAIN_TURN_RATE, dt);
    st.pitch += std::clamp(rawPitch - st.pitch, -ViewConfig::CAPTAIN_TURN_RATE * dt, ViewConfig::CAPTAIN_TURN_RATE * dt);
    st.eye += (eyeLocal - st.eye) * (1.0f - std::exp(-10.0f * dt));
}
