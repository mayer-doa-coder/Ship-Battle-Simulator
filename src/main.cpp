// Ship Battle Simulator - Object Scene (outside the phase plan).
//
// The step-by-step phase plan (docs/PHASE_PLAN.md) stopped after Phase 14.
// This file is a separate, requested piece of work: build the project's
// actual objects - a ship (with a cannon, a flag, and a wheel), an enemy
// ship, a five-person crew, the sun, the sea, and a few deck props - so they
// exist and can be shown to a teacher now, using only what the phases
// already built (a camera, a shader, and the Mesh class).
//
// Every object can also be shown ALONE: number keys 0-9 (and 'E' for the
// enemy ship) switch which single object is on screen, so each one can be
// inspected and demonstrated on its own instead of only as part of the full
// scene. See docs/OBJECTS_BUILD.md for the full explanation.
//
// Nothing here is lit and nothing here moves on its own. Both of those are
// real, separate ideas (lighting, then motion) that the phase plan still
// covers in order; this file only answers "what does each object look like
// and where does it sit," using flat colours exactly like every phase so
// far.
//
// Every frame still follows the same order the phase plan established:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Crew.h"
#include "Mesh.h"
#include "Props.h"
#include "Scenery.h"
#include "Shader.h"
#include "Ship.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Ship, Cannon, Crew, Sun, and Sea";
constexpr const char* VERTEX_SHADER_PATH = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_PATH = "shaders/basic.frag";

// A delayed or dragged window can create one unusually large frame time.
// Limiting dt prevents future movement from jumping a large distance at once.
constexpr float MAX_DELTA_TIME = 0.10f;

// Print one timing report per second instead of printing every frame.
constexpr float REPORT_INTERVAL = 1.0f;

// 1 enables V-sync. Change this to 0 only when measuring uncapped performance.
constexpr int VSYNC_INTERVAL = 1;

// How many samples each pixel is averaged from (MSAA - multisample
// anti-aliasing). This is what actually smooths every edge in the scene:
// without it, a diagonal or curved edge is made of hard, square pixel
// steps ("jaggies"); with it, the GPU blends a few samples per pixel along
// every edge so it looks smooth instead. 4 is a common, inexpensive choice.
constexpr int MSAA_SAMPLES = 4;

// A plain sky colour behind the objects, now that the scene is meant to look
// like a real place rather than a shader test pattern.
const glm::vec3 CLEAR_COLOR(0.53f, 0.75f, 0.90f);

} // namespace AppConfig

namespace CameraConfig {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);

// The viewing frustum: a narrow pyramid of visible space with its point at
// the camera. FIELD_OF_VIEW_DEGREES sets how wide that pyramid opens.
// NEAR_PLANE and FAR_PLANE cut off anything closer or farther than that -
// the sun sits about 12 units away, so FAR_PLANE must comfortably clear that.
constexpr float FIELD_OF_VIEW_DEGREES = 45.0f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 100.0f;

} // namespace CameraConfig

