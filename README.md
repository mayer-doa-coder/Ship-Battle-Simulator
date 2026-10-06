# Ship Battle Simulator

This is the step-by-step teaching version of `Broadside`. It now includes the playable procedural naval battle while retaining the phase demonstrations used to explain the rendering pipeline.

## Current implementation

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
- **Stage C, lighting (Phases 26-33).** The normal matrix `(M^-1)^T` with a stretched-cube proof and an `M` toggle; ambient plus diffuse from a directional sun; the specular term, whose highlight sits within 1 pixel of the analytic prediction as the camera orbits; `src/Material.h` with the L8 slide 60 table (brass, polished silver, black plastic verbatim) plus tuned ocean, hull wood and sailcloth, shown as three spheres from one mesh; a second, attenuated point light with an `L` key to isolate each light; **Flat, Gouraud and Phong in one shader program** on keys `1`/`2`/`3`, with the lighting maths written once in `src/Lighting.h` and spliced into both stages; a `K` key that walks L8 slide 54 (the three terms sum to the full picture within 0.2 of 255 levels) and a `B` key for Blinn-Phong; and two measured demonstrations - **Demo A** (Gouraud misses the sea's highlight at low tessellation, while Phong's image is identical across tessellations) and **Demo B** (Flat banding on an 8-segment brass tube). At the end of Stage C the scene was 14 draws, 1840 triangles, 1045 vertices.
- **Stage D, the ship (Phases 34-78).** `src/Ship.h`, which contains no OpenGL, builds a rigid frame hierarchy for the lofted hull, stepped decks, eight gunports and an eight-gun battery on each side, bowsprit, figurehead and three proportional masts. Each mast has course, topsail and topgallant yards; `makeSail()` supplies bowed, analytic-normal, double-sided cloth with deterministic tears. The playable ship has progressive broadside damage, chain-shot mast breaks, round/chain/grape ammunition, black pirate flags and crew-operated sail, anchor and steering systems. Every frame remains translation/rotation only, with all sizes applied at draw time. Static battery hardware can be startup-batched without merging recoil, damage, crew, or particle animation; the complete Stage-D record is in `docs/STAGE_D_COMPLETION.md`.

The playable scene now includes keyboard-controlled ships, ballistic cannon fire, pooled impact/water/weather particles, moving crews, procedural islands and wildlife, dynamic weather and time of day, multiple cameras, treasure and an explorable ship interior. The living-war systems in `src/Fx.h` and `src/FxDraw.h` add progressive hull holes, broken railings, torn sails, debris, spreading fire and wind-driven smoke; reusable powder-keg/cannon explosions; full/half/furled sail orders; an animated anchor and chain; eased helm, wheel and rudder motion; and deterministic pirate, merchant and naval ships moving in the distance with simplified LOD geometry. Everything is generated from C++/OpenGL primitives—there are no downloaded models, textures, particle engines or game engines.

The campaign layer in `src/Campaign.h` and `src/CampaignDraw.h` now supports thirteen persistent ports, scheduled dock workers, merchants, guards, sailors, fishermen, civilians and smiths, visible timed cargo handling, fourteen supply/demand goods, changing market prices, automatic docking, shore exploration, clue-driven treasure recovery, a compass, an interactive world chart and a ten-mission connected campaign. Delivery, escort, trade, rescue, storm, treasure, raid, named-captain, port-defence and pursuit missions all reuse the existing sailing, combat, damage, weather, crew, treasure and camera state instead of running as isolated scenes.

`src/LivingWorld.h` and `src/LivingWorldDraw.h` add a deterministic outer archipelago without replacing the campaign waters: eight named regions, forty-eight persistent points of interest, thirty-two routed faction vessels, state-aware encounters, reputation, regional rare goods, salvage, multi-step environmental clues, diveable wrecks and reefs, migrating whales, large landmarks and distance-gated procedural detail. Sites stay discovered, caches stay collected, destroyed traffic remains gone for ten minutes, damaged/attacked ports affect trade, and collected sites later produce a regional resource as a reason to return. See `docs/LIVING_WORLD.md` for the state flow and verification coverage.

The world layout uses sailing-scale spacing rather than the original showcase spacing. The five campaign ports are more than 150 units apart, outer regions sit on 430/620-unit rings, regional sites use separate 95-160-unit approaches, and ordinary scenery translations are spread without enlarging their models. The principal enemy starts more than 120 units away, outside cannon range; hostile encounters form 70-110 units away and must approach before firing. Ocean buoys, stacks, traffic departure lanes, mission targets, treasure uncertainty and the chart extent use the same expanded scale.

The optional `0` toggle demonstrates limited **hybrid ray tracing** without replacing the stable renderer. The ships, islands, particles, UI and reflections remain rasterized; only each above-water ocean fragment casts one analytic visibility ray toward the existing sun against at most sixteen nearby hull/rig/land proxy spheres. A hit suppresses direct diffuse/specular sunlight while ambient and local lights remain, producing clearly comparable ship and island shadows on the water. `RT OFF` is the original raster path; `RT ON` is shown in the compact HUD. This is deliberately not advertised as full-scene or recursive ray tracing.

### Play controls

- `W A S D` or arrow keys: sail and steer. Steering eases through the helm, wheel and rudder.
- `Z`: cycle sails through full, half and furled (`Shift+Z` cycles backward).
- `J`: lower or raise the anchor.
- Mouse: aim the cannon; `Space` or right mouse button: fire the aimed gun; `Ctrl`: broadside toward the enemy; `Y`: cycle round, chain, and grape shot; `P`: toggle static battery batching.
- `Enter`: reset the battle; `Esc`: pause/options.
- `F10`: open/close the world chart; arrows pan it and `+`/`-` zoom it.
- `Backspace`: hide/show the compact play HUD for a completely clear view.
- `0`: toggle hybrid ray-traced sun shadows on the ocean (`RT OFF` / `RT ON` in the compact HUD).
- Approach a port slowly and lower the anchor with `J`; once docked, `[`/`]` selects a good, `F` buys/sells one unit, `Tab` goes ashore, and raising the anchor departs after cargo work finishes.
- Near a discovered island, `Tab` goes ashore. `F` inspects shore markers, paired stones and revealed caches. At underwater wrecks, use the free camera (`F6`) to dive and press `F` at the physical chest.
- `F6`–`F9`: free, first-person, cinematic and captain views; `Tab`: explore the ship, a docked port, or a revealed treasure site.
- `\`: toggle water reflections; `U`: toggle crew rendering.

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
