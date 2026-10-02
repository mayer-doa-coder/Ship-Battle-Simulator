// Ship Battle Simulator - Phase 15: normals as colour.
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// Phase 14 gave every vertex a normal, and nothing read it. This phase reads
// it: pressing 'N' makes the fragment shader paint each pixel from its normal
// instead of its colour, using N * 0.5 + 0.5 to turn a -1..+1 direction into
// a 0..1 colour. Each cube face then shows one flat, predictable colour for
// the axis it faces.
//
// This is a tool, not a feature. A wrong normal is nearly invisible once it
// is buried inside a lighting equation, so it is worth being able to see every
// normal in the scene directly BEFORE any light depends on one - which is why
// the plan puts this key here, eleven phases before the first light.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 15: Normals as Colour";
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

// Phase 14: each row is now one `Vertex` from src/Mesh.h - a position, a
// normal, and a colour - instead of six loose floats whose meaning depended
// on counting. The position and colour numbers are exactly the ones Phase 2
// typed in; only the way they are written down is new.
//
// The triangle is flat and lies in the XY plane, so all three of its corners
// face the same way: straight out of the screen along +Z. No line of shader
// code reads that normal yet, but the mesh uploads it, so it has to be right
// now rather than guessed at later.
//
// These are still easy viva values: edit positions to reshape/move the
// triangle and edit colours to change its three corners.
const glm::vec3 FLAT_NORMAL(0.0f, 0.0f, 1.0f);

const Vertex VERTICES[] = {
    { {  0.40f,  0.37f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 0.1f } },
    { { -0.40f,  0.37f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 0.1f } },
    { {  0.00f, -0.73f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 1.0f } },
};

// Phase 14: the triangle needs an index list for the first time. `Mesh`
// always draws with glDrawElements, so even a shape with no shared corners
// needs one - here simply "corner 0, corner 1, corner 2", which is the same
// order glDrawArrays walked through on its own until now.
const unsigned int INDICES[] = { 0, 1, 2 };

constexpr int VERTEX_COUNT = 3;
constexpr int INDEX_COUNT = 3;

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

// Phase 9: 4 UNIQUE corners, one row each. Phase 14 turned each row into a
// `Vertex` - position, normal, colour - with the same numbers as before.
// A different colour on each corner makes the shared diagonal easy to see,
// and makes a wrong index (see INDICES below) obvious: the colours would no
// longer blend the way they are supposed to.
//   0: top-left (red)  1: top-right (green)
//   3: bottom-left (yellow)  2: bottom-right (blue)
//
// Like the triangle, the quad is flat in the XY plane, so every corner's
// normal points the same way: +Z, straight at the camera's starting position.
const glm::vec3 FLAT_NORMAL(0.0f, 0.0f, 1.0f);

