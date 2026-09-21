# Phase 8 - Proving GL_DEPTH_TEST

## Status

Phase 8 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed two
overlapping copies of the same triangle correctly sorted by depth. Nothing was
printed to the error output, the loop stayed near `120 FPS`, and the program
exited with code `0`.

This is the first phase to draw more than one object, so it is also the first
phase where depth testing has anything to prove. The result was verified with
real screenshots, not assumed:

| State | What was seen |
|---|---|
| `GL_DEPTH_TEST` **on** (default) | The larger, nearer blue triangle correctly covers the smaller, farther red one everywhere they overlap |
| `GL_DEPTH_TEST` **off** (`D` pressed once) | The farther, smaller red triangle - drawn *second* - incorrectly paints over the nearer, larger blue one |
| `GL_DEPTH_TEST` back **on** (`D` pressed again) | Correct result returns; console printed both toggle lines in order, once each |

The key-repeat guard was also tested directly, the same way Phase 6 tested the
`O` key: holding `D` down for about one second, roughly `120` frames at this
frame rate, produced exactly **one** `[depth]` line, not one hundred and
twenty.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | New `DepthTestConfig` namespace: `FAR_COPY_Z_OFFSET`, `FAR_COPY_TINT` |
| `src/main.cpp` | `SceneState` gained `farCopyModel`, `depthTestEnabled`, `depthKeyWasDown` |
| `src/main.cpp` | `processInput()` edge-detects `D` and flips `depthTestEnabled` |
| `src/main.cpp` | `updateScene()` builds `farCopyModel` from the already-finished `triangleModel` |
| `src/main.cpp` | `renderScene()` sets `GL_DEPTH_TEST` on or off each frame, then draws the same VAO **twice** |

No shader changed. No new mesh, no new VBO, no new VAO. This phase reuses the
one triangle this project has always had.

## The one idea

`glEnable(GL_DEPTH_TEST)` has actually been switched on since Phase 0. It had
nothing to prove until now, because every earlier phase drew exactly one
object - there was never a "which one is in front" question to get right or
wrong.

Depth testing answers that question **per pixel**, using a second buffer
alongside the colour buffer: the **depth buffer**. Every time a fragment is
about to be written, its distance from the camera is compared against
whatever distance is already stored at that pixel. The fragment is only kept
if it is nearer. This happens automatically, silently, for every pixel of
every draw call - there is no code to write for the comparison itself. The
only code this phase adds is a way to **prove** it is happening, by turning it
off and watching the picture become wrong.

## Reusing one mesh for two draw calls

```cpp
glBindVertexArray(triangle.vao);

shader.setVec3("uTint", AppConfig::TINT);
shader.setMat4("uModel", scene.triangleModel);
glDrawArrays(GL_TRIANGLES, 0, TriangleConfig::VERTEX_COUNT);

shader.setVec3("uTint", DepthTestConfig::FAR_COPY_TINT);
shader.setMat4("uModel", scene.farCopyModel);
glDrawArrays(GL_TRIANGLES, 0, TriangleConfig::VERTEX_COUNT);

glBindVertexArray(0);
```

No second triangle was created. The **same** VAO, holding the same three
vertices, is bound once and drawn twice, with a different `uModel` and a
different `uTint` each time. This is worth noticing early: almost every object
in the finished project - every ship, every crew member, every raindrop - is
this same idea at a larger scale. One mesh, many draw calls, each with its own
matrix and colour.

## Building the far copy from the near copy, not from scratch

```cpp
const glm::mat4 farCopyShift = glm::translate(
    glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, DepthTestConfig::FAR_COPY_Z_OFFSET));
scene.farCopyModel = farCopyShift * scene.triangleModel;
```

`scene.triangleModel` is whatever Phases 4-7 already built this frame: slid,
spun, scaled, and placed in depth. The far copy takes that **entire finished
matrix** and adds one more translation on the left.

Left means "done last" (Phase 4). A translation added last, in **world**
space, moves the whole already-placed object by a fixed amount without
touching its rotation or its size at all. That is exactly what is wanted here:
a second copy that is identical in every way except sitting a fixed distance
farther from the camera, whatever the first copy happens to be doing at that
instant.

`FAR_COPY_Z_OFFSET` is a **constant**, not something that follows `now`. The
demonstration needs one copy to be reliably nearer than the other at every
single instant. If both offsets came from independent formulas of time, there
would be moments where it was unclear, by eye, which one was actually nearer -
exactly the ambiguity a good demonstration needs to avoid.

## Why the draw order was chosen on purpose

```text
draw 1: the NEAR copy   (scene.triangleModel)
draw 2: the FAR  copy   (scene.farCopyModel)   <- deliberately drawn LAST
```

This order was not arbitrary. With depth testing behaving correctly, draw
order should not matter at all - the nearer fragment wins regardless of which
was submitted first. But if depth testing is switched off, whichever fragment
is written **last** simply overwrites whatever was there, correct or not.

