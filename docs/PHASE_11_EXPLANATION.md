# Phase 11 - Winding Order and GL_CULL_FACE, On Purpose

## Status

Phase 11 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed the
solid scene, the wireframe view, and a deliberately broken face all behaving
exactly as expected. Nothing was printed to the error output, the loop stayed
near `120 FPS`, and the program exited with code `0`.

This phase's whole idea was tested directly, with screenshots, not assumed:

| Test | Result |
|---|---|
| Normal solid view | Every shape from Phases 2-10 still correct, no change |
| `W` pressed once | Every shape switched to line-only outlines; the cube showed all six of its coloured edges at once, including the ones normally hidden |
| `W` pressed again | Everything returned to the normal solid view |
| One face's winding deliberately reversed, solid view | The cube's front face vanished completely - a real hole, straight through to the background |
| Same broken cube, wireframe view | The missing face's outline reappeared, proving its data was never gone |
| `D` (Phase 8's key) tested afterward | Still breaks the two triangles' depth sorting exactly as before |

The held-key guard was tested too: holding `W` down for about one second,
roughly `120` frames at this frame rate, produced exactly **one**
`[wireframe]` console line, not one hundred and twenty.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | `SceneState` gained `wireframeEnabled` and `wireframeKeyWasDown` |
| `src/main.cpp` | `processInput()` edge-detects `W` and flips `wireframeEnabled` |
| `src/main.cpp` | `renderScene()` now sets `glPolygonMode` and `GL_CULL_FACE` together, every frame |
| `src/main.cpp` | `main()`'s comment about `GL_CULL_FACE` updated - it is a dynamic, per-frame setting now, like `GL_DEPTH_TEST` |

No shader changed, and no vertex or index data changed permanently. The
"broken face" shown above was a temporary test, done and then undone, the
same way Phase 9's broken quad index was.

## The idea this phase names out loud

`glEnable(GL_CULL_FACE)` has been switched on since Phase 1. Getting the
cube's six faces wound correctly in Phase 10 was already this exact idea,
just used without stopping to explain it. This phase gives the idea a name
and a way to inspect it directly.

**Winding order** is simply the order a triangle's three corners are listed
in. Looking at a triangle from one particular side, that order is either
clockwise or counter-clockwise. OpenGL uses this, and only this, to decide
which side of the triangle is its "front".

**`GL_CULL_FACE`** then throws away every triangle whose front, by that rule,
is facing *away* from the camera - the side you would only see if the object
were see-through. This is a genuine optimisation: roughly half of any solid
object's triangles face away from the camera at any moment, and there is no
reason to spend time colouring in pixels nobody can ever see.

## Why wireframe alone was not enough

The first version of the wireframe key only changed `glPolygonMode`:

```cpp
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
```

This was not enough to see a culled face's outline, and the reason matters.
Culling happens **before** a triangle is rasterised at all - before the GPU
has decided whether it will be filled in or drawn as a line. A triangle culled
for facing the wrong way is discarded at that earlier step, so switching to
line mode changes nothing about it: it was never going to be drawn either way.

The fix was to change two settings together:

```cpp
if (scene.wireframeEnabled) {
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
} else {
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
```

With culling switched off as well, **every** triangle survives to the
rasteriser, front-facing and back-facing alike, and line mode draws all of
their edges. This was checked directly: the wireframe cube showed all five
remaining face colours on its edges at once - something only possible with
culling off, since normal solid rendering never shows more than three faces
of a cube at a time.

## The test that proves the whole idea

To show what a winding mistake actually looks like, one face's index order
was deliberately reversed:

```cpp
// correct:
0, 1, 2,   2, 3, 0,

// reversed on purpose, for testing:
0, 2, 1,   2, 0, 3,
```

Swapping two corners in each triangle flips which way that triangle's corners
turn, without moving a single vertex. The result, in the normal solid view,
was **not** a distorted or discoloured face. It was a hole - the blue front
face disappeared completely, and the golden background showed straight
through where a solid wall of the cube used to be.

Switching to wireframe with the same broken cube brought the missing face's
outline back, drawn in blue like its neighbours. This is the point of the
whole phase: **the face's vertex and index data was never wrong or missing**.
Every corner was still exactly where it belonged. The only thing wrong was
the *order* those three corners were listed in, and that one detail was
enough for `GL_CULL_FACE` to throw the entire face away, silently, with no
error and no warning.

The winding was put back to `0, 1, 2, 2, 3, 0` immediately afterward. The
shipped cube has no broken faces; the hole was a test, not a feature.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `processInput()` | Edge-detects `W` and flips `scene.wireframeEnabled` |
| `renderScene()` | Sets `GL_CULL_FACE` and `glPolygonMode` together, every frame, from that one flag |

## Likely teacher questions

### What is winding order?

The order a triangle's three corners are listed in, read from one particular
side. OpenGL treats that order as clockwise or counter-clockwise, and by
default a counter-clockwise order (as seen by the camera) is called the
triangle's front.

### What does `GL_CULL_FACE` actually remove?

Any triangle whose front, decided purely by its winding order, faces away
from the camera. It is removed before the GPU spends any time colouring it
in.

### Why bother culling triangles at all?

A closed solid object always has roughly half its triangles facing away from
the viewer. There is no need to compute a colour for a pixel that could never
be seen, so removing those triangles early is a real, free performance gain
on every solid mesh in the project.

### Why did switching to wireframe alone not reveal the missing face?

Because culling happens before the polygon mode has any say in how a
triangle is drawn. A face discarded by culling is gone before line mode ever
gets a turn. Wireframe only reveals a culled face if culling is switched off
at the same time.

### What actually goes wrong when a face's winding is reversed?

Nothing about its position, size, or colour data. Every one of its vertices
stays exactly where it should be. The only change is which direction its
corners are read in, and that single detail decides whether `GL_CULL_FACE`
keeps the face or throws it away.

### Why does a reversed face produce a hole instead of a visible error?

Because there is no error. OpenGL does exactly what it was told: discard
triangles facing the wrong way. A silently discarded triangle looks
identical to one that was never drawn at all - which is exactly why this bug
is worth knowing how to recognise on sight.

### Is the hole a permanent part of this project?

No. It was created, screenshotted twice - once to see the hole, once to see
its outline in wireframe - and then undone. The cube ships correct.

### Why does the 'W' key need an edge-detection check?

For the same reason `D` and `O` do: `glfwGetKey` reports the key as pressed
for every frame it is held down. Without remembering the previous frame's
state, holding the key for one second would flip the toggle roughly `120`
times at this frame rate instead of once.

## Simple viva modifications

- Reverse one face's winding on purpose: swap two numbers within one of
  `CubeConfig::INDICES`'s six-number groups, rebuild, and point out the hole.
  Then press `W` and point out that the face's outline is still there. Put
  the numbers back afterward.
- Turn off wireframe's culling change on its own: comment out just the
  `glDisable(GL_CULL_FACE);` line while leaving `glPolygonMode` alone, rebuild,
  and show that a broken face's outline stays invisible even in wireframe -
  proving culling, not polygon mode, was always the real cause.
- Cull the *other* direction: change `glCullFace` is not called anywhere yet
  (the default is `GL_BACK`), but as a thought exercise, ask what would happen
  if front faces were culled instead - every visible face would vanish and
  only the *inside* of each object would remain, which is a good way to
  confirm the direction of the rule.

## Checkpoint

Phase 11 passes when:

- Debug and Release builds succeed with no compiler warnings;
- all shaders compile and link, with no missing-uniform warning;
- pressing `W` switches every shape to a wireframe view showing all faces,
  front and back, and prints the `ON` message;
- pressing `W` again returns to the normal solid view and prints the `OFF`
  message;
- holding `W` down produces exactly one toggle per press, not one per frame;
- reversing one face's winding on purpose leaves a visible hole in solid
  mode, and that face's outline reappears in wireframe mode;
- the earlier triangles, quad, cube, and the `D` and `O` keys still behave
  exactly as before;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 11

No camera movement yet - the fixed view from Phase 7 is still in use. No new
geometry. No reusable mesh functions.

Phase 12 begins `src/Camera.h`: an orbit camera driven first by the arrow
keys, so the cube - and the hole a mistake could leave in it - can finally be
inspected from any angle instead of one fixed viewpoint.