// Everything about the objects that are NOT the ship or the crew member -
// those two have their own files, Ship.h and Crew.h, because each is built
// from several named parts. The sea and the sun are each just one shape, so
// they stay here. src/Props.h holds the smaller, standalone deck props.
namespace SceneConfig {

// How detailed the round shapes are. Raised well above Mesh.h's own
// defaults (16 segments, 12x16 stacks/slices) for a visibly smoother,
// rounder mast, barrel, cannonball, head, and sun - the "make it look like
// HD" request. More segments means more triangles for the same shape; at
// this project's scene size (a handful of objects) the cost is negligible.
constexpr int CYLINDER_SEGMENTS = 32;
constexpr int SPHERE_STACKS = 24;
constexpr int SPHERE_SLICES = 32;

// The sea: a very flat, wide box. Its top face sits exactly at y = 0, which
// is also where the ship's hull rests (see ShipShape::HULL_HEIGHT in Ship.h)
// AND where every mountain's base sits (drawMountain(), src/Scenery.h) - the
// water has to reach at least as far out as the mountain ring's own outer
// edge (MOUNTAIN_RING_RADIUS + half a mountain's width, below), or there is
// a ring of bare nothing between the shoreline and the mountains instead of
// the mountains appearing to rise out of the sea.
constexpr float WATER_WIDTH = 130.0f;
constexpr float WATER_LENGTH = 130.0f;
constexpr float WATER_THICKNESS = 0.06f;
const glm::vec3 WATER_COLOR(0.10f, 0.35f, 0.55f);

// The sun: a bright sphere, fixed high in the sky, built from TWO
// concentric spheres (a duller outer body and a brighter inner core) rather
// than one flat colour - a cheap way to make it read as glowing rather than
// as a plain painted ball, with no lighting involved.
//
// Its X position is deliberately modest (5, not far out toward a screen
// edge): a sphere viewed from near the edge of a wide perspective view is
// stretched into an oval by the projection itself - a real property of
// perspective projection, not a rendering mistake - so keeping the sun
// closer to the middle of the default view keeps it reading as a circle.
const glm::vec3 SUN_POSITION(5.0f, 6.0f, -8.0f);
constexpr float SUN_RADIUS = 1.2f;
constexpr float SUN_CORE_RADIUS = SUN_RADIUS * 0.65f;
const glm::vec3 SUN_COLOR(1.0f, 0.80f, 0.20f);
const glm::vec3 SUN_CORE_COLOR(1.0f, 0.96f, 0.75f);

// Where the player's ship sits, and where the enemy ship sits. The enemy
// ship is turned 180 degrees so its bow faces the player's, instead of both
// ships pointing the same way.
const glm::vec3 SHIP_POSITION(0.0f, 0.0f, 0.0f);
const glm::vec3 ENEMY_SHIP_POSITION(6.5f, 0.0f, -3.5f);

// Five crew members, each at their own deck post, each in their own colour
// so a teacher can tell them apart at a glance even with no roles or
// animation yet (Stage K gives them both, later).
constexpr int CREW_COUNT = 5;
// Every position below is chosen so no crew member's X falls within a
// sail's own width (+-SAIL_WIDTH / 2 either side of X = 0) unless their Z
// is also comfortably clear of that sail's mast - the two ways a head can
// end up visually fused with a sail, both fixed here by keeping well away
// from at least one of them.
const glm::vec3 CREW_LOCAL_POSITIONS[CREW_COUNT] = {
    glm::vec3(-0.75f, ShipShape::DECK_TOP_Y, 2.50f),    // on the quarterdeck, near the wheel
    glm::vec3(0.75f, ShipShape::DECK_TOP_Y, 2.45f),     // also near the wheel
    glm::vec3(0.35f, ShipShape::DECK_TOP_Y, -0.55f),    // at the cannon
    glm::vec3(0.35f, ShipShape::DECK_TOP_Y, -1.35f),    // near the foremast
    glm::vec3(-0.35f, ShipShape::DECK_TOP_Y, 0.75f),    // midship
};
const glm::vec3 CREW_SHIRT_COLORS[CREW_COUNT] = {
    glm::vec3(0.25f, 0.35f, 0.65f),   // blue
    glm::vec3(0.55f, 0.18f, 0.18f),   // red
    glm::vec3(0.20f, 0.45f, 0.25f),   // green
    glm::vec3(0.35f, 0.35f, 0.40f),   // grey
    glm::vec3(0.55f, 0.42f, 0.25f),   // tan
};

// The three deck props, on the opposite side of the deck from the cannon.
const glm::vec3 BARREL_LOCAL_POSITION(-0.75f, ShipShape::DECK_TOP_Y, -0.50f);
const glm::vec3 CRATE_LOCAL_POSITION(-0.75f, ShipShape::DECK_TOP_Y, -0.95f);
const glm::vec3 CANNONBALL_LOCAL_POSITION(0.90f, ShipShape::DECK_TOP_Y, -0.55f);

// Background scenery: a ring of mountains running all the way around the
// scene (not just behind the ships, so there is a horizon whichever way the
// camera ends up facing after an orbit), a few drifting clouds, and rocks
// breaking the water's surface. None of these are individually selectable
// with a view key (see ViewMode below) - they are backdrop, not objects a
// teacher would ask to inspect up close - but they are real, positioned,
// reusable-mesh objects like everything else here.
// The ring's radius has to clear the FARTHEST the camera can ever get from
// the world origin, or an orbit and a zoom-out can fly the camera straight
// into a mountain, filling the whole screen with one flat colour - exactly
// what happened during testing before this comment was written. The
// worst case is the "All" view's own look-at point (a few units from the
// origin) plus the camera's own MAX_RADIUS (Camera.h, 25) - roughly 29 - so
// the ring sits well beyond that, with room to spare.
constexpr int MOUNTAIN_RING_COUNT = 12;
constexpr float MOUNTAIN_RING_RADIUS = 45.0f;
constexpr float MOUNTAIN_RING_RADIUS_VARIATION = 5.0f;   // how uneven the ring's distance is
constexpr float MOUNTAIN_BASE_WIDTH = 14.0f;
constexpr float MOUNTAIN_BASE_HEIGHT = 9.0f;
constexpr float MOUNTAIN_SIZE_VARIATION = 4.0f;           // how much taller/shorter peaks get
constexpr float MOUNTAIN_DEPTH = 12.0f;

constexpr int CLOUD_COUNT = 3;
const glm::vec3 CLOUD_POSITIONS[CLOUD_COUNT] = {
    glm::vec3(-6.0f, 8.0f, -10.0f),
    glm::vec3(8.0f, 9.0f, -13.0f),
    glm::vec3(1.0f, 7.5f, 7.0f),
};
constexpr float CLOUD_SCALES[CLOUD_COUNT] = { 2.2f, 2.6f, 1.8f };

constexpr int ROCK_COUNT = 3;
const glm::vec3 ROCK_POSITIONS[ROCK_COUNT] = {
    glm::vec3(-4.0f, 0.0f, 5.0f),
    glm::vec3(9.0f, 0.0f, 3.0f),
    glm::vec3(-7.5f, 0.0f, -6.0f),
};
constexpr float ROCK_SCALES[ROCK_COUNT] = { 0.5f, 0.4f, 0.45f };

} // namespace SceneConfig