Drawing the **farther** copy last means that disabling depth testing produces
a **visibly wrong** picture: the smaller, farther, "should be hidden" red
triangle appears on top of the larger, nearer blue one. If the near copy had
been drawn last instead, turning depth testing off would have produced the
same correct-looking picture by accident, and the demonstration would prove
nothing. Getting this backwards is the single easiest way to make this phase
look like it works when it does not.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `processInput()` | Edge-detects `D` and flips `scene.depthTestEnabled` |
| `updateScene()` | Builds `farCopyModel` from the finished `triangleModel` |
| `renderScene()` | Sets `GL_DEPTH_TEST` on or off for this frame, then issues two draw calls |

## Setting the state fresh every frame, not once at startup

```cpp
if (scene.depthTestEnabled)
    glEnable(GL_DEPTH_TEST);
else
    glDisable(GL_DEPTH_TEST);
```

`main()` still calls `glEnable(GL_DEPTH_TEST)` once at startup, exactly as it
has since Phase 0. That line now only matters for the very first frame, before
any key has been read. From then on, `renderScene()` sets the real state every
single frame, straight from `scene.depthTestEnabled`. This makes the effect of
pressing `D` visible on the very next frame, with no doubt about which piece
of code is responsible for the current picture.

## Likely teacher questions

### What is depth testing?

A per-pixel comparison, performed automatically by the GPU, between a new
fragment's distance from the camera and whatever distance is already stored
for that pixel. The fragment is kept only if it is nearer. It requires no
code from the programmer beyond enabling it and making sure the depth buffer
is cleared each frame.

### Why did nothing prove it was working before this phase?

Because every earlier phase only ever drew one object. With nothing to
compare against, the depth test could not have failed even if it had been
switched off the whole time.

### Why does this phase reuse one mesh instead of creating a second triangle?

Because the lesson is about depth, not about geometry. Reusing the same VAO
with a different matrix and a different tint proves the point with the
smallest possible change, and previews the mesh-reuse pattern the whole
project is built on.

### Why is the far copy built by translating the near copy's finished matrix, instead of building it from scratch?

Multiplying a translation onto the LEFT of an already-built matrix moves the
whole result without touching its rotation or scale. Building the far copy
this way guarantees it is identical to the near copy in every way except
distance, with no risk of the two drifting out of sync as later phases change
how the near copy moves.

### Why is the depth offset a constant instead of animated?

The demonstration needs the "which one is nearer" answer to be unambiguous at
every instant. An animated offset could let the two copies swap which one is
nearer at some point in time, which would make the picture harder to reason
about, not easier.

### Why does the draw order matter here?

Because it is the entire point of the test. With depth testing on, draw order
should never matter. Drawing the farther copy last means that disabling depth
testing produces a visibly wrong picture, which is what proves the depth test
was doing real work.

### Why does `GL_DEPTH_TEST` get set every frame instead of once?

So that pressing `D` has an effect on the very next frame, and so that the
picture on screen always matches the current value of `scene.depthTestEnabled`
rather than some earlier state.

### Why does the small red sliver show through even with depth testing on?

The two triangles are not identical in outline on screen: the farther one is
smaller, and perspective can shift it slightly relative to the nearer one.
Anywhere the two shapes do not overlap, there is nothing to hide the farther
triangle behind, so its own, correctly rendered edge is visible against the
background. Depth testing only decides what happens where the two genuinely
overlap.

### Why does the 'D' key need an edge-detection check?

For the same reason 'O' does (Phase 6): `glfwGetKey` reports the key as
pressed for every frame it is held down. Without remembering the previous
frame's state, a one-second key hold would flip the toggle roughly `120` times
at this frame rate.

## Simple viva modifications

- Swap the draw order: draw the far copy first and the near copy second, then
  turn depth testing off again. The picture now looks correct by accident,
  which is exactly why the original order matters - point this out.
- Move the far copy closer: change `FAR_COPY_Z_OFFSET` toward `0.0f` and watch
  the two triangles nearly coincide.
- Move the far copy much farther: make `FAR_COPY_Z_OFFSET` more negative and
  watch it shrink further, due to the same perspective divide from Phase 7.
- Change its colour: edit `FAR_COPY_TINT`.
- Demonstrate the debounce: hold `D` down for a few seconds and count the
  console lines. There should be exactly one per physical press.

## Checkpoint

Phase 8 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- two overlapping copies of the triangle are visible, correctly sorted by
  depth by default;
- pressing `D` visibly breaks the sorting - the farther, drawn-last copy
  incorrectly appears on top - and prints the `OFF` message;
- pressing `D` again restores correct sorting and prints the `ON` message;
- holding `D` down produces exactly one toggle per press, not one per frame;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 8

No indexed drawing yet - both triangles are still drawn with `glDrawArrays`
from the same three raw vertices. No real mesh class. No cube. The two
triangles are still flat and share identical shape data.

Phase 9 introduces indexed drawing: an EBO and `glDrawElements`, building a
quad from four vertices and six indices instead of two separately-listed
triangles.
