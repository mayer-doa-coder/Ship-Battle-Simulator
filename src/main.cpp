// Ship Battle Simulator - Phase 13: mouse-driven orbit, and real limits.
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// Phase 12's arrow keys are gone. In their place: click and drag to orbit,
// scroll to zoom, both handled by two GLFW callbacks living in src/Camera.h.
// Pitch is now clamped to 89 degrees each way, so the flip Phase 12
// deliberately left in can never happen again, and radius is clamped too, so
// scrolling cannot zoom through the target or vanish into the distance.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 13: Mouse Orbit";
constexpr const char* VERTEX_SHADER_PATH = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_PATH = "shaders/basic.frag";

// A delayed or dragged window can create one unusually large frame time.
// Limiting dt prevents future movement from jumping a large distance at once.
constexpr float MAX_DELTA_TIME = 0.10f;

// Print one timing report per second instead of printing every frame.
constexpr float REPORT_INTERVAL = 1.0f;

// 1 enables V-sync. Change this to 0 only when measuring uncapped performance.
constexpr int VSYNC_INTERVAL = 1;

// GLM is used here so Phase 0 verifies that the math library is configured too.
const glm::vec3 CLEAR_COLOR(0.82f, 0.66f, 0.04f);

// Phase 3: a colour filter sent to the fragment shader every frame as the
// uniform 'uTint'. Each vertex colour is multiplied by it.
//   (1, 1, 1)    leaves the triangle exactly as its vertex colours describe;
//   below 1      dims that channel;
//   above 1      brightens it, up to the display limit of 1.
// This is the easiest viva value in the phase: change it, rebuild, and the
// triangle changes colour without any edit to shaders/basic.frag.
const glm::vec3 TINT(0.6f, 2.4f, 3.0f);
} // namespace AppConfig

namespace CameraConfig {

// Phase 7 gave the camera a fixed position, EYE. Phase 12 replaced that with
// a real OrbitCamera (src/Camera.h), driven by the mouse since Phase 13, so
// only the point it looks at and which way is "up" stay as constants here.
const glm::vec3 TARGET(0.0f, 0.0f, 0.0f);    // the point it looks at
const glm::vec3 UP(0.0f, 1.0f, 0.0f);        // which way is "up" for this camera

// The viewing frustum: a narrow pyramid of visible space with its point at
// the camera. FIELD_OF_VIEW_DEGREES sets how wide that pyramid opens.
// NEAR_PLANE and FAR_PLANE cut off anything closer or farther than that -
// nothing outside this range is ever drawn, which is why both must comfortably
// contain the triangle's whole depth range (Phase 7 moves it between 2.5 and
// 5.5 units from EYE).
constexpr float FIELD_OF_VIEW_DEGREES = 45.0f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 100.0f;

} // namespace CameraConfig

namespace TriangleConfig {

// Each row is: position x/y/z, then colour red/green/blue.
// These are easy viva values: edit positions to reshape/move the triangle and
// edit colours to change its three corners.
constexpr float VERTICES[] = {
     0.40f,  0.37f, 0.0f,   0.1f, 0.1f, 0.1f,
    -0.40f,  0.37f, 0.0f,   0.1f, 0.1f, 0.1f,
     0.00f, -0.73f, 0.0f,   0.1f, 0.1f, 1.0f
};

constexpr int VERTEX_COUNT = 3;
constexpr int FLOATS_PER_VERTEX = 6;
constexpr int POSITION_COMPONENTS = 3;
constexpr int COLOR_COMPONENTS = 3;

} // namespace TriangleConfig

namespace TriangleMotion {

// Phase 4: how the triangle slides. The x offset at any moment is
//     offset = SLIDE_DISTANCE * sin(SLIDE_SPEED * now)
// so it swings between -SLIDE_DISTANCE and +SLIDE_DISTANCE.
// The visible x range is -1 to +1 and the triangle is 0.4 wide on each side of
// its centre, so a distance above 0.6 pushes part of it off the screen.
constexpr float SLIDE_DISTANCE = 0.5f;   // how far each way, in clip-space units
constexpr float SLIDE_SPEED = 1.0f;      // radians per second; one full swing is 2*pi / this

} // namespace TriangleMotion

namespace TriangleSpin {

// Phase 5: how the triangle turns. The angle at any moment is
//     angle = SPIN_SPEED * now
// measured in RADIANS. A full turn is 2*pi = 6.28 radians, so a speed of 2.0
// completes one turn in about 3.14 seconds. A positive angle turns
// counter-clockwise when the axis points toward the viewer.
//
// The turn happens around the model-space ORIGIN (0, 0), which is not exactly
// the visual middle of this triangle (its corners average to y = +0.13), so the
// triangle wobbles a little as it turns. To spin exactly on its middle, edit
// TriangleConfig::VERTICES so the three y values add up to zero.
constexpr float SPIN_SPEED = 0.5f;                        // radians per second
const glm::vec3 SPIN_AXIS(0.0f, 0.0f, 1.0f);              // Z: straight out of the screen

} // namespace TriangleSpin

