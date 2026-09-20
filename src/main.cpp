// Ship Battle Simulator - Phase 7: the view and projection matrices.
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// Phases 4-6 built one model matrix that places, turns, and resizes the
// triangle in the WORLD. Every phase before this one then sent that world
// position straight to the screen's flat -1..+1 box, which is why a spinning
// triangle stretched in a wide window: there was no real 3D space yet.
//
// This phase adds the other two matrices a real scene needs, together,
// because either one alone leaves nothing visible:
//   uView       - where the camera is, and which way it faces;
//   uProjection - how far things look smaller, and what is visible at all.
// The triangle also gains real depth motion, so perspective has something to
// prove: it now drifts toward and away from a fixed camera, and grows or
// shrinks exactly the way a real object at that changing distance would.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 7: View & Projection";
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

// Phase 7: a fixed camera. It does not move or look around yet - that is
// Phase 12's orbit camera. For now it only needs a position, a point to look
// at, and which way is "up" from its own point of view.
const glm::vec3 EYE(0.0f, 0.0f, 4.0f);       // the camera's position in the world
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
// which swings between -DEPTH_AMPLITUDE and +DEPTH_AMPLITUDE. The camera sits
// at CameraConfig::EYE.z = 4.0, so the triangle's actual distance from the
// camera swings between (4 - DEPTH_AMPLITUDE) when it is nearest and
// (4 + DEPTH_AMPLITUDE) when it is farthest.
//
// This is the phase's real demonstration. Before Phase 7, changing an
// object's z did nothing useful: with no view or projection matrix, z never
// affected how big anything looked, only whether it was clipped away. Now the
// SAME triangle visibly grows as it nears the camera and shrinks as it
// recedes, because uProjection performs a genuine perspective divide.
constexpr float DEPTH_AMPLITUDE = 1.5f;   // world units nearer/farther than TARGET
constexpr float DEPTH_SPEED = 0.8f;       // radians per second

} // namespace TriangleDepth

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

// Data that changes as the scene changes. updateScene() writes it and
// renderScene() reads it, so neither function needs to know about the other.
struct SceneState {
    // glm::mat4(1.0f) is the identity matrix: it moves nothing. Writing the 1.0f
    // explicitly keeps this correct in every glm version.
    glm::mat4 triangleModel = glm::mat4(1.0f);

    // Phase 7: the camera's two matrices. Unlike triangleModel these do not
    // depend on 'now' yet, because the camera itself does not move until
    // Phase 12. They ARE rebuilt every frame, because uProjection depends on
    // the window's aspect ratio, and the window can be resized at any time.
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

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

    // Phase 7: the triangle's world-space depth. Positive moves it toward
    // CameraConfig::EYE (nearer, so it looks bigger); negative moves it away.
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
        (TriangleScale::PULSE_MIN + TriangleScale::PULSE_MAX) * 0.5f;
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

    // Phase 7: glm::lookAt(eye, target, up) builds the view matrix from three
    // vectors instead of a translate/rotate/scale recipe. It re-measures every
    // WORLD position as seen from the camera, so the camera can stay at the
    // origin of its own space while everything else moves around it.
    scene.view = glm::lookAt(CameraConfig::EYE, CameraConfig::TARGET, CameraConfig::UP);

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

// The shader is no longer passed as const: setting a uniform changes the
// shader program, so this function can no longer promise to leave it alone.
static void renderScene(ShaderProgram& shader, const TriangleGpu& triangle, const SceneState& scene)
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
    shader.setVec3("uTint", AppConfig::TINT);

    // Phase 4: the first use of setMat4. The matrix is rebuilt every frame in
    // updateScene() and uploaded here, before the draw call that needs it.
    shader.setMat4("uModel", scene.triangleModel);

    // Phase 7: the other two matrices the shader now multiplies by. Order of
    // upload does not matter here, only the order they are multiplied in
    // inside the shader.
    shader.setMat4("uView", scene.view);
    shader.setMat4("uProjection", scene.projection);

    glBindVertexArray(triangle.vao);
    glDrawArrays(GL_TRIANGLES, 0, TriangleConfig::VERTEX_COUNT);
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

    // These states do not change the empty Phase 1 window. They prepare the
    // renderer for correct and efficient 3D drawing in later phases.
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

    std::printf("Phase 7 ready. Press O to compare transform order. Press ESC to close.\n");

    SceneState scene;

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
        renderScene(shader, triangle, scene);

        glfwSwapBuffers(window);
        glfwPollEvents();

        reportFrame(stats, clock);
    }

    // OpenGL resources must be deleted while the context still exists.
    destroyTriangle(triangle);
    shader.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
