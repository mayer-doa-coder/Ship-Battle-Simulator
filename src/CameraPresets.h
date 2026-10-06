#pragma once

// Phase 39: five named camera positions for looking at the ship.
//
// Until now the camera had exactly one way to be moved: drag the mouse. That is fine for
// exploring and poor for presenting - to show the stern in a viva, someone has to orbit round to
// it and hope to stop in the right place. A PRESET is a camera position with a name that can be
// returned to by pressing one key.
//
// A preset is the four numbers an OrbitCamera already has - yaw, pitch, radius and target - and
// nothing else. Nothing new is added to the camera.
//
// WHERE THE DISTANCES ARE MEASURED IN. The radius and the target offset are stored in HULL
// LENGTHS, not world units. If they were world units, then changing ShowcaseConfig::SHIP_SCALE
// (Phase 38) would leave every preset looking at a ship that had grown or shrunk underneath
// it, and all five would have to be retyped. In hull lengths a preset keeps the ship the same
// size on screen at any scale: "1.6 hull lengths away" is 1.6 hull lengths whether the hull is
// 1.4 long or 14. presetCamera() does the one multiplication that turns them into world units.
//
// This file contains no OpenGL beyond what Camera.h already includes, and no ship: it only
// needs to be told how long the hull is and where the ship stands. That keeps it testable.

#include "Camera.h"

#include <glm/glm.hpp>

struct CameraPreset {
    const char* name;
    const char* key;                // the key that selects it, for the console message

    // Degrees. Yaw 0 is straight in front of the bow (the ship's +z points at the camera);
    // positive yaw swings the camera round towards the ship's +x side.
    float yawDegrees;
    float pitchDegrees;

    float radiusInHullLengths;      // distance from the target
    glm::vec3 targetInHullLengths;  // where it looks, as an offset from the ship's root
};

namespace CameraPresetConfig {

constexpr int COUNT = 5;

// The five views. They are DATA: a teacher who asks "make F3 look from the other side" is asking
// for one yaw to change, and nothing else has to be touched.
//
//   F1 is the default and the view every report screenshot of "the ship" is taken from: from
//      in front and to one side, a little above the water, so the bow, the near flank and the
//      sails are all in view at once.
//   F2 looks straight at the bow - the figurehead and the narrowness of the hull.
//   F3 looks straight at the near flank - the profile: stepped decks, gunports, mast spacing.
//   F4 looks straight at the stern - the sterncastle and its windows.
//   F5 looks down from high up - the deck layout.
//
// The target is a little above the waterline so that the masts and the hull are both centred;
// the elevated view aims lower, because from above the deck is the interesting part.
const CameraPreset PRESETS[COUNT] = {
    //  name                  key   yaw    pitch  radius  target (x, y, z) in hull lengths
    { "chase from astern",    "F1",  180.0f, 13.0f, 3.20f, glm::vec3(0.0f, 0.39f, 0.0f) },
    { "front",               "F2",    0.0f,  8.0f, 2.30f, glm::vec3(0.0f, 0.39f, 0.0f) },
    { "side",                "F3",   90.0f,  4.0f, 2.40f, glm::vec3(0.0f, 0.39f, 0.0f) },
    { "rear",                "F4",  180.0f, 10.0f, 2.35f, glm::vec3(0.0f, 0.39f, 0.0f) },
    { "elevated",            "F5",   35.0f, 55.0f, 2.40f, glm::vec3(0.0f, 0.26f, 0.0f) },
};

// The preset 'R' returns to, and the one the program starts in.
constexpr int DEFAULT_PRESET = 0;

} // namespace CameraPresetConfig

// Turns a preset into a camera: yaw and pitch in radians, and the two hull-length quantities
// multiplied out into world units. `hullLength` is the ship's length in the units the world is
// drawn in (so already including the showcase scale), and `shipPosition` is its root.
//
// It returns the view fields only. The cursor memory (lastCursorX/Y) belongs to the mouse and is
// left at its default here; callers that already hold a live camera copy the four view fields
// out of the result rather than replacing the whole struct.
inline OrbitCamera presetCamera(const CameraPreset& preset, float hullLength, const glm::vec3& shipPosition)
{
    OrbitCamera camera;
    camera.yaw = glm::radians(preset.yawDegrees);
    camera.pitch = glm::radians(preset.pitchDegrees);
    camera.radius = preset.radiusInHullLengths * hullLength;
    camera.target = shipPosition + preset.targetInHullLengths * hullLength;
    return camera;
}