namespace TriangleScale {

// Phase 6: how the triangle's size pulses. The scale factor at any moment is
//     factor = mid + amp * sin(PULSE_SPEED * now)
// where mid is halfway between PULSE_MIN and PULSE_MAX, and amp is half their
// difference. This keeps the factor inside [PULSE_MIN, PULSE_MAX] without a
// clamp. A factor of 1.0 is the triangle's original size; below 1.0 shrinks
// it, above 1.0 enlarges it.
constexpr float PULSE_MIN = 0.5f;     // smallest size, as a fraction of the original
constexpr float PULSE_MAX = 1.3f;     // largest size, as a fraction of the original
constexpr float PULSE_SPEED = 1.2f;   // radians per second

} // namespace TriangleScale

namespace TriangleDepth {

// Phase 7: how the triangle drifts toward and away from the camera. The world
// z position at any moment is
//     z = DEPTH_AMPLITUDE * sin(DEPTH_SPEED * now)
// which swings between -DEPTH_AMPLITUDE and +DEPTH_AMPLITUDE. The camera
// starts at a distance of 4.0 (OrbitCamera's default radius, Phase 12), so at
// that starting view the triangle's actual distance from the camera swings
// between (4 - DEPTH_AMPLITUDE) when it is nearest and (4 + DEPTH_AMPLITUDE)
// when it is farthest. Orbiting the camera changes the real distance, since
// the camera itself can move now - these two numbers describe the ORIGINAL
// fixed view, not a promise that holds from every angle.
//
// This is the phase's real demonstration. Before Phase 7, changing an
// object's z did nothing useful: with no view or projection matrix, z never
// affected how big anything looked, only whether it was clipped away. Now the
// SAME triangle visibly grows as it nears the camera and shrinks as it
// recedes, because uProjection performs a genuine perspective divide.
constexpr float DEPTH_AMPLITUDE = 1.5f;   // world units nearer/farther than TARGET
constexpr float DEPTH_SPEED = 0.8f;       // radians per second

} // namespace TriangleDepth

namespace DepthTestConfig {

// Phase 8: a second draw of the SAME triangle mesh, shifted this much
// FARTHER from the camera than the first. The camera sits at positive z
// looking toward the origin (CameraConfig), so a NEGATIVE z shift moves an
// object farther away.
//
// This value is fixed, not animated, on purpose: the demonstration only
// works if one copy is reliably nearer than the other at every instant,
// regardless of where TriangleDepth's oscillation happens to be right now.
constexpr float FAR_COPY_Z_OFFSET = -1.2f;   // world units farther than the near copy

// A strong, easily distinguished colour for the far copy, sent as 'uTint'
// just like the near copy's AppConfig::TINT. No second mesh or second vertex
// buffer is needed - the same VAO is bound and drawn twice with a different
// uModel and a different uTint.
const glm::vec3 FAR_COPY_TINT(3.0f, 0.5f, 0.4f);

} // namespace DepthTestConfig

namespace QuadConfig {

// Phase 9: 4 UNIQUE corners, one row each: position x/y/z, then colour r/g/b.
// A different colour on each corner makes the shared diagonal easy to see,
// and makes a wrong index (see INDICES below) obvious: the colours would no
// longer blend the way they are supposed to.
//   0: top-left (red)  1: top-right (green)
//   3: bottom-left (yellow)  2: bottom-right (blue)
constexpr float VERTICES[] = {
    -0.4f,  0.4f, 0.0f,   1.0f, 0.0f, 0.0f,   // 0: top-left,     red
     0.4f,  0.4f, 0.0f,   0.5f, 1.0f, 0.0f,   // 1: top-right,    green
     0.4f, -0.4f, 0.0f,   0.1f, 0.7f, 1.0f,   // 2: bottom-right, blue
    -0.4f, -0.4f, 0.0f,   1.0f, 1.0f, 0.9f,   // 3: bottom-left,  yellow
};

// Two triangles, sharing the diagonal that runs from corner 2 to corner 0.
// Corners 0 and 2 are each named ONCE in VERTICES above but used TWICE here -
// that reuse is the entire point of indexed drawing. Listed the old way,
// without indices, this quad would need 6 rows of vertex data (36 floats)
// with corners 0 and 2 typed out twice each. This way it needs 4 rows
// (24 floats) plus these 6 small integers.
constexpr unsigned int INDICES[] = {
    0, 3, 2,   // top-left, bottom-left, bottom-right
    2, 1, 0,   // bottom-right, top-right, top-left
};

constexpr int VERTEX_COUNT = 4;
constexpr int INDEX_COUNT = 6;
constexpr int FLOATS_PER_VERTEX = 6;
constexpr int POSITION_COMPONENTS = 3;
constexpr int COLOR_COMPONENTS = 3;

// Phase 9: the quad does not move. It sits to one side, at the same distance
// from the camera as the other two triangles rest at, so it is easy to find
// and does not overlap them. Its ONLY job is to prove indexed drawing works;
// giving it motion too would blur that one idea with Phases 4-7's.
const glm::vec3 POSITION(-1.8f, 0.0f, 0.0f);

} // namespace QuadConfig

