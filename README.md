# Ship Battle Simulator

This is the step-by-step teaching version of `Broadside`. It will eventually demonstrate the same naval battle, but every phase must stay small enough to explain, modify, and demonstrate independently.

## Current progress: Phase 25 - Stage B complete

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
- an `O` key that rebuilds the same matrices in reverse, to compare the correct transform order against a deliberately wrong one;
- a real camera with view and projection matrices, so every object moves in genuine 3D space, keeps its true proportions at any window size, and grows or shrinks correctly with distance;
- the same triangle mesh drawn twice at different depths, and a `D` key that switches `GL_DEPTH_TEST` off and on to prove the nearer one wins only because depth testing is on;
- a small static quad built from 4 vertices and 6 indices with an EBO, proving that a shared corner can be uploaded once and reused, instead of being typed out twice;
- the project's first real 3D object: a solid, spinning cube built from 24 vertices and 36 indices, one flat colour per face, correctly wound on all six sides;
- a `W` key that shows every triangle's edges and switches face culling off, so a face a winding mistake would normally hide silently can be proven to still exist;
- `src/Camera.h`: an orbit camera holding a distance and two angles instead of a fixed position, now driven by left-drag to orbit and the scroll wheel to zoom, with pitch clamped to 89 degrees and radius clamped to a sensible range;
- `src/Mesh.h`: one `Vertex` layout and one `Mesh` type that owns its own VAO, VBO, and EBO and draws itself, replacing three near-identical hand-written copies of that code, plus `makeCube()`, the first generator - the cube's 24 vertices and 36 indices are now produced by a loop rather than typed out, and produce exactly the same numbers;
- an `N` key that paints every pixel from its surface normal instead of its colour, so a normal stops being an invisible number and becomes one flat, predictable colour per face that can be checked against a prediction before any lighting exists to hide a mistake;
- the unit-mesh rule and `drawMesh()`: `makeCube()` takes no size at all and builds a cube exactly 1 x 1 x 1, and an object's real size is a `glm::scale` applied in its model matrix at the moment it is drawn. Three cubes of three different sizes are now on screen, drawn from that one mesh and one VAO, with nothing extra stored on the graphics card;
- `makeQuad()`, the second generator and the simplest one: the quad's four vertices and six indices are no longer typed out but built from a short recipe, as a 1 x 1 unit quad facing `+Z`, and drawn at `QuadConfig::SIZE` by the same draw-time `glm::scale` the cubes use. The rendered frame is byte-for-byte identical to the hand-written version's; the `N` key now confirms the quad faces `+Z`;
- `makeGrid(N)`, the first **parameterised** generator: nested loops build `(N+1)^2` vertices and `2N^2` triangles for a flat grid that is always 1 x 1 but divided as finely as `GridConfig::CELLS` asks. It lies horizontally, facing straight up, because this is the mesh that becomes the sea; how finely it is divided and how big it is are now two separate values;
- the wireframe comparison that makes a parameterised mesh legible: pressing `W` prints the grid's expected quad, triangle and vertex counts beside the arithmetic, so `2 * N * N` can be checked against what you can count by eye. The test triangle's pulse, which had been running over the wrong range since Phase 12, is also fixed here;
- `makeCylinder(segments)`, the first **curved** surface: a ring of `sinf`/`cosf` vertices, each carrying the exact normal of a smooth cylinder, `normalize(vec3(x, 0, z))`. It is the first mesh whose normals differ from vertex to vertex, so pressing `N` makes the colours sweep round its circumference instead of showing one flat value per face. It is deliberately an open tube - you can see up through the missing lids, which is what Phase 21 fixes;
- the cylinder's two end caps, each a centre vertex and a triangle fan with a flat normal, which turn the open tube into a **closed, watertight solid** - every edge shared by exactly two triangles. The caps need their own copies of the rim vertices, because a vertex carries one normal and the wall's points sideways while a cap's points straight up: the same reason the cube needs 24 vertices rather than 8;
- `makeSphere(stacks, slices)`, the first **two-parameter** surface and the fifth and last of the meshes the whole project is built from. Its normal is the simplest of all - for a ball centred on its own origin, `normalize(position)` **is** the surface normal - and pressing `N` renders it as the classic RGB ball: red sweeping across, green down, blue brightest where the surface faces you. Latitude and longitude are independent, and the poles are shared triangle fans so the mesh contains no degenerate triangles;
- `computeSmoothNormals()`, the L9 slide 20 averaging formula, and a second cube built from 8 **shared** corners to demonstrate it on. The two cubes are the same twelve triangles at the same size in the same rotation, and in the `N` view the flat one shows 3 colours while the smooth one shows 15,952. Running the formula on the flat 24-vertex cube provably changes nothing, because each of its vertices belongs to only one face - which is why it cannot be smoothed;
- live counters in the window title - `draws 10 | tris 640 | verts 437` - accumulated inside `drawMesh()`, the one place every draw in the project passes through, so they report what the frame actually **did** rather than what the configuration says it should. The total matches a hand calculation from the `[mesh]` lines printed at startup. `verts` is what is submitted per frame, not what is stored on the graphics card (386), and the 51-vertex gap is exactly the saving from drawing the triangle twice and the cube three times out of one mesh each;
- `+` and `-`, which **rebuild the grid, cylinder and sphere while the program runs**, across six levels of detail from 92 to 36,116 triangles. One detail level drives all three, and each mesh works out its own divisions from its level-0 value rather than from the level before, so pressing `-` twice and `+` twice returns to exactly where it started. `Mesh::upload()` has called `destroy()` first since Phase 14, which is the one line that stops a rebuild abandoning GPU buffers; 100 rebuilds raise no OpenGL error and leak nothing, proven by the driver reissuing the same object names and verified against a negative control with `destroy()` disabled. Throughout all six levels the **draw count stays at 10** - detail changes how finely the same objects are divided, never how many objects there are.

There is deliberately no lighting, and there are no ships, cannons, people, or environment modes yet. Phase 26 begins Stage C, where lighting starts. All five generators are in place. The transformations are a translation, a rotation, a scale, and a camera the mouse can actually move. This is the first phase to draw more than one object, the first to use indexed drawing, the first genuinely 3D solid, the first to explain winding order and face culling directly, and the first to use real GLFW input callbacks instead of polling.

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
| Done | 0-23 | Window, render loop, shader loader, test triangle, uniforms, the model matrix (translate, rotate, scale, and the T * R * S order), a real camera with view and projection matrices, depth testing proved with two overlapping draws, indexed drawing with an EBO, the first cube, winding order and culling explained with a wireframe key, a mouse-driven orbit camera with real limits, one reusable `Mesh` type, the normals debug view, the unit-mesh rule, a generated quad, and a parameterised grid |
| B | 24-25 | Live counters, runtime tessellation |
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