const Vertex VERTICES[] = {
    { { -0.4f,  0.4f, 0.0f }, FLAT_NORMAL, { 1.0f, 0.0f, 0.0f } },   // 0: top-left,     red
    { {  0.4f,  0.4f, 0.0f }, FLAT_NORMAL, { 0.5f, 1.0f, 0.0f } },   // 1: top-right,    green
    { {  0.4f, -0.4f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.7f, 1.0f } },   // 2: bottom-right, blue
    { { -0.4f, -0.4f, 0.0f }, FLAT_NORMAL, { 1.0f, 1.0f, 0.9f } },   // 3: bottom-left,  yellow
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

// Phase 14: the cube's six face colours, one per face, in the same order
// makeCube() walks its faces: +Z, -Z, +X, -X, +Y, -Y.
//
// Phase 10 wrote out all 24 vertices and all 36 indices by hand, repeating
// each face's colour on four rows. Those two arrays are gone: makeCube() in
// src/Mesh.h generates exactly the same numbers from a loop. The colours stay
// here, in main.cpp, because they are a choice about how the cube LOOKS,
// while the generator's job is only its SHAPE. That split is the same one
// materials formalise in Phase 29.
//
// Each face still gets its own 4 vertices rather than sharing the cube's 8
// real corners, for the reason Phase 10 gave: a vertex carries one colour and
// one normal, and a cube corner belongs to three faces that need different
// values for both.
const glm::vec3 FACE_COLORS[6] = {
    { 0.0f, 0.0f, 1.0f },   // +Z front  - blue
    { 1.0f, 1.0f, 0.0f },   // -Z back   - yellow
    { 1.0f, 0.0f, 0.0f },   // +X right  - red
    { 0.0f, 1.0f, 1.0f },   // -X left   - cyan
    { 0.0f, 1.0f, 0.0f },   // +Y top    - green
    { 1.0f, 0.0f, 1.0f },   // -Y bottom - magenta
};

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

// Phase 14: `TriangleGpu`, `QuadGpu`, and `CubeGpu` used to sit here - three
// structs of raw GLuint handles, the second and third identical to each
// other. The `Mesh` class in src/Mesh.h replaced all three. It holds the same
// handles, but it also knows how many indices to draw and deletes its own
// buffers, which a bare struct of handles could never do on its own.

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

    // Phase 15: toggled by the 'N' key. False is the normal picture, coloured
    // from each vertex's own colour. True makes the fragment shader ignore
    // colour and tint completely and paint each pixel from the NORMAL
    // instead, so every normal in the scene can be checked by eye.
    bool debugNormalsEnabled = false;
    bool debugNormalsKeyWasDown = false;
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

    // Phase 15: 'N' switches the normals debug view on and off. Same
    // edge-detection reason as every other toggle in this function.
    const bool debugNormalsKeyIsDown = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    if (debugNormalsKeyIsDown && !scene.debugNormalsKeyWasDown) {
        scene.debugNormalsEnabled = !scene.debugNormalsEnabled;
        std::printf(
            "[normals] debug view %s\n",
            scene.debugNormalsEnabled
                ? "ON  (each pixel is painted from its normal: N * 0.5 + 0.5)"
                : "OFF (back to the vertex colours, filtered by uTint)");
    }
    scene.debugNormalsKeyWasDown = debugNormalsKeyIsDown;

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

// Phase 14: one function replaces createTriangle/destroyTriangle,
// createQuad/destroyQuad, and createCube/destroyCube - six functions and
// about 210 lines of near-identical glGen/glBind/glBufferData/
// glVertexAttribPointer calls. All of that work now happens once, inside
// Mesh::upload(), and each Mesh frees itself in Mesh::destroy(), so the
// three destroy functions have no work left to do at all.
//
// The triangle and the quad still carry their vertices written out by hand,
// exactly as Phases 2 and 9 wrote them. Only the cube is GENERATED, because
// the plan gives it makeCube() in this phase; makeQuad() and the rest of the
// generators arrive in Phases 17-22.
static bool createMeshes(Mesh& triangleMesh, Mesh& quadMesh, Mesh& cubeMesh)
{
    // Mesh::upload takes std::vector, and these config arrays are fixed-size
    // C arrays, so each one is copied into a vector by naming its first
    // element and one-past-its-last. The copy happens once, at startup.
    const std::vector<Vertex> triangleVertices(
        TriangleConfig::VERTICES,
        TriangleConfig::VERTICES + TriangleConfig::VERTEX_COUNT);
    const std::vector<unsigned int> triangleIndices(
        TriangleConfig::INDICES,
        TriangleConfig::INDICES + TriangleConfig::INDEX_COUNT);

    if (!triangleMesh.upload("triangle", triangleVertices, triangleIndices))
        return false;

    const std::vector<Vertex> quadVertices(
        QuadConfig::VERTICES,
        QuadConfig::VERTICES + QuadConfig::VERTEX_COUNT);
    const std::vector<unsigned int> quadIndices(
        QuadConfig::INDICES,
        QuadConfig::INDICES + QuadConfig::INDEX_COUNT);

    if (!quadMesh.upload("quad", quadVertices, quadIndices))
        return false;

    // The cube's 24 vertices and 36 indices are built by a loop in
    // src/Mesh.h instead of typed out here. makeCube() produces the same
    // numbers, in the same order, that Phase 10 wrote by hand.
    return makeCube(cubeMesh, CubeConfig::HALF_SIZE, CubeConfig::FACE_COLORS);
}

// The shader is no longer passed as const: setting a uniform changes the
// shader program, so this function can no longer promise to leave it alone.
static void renderScene(
    ShaderProgram& shader,
    const Mesh& triangleMesh,
    const Mesh& quadMesh,
    const Mesh& cubeMesh,
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

    // Phase 15: the debug view applies to the whole scene, not to one object,
    // so like the camera matrices it is uploaded once per frame rather than
    // once per draw. setInt() is used here for the first time; it has existed,
    // unused, since Phase 3.
    shader.setInt("uDebugNormals", scene.debugNormalsEnabled ? 1 : 0);

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

    // Phase 14: every draw below now reads the same three lines - set the
    // tint, set the model matrix, tell the mesh to draw itself. Binding the
    // right VAO, knowing whether to call glDrawArrays or glDrawElements, and
    // knowing how many indices there are have all moved inside Mesh::draw().
    // This function is left saying only what is different about each object.

    // Draw 1: the near copy of the triangle, drawn FIRST.
    shader.setVec3("uTint", AppConfig::TINT);
    shader.setMat4("uModel", scene.triangleModel);
    triangleMesh.draw();

    // Draw 2: the SAME mesh again, at DepthTestConfig::FAR_COPY_Z_OFFSET
    // farther away, drawn SECOND. Nothing here is duplicated except the draw
    // call itself: same mesh, same vertex data, just a different uModel and
    // uTint. Drawing the farther copy LAST is deliberate - with depth testing
    // off, its "wrong" pixels are the ones that end up on screen, which is
    // exactly what makes GL_DEPTH_TEST worth having.
    shader.setVec3("uTint", DepthTestConfig::FAR_COPY_TINT);
    shader.setMat4("uModel", scene.farCopyModel);
    triangleMesh.draw();

    // Draw 3: the quad.
    //
    // uTint is (1, 1, 1) here on purpose: this quad's four corners already
    // carry their own distinct colours (Phase 2's idea), so the tint should
    // leave them alone rather than filtering them the way Phase 3 does for
    // the triangle.
    shader.setVec3("uTint", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setMat4("uModel", scene.quadModel);
    quadMesh.draw();

    // Draw 4: the cube.
    //
    // uTint stays (1, 1, 1): each of the cube's 24 vertices already carries
    // its own face colour, so nothing should filter it.
    shader.setVec3("uTint", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setMat4("uModel", scene.cubeModel);
    cubeMesh.draw();
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

    // Phase 14: three Mesh objects replace three structs of raw handles and
    // three create/destroy pairs. Each one owns its own VAO, VBO, and EBO.
    // They are declared here, before the loop, so they live for as long as
    // the window does.
    Mesh triangleMesh;
    Mesh quadMesh;
    Mesh cubeMesh;

    if (!createMeshes(triangleMesh, quadMesh, cubeMesh)) {
        std::fprintf(stderr, "Failed to create the scene meshes.\n");
        cubeMesh.destroy();
        quadMesh.destroy();
        triangleMesh.destroy();
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("Phase 15 ready. Drag with the left mouse button to orbit, scroll to zoom. Press N for the normals debug view, W for wireframe, D to toggle depth test, O to compare transform order. Press ESC to close.\n");

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
        renderScene(shader, triangleMesh, quadMesh, cubeMesh, scene);

        glfwSwapBuffers(window);
        glfwPollEvents();

        reportFrame(stats, clock);
    }

    // OpenGL resources must be deleted while the context still exists.
    //
    // Phase 14: each Mesh also frees itself in its destructor, but a Mesh
    // declared in main() is destroyed AFTER glfwTerminate() has already
    // destroyed the context, and deleting a GPU object with no context is
    // not valid. Calling destroy() here, explicitly, deletes the buffers at
    // the right moment and leaves every handle at 0, so the later destructor
    // finds nothing to do. This is the same arrangement ShaderProgram has
    // used since Phase 2.
    cubeMesh.destroy();
    quadMesh.destroy();
    triangleMesh.destroy();
    shader.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