namespace CubeConfig {

// Phase 10: the cube's size. It is the ONLY dimension this cube has - every
// face reaches exactly this far from the centre on every axis, so the cube
// stays a true cube. Doubling it makes the cube twice as wide, tall, AND
// deep at once. Giving width, height, and depth their own separate values
// waits until Phase 16's reusable, parameterised mesh generators.
constexpr float HALF_SIZE = 0.5f;

// 24 vertices - 4 for EACH of the 6 faces, not 8 shared corners. A real cube
// only has 8 corners, but a corner where three faces meet cannot share one
// vertex between those faces here, because each face needs its OWN flat
// colour, and a shared vertex can only carry one colour. Paying for that
// with 24 vertices instead of 8 is a small, deliberate cost.
//
// Every face lists its 4 corners in the same order the quad already used:
// a CCW (counter-clockwise) loop AS SEEN FROM OUTSIDE the cube, which is
// what GL_CULL_FACE needs to keep a face visible instead of discarding it.
constexpr float VERTICES[] = {
    // +Z face (front, facing the camera) - blue
    -HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   0.0f, 0.0f, 1.0f,
     HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   0.0f, 0.0f, 1.0f,
     HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   0.0f, 0.0f, 1.0f,
    -HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   0.0f, 0.0f, 1.0f,

    // -Z face (back) - yellow
     HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   1.0f, 1.0f, 0.0f,
    -HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   1.0f, 1.0f, 0.0f,
    -HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   1.0f, 1.0f, 0.0f,
     HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   1.0f, 1.0f, 0.0f,

    // +X face (right) - red
     HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   1.0f, 0.0f, 0.0f,
     HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   1.0f, 0.0f, 0.0f,
     HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   1.0f, 0.0f, 0.0f,
     HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   1.0f, 0.0f, 0.0f,

    // -X face (left) - cyan
    -HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   0.0f, 1.0f, 1.0f,
    -HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   0.0f, 1.0f, 1.0f,
    -HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   0.0f, 1.0f, 1.0f,
    -HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   0.0f, 1.0f, 1.0f,

    // +Y face (top) - green
    -HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   0.0f, 1.0f, 0.0f,
     HALF_SIZE,  HALF_SIZE,  HALF_SIZE,   0.0f, 1.0f, 0.0f,
     HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   0.0f, 1.0f, 0.0f,
    -HALF_SIZE,  HALF_SIZE, -HALF_SIZE,   0.0f, 1.0f, 0.0f,

    // -Y face (bottom) - magenta
    -HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   1.0f, 0.0f, 1.0f,
     HALF_SIZE, -HALF_SIZE, -HALF_SIZE,   1.0f, 0.0f, 1.0f,
     HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   1.0f, 0.0f, 1.0f,
    -HALF_SIZE, -HALF_SIZE,  HALF_SIZE,   1.0f, 0.0f, 1.0f,
};

// 36 indices - 6 per face, following the exact {corner,corner,corner,
// corner,corner,corner} pattern QuadConfig::INDICES already used, once for
// each face's own 4 vertices. Face f's vertices start at row f*4, so its
// two triangles are (f*4+0, f*4+1, f*4+2) and (f*4+2, f*4+3, f*4+0).
constexpr unsigned int INDICES[] = {
     0,  1,  2,   2,  3,  0,    // +Z face
     4,  5,  6,   6,  7,  4,    // -Z face
     8,  9, 10,  10, 11,  8,    // +X face
    12, 13, 14,  14, 15, 12,    // -X face
    16, 17, 18,  18, 19, 16,    // +Y face
    20, 21, 22,  22, 23, 20,    // -Y face
};

constexpr int VERTEX_COUNT = 24;
constexpr int INDEX_COUNT = 36;
constexpr int FLOATS_PER_VERTEX = 6;
constexpr int POSITION_COMPONENTS = 3;
constexpr int COLOR_COMPONENTS = 3;

// Phase 10: where the cube sits, away from the triangles and mirrored across
// the quad so all three objects are easy to tell apart on screen.
const glm::vec3 POSITION(1.8f, 0.0f, 0.0f);

// How the cube turns. The axis is deliberately NOT one of X, Y, or Z alone -
// a tilted axis means every face eventually faces the camera as the cube
// spins, which is the real proof that this is a solid 3D object and not six
// flat squares that happen to be glued together.
constexpr float SPIN_SPEED = 0.6f;   // radians per second

// glm::rotate expects its axis to already be unit length; glm::normalize is
// not a compile-time function, so this cannot be constexpr like SPIN_SPEED,
// but it only ever runs once, at program startup.
const glm::vec3 SPIN_AXIS = glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f));

} // namespace CubeConfig

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