// Every object that can be shown, either together (All) or alone. Pressing
// the matching number key switches to that one; 'E' switches to the enemy
// ship. See processInput().
enum class ViewMode {
    All = 0,
    Ship,
    Cannon,
    Flag,
    Crew,
    Sun,
    Water,
    Barrel,
    Crate,
    Cannonball,
    EnemyShip
};

// What the camera should look at, how far back it should start, and how far
// down it should tilt, for one ViewMode. Switching object needs a different
// look-at point and a different starting distance - a cannonball and the
// whole ship are not usefully framed from the same spot - and a flat object
// like the sea shows almost nothing at a level (0 degree) angle, since a
// perfectly flat surface viewed edge-on is just a thin line.
struct ViewPreset {
    glm::vec3 target;
    float radius;
    float pitchDegrees = 0.0f;
};

static ViewPreset viewPresetFor(ViewMode mode)
{
    using namespace ShipShape;

    switch (mode) {
        case ViewMode::All:
            // Midway between the two ships, pulled back far enough to see
            // both at once.
            return { glm::vec3(3.0f, 1.3f, -1.7f), 14.0f };
        case ViewMode::Ship:       return { glm::vec3(0.0f, 1.5f, 0.0f), 9.0f };
        case ViewMode::EnemyShip:  return { glm::vec3(0.0f, 1.5f, 0.0f), 9.0f };
        case ViewMode::Cannon:     return { glm::vec3(MOUNT_X, DECK_TOP_Y + 0.35f, MOUNT_Z), 1.4f };
        case ViewMode::Flag:       return { glm::vec3(0.0f, DECK_TOP_Y + MAST_HEIGHT + 0.13f, MIZZEN_MAST_Z), 1.0f };
        case ViewMode::Crew:       return { glm::vec3(0.0f, 0.5f, 0.0f), 1.2f };
        case ViewMode::Sun:        return { glm::vec3(0.0f, 0.0f, 0.0f), 3.5f };
        // Tilted down 40 degrees - a level view of a flat surface is just an
        // edge-on line, so this is the one object that needs to be looked
        // AT from above to show anything useful at all.
        case ViewMode::Water:      return { glm::vec3(0.0f, 0.0f, 0.0f), 10.0f, 40.0f };
        case ViewMode::Barrel:     return { glm::vec3(0.0f, 0.15f, 0.0f), 0.8f };
        // Tilted down slightly so the crossed lid straps - flat on top of
        // the crate - are visible instead of edge-on.
        case ViewMode::Crate:      return { glm::vec3(0.0f, 0.11f, 0.0f), 0.75f, 25.0f };
        case ViewMode::Cannonball: return { glm::vec3(0.0f, 0.09f, 0.0f), 0.5f };
    }
    return { glm::vec3(0.0f), 5.0f };
}

static const char* viewModeName(ViewMode mode)
{
    switch (mode) {
        case ViewMode::All:        return "All (the whole scene)";
        case ViewMode::Ship:       return "Ship";
        case ViewMode::EnemyShip:  return "Enemy ship";
        case ViewMode::Cannon:     return "Cannon";
        case ViewMode::Flag:       return "Flag";
        case ViewMode::Crew:       return "Crew";
        case ViewMode::Sun:        return "Sun";
        case ViewMode::Water:      return "Water";
        case ViewMode::Barrel:     return "Barrel";
        case ViewMode::Crate:      return "Crate";
        case ViewMode::Cannonball: return "Cannonball";
    }
    return "?";
}

// Time values needed by one frame. Keeping them together makes it clear which
// time is absolute and which value describes only the previous frame.
struct FrameClock {
    float now = 0.0f;
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
};

// Small counters used only for the once-per-second console report.
struct FrameStats {
    float reportStartTime = 0.0f;
    int renderedFrames = 0;
};

// Data that changes as the scene changes. updateScene() writes it and
// renderScene() reads it, so neither function needs to know about the other.
struct SceneState {
    OrbitCamera camera;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    // Which single object is being shown, and where the camera should be
    // looking. Both start at their real values only once main() calls
    // applyViewPreset() for ViewMode::All - see the comment there for why a
    // default member initializer here is not enough.
    ViewMode viewMode = ViewMode::All;
    glm::vec3 viewTarget = glm::vec3(0.0f);

