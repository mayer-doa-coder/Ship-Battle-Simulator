# Ship Battle Simulator

This is the step-by-step teaching version of `Broadside`. It will eventually demonstrate the same naval battle, but every phase must stay small enough to explain, modify, and demonstrate independently.

## Current progress: Phase 1

The repository currently contains only the OpenGL foundation:

- C++17 and CMake;
- OpenGL 3.3 Core Profile;
- GLFW 3.5.1 for the window and keyboard input;
- GLAD for loading OpenGL functions;
- GLM 1.0.3 for graphics mathematics;
- a resizable clear-colour window that closes with `ESC`;
- a structured render loop with separate input, update, render, and timing jobs;
- frame time and FPS reporting.

There are deliberately no ships, shaders, meshes, transformations, animations, cannons, people, or environment modes yet.

## Build on Windows

Use VS Code with the CMake Tools and C/C++ extensions. Select the Visual Studio Build Tools 2022 **amd64** kit, then run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Run `build/Release/ship_battle_simulator.exe`, or use `Shift+F5` in VS Code.

The healthy startup output lists the OpenGL version, GLSL version, and renderer. A dark blue window should remain open until `ESC` is pressed. The console prints a frame-time and FPS report approximately once per second.

## Why each library exists

| Library | One job in this project |
|---|---|
| OpenGL | Draws pixels and, in later phases, 3D geometry |
| GLFW | Creates the window and reads input |
| GLAD | Loads OpenGL function addresses from the graphics driver |
| GLM | Supplies vectors and matrices with GLSL-like notation |
| CMake | Builds the same source structure consistently |

These are support libraries, not a game engine. Scene construction, transformations, animation, lighting, collision, and behavior will be written directly in C++ and GLSL.

## Non-negotiable development rules

- Do not use Python, Python scripts, Python-generated project steps, or Python tooling.
- Add one small, demonstrable phase at a time.
- Keep important values together and give them descriptive names so they are easy to change during a viva.
- Explain every phase using **what it does, why it exists, and how it works**.
- Do not copy the completed `Broadside` application into this repository.
- Do not start a new phase until the current visual checkpoint passes.

## Planned phase order

1. OpenGL window and libraries — **complete**.
2. Stable render loop and frame time — **complete**.
3. First shader and triangle.
4. Model, view, and projection transformations.
5. Reusable primitive meshes.
6. Lighting and material comparison.
7. One simple hierarchical ship.
8. Player ship movement and steering.
9. Animated sea and ship rocking.
10. One manually aimable cannon and adjustable trajectory.
11. One additional ship, then a small fleet using the same reusable ship code.
12. Cannonball collision, reload state, and clear feedback.
13. Simple transform-based crew roles: lookout, helm, cannon crew, and helpers.
14. Optimization, teaching controls, and viva-friendly configuration.
15. Environment modes, implemented last: sun, moonlight, rain, and winter.

This order may be refined in documentation, but environment modes remain the last functional feature as requested.