// GPU handles for this phase's temporary triangle.
// VAO remembers the vertex layout; VBO stores the vertex numbers.
struct TriangleGpu {
    GLuint vao = 0;
    GLuint vbo = 0;
};

// Phase 9: the quad's GPU handles. It needs everything the triangle needs,
// plus an EBO (Element Buffer Object): a third buffer holding QuadConfig's 6
// indices, so the driver knows which of the 4 uploaded vertices to use for
// each triangle corner.
struct QuadGpu {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
};

// Phase 10: the cube's GPU handles. Same shape as QuadGpu - a VAO, a VBO, and
// an EBO - because a cube is drawn exactly the same way a quad is, just with
// more vertices and more indices. A shared `Mesh` type that both of these
// could use instead of two near-identical structs arrives in Phase 14, once
// there is enough repetition to justify it.
struct CubeGpu {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
};

// Data that changes as the scene changes. updateScene() writes it and
// renderScene() reads it, so neither function needs to know about the other.
struct SceneState {
    // Phase 12: the camera's own state - a distance and two angles, turned
    // by mouse drag and scroll since Phase 13. Its default values give the
    // exact same starting view Phases 7-11 used, so nothing changes on
    // screen until the mouse is actually used.
    OrbitCamera camera;

    // glm::mat4(1.0f) is the identity matrix: it moves nothing. Writing the 1.0f
    // explicitly keeps this correct in every glm version.
    glm::mat4 triangleModel = glm::mat4(1.0f);

    // Phase 7: the camera's two matrices. Unlike triangleModel these do not
    // depend on 'now' yet, because the camera itself does not move until
    // Phase 12. They ARE rebuilt every frame, because uProjection depends on
    // the window's aspect ratio, and the window can be resized at any time.
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    // Phase 8: the second, reused draw of the same mesh. It is always the
    // first triangle's own model matrix, shifted farther from the camera, so
    // the two stay overlapping on screen no matter how the first one moves.
    glm::mat4 farCopyModel = glm::mat4(1.0f);

    // Phase 9: the quad's model matrix. It never changes shape, only where it
    // sits, so this is really just QuadConfig::POSITION turned into a matrix.
    // It is still rebuilt every frame, for the same reason as everything
    // else here: renderScene() should only ever read scene state, never
    // calculate it.
    glm::mat4 quadModel = glm::mat4(1.0f);

    // Phase 10: the cube's model matrix. Unlike the quad, this one DOES
    // depend on 'now' - the cube spins - so it earns being rebuilt every
    // frame rather than just sitting there out of habit.
    glm::mat4 cubeModel = glm::mat4(1.0f);

    // Phase 8: toggled by the 'D' key. True matches the driver's normal
    // behaviour: the nearer fragment wins regardless of draw order. False
    // disables GL_DEPTH_TEST, so whichever triangle is drawn LAST simply
    // overwrites the other's pixels, correct or not.
    bool depthTestEnabled = true;
    bool depthKeyWasDown = false;

    // Phase 11: toggled by the 'W' key. False is the normal, solid view:
    // GL_CULL_FACE stays on, so a wrongly-wound face is silently discarded.
    // True switches to line-only rendering AND switches culling off, so
    // every triangle's outline is visible, including one that solid mode
    // would have thrown away.
    bool wireframeEnabled = false;
    bool wireframeKeyWasDown = false;

    // Phase 6: toggled by the 'O' key. False builds the correct T * R * S
    // order; true builds the same three matrices back to front, on purpose,
    // so the two can be compared live.
    bool reverseOrder = false;