    // Toggled by the 'W' key: false is the normal solid view, true switches
    // to line-only rendering so the hierarchy and the round shapes'
    // triangles can be inspected directly.
    bool wireframeEnabled = false;
    bool wireframeKeyWasDown = false;
};

// Applies one ViewMode's preset to the camera: what it looks at, how far
// back it starts, and how far it tilts. Used both for switching object
// during play and for setting up the very first frame's camera in main() -
// a default member initializer on SceneState could not do this instead,
// since OrbitCamera's own radius/yaw/pitch are declared before viewMode and
// viewTarget, in a field SceneState does not own the definition of.
static void applyViewPreset(SceneState& scene, ViewMode mode)
{
    const ViewPreset preset = viewPresetFor(mode);
    scene.viewMode = mode;
    scene.viewTarget = preset.target;
    scene.camera.radius = preset.radius;
    scene.camera.yaw = 0.0f;
    scene.camera.pitch = glm::radians(preset.pitchDegrees);
}

// Switches to a new object, unless it is already the one being shown -
// shared by every key that can switch object (the number keys and 'E'), so
// there is only one place that decides what "switching object" means.
static void switchViewMode(SceneState& scene, ViewMode newMode)
{
    if (newMode == scene.viewMode)
        return;

    applyViewPreset(scene, newMode);
    std::printf("[view] %s\n", viewModeName(newMode));
}

static void glfwErrorCallback(int errorCode, const char* description)
{
    std::fprintf(stderr, "[GLFW error %d] %s\n", errorCode, description);
}

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    // OpenGL draws into the framebuffer, whose pixel size can differ from the
    // window size on high-DPI displays. Updating the viewport prevents stretching.
    glViewport(0, 0, width, height);
}

static void processInput(GLFWwindow* window, SceneState& scene)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // The camera is driven by the two mouse callbacks in src/Camera.h, which
    // GLFW calls directly whenever the mouse actually moves or scrolls -
    // there is nothing for processInput() to poll for it every frame.

    // 'W' toggles wireframe. The "was it already down" check stops a held
    // key from flipping the state roughly 120 times a second.
    const bool wireframeKeyIsDown = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    if (wireframeKeyIsDown && !scene.wireframeKeyWasDown) {
        scene.wireframeEnabled = !scene.wireframeEnabled;
        std::printf("[wireframe] %s\n", scene.wireframeEnabled ? "ON" : "OFF");
    }
    scene.wireframeKeyWasDown = wireframeKeyIsDown;

    // Number keys 0-9 each pick one ViewMode. Holding a key just keeps
    // re-selecting the SAME mode every frame, which is harmless - unlike
    // 'W' above, this needs no "was it already down" edge detection, since
    // setting a value to what it already is changes nothing.
    for (int digit = 0; digit <= 9; ++digit) {
        if (glfwGetKey(window, GLFW_KEY_0 + digit) == GLFW_PRESS) {
            switchViewMode(scene, static_cast<ViewMode>(digit));
            break;
        }
    }

    // 'E' shows the enemy ship alone - it needs a letter key, since the
    // ten ViewModes above already use every digit.
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        switchViewMode(scene, ViewMode::EnemyShip);
}

static void startClock(FrameClock& clock)
{
    clock.now = static_cast<float>(glfwGetTime());
    clock.lastFrameTime = clock.now;
    clock.deltaTime = 0.0f;
}

static void updateClock(FrameClock& clock)
{
    clock.now = static_cast<float>(glfwGetTime());

    const float measuredDelta = clock.now - clock.lastFrameTime;
    clock.deltaTime = std::min(measuredDelta, AppConfig::MAX_DELTA_TIME);
    clock.lastFrameTime = clock.now;
}

// Nothing in this scene moves yet, so the only per-frame scene data is the
// camera's view and projection matrices - the projection is rebuilt every
// frame because it depends on the window's current aspect ratio, which can
// change if the window is resized.
static void updateScene(SceneState& scene, int framebufferWidth, int framebufferHeight)
{
    // orbitCameraPosition() (src/Camera.h) returns an OFFSET from radius,
    // yaw, and pitch - it says nothing about what point that offset is
    // measured from. Phases 12-13 only ever looked at one fixed point near
    // the origin, so adding it was never needed to get a correct picture.
    // Now that different objects are looked at from very different points
    // (the flag sits over two units up; a cannonball sits a few centimetres
    // off the ground), the offset must be added to THIS object's own
    // viewTarget, not left to orbit the origin regardless of where the
    // object actually is.
    const glm::vec3 eye = scene.viewTarget + orbitCameraPosition(scene.camera);
    scene.view = glm::lookAt(eye, scene.viewTarget, CameraConfig::UP);

    const int safeHeight = std::max(framebufferHeight, 1);
    const float aspectRatio =
        static_cast<float>(framebufferWidth) / static_cast<float>(safeHeight);
    scene.projection = glm::perspective(
        glm::radians(CameraConfig::FIELD_OF_VIEW_DEGREES),
        aspectRatio,
        CameraConfig::NEAR_PLANE,
        CameraConfig::FAR_PLANE);
}

