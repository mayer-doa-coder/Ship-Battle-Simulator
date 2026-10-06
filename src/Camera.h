#pragma once

// Phase 12 built the first orbit camera and drove it with the arrow keys.
// Phase 13 replaces that driver with a mouse: click and drag to orbit,
// scroll to zoom, and both angles now have real limits instead of letting
// the camera spin all the way around a pole.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

// Phase 40: a motion from where the camera is to where a preset wants it, over a fixed time.
//
// It is STATE, not a precomputed path: it stores where the motion started, where it ends and how
// much of the time has passed, and the camera's values at any moment are worked out from those
// three things. Nothing is recorded frame by frame, so this is not a keyframe table.
struct CameraEase {
    bool active = false;
    float elapsed = 0.0f;     // seconds since the motion began
    float duration = 1.0f;    // seconds the whole motion takes

    float fromYaw = 0.0f, fromPitch = 0.0f, fromRadius = 1.0f;
    glm::vec3 fromTarget = glm::vec3(0.0f);
    float toYaw = 0.0f, toPitch = 0.0f, toRadius = 1.0f;
    glm::vec3 toTarget = glm::vec3(0.0f);
};

// An orbit camera always looks at one fixed point (the origin, for now) from
// a distance called its RADIUS, turned left/right by YAW and up/down by
// PITCH. This is the same idea as a satellite circling a planet: one
// distance and two angles describe every possible position around it.
struct OrbitCamera {
    float radius = 4.0f;   // distance from the target, in world units
    float yaw = 0.0f;      // radians, turning left/right around the target
    float pitch = 0.0f;    // radians, tilting up/down toward the target

    // Phase 38: the point the camera looks at and orbits. It was the world origin,
    // fixed, in main.cpp's CameraConfig::TARGET until now. The pirate ship stands
    // away from the origin and has to be framed, so the target becomes part of the
    // camera - and the default stays (0, 0, 0) so every earlier view is unchanged.
    glm::vec3 target = glm::vec3(0.0f);

    // Phase 13: the cursor position last seen by handleOrbitCameraCursorMove.
    // Every new cursor position is compared against these to find how far
    // the mouse moved since the last event - GLFW only ever reports an
    // absolute position, never a delta, so something has to remember the
    // previous one. main() seeds this from the REAL cursor position at
    // startup, so the very first drag does not jump by whatever the cursor's
    // starting position happens to be relative to (0, 0).
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;

    // Phase 40: the motion in progress, if any. Dragging or scrolling cancels it.
    CameraEase ease;

    // Environment build: true while the mouse belongs to the on-screen control panel (the cursor is over it, or a click that began on it
    // is still held). The two callbacks below then ignore the mouse, so clicking a button does not also turn the camera and scrolling
    // over the panel does not zoom the sea behind it. main() sets it every frame; the camera itself never decides.
    bool uiCapture = false;
};

// Named orbit values, kept together so a teacher can ask for a sensitivity
// or zoom-limit change with one small edit.
namespace OrbitCameraConfig {

constexpr float MOUSE_SENSITIVITY = 0.005f;   // radians turned per pixel dragged

constexpr float ZOOM_SPEED = 0.5f;    // world units per scroll step
constexpr float MIN_RADIUS = 1.5f;    // closest the camera may zoom in
// Environment build: was 15. The five named views all sit inside that, but the new scenery stands 25 to 80 units out and 15 units
// only ever showed the ship. 30 lets the player pull back far enough to see a whole island, or the ship against a mountain range.
constexpr float MAX_RADIUS = 30.0f;   // farthest the camera may zoom out

// Beyond this many degrees up or down, the camera would pass directly over
// the target and start descending the far side - the flip Phase 12
// deliberately left in. Clamping pitch just short of a full quarter turn
// keeps that from ever happening, while still allowing an almost-overhead
// view.
constexpr float MAX_PITCH_DEGREES = 89.0f;

// Phase 40: how long a move to a preset takes, in seconds. Long enough to read as a motion and
// short enough that nobody waits on it; this is the number the viva asks you to change.
constexpr float EASE_SECONDS = 0.8f;

// Phase 41: the keyboard camera. Each is a RATE - per second of held key - so a held key turns
// the camera at the same speed whatever the frame rate. These are the numbers the viva asks you
// to change.
constexpr float KEY_YAW_DEGREES_PER_SECOND = 60.0f;     // ',' and '.'
constexpr float KEY_PITCH_DEGREES_PER_SECOND = 40.0f;   // PageUp and PageDown

// Zoom is a rate of CHANGE of distance, not a distance per second: the radius is multiplied by
// exp(+-rate * dt) each frame. At 0.9 the distance changes by a factor of 2.46 per second held,
// which crosses the whole 1.5 to 15 range in about 2.6 s. A fixed number of units per second
// would crawl when the camera is far away and lurch when it is close; a multiplicative rate feels
// the same at any distance, and two frames of dt/2 multiply to exactly one frame of dt.
constexpr float KEY_ZOOM_RATE = 0.9f;

} // namespace OrbitCameraConfig