    // Remembers last frame's key state so a held-down key flips the toggle
    // only once, on the frame it is first pressed, instead of roughly 120
    // times a second for as long as it is held.
    bool orderKeyWasDown = false;
};

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

    // Phase 13: the camera is no longer read here at all. It is driven by
    // two GLFW callbacks in src/Camera.h instead, which GLFW calls directly
    // from glfwPollEvents() whenever the mouse actually moves or scrolls -
    // there is nothing for processInput() to poll every frame any more.

    // Phase 8: 'D' switches GL_DEPTH_TEST off and on. Same edge-detection
    // reason as 'O' below: without the "was it already down" check, holding
    // the key would flip the state roughly 120 times a second.
    const bool depthKeyIsDown = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    if (depthKeyIsDown && !scene.depthKeyWasDown) {
        scene.depthTestEnabled = !scene.depthTestEnabled;
        std::printf(
            "[depth] GL_DEPTH_TEST %s\n",
            scene.depthTestEnabled
                ? "ON (the nearer triangle wins, regardless of draw order)"
                : "OFF (whichever triangle is drawn LAST wins, correct or not)");
    }
    scene.depthKeyWasDown = depthKeyIsDown;

    // Phase 11: 'W' switches between solid and wireframe rendering, and
    // between culling on and culling off. Same edge-detection reason as
    // 'D' and 'O': without the "was it already down" check, holding the key
    // would flip the state roughly 120 times a second.
    const bool wireframeKeyIsDown = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    if (wireframeKeyIsDown && !scene.wireframeKeyWasDown) {
        scene.wireframeEnabled = !scene.wireframeEnabled;
        std::printf(
            "[wireframe] %s\n",
            scene.wireframeEnabled
                ? "ON, culling OFF (every triangle's outline is visible, front and back)"
                : "OFF, culling ON (the normal solid view)");
    }
    scene.wireframeKeyWasDown = wireframeKeyIsDown;

    // Phase 6: 'O' compares the correct T * R * S order against the same
    // three matrices multiplied back to front. glfwGetKey reports the key as
    // PRESSED for every frame it is held down, so without the "was it already
    // down" check the order would flip roughly 120 times a second while the
    // key is held, which looks like it does nothing.
    const bool orderKeyIsDown = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
    if (orderKeyIsDown && !scene.orderKeyWasDown) {
        scene.reverseOrder = !scene.reverseOrder;
        std::printf(
            "[order] %s\n",
            scene.reverseOrder
                ? "S * R * T (reversed on purpose - watch it smear)"
                : "T * R * S (correct)");
    }
    scene.orderKeyWasDown = orderKeyIsDown;
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

static void updateScene(
    SceneState& scene,
    float now,
    float deltaTime,
    int framebufferWidth,
    int framebufferHeight)
{
    // 'now' drives motion that follows a formula, like this slide.
    // Nothing is stored between frames: the position is recalculated from the
    // clock every time, so there is no table of positions to pre-compute.
    const float offsetX =
        TriangleMotion::SLIDE_DISTANCE * std::sin(TriangleMotion::SLIDE_SPEED * now);

    // Phase 7: the triangle's world-space depth. Positive moves it toward the
    // camera's starting position (nearer, so it looks bigger); negative moves
    // it away.
    const float offsetZ =
        TriangleDepth::DEPTH_AMPLITUDE * std::sin(TriangleDepth::DEPTH_SPEED * now);

    // glm::translate(matrix, vector) returns 'matrix' with a move of 'vector'
    // added. Starting from the identity matrix gives a pure translation.
    const glm::mat4 slide =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, offsetX, offsetZ));

    // Phase 5: glm::rotate(matrix, angle, axis) works the same way, but adds a
    // turn. The angle is in radians and comes from the clock, like the slide.
    const float spinAngle = TriangleSpin::SPIN_SPEED * now;
    const glm::mat4 spin =
        glm::rotate(glm::mat4(1.0f), spinAngle, TriangleSpin::SPIN_AXIS);

    // Phase 6: glm::scale(matrix, vector) works the same way again, but
    // resizes instead of moving or turning. A pulsing factor between
    // PULSE_MIN and PULSE_MAX, applied equally on x and y, keeps the
    // triangle's proportions correct while it grows and shrinks.
    const float scalePulseMid =
        (TriangleScale::PULSE_MIN + TriangleScale::PULSE_MAX) * 0.1f;
    const float scalePulseAmp =
        (TriangleScale::PULSE_MAX - TriangleScale::PULSE_MIN) * 0.5f;
    const float scaleFactor =
        scalePulseMid + scalePulseAmp * std::sin(TriangleScale::PULSE_SPEED * now);
    const glm::mat4 scaleMat =
        glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor, scaleFactor, 1.0f));

    // Join all three by multiplying. Read the product from RIGHT to LEFT,
    // because the vertex meets the rightmost matrix first. The correct order
    // is T * R * S:
    //   1. scale - resize around the origin, where the corners sit;
    //   2. spin  - turn the resized triangle around the origin;
    //   3. slide - carry the turned, resized triangle to its place on screen.
    // Each step only ever acts on the origin-centred result of the step
    // before it, so the triangle grows and shrinks on the spot, spins on the
    // spot, and both of those together slide as one rigid trip.
    //
    // 'O' rebuilds the same three matrices back to front: S * R * T. That
    // order slides first, so a factor meant to resize the shape instead
    // stretches how FAR it slides, and a spin meant to turn the shape instead
    // swings the whole slid-out trip around the origin. The shape looks like
    // it smears through a wide, pulsing loop instead of pulsing and spinning
    // on the spot. Nothing here is broken; only the sequence of operations,
    // read right to left, is different.
    scene.triangleModel = scene.reverseOrder
        ? scaleMat * spin * slide    // S * R * T, deliberately backwards
        : slide * spin * scaleMat;   // T * R * S, correct

    // Phase 8: build the far copy by adding ONE more translation on the LEFT
    // of the already-finished near-copy matrix. Left means "done last", so
    // this shifts the whole placed-turned-scaled triangle straight along
    // world z, without touching its shape, rotation, or size at all - the
    // same "translate the finished result" idea Phase 4 introduced, just
    // applied to a matrix instead of a raw vertex.
    const glm::mat4 farCopyShift = glm::translate(
        glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, DepthTestConfig::FAR_COPY_Z_OFFSET));
    scene.farCopyModel = farCopyShift * scene.triangleModel;

    // Phase 9: the quad's matrix is a single, unmoving translation.
    scene.quadModel = glm::translate(glm::mat4(1.0f), QuadConfig::POSITION);

    // Phase 10: the cube's matrix is T * R, the same pattern Phase 5
    // introduced: spin the cube around its own centre first (the origin,
    // where every one of its 24 vertices is measured from), THEN carry the
    // already-turning cube out to its resting place. Doing it the other way
    // round would make the cube orbit CubeConfig::POSITION instead of
    // spinning on the spot - exactly Phase 5's wobble lesson, at a larger
    // scale.
    const float cubeSpinAngle = CubeConfig::SPIN_SPEED * now;
    const glm::mat4 cubeSpin =
        glm::rotate(glm::mat4(1.0f), cubeSpinAngle, CubeConfig::SPIN_AXIS);
    const glm::mat4 cubeSlide =
        glm::translate(glm::mat4(1.0f), CubeConfig::POSITION);
    scene.cubeModel = cubeSlide * cubeSpin;

    // Phase 7: glm::lookAt(eye, target, up) builds the view matrix from three
    // vectors instead of a translate/rotate/scale recipe. It re-measures every
    // WORLD position as seen from the camera, so the camera can stay at the
    // origin of its own space while everything else moves around it.
    const glm::vec3 eye = orbitCameraPosition(scene.camera);
    scene.view = glm::lookAt(eye, CameraConfig::TARGET, CameraConfig::UP);

    // The projection depends on the window's shape, not the clock, so it is
    // rebuilt from the CURRENT framebuffer size every frame. A minimised
    // window can report a height of 0, and dividing by that would be
    // undefined, so a height of at least 1 is always used for the aspect
    // ratio.
    const int safeHeight = std::max(framebufferHeight, 1);
    const float aspectRatio =
        static_cast<float>(framebufferWidth) / static_cast<float>(safeHeight);
    scene.projection = glm::perspective(
        glm::radians(CameraConfig::FIELD_OF_VIEW_DEGREES),
        aspectRatio,
        CameraConfig::NEAR_PLANE,
        CameraConfig::FAR_PLANE);

    // deltaTime is for input-driven motion such as steering (a later phase).
    // This cast tells the compiler that leaving it unused is intentional.
    static_cast<void>(deltaTime);
}