// Where the mouse is "pointing at" in the 3D scene, at one particular
// height (planeY). This is how a 2D mouse position ever becomes a 3D
// coordinate at all: the mouse does not point at a single spot in 3D on
// its own (a flat screen has no depth), so instead this builds the entire
// LINE the mouse is looking along - from right behind the screen glass, to
// far into the distance - and then asks where that line crosses one flat,
// horizontal plane at height 'planeY'. That crossing point is the answer.
struct MouseWorldPoint {
    glm::vec3 position = glm::vec3(0.0f);
    bool valid = false;   // false if the mouse is looking along the plane, or away from it
};

static MouseWorldPoint mouseToWorldPoint(
    GLFWwindow* window,
    int framebufferWidth,
    int framebufferHeight,
    const glm::mat4& view,
    const glm::mat4& projection,
    float planeY)
{
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // glm::unProject expects window coordinates measured with Y growing
    // UPWARD from the window's bottom edge; GLFW reports the cursor with Y
    // growing DOWNWARD from the top, so it has to be flipped here first.
    const float flippedY = static_cast<float>(framebufferHeight) - static_cast<float>(mouseY);
    const glm::vec4 viewport(0.0f, 0.0f, static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));

    // Un-projecting the SAME (x, y) screen position at two different depths
    // (z = 0, the near plane, and z = 1, the far plane) gives two points in
    // the 3D world - and the straight line through both of them is exactly
    // the line the mouse is looking along.
    const glm::vec3 nearPoint = glm::unProject(glm::vec3(mouseX, flippedY, 0.0f), view, projection, viewport);
    const glm::vec3 farPoint = glm::unProject(glm::vec3(mouseX, flippedY, 1.0f), view, projection, viewport);
    const glm::vec3 rayDirection = glm::normalize(farPoint - nearPoint);

    MouseWorldPoint result;

    // If the line is (almost) perfectly level, it never reaches a
    // different height at all, so it either never crosses 'planeY' or lies
    // flat along it everywhere - neither gives one useful answer.
    if (std::fabs(rayDirection.y) < 0.0001f)
        return result;

    const float distanceAlongLine = (planeY - nearPoint.y) / rayDirection.y;

    // A negative distance would mean the crossing point is BEHIND the
    // camera - looking up and away from a low plane, for instance - which
    // is not somewhere the mouse could actually be "pointing at".
    if (distanceAlongLine < 0.0f)
        return result;

    result.position = nearPoint + rayDirection * distanceAlongLine;
    result.valid = true;
    return result;
}

// Shows where the mouse is pointing, live, in the window's own title bar -
// the same "HUD via window title" technique this project has used since
// its very first frame-timing report, just applied to a second piece of
// information now. Two heights are shown together: y = 0 (sea level, and
// also every object's own natural "base" - a hull's bottom, a crew
// member's feet - since every isolated single-object view draws that
// object with its root sitting at the world origin), and whatever height
// the camera is currently looking at (scene.viewTarget.y), which is
// usually a more useful reading for a part that sits well above the
// ground, like the flag or the wheel.
static void updateMouseCoordinateTitle(
    GLFWwindow* window,
    const SceneState& scene,
    int framebufferWidth,
    int framebufferHeight)
{
    const MouseWorldPoint atGround = mouseToWorldPoint(
        window, framebufferWidth, framebufferHeight, scene.view, scene.projection, 0.0f);
    const MouseWorldPoint atTarget = mouseToWorldPoint(
        window, framebufferWidth, framebufferHeight, scene.view, scene.projection, scene.viewTarget.y);

    char groundText[64];
    if (atGround.valid) {
        std::snprintf(groundText, sizeof(groundText), "X=%6.2f Z=%6.2f", atGround.position.x, atGround.position.z);
    } else {
        std::snprintf(groundText, sizeof(groundText), "(not looking at this height)");
    }

    char targetText[64];
    if (atTarget.valid) {
        std::snprintf(targetText, sizeof(targetText), "X=%6.2f Z=%6.2f", atTarget.position.x, atTarget.position.z);
    } else {
        std::snprintf(targetText, sizeof(targetText), "(not looking at this height)");
    }

    char title[320];
    std::snprintf(
        title, sizeof(title),
        "%s | Mouse at y=0.00: %s | at y=%5.2f: %s",
        AppConfig::WINDOW_TITLE,
        groundText,
        static_cast<double>(scene.viewTarget.y),
        targetText);

    glfwSetWindowTitle(window, title);
}