// Turns the camera's three numbers into an actual world position. This is
// the same idea as Phase 5's rotation matrix - going from an angle to a
// position - but for a POINT ORBITING A SPHERE instead of an object turning
// on the spot.
//
//     x = radius * cos(pitch) * sin(yaw)
//     y = radius * sin(pitch)
//     z = radius * cos(pitch) * cos(yaw)
//
// At yaw = 0 and pitch = 0 this gives (0, 0, radius): straight down the +Z
// axis, exactly where the fixed camera sat in Phases 7-11. Nothing about the
// starting view changes until the mouse is actually used.
//
// 'inline' matters here because this is a header, not a .cpp file: if it
// were ever included from more than one source file, a plain function
// definition would be compiled twice and the linker would refuse to combine
// them. Shader.h never needed this because its functions are class methods
// defined inside the class body, which the compiler treats as inline
// automatically.
inline glm::vec3 orbitCameraPosition(const OrbitCamera& camera)
{
    const float x = camera.radius * std::cos(camera.pitch) * std::sin(camera.yaw);
    const float y = camera.radius * std::sin(camera.pitch);
    const float z = camera.radius * std::cos(camera.pitch) * std::cos(camera.yaw);
    return glm::vec3(x, y, z);
}

// Phase 38: where the camera actually IS in the world - the orbit offset above, moved to
// the camera's target. With the default target (0, 0, 0) this is the same number as
// orbitCameraPosition(), because adding 0.0f changes no float; that is why moving the
// target into the camera cannot disturb a single earlier render.
inline glm::vec3 orbitCameraEye(const OrbitCamera& camera)
{
    return camera.target + orbitCameraPosition(camera);
}

// ---- Phase 40: easing between presets ----------------------------------------------------

// The SHORTEST signed way round: any angle, in radians, folded into [-pi, pi).
//
// An angle and the same angle plus a whole turn are the same direction, but the camera's yaw is
// just a number that mouse drags keep adding to, so it can be 7.2 or -40. Moving it "towards"
// 35 degrees by subtracting would, from 350 degrees, swing 315 degrees the long way round
// instead of the 45 the eye sees. Folding the difference into [-pi, pi) always picks the short
// way. Phase 87 uses the same function to turn the cannon.
inline float wrapAngle(float angle)
{
    constexpr float PI = 3.14159265358979f;
    constexpr float TWO = 6.28318530717959f;
    angle = std::fmod(angle + PI, TWO);   // fmod keeps the sign of its first argument...
    if (angle < 0.0f)
        angle += TWO;                     // ...so a negative result has to be lifted back up
    return angle - PI;
}

// Begins moving the camera from where it is now to the given view. If a move is already under
// way this simply starts a new one from the CURRENT position - the camera does not jump back to
// where the first one began, and it does not wait.
//
// The yaw is first replaced by its nearest equivalent to the destination (a whole number of
// turns added or removed), which looks identical on screen and leaves a straight line between
// the two numbers that is also the short way round.
inline void startCameraEase(OrbitCamera& camera, float yaw, float pitch, float radius,
                            const glm::vec3& target, float seconds)
{
    camera.yaw = yaw + wrapAngle(camera.yaw - yaw);

    CameraEase& e = camera.ease;
    e.fromYaw = camera.yaw;
    e.fromPitch = camera.pitch;
    e.fromRadius = camera.radius;
    e.fromTarget = camera.target;
    e.toYaw = yaw;
    e.toPitch = pitch;
    e.toRadius = radius;
    e.toTarget = target;
    e.elapsed = 0.0f;
    e.duration = (seconds > 0.0001f) ? seconds : 0.0001f;   // never divide by zero
    e.active = true;
}

