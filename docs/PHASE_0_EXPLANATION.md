# Phase 0 - OpenGL Setup

## Status

Phase 0 is complete. Release and Debug builds succeeded on Windows with MSVC Build Tools 2022, the application created an OpenGL 3.3 context, and the normal window-close path exited cleanly.

## What this phase does

This phase creates the smallest useful OpenGL application. It opens a window, loads the OpenGL functions supplied by the graphics driver, clears the framebuffer to a fixed colour every frame, and closes safely.

It does not draw an object yet. That separation is intentional: if setup fails, there is only one small file to inspect.

## Why the startup order matters

The program starts in this order:

1. Set a GLFW error callback.
2. Initialize GLFW.
3. Request OpenGL 3.3 Core Profile.
4. Create the window.
5. make the window's OpenGL context current.
6. Load OpenGL functions through GLAD.
7. Set the framebuffer viewport and enable depth testing.
8. Enter the render loop.
9. Destroy the window and terminate GLFW.

GLAD cannot load OpenGL functions before a context exists. Calling OpenGL before step 6 can cause null function calls or a crash.

## How the render loop works

The loop repeats four actions:

1. Read `ESC` input.
2. Clear the colour and depth buffers.
3. Swap the finished back buffer onto the screen.
4. Ask GLFW to process window and keyboard events.

Double buffering prevents the user from seeing a half-drawn frame. Depth testing is enabled now so the OpenGL state is ready for 3D objects in a later phase.

## Easy values to modify

At the top of `src/main.cpp`, the `AppConfig` namespace contains:

| Value | Effect |
|---|---|
| `WINDOW_WIDTH` | Starting window width |
| `WINDOW_HEIGHT` | Starting window height |
| `WINDOW_TITLE` | Text in the title bar |
| `CLEAR_COLOR` | Red, green, and blue background values from `0.0` to `1.0` |

Changing the background is the first safe viva modification. For example, increasing the red component of `CLEAR_COLOR` makes the window warmer without changing any OpenGL setup code.

## Build and verify

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The verified checkpoint is:

- configuration and compilation finish without source errors;
- startup prints OpenGL, GLSL, and renderer information;
- a dark-blue `1280 x 720` window opens;
- resizing keeps the whole framebuffer valid;
- `ESC` closes the application cleanly.

The runtime reported OpenGL `3.3.0`, GLSL `3.30`, and the NVIDIA GeForce RTX 5050 Laptop GPU renderer on the audit machine. Exact driver and renderer text will differ on another computer.

The vendored GLFW binary can produce MSVC warning `LNK4098` in a Debug build because that prebuilt library uses the Release C runtime. The project source still builds, the Release configuration is clean, and no `/NODEFAULTLIB` workaround is added because hiding a runtime mismatch globally would be riskier than keeping the known Debug-only warning visible.

## Why there is no Python

The required GLAD source and headers are already stored in the repository. The build compiles `src/glad.c` directly with CMake, so no generator or scripting language is needed.