// The sea, drawn at whatever root it is given - the world origin in every
// current use, but kept as a parameter for the same reason every other
// draw function here takes one: nothing assumes it always sits at (0, 0, 0).
static void drawWater(ShaderProgram& shader, const Mesh& unitCube, const glm::mat4& root)
{
    const glm::mat4 frame = glm::scale(
        glm::translate(root, glm::vec3(0.0f, -SceneConfig::WATER_THICKNESS * 0.5f, 0.0f)),
        glm::vec3(SceneConfig::WATER_WIDTH, SceneConfig::WATER_THICKNESS, SceneConfig::WATER_LENGTH));
    shader.setMat4("uModel", frame);
    shader.setVec3("uTint", SceneConfig::WATER_COLOR);
    unitCube.draw();
}

// The sun: an outer sphere and a smaller, brighter inner sphere sharing the
// same centre - see SceneConfig::SUN_POSITION's comment for why it sits
// where it does, and why two spheres instead of one.
static void drawSun(ShaderProgram& shader, const Mesh& unitSphere, const glm::mat4& root)
{
    shader.setMat4("uModel", glm::scale(root, glm::vec3(SceneConfig::SUN_RADIUS * 2.0f)));
    shader.setVec3("uTint", SceneConfig::SUN_COLOR);
    unitSphere.draw();

    shader.setMat4("uModel", glm::scale(root, glm::vec3(SceneConfig::SUN_CORE_RADIUS * 2.0f)));
    shader.setVec3("uTint", SceneConfig::SUN_CORE_COLOR);
    unitSphere.draw();
}

// Where the 'index'-th mountain out of 'count' evenly spaced around a full
// circle sits, and how big it is. A small amount of size and distance
// variation - based on the index alone, through sin/cos, not a random
// number generator - keeps every mountain from being an identical copy
// spaced in a perfect, obviously mechanical circle, while still producing
// the exact same scene on every run.
struct MountainPlacement {
    glm::vec3 position;
    float width;
    float height;
    float depth;
};

static MountainPlacement mountainRingPlacement(int index, int count)
{
    const float angle = 360.0f * static_cast<float>(index) / static_cast<float>(count);
    const float angleRad = glm::radians(angle);

    const float radius = SceneConfig::MOUNTAIN_RING_RADIUS
        + std::sin(angleRad * 3.0f) * SceneConfig::MOUNTAIN_RING_RADIUS_VARIATION;
    const float sizeFactor = std::cos(angleRad * 5.0f);

    MountainPlacement placement;
    placement.position = glm::vec3(radius * std::cos(angleRad), 0.0f, radius * std::sin(angleRad));
    placement.width = SceneConfig::MOUNTAIN_BASE_WIDTH + sizeFactor * SceneConfig::MOUNTAIN_SIZE_VARIATION;
    placement.height = SceneConfig::MOUNTAIN_BASE_HEIGHT + sizeFactor * (SceneConfig::MOUNTAIN_SIZE_VARIATION * 0.6f);
    placement.depth = SceneConfig::MOUNTAIN_DEPTH;
    return placement;
}

