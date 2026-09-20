// Ship Battle Simulator - Phase 5: the model matrix (translation and rotation).
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// Phase 4 built a matrix that moves the triangle. This phase adds a second
// matrix that turns it, and joins the two by multiplying them together. The
// shader and the upload code are unchanged: one matrix can hold both jobs.

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
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 5: Rotation";
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

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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

static void updateScene(SceneState& scene, float now, float deltaTime)
{
    // 'now' drives motion that follows a formula, like this slide.
    // Nothing is stored between frames: the position is recalculated from the
    // clock every time, so there is no table of positions to pre-compute.
    const float offsetX =
        TriangleMotion::SLIDE_DISTANCE * std::sin(TriangleMotion::SLIDE_SPEED * now);

    // glm::translate(matrix, vector) returns 'matrix' with a move of 'vector'
    // added. Starting from the identity matrix gives a pure translation.
    const glm::mat4 slide =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, offsetX, 0.0f));

    // Phase 5: glm::rotate(matrix, angle, axis) works the same way, but adds a
    // turn. The angle is in radians and comes from the clock, like the slide.
    const float spinAngle = TriangleSpin::SPIN_SPEED * now;
    const glm::mat4 spin =
        glm::rotate(glm::mat4(1.0f), spinAngle, TriangleSpin::SPIN_AXIS);

    // Join the two by multiplying. Read the product from RIGHT to LEFT, because
    // the vertex meets the rightmost matrix first:
    //   1. spin  - turns the triangle around the origin, where its corners sit;
    //   2. slide - then carries the turned triangle to its place on screen.
    // Swapping the two would carry the triangle first and then turn the whole
    // trip around the origin, so it would circle instead of spin. Phase 6 makes
    // this order the main lesson.
    scene.triangleModel = slide * spin;

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    ;

    // use() first: a uniform is written into whichever program is currently
    // in use, so setting it before use() would send the value nowhere.
    shader.use();
    shader.setVec3("uTint", AppConfig::TINT);

    // Phase 4: the first use of setMat4. The matrix is rebuilt every frame in
    // updateScene() and uploaded here, before the draw call that needs it.
    shader.setMat4("uModel", scene.triangleModel);

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

    std::printf("Phase 5 ready. Press ESC to close.\n");

    SceneState scene;

    FrameClock clock;
    startClock(clock);

    FrameStats stats;
    stats.reportStartTime = clock.now;

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        updateClock(clock);
        processInput(window);

        updateScene(scene, clock.now, clock.deltaTime);
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
