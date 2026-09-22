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

// An orbit camera always looks at one fixed point (the origin, for now) from
// a distance called its RADIUS, turned left/right by YAW and up/down by
// PITCH. This is the same idea as a satellite circling a planet: one
// distance and two angles describe every possible position around it.
struct OrbitCamera {
    float radius = 4.0f;   // distance from the target, in world units
    float yaw = 0.0f;      // radians, turning left/right around the target
    float pitch = 0.0f;    // radians, tilting up/down toward the target

    // Phase 13: the cursor position last seen by handleOrbitCameraCursorMove.
    // Every new cursor position is compared against these to find how far
    // the mouse moved since the last event - GLFW only ever reports an
    // absolute position, never a delta, so something has to remember the
    // previous one. main() seeds this from the REAL cursor position at
    // startup, so the very first drag does not jump by whatever the cursor's
    // starting position happens to be relative to (0, 0).
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;
};

// Named orbit values, kept together so a teacher can ask for a sensitivity
// or zoom-limit change with one small edit.
namespace OrbitCameraConfig {

constexpr float MOUSE_SENSITIVITY = 0.005f;   // radians turned per pixel dragged

constexpr float ZOOM_SPEED = 0.5f;    // world units per scroll step
constexpr float MIN_RADIUS = 1.5f;    // closest the camera may zoom in
constexpr float MAX_RADIUS = 15.0f;   // farthest the camera may zoom out

// Beyond this many degrees up or down, the camera would pass directly over
// the target and start descending the far side - the flip Phase 12
// deliberately left in. Clamping pitch just short of a full quarter turn
// keeps that from ever happening, while still allowing an almost-overhead
// view.
constexpr float MAX_PITCH_DEGREES = 89.0f;

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

    camera->yaw += static_cast<float>(dx) * OrbitCameraConfig::MOUSE_SENSITIVITY;

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
    if (camera == nullptr)
        return;

    camera->radius -= static_cast<float>(yoffset) * OrbitCameraConfig::ZOOM_SPEED;
    camera->radius = std::clamp(
        camera->radius, OrbitCameraConfig::MIN_RADIUS, OrbitCameraConfig::MAX_RADIUS);
}
