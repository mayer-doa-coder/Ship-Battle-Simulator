// Ship Battle Simulator - Phase 2: first shader and test triangle.
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// This phase adds one temporary coloured triangle to prove that shader files,
// vertex data, and the OpenGL draw pipeline are connected correctly.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "Shader.h"

#include <algorithm>
#include <cstdio>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 2: Shader Test";
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

} // namespace AppConfig

namespace TriangleConfig {

// Each row is: position x/y/z, then colour red/green/blue.
// These are easy viva values: edit positions to reshape/move the triangle and
// edit colours to change its three corners.
constexpr float VERTICES[] = {
     0.40f,  0.50f, 0.0f,   0.1f, 0.1f, 0.1f,
    -0.40f,  0.50f, 0.0f,   0.1f, 0.1f, 0.1f,
     0.00f, -0.60f, 0.0f,   0.1f, 0.1f, 1.0f
};

constexpr int VERTEX_COUNT = 3;
constexpr int FLOATS_PER_VERTEX = 6;
constexpr int POSITION_COMPONENTS = 3;
constexpr int COLOR_COMPONENTS = 3;

} // namespace TriangleConfig

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

static void updateScene(float now, float deltaTime)
{
    // Phase 2 has no moving scene data yet. Later phases will use:
    //   now       for time-based motion such as waves;
    //   deltaTime for input-driven motion such as steering.
    // These casts tell the compiler that the unused parameters are intentional.
    static_cast<void>(now);
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

static void renderScene(const ShaderProgram& shader, const TriangleGpu& triangle)
{
    glClearColor(
        AppConfig::CLEAR_COLOR.r,
        AppConfig::CLEAR_COLOR.g,
        AppConfig::CLEAR_COLOR.b,
        1.0f);

    // Clear both buffers every frame. The colour buffer holds visible pixels;
    // the depth buffer will decide which 3D surfaces are closest in later phases.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.use();
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

    std::printf("Phase 2 ready. Press ESC to close.\n");

    FrameClock clock;
    startClock(clock);

    FrameStats stats;
    stats.reportStartTime = clock.now;

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        updateClock(clock);
        processInput(window);

        updateScene(clock.now, clock.deltaTime);
        renderScene(shader, triangle);

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