// Advances a move in progress by one frame. `deltaTime` is the frame's duration in seconds, so
// the camera takes the same real time to arrive at 30 frames a second as at 144.
//
// The shape is SMOOTHSTEP, s = 3t^2 - 2t^3. It runs from 0 to 1 as t does, and its slope is zero
// at both ends, so the camera starts and stops gently instead of lurching. Its steepest slope
// is 1.5 times the average, in the middle.
//
// On the frame the time runs out the camera is set to the destination EXACTLY, not to whatever
// the formula gives at t = 1, so a preset's arrival is the preset's own numbers.
inline void updateCameraEase(OrbitCamera& camera, float deltaTime)
{
    CameraEase& e = camera.ease;
    if (!e.active)
        return;

    e.elapsed += deltaTime;
    const float t = e.elapsed / e.duration;

    if (t >= 1.0f) {
        camera.yaw = e.toYaw;
        camera.pitch = e.toPitch;
        camera.radius = e.toRadius;
        camera.target = e.toTarget;
        e.active = false;
        return;
    }

    const float s = t * t * (3.0f - 2.0f * t);
    camera.yaw = e.fromYaw + (e.toYaw - e.fromYaw) * s;
    camera.pitch = e.fromPitch + (e.toPitch - e.fromPitch) * s;
    camera.radius = e.fromRadius + (e.toRadius - e.fromRadius) * s;
    camera.target = e.fromTarget + (e.toTarget - e.fromTarget) * s;
}

// ---- Phase 41: the keyboard camera ------------------------------------------------------

// Which camera keys are held RIGHT NOW. This is a level, not an edge: a held key stays true for
// every frame it is down, because the camera turns for as long as it is held (the toggles
// elsewhere in the program are the opposite - they act once, on the frame a key goes down).
struct CameraKeys {
    bool yawLeft = false;     // ','
    bool yawRight = false;    // '.'
    bool pitchUp = false;     // PageUp
    bool pitchDown = false;   // PageDown
    bool zoomIn = false;      // 'E'
    bool zoomOut = false;     // 'Q'
};

// Applies one frame of held camera keys. Returns true if any of them moved the camera.
//
//   yaw     += (right - left) * rate * dt           radians; not clamped - it can spin freely
//   pitch   += (up - down)    * rate * dt           clamped to +-89 degrees, like the mouse
//   radius  *= exp((out - in) * rate * dt)          clamped to [MIN_RADIUS, MAX_RADIUS]
//
// Multiplying by dt is what makes the speed independent of the frame rate: a frame twice as long
// turns the camera twice as far. Holding two opposite keys cancels exactly, because the two terms
// are equal and opposite.
//
// Any key that actually moves the camera also cancels a move to a preset (Phase 40), for the same
// reason a mouse drag does: the two would be driving the same numbers. (Two opposite keys held
// together move nothing, so they cancel nothing.)
inline bool applyCameraKeys(OrbitCamera& camera, const CameraKeys& keys, float deltaTime)
{
    const float yawDirection = (keys.yawRight ? 1.0f : 0.0f) - (keys.yawLeft ? 1.0f : 0.0f);
    const float pitchDirection = (keys.pitchUp ? 1.0f : 0.0f) - (keys.pitchDown ? 1.0f : 0.0f);
    const float zoomDirection = (keys.zoomOut ? 1.0f : 0.0f) - (keys.zoomIn ? 1.0f : 0.0f);

    const bool moved = yawDirection != 0.0f || pitchDirection != 0.0f || zoomDirection != 0.0f;
    if (moved)
        camera.ease.active = false;

    const float maxPitch = glm::radians(OrbitCameraConfig::MAX_PITCH_DEGREES);

    if (yawDirection != 0.0f)
        camera.yaw += yawDirection * glm::radians(OrbitCameraConfig::KEY_YAW_DEGREES_PER_SECOND) * deltaTime;

    if (pitchDirection != 0.0f) {
        camera.pitch += pitchDirection * glm::radians(OrbitCameraConfig::KEY_PITCH_DEGREES_PER_SECOND) * deltaTime;
        camera.pitch = std::clamp(camera.pitch, -maxPitch, maxPitch);
    }

    if (zoomDirection != 0.0f) {
        camera.radius *= std::exp(zoomDirection * OrbitCameraConfig::KEY_ZOOM_RATE * deltaTime);
        camera.radius = std::clamp(camera.radius, OrbitCameraConfig::MIN_RADIUS, OrbitCameraConfig::MAX_RADIUS);
    }

    return moved;
}

