# Ship Battle Simulator

This is the step-by-step teaching version of `Broadside`. It will eventually demonstrate the same naval battle, but every phase must stay small enough to explain, modify, and demonstrate independently.

## Current progress: Phase 2

The repository currently contains only the OpenGL foundation:

- C++17 and CMake;
- OpenGL 3.3 Core Profile;
- GLFW 3.5.1 for the window and keyboard input;
- GLAD for loading OpenGL functions;
- GLM 1.0.3 for graphics mathematics;
- a resizable clear-colour window that closes with `ESC`;
- a structured render loop with separate input, update, render, and timing jobs;
- frame time and FPS reporting;
- a checked shader loader;
- a temporary coloured triangle proving the vertex/fragment pipeline works.

There are deliberately no ships, shaders, meshes, transformations, animations, cannons, people, or environment modes yet.

## Build on Windows

Use VS Code with the CMake Tools and C/C++ extensions. Select the Visual Studio Build Tools 2022 **amd64** kit, then run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Run `build/Release/ship_battle_simulator.exe`, or use `Shift+F5` in VS Code.

The healthy startup output lists the OpenGL version, GLSL version, renderer, and a successful shader-link message. A coloured triangle should appear over the configured background until `ESC` is pressed. The console prints a frame-time and FPS report approximately once per second.

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

The full phase-by-phase plan lives in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md).
Phases are numbered from `0` to match the explanation file names, and each one
is a single concept with a single visual checkpoint.

| Stage | Phases | Outcome |
|---|---|---|
| Done | 0-2 | Window, render loop, shader loader, test triangle |
| A | 3-6 | Projection, view, model matrices, first cube, orbit camera |
| B | 7-11 | Reusable cube/grid/cylinder/sphere meshes and runtime tessellation |
| C | 12-19 | Ambient, diffuse, specular, materials, second light, Flat/Gouraud/Phong |
| D | 20-23 | One static hierarchical ship and the hierarchy proof |
| E | 24-25 | Keyboard-controlled player ship |
| F | 26-29 | Animated sea, ship rocking, idle rigging motion |
| G | 30-32 | Cannon aiming and the muzzle transform |
| H | 33-35 | Ballistic firing, trajectory control, reload |
| I | 36-38 | A second ship, then a reusable fleet and target selection |
| J | 39-41 | Hit and splash resolution, pooled particles |
| K | 42-46 | Crew roles: lookout, helmsman, cannon crew, helpers |
| L | 47-48 | Teaching HUD and the optimization measurement pass |
| M | 49-51 | Sun, moonlight, rain, and winter modes |
| N | 52 | Report and viva rehearsal |

This order may be refined in documentation, but environment modes remain the last functional feature as requested.