static bool createTriangle(TriangleGpu& triangle)
{
    glGenVertexArrays(1, &triangle.vao);
    glGenBuffers(1, &triangle.vbo);

    glBindVertexArray(triangle.vao);
    glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(TriangleConfig::VERTICES),
        TriangleConfig::VERTICES,
        GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(
        TriangleConfig::FLOATS_PER_VERTEX * sizeof(float));

    // Attribute 0 reads the first three floats: x, y, z.
    glVertexAttribPointer(
        0,
        TriangleConfig::POSITION_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        nullptr);
    glEnableVertexAttribArray(0);

    // Attribute 1 starts after the three position floats and reads r, g, b.
    glVertexAttribPointer(
        1,
        TriangleConfig::COLOR_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(
            TriangleConfig::POSITION_COMPONENTS * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "[triangle] OpenGL setup error: 0x%04X\n", error);
        return false;
    }

    return triangle.vao != 0 && triangle.vbo != 0;
}

static void destroyTriangle(TriangleGpu& triangle)
{
    if (triangle.vbo != 0)
        glDeleteBuffers(1, &triangle.vbo);

    if (triangle.vao != 0)
        glDeleteVertexArrays(1, &triangle.vao);

    triangle.vbo = 0;
    triangle.vao = 0;
}

