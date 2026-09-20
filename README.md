# Ship Battle Simulator

This is the step-by-step teaching version of `Broadside`. It will eventually demonstrate the same naval battle, but every phase must stay small enough to explain, modify, and demonstrate independently.

## Current progress: Phase 6

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
- a temporary coloured triangle proving the vertex/fragment pipeline works;
- uniform setters that send values from C++ into the running shader;
- a model matrix, built from the clock each frame, that slides, spins, and pulses the triangle in size;
- an `O` key that rebuilds the same matrices in reverse, to compare the correct transform order against a deliberately wrong one.

There are deliberately no ships, reusable meshes, camera, lighting, cannons, people, or environment modes yet. The only transformations are a translation, a rotation, and a scale.

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
| Done | 0-6 | Window, render loop, shader loader, test triangle, uniforms, the model matrix (translate, rotate, scale, and the T * R * S order) |
| A | 7-13 | View/projection matrices, depth, indices, the first cube, culling, orbit camera |
| B | 14-25 | Reusable cube/quad/grid/cylinder/sphere meshes, smooth normals, runtime tessellation |
| C | 26-33 | Ambient, diffuse, specular, materials, second light, Flat/Gouraud/Phong |
| D | 34-37 | One static hierarchical ship and the hierarchy proof |
| E | 38-39 | Keyboard-controlled player ship |
| F | 40-43 | Animated sea, ship rocking, idle rigging motion |
| G | 44-46 | Cannon aiming and the muzzle transform |
| H | 47-49 | Ballistic firing, trajectory control, reload |
| I | 50-52 | A second ship, then a reusable fleet and target selection |
| J | 53-55 | Hit and splash resolution, pooled particles |
| K | 56-60 | Crew roles: lookout, helmsman, cannon crew, helpers |
| L | 61-62 | Teaching HUD and the optimization measurement pass |
| M | 63-65 | Sun, moonlight, rain, and winter modes |
| N | 66 | Report and viva rehearsal |

This order may be refined in documentation, but environment modes remain the last functional feature as requested.