// A GLFW cursor-position callback: GLFW calls this itself, through
// glfwSetCursorPosCallback, every time the mouse moves - this project never
// calls it directly. Its signature is fixed by GLFW and cannot be changed.
//
// GLFW's C callbacks cannot capture C++ variables the way a lambda can, so
// there is no way to hand this function "the camera" as an ordinary
// parameter. glfwSetWindowUserPointer/glfwGetWindowUserPointer is GLFW's
// answer: main() stores a pointer to the camera on the window once, and any
// callback can retrieve it back from the SAME window it was given.
inline void handleOrbitCameraCursorMove(GLFWwindow* window, double xpos, double ypos)
{
    OrbitCamera* camera = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window));
    if (camera == nullptr)
        return;

    // How far the cursor moved since the last time this callback fired.
    const double dx = xpos - camera->lastCursorX;
    const double dy = ypos - camera->lastCursorY;
    camera->lastCursorX = xpos;
    camera->lastCursorY = ypos;

    // Only orbit while the left button is actually held - this is the
    // "click and drag" gesture, not "the camera follows the mouse
    // everywhere". Tracking the cursor position above regardless of the
    // button state is what keeps the FIRST movement of an actual drag from
    // jumping: lastCursorX/Y is always current, drag or no drag.
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        return;

    // Environment build: a drag that belongs to the control panel is not a camera drag. (The cursor position above is still tracked, so
    // the first movement after the panel is released does not jump.)
    if (camera->uiCapture)
        return;

    // Phase 40: grabbing the camera cancels a move to a preset. The camera stays exactly where
    // the move had got to, and the drag carries on from there - fighting the motion for control
    // would make the view lurch.
    camera->ease.active = false;

    camera->yaw +=static_cast<float>(dx) * OrbitCameraConfig::MOUSE_SENSITIVITY;

    // Screen y grows DOWNWARD, but dragging the mouse up should tilt the
    // camera's view up, so the pitch change is subtracted, not added.
    camera->pitch -= static_cast<float>(dy) * OrbitCameraConfig::MOUSE_SENSITIVITY;

    const float maxPitch = glm::radians(OrbitCameraConfig::MAX_PITCH_DEGREES);
    camera->pitch = std::clamp(camera->pitch, -maxPitch, maxPitch);
}

// A GLFW scroll callback, registered with glfwSetScrollCallback. 'yoffset'
// is positive when scrolling away from the user (the usual "zoom in"
// direction), so it is SUBTRACTED from radius: scrolling forward shrinks
// the distance to the target.
inline void handleOrbitCameraScroll(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    OrbitCamera* camera = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window));
    if (camera == nullptr || camera->uiCapture)
        return;

    // Phase 40: zooming takes over the radius, which a move in progress is also driving.
    camera->ease.active = false;

    // Environment build: a scroll step is 0.5 units up to a radius of 15 (exactly as before), and proportionally more beyond it, so
    // pulling back from 25 to 30 does not take ten notches.
    const float stepScale = std::max(1.0f, camera->radius / 15.0f);
    camera->radius -= static_cast<float>(yoffset) * OrbitCameraConfig::ZOOM_SPEED * stepScale;
    camera->radius = std::clamp(
        camera->radius, OrbitCameraConfig::MIN_RADIUS, OrbitCameraConfig::MAX_RADIUS);
}