static bool createQuad(QuadGpu& quad)
{
    glGenVertexArrays(1, &quad.vao);
    glGenBuffers(1, &quad.vbo);
    glGenBuffers(1, &quad.ebo);

    glBindVertexArray(quad.vao);

    // The vertex data: this half is identical in kind to createTriangle().
    glBindBuffer(GL_ARRAY_BUFFER, quad.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(QuadConfig::VERTICES),
        QuadConfig::VERTICES,
        GL_STATIC_DRAW);

    // The index data: new in this phase. GL_ELEMENT_ARRAY_BUFFER is a
    // different KIND of buffer to GL_ARRAY_BUFFER - it does not hold
    // per-vertex data at all, it holds a list of which vertex to use next.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(QuadConfig::INDICES),
        QuadConfig::INDICES,
        GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(
        QuadConfig::FLOATS_PER_VERTEX * sizeof(float));

    glVertexAttribPointer(
        0,
        QuadConfig::POSITION_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        QuadConfig::COLOR_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(
            QuadConfig::POSITION_COMPONENTS * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VBO - safe any time. The EBO is NOT unbound here: a VAO
    // remembers which GL_ELEMENT_ARRAY_BUFFER was bound while it was bound,
    // and unbinding the EBO now would erase that memory, leaving this VAO
    // with no index buffer at all. Unbinding the VAO first, below, protects
    // the EBO binding it just recorded.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "[quad] OpenGL setup error: 0x%04X\n", error);
        return false;
    }

    return quad.vao != 0 && quad.vbo != 0 && quad.ebo != 0;
}

static void destroyQuad(QuadGpu& quad)
{
    if (quad.ebo != 0)
        glDeleteBuffers(1, &quad.ebo);

    if (quad.vbo != 0)
        glDeleteBuffers(1, &quad.vbo);

    if (quad.vao != 0)
        glDeleteVertexArrays(1, &quad.vao);

    quad.ebo = 0;
    quad.vbo = 0;
    quad.vao = 0;
}

// Line for line the same recipe as createQuad(): more vertices and indices,
// but the exact same VAO/VBO/EBO steps, in the exact same order.
static bool createCube(CubeGpu& cube)
{
    glGenVertexArrays(1, &cube.vao);
    glGenBuffers(1, &cube.vbo);
    glGenBuffers(1, &cube.ebo);

    glBindVertexArray(cube.vao);

    glBindBuffer(GL_ARRAY_BUFFER, cube.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(CubeConfig::VERTICES),
        CubeConfig::VERTICES,
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(CubeConfig::INDICES),
        CubeConfig::INDICES,
        GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(
        CubeConfig::FLOATS_PER_VERTEX * sizeof(float));

    glVertexAttribPointer(
        0,
        CubeConfig::POSITION_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        CubeConfig::COLOR_COMPONENTS,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(
            CubeConfig::POSITION_COMPONENTS * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Same rule as createQuad(): unbind the VBO, but not the EBO, and unbind
    // the VAO last so the EBO binding it recorded survives.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "[cube] OpenGL setup error: 0x%04X\n", error);
        return false;
    }

    return cube.vao != 0 && cube.vbo != 0 && cube.ebo != 0;
}

static void destroyCube(CubeGpu& cube)
{
    if (cube.ebo != 0)
        glDeleteBuffers(1, &cube.ebo);

    if (cube.vbo != 0)
        glDeleteBuffers(1, &cube.vbo);

    if (cube.vao != 0)
        glDeleteVertexArrays(1, &cube.vao);

    cube.ebo = 0;
    cube.vbo = 0;
    cube.vao = 0;
}

// The shader is no longer passed as const: setting a uniform changes the
// shader program, so this function can no longer promise to leave it alone.
static void renderScene(
    ShaderProgram& shader,
    const TriangleGpu& triangle,
    const QuadGpu& quad,
    const CubeGpu& cube,
    const SceneState& scene)
{
    glClearColor(
        AppConfig::CLEAR_COLOR.r,
        AppConfig::CLEAR_COLOR.g,
        AppConfig::CLEAR_COLOR.b,
        1.0f);

    // Clear both buffers every frame. The colour buffer holds visible pixels;
    // the depth buffer will decide which 3D surfaces are closest in later phases.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use() first: a uniform is written into whichever program is currently
    // in use, so setting it before use() would send the value nowhere.
    shader.use();

    // Phase 7: the camera matrices are the same for every object drawn this
    // frame, so they are uploaded once, before either draw call.
    shader.setMat4("uView", scene.view);
    shader.setMat4("uProjection", scene.projection);

    // Phase 8: this ONE line decides whether depth is honoured at all. It is
    // set fresh every frame from the 'D' key's state, rather than relying on
    // whatever main() enabled once at startup, so the effect is visible the
    // instant the key is pressed.
    if (scene.depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    // Phase 11: 'W' controls TWO pieces of GL state together, for one reason.
    // glPolygonMode alone would not be enough: a triangle that GL_CULL_FACE
    // discards for facing the wrong way is thrown away BEFORE the polygon
    // mode ever gets a chance to draw its outline. Switching culling off at
    // the same moment as switching to line mode is what lets a culled
    // face's edges actually appear.
    if (scene.wireframeEnabled) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(triangle.vao);

    // Draw 1: the near copy, drawn FIRST.
    shader.setVec3("uTint", AppConfig::TINT);
    shader.setMat4("uModel", scene.triangleModel);
    glDrawArrays(GL_TRIANGLES, 0, TriangleConfig::VERTEX_COUNT);

    // Draw 2: the SAME mesh again, at DepthTestConfig::FAR_COPY_Z_OFFSET
    // farther away, drawn SECOND. Nothing here is duplicated except the draw
    // call itself: same VAO, same vertex data, just a different uModel and
    // uTint. Drawing the farther copy LAST is deliberate - with depth testing
    // off, its "wrong" pixels are the ones that end up on screen, which is
    // exactly what makes GL_DEPTH_TEST worth having.
    shader.setVec3("uTint", DepthTestConfig::FAR_COPY_TINT);
    shader.setMat4("uModel", scene.farCopyModel);
    glDrawArrays(GL_TRIANGLES, 0, TriangleConfig::VERTEX_COUNT);

    // Draw 3: the indexed quad. A different VAO must be bound first, because
    // the two triangles' VAO has no knowledge of the quad's vertices or its
    // EBO - each VAO only remembers the buffers it was bound to at the time.
    //
    // uTint is (1, 1, 1) here on purpose: this quad's four corners already
    // carry their own distinct colours (Phase 2's idea), so the tint should
    // leave them alone rather than filtering them the way Phase 3 does for
    // the triangle.
    glBindVertexArray(quad.vao);
    shader.setVec3("uTint", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setMat4("uModel", scene.quadModel);

    // glDrawElements reads QuadConfig::INDICES from the EBO already bound
    // inside quad.vao, rather than walking through the VBO in order the way
    // glDrawArrays does. The last argument is nullptr because the indices
    // live in a real GPU buffer, at offset 0 - it is not a CPU array pointer.
    glDrawElements(GL_TRIANGLES, QuadConfig::INDEX_COUNT, GL_UNSIGNED_INT, nullptr);

    // Draw 4: the cube. A third VAO, for the same reason the quad needed a
    // second one - the triangles' VAO and the quad's VAO each only know
    // about their own buffers.
    //
    // uTint stays (1, 1, 1): each of the cube's 24 vertices already carries
    // its own face colour, so nothing should filter it.
    glBindVertexArray(cube.vao);
    shader.setVec3("uTint", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setMat4("uModel", scene.cubeModel);
    glDrawElements(GL_TRIANGLES, CubeConfig::INDEX_COUNT, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
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

    // Ask the driver for modern OpenGL 3.3 Core. Later phases will use GLSL 330.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // Both of these are only the startup defaults now. GL_DEPTH_TEST has been
    // dynamic since Phase 8, and GL_CULL_FACE joins it in Phase 11:
    // renderScene() sets both fresh every frame from the 'D' and 'W' keys'
    // state, so these two lines matter only for the very first frame, before
    // either key has been read.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

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

    TriangleGpu triangle;
    if (!createTriangle(triangle)) {
        std::fprintf(stderr, "Failed to create the shader-test triangle.\n");
        destroyTriangle(triangle);
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    QuadGpu quad;
    if (!createQuad(quad)) {
        std::fprintf(stderr, "Failed to create the indexed quad.\n");
        destroyQuad(quad);
        destroyTriangle(triangle);
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    CubeGpu cube;
    if (!createCube(cube)) {
        std::fprintf(stderr, "Failed to create the cube.\n");
        destroyCube(cube);
        destroyQuad(quad);
        destroyTriangle(triangle);
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("Phase 13 ready. Drag with the left mouse button to orbit, scroll to zoom. Press W for wireframe, D to toggle depth test, O to compare transform order. Press ESC to close.\n");

    SceneState scene;

    // Phase 13: give the two mouse callbacks in src/Camera.h a way to reach
    // this camera. GLFW's callbacks are plain C function pointers - they
    // cannot capture 'scene' the way a lambda could - so a pointer to the
    // one camera they need is attached to the window itself, and each
    // callback reads it back out with glfwGetWindowUserPointer.
    //
    // Seeding lastCursorX/Y from the REAL current cursor position, rather
    // than leaving them at their 0.0 default, stops the very first drag from
    // jumping by however far the cursor's true starting position happens to
    // be from the corner of the screen.
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

        // Phase 7: read the CURRENT framebuffer size every frame, not just
        // once at startup, so uProjection keeps a correct aspect ratio if the
        // window is resized while the program runs.
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        updateScene(scene, clock.now, clock.deltaTime, framebufferWidth, framebufferHeight);
        renderScene(shader, triangle, quad, cube, scene);

        glfwSwapBuffers(window);
        glfwPollEvents();

        reportFrame(stats, clock);
    }

    // OpenGL resources must be deleted while the context still exists.
    destroyCube(cube);
    destroyQuad(quad);
    destroyTriangle(triangle);
    shader.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