// The shader is no longer passed as const: setting a uniform changes the
// shader program, so this function can no longer promise to leave it alone.
static void renderScene(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitCylinder,
    const Mesh& unitSphere,
    const Mesh& hullShape,
    const Mesh& mountainShape,
    const SceneState& scene)
{
    glClearColor(
        AppConfig::CLEAR_COLOR.r,
        AppConfig::CLEAR_COLOR.g,
        AppConfig::CLEAR_COLOR.b,
        1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use() first: a uniform is written into whichever program is currently
    // in use, so setting it before use() would send the value nowhere.
    shader.use();
    shader.setMat4("uView", scene.view);
    shader.setMat4("uProjection", scene.projection);

    // 'W' switches to line-only rendering so the round shapes' triangles and
    // the hierarchy's separate parts can be inspected directly.
    glPolygonMode(GL_FRONT_AND_BACK, scene.wireframeEnabled ? GL_LINE : GL_FILL);

    const glm::mat4 origin(1.0f);
    const glm::mat4 shipRoot = glm::translate(glm::mat4(1.0f), SceneConfig::SHIP_POSITION);

    switch (scene.viewMode) {
        case ViewMode::All: {
            drawWater(shader, unitCube, glm::mat4(1.0f));
            drawSun(shader, unitSphere, glm::translate(glm::mat4(1.0f), SceneConfig::SUN_POSITION));

            // Background scenery: a full ring of mountains all the way
            // around the scene, drifting clouds, and rocks breaking the
            // water's surface.
            for (int i = 0; i < SceneConfig::MOUNTAIN_RING_COUNT; ++i) {
                const MountainPlacement placement =
                    mountainRingPlacement(i, SceneConfig::MOUNTAIN_RING_COUNT);
                drawMountain(
                    shader, mountainShape, unitSphere,
                    glm::translate(glm::mat4(1.0f), placement.position),
                    placement.width, placement.height, placement.depth);
            }
            for (int i = 0; i < SceneConfig::CLOUD_COUNT; ++i) {
                drawCloud(
                    shader, unitSphere,
                    glm::translate(glm::mat4(1.0f), SceneConfig::CLOUD_POSITIONS[i]),
                    SceneConfig::CLOUD_SCALES[i]);
            }
            for (int i = 0; i < SceneConfig::ROCK_COUNT; ++i) {
                drawRock(
                    shader, unitSphere,
                    glm::translate(glm::mat4(1.0f), SceneConfig::ROCK_POSITIONS[i]),
                    SceneConfig::ROCK_SCALES[i]);
            }

            // The player's ship, its crew, and its deck props.
            drawShip(shader, unitCube, unitCylinder, unitSphere, hullShape, shipRoot);
            for (int i = 0; i < SceneConfig::CREW_COUNT; ++i) {
                drawCrewMember(
                    shader, unitCube, unitSphere,
                    glm::translate(shipRoot, SceneConfig::CREW_LOCAL_POSITIONS[i]),
                    SceneConfig::CREW_SHIRT_COLORS[i]);
            }
            drawBarrel(shader, unitCylinder, glm::translate(shipRoot, SceneConfig::BARREL_LOCAL_POSITION));
            drawCrate(shader, unitCube, glm::translate(shipRoot, SceneConfig::CRATE_LOCAL_POSITION));
            drawCannonball(shader, unitSphere, glm::translate(shipRoot, SceneConfig::CANNONBALL_LOCAL_POSITION));

            // The enemy ship, turned to face the player's.
            const glm::mat4 enemyShipRoot = glm::rotate(
                glm::translate(glm::mat4(1.0f), SceneConfig::ENEMY_SHIP_POSITION),
                glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            drawShip(shader, unitCube, unitCylinder, unitSphere, hullShape, enemyShipRoot, ShipShape::ENEMY_PALETTE);
            break;
        }
        case ViewMode::Ship:
            drawShip(shader, unitCube, unitCylinder, unitSphere, hullShape, origin);
            break;
        case ViewMode::EnemyShip:
            drawShip(shader, unitCube, unitCylinder, unitSphere, hullShape, origin, ShipShape::ENEMY_PALETTE);
            break;
        case ViewMode::Cannon:
            drawCannon(shader, unitCube, unitCylinder, origin);
            break;
        case ViewMode::Flag:
            drawFlag(shader, unitCube, origin);
            break;
        case ViewMode::Crew:
            drawCrewMember(shader, unitCube, unitSphere, origin);
            break;
        case ViewMode::Sun:
            drawSun(shader, unitSphere, origin);
            break;
        case ViewMode::Water:
            drawWater(shader, unitCube, origin);
            break;
        case ViewMode::Barrel:
            drawBarrel(shader, unitCylinder, origin);
            break;
        case ViewMode::Crate:
            drawCrate(shader, unitCube, origin);
            break;
        case ViewMode::Cannonball:
            drawCannonball(shader, unitSphere, origin);
            break;
    }
}

static void reportFrame(FrameStats& stats, const FrameClock& clock)
{
    ++stats.renderedFrames;

    const float reportDuration = clock.now - stats.reportStartTime;
    if (reportDuration < AppConfig::REPORT_INTERVAL)
        return;

    const float averageFps = static_cast<float>(stats.renderedFrames) / reportDuration;
    const float averageDelta = reportDuration / static_cast<float>(stats.renderedFrames);

    std::printf(
        "[frame] time=%7.2f s  dt=%7.4f s  average dt=%7.4f s  fps=%6.1f\n",
        clock.now,
        clock.deltaTime,
        averageDelta,
        averageFps);
    std::fflush(stdout);

    stats.reportStartTime = clock.now;
    stats.renderedFrames = 0;
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (glfwInit() != GLFW_TRUE) {
        std::fprintf(stderr, "Failed to initialize GLFW.\n");
        return 1;
    }

    // Ask the driver for modern OpenGL 3.3 Core.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Ask for a multisampled framebuffer - see AppConfig::MSAA_SAMPLES.
    // This must be requested before the window is created; the framebuffer
    // it produces cannot be changed afterward.
    glfwWindowHint(GLFW_SAMPLES, AppConfig::MSAA_SAMPLES);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        AppConfig::WINDOW_WIDTH,
        AppConfig::WINDOW_HEIGHT,
        AppConfig::WINDOW_TITLE,
        nullptr,
        nullptr);

    if (window == nullptr) {
        std::fprintf(stderr, "Failed to create the OpenGL window.\n");
        glfwTerminate();
        return 1;
    }

    // OpenGL functions belong to a context, so the context must be current first.
    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        std::fprintf(stderr, "Failed to load OpenGL functions with GLAD.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    if (GLAD_GL_VERSION_3_3 == 0) {
        std::fprintf(stderr, "OpenGL 3.3 is not available on this computer.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(AppConfig::VSYNC_INTERVAL);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Depth testing stays on always now - the scene has real, overlapping 3D
    // geometry (the ship sits in front of and above the sea, the crew member
    // stands in front of the mast, and so on), so this is no longer an
    // on/off teaching toggle the way it was in the phase plan.
    //
    // Face culling stays OFF: it is a performance optimisation, not a
    // correctness requirement, and this scene's thin parts (sails, the flag)
    // and round parts (masts, the barrel, the sun, the head) are far more
    // useful to see from every angle while still being built and tuned than
    // to have half-invisible by default.
    glEnable(GL_DEPTH_TEST);

    // Turns on the multisampling the window was created with above -
    // without this, the extra samples GLFW allocated are never blended, and
    // edges stay just as hard-edged as before.
    glEnable(GL_MULTISAMPLE);

    std::printf("OpenGL   : %s\n", glGetString(GL_VERSION));
    std::printf("GLSL     : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::printf("Renderer : %s\n", glGetString(GL_RENDERER));

    ShaderProgram shader;
    if (!shader.loadFromFiles(
            AppConfig::VERTEX_SHADER_PATH,
            AppConfig::FRAGMENT_SHADER_PATH)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Five reusable unit meshes. Every object in the scene - the sea, the
    // sun, both ships, every crew member, every prop, and the background
    // scenery - is built from just these five, scaled and tinted
    // differently per part. Nothing else creates its own geometry.
    //
    // 'hullShape' (makeUnitShipHull(), src/Mesh.h) is shaped like a boat:
    // pinched to a point at the bow, full width at its widest point, a
    // flat stern, and narrower at the keel than at the deck line.
    //
    // 'mountainShape' reuses makeUnitTaperedBox() - the same generator an
    // earlier version of this file used for the hull - but with a much
    // more extreme taper and drawn upside down (see drawMountain(),
    // src/Scenery.h), turning "narrow at the bottom" into "wide base,
    // narrow peak".
    Mesh unitCube = makeUnitCube();
    Mesh unitCylinder = makeCylinder(SceneConfig::CYLINDER_SEGMENTS);
    Mesh unitSphere = makeSphere(SceneConfig::SPHERE_STACKS, SceneConfig::SPHERE_SLICES);
    Mesh hullShape = makeUnitShipHull(ShipShape::HULL_BOTTOM_SCALE);
    Mesh mountainShape = makeUnitTaperedBox(0.12f);
    if (!unitCube.valid() || !unitCylinder.valid() || !unitSphere.valid()
        || !hullShape.valid() || !mountainShape.valid()) {
        std::fprintf(stderr, "Failed to create the reusable meshes.\n");
        mountainShape.destroy();
        hullShape.destroy();
        unitSphere.destroy();
        unitCylinder.destroy();
        unitCube.destroy();
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf(
        "Ready. Drag with the left mouse button to orbit, scroll to zoom. "
        "Press W for wireframe. Press 0 for the whole scene, or view one "
        "object alone: 1 Ship, 2 Cannon, 3 Flag, 4 Crew, 5 Sun, 6 Water, "
        "7 Barrel, 8 Crate, 9 Cannonball, E Enemy ship. Press ESC to close.\n");

    SceneState scene;
    applyViewPreset(scene, ViewMode::All);

    // Give the two mouse callbacks in src/Camera.h a way to reach this
    // camera - see the Phase 13 explanation for why this indirection exists.
    glfwSetWindowUserPointer(window, &scene.camera);
    glfwGetCursorPos(window, &scene.camera.lastCursorX, &scene.camera.lastCursorY);
    glfwSetCursorPosCallback(window, handleOrbitCameraCursorMove);
    glfwSetScrollCallback(window, handleOrbitCameraScroll);

    FrameClock clock;
    startClock(clock);

    FrameStats stats;
    stats.reportStartTime = clock.now;

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        updateClock(clock);
        processInput(window, scene);

        // Read the CURRENT framebuffer size every frame, not just once at
        // startup, so uProjection keeps a correct aspect ratio if the window
        // is resized while the program runs.
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        updateScene(scene, framebufferWidth, framebufferHeight);
        updateMouseCoordinateTitle(window, scene, framebufferWidth, framebufferHeight);
        renderScene(shader, unitCube, unitCylinder, unitSphere, hullShape, mountainShape, scene);

        glfwSwapBuffers(window);
        glfwPollEvents();

        reportFrame(stats, clock);
    }

    // OpenGL resources must be deleted while the context still exists.
    mountainShape.destroy();
    hullShape.destroy();
    unitSphere.destroy();
    unitCylinder.destroy();
    unitCube.destroy();
    shader.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
