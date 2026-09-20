# Phase 7 - The View and Projection Matrices

## Status

Phase 7 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed the
triangle moving, spinning, pulsing, and now genuinely growing and shrinking
with distance. Nothing was printed to the error output, the loop stayed near
`120 FPS`, and the program exited with code `0`.

The two claims that matter for this milestone were both measured, with the
triangle's other motion frozen so each effect could be isolated:

| Claim | Predicted | Measured |
|---|---:|---:|
| Perspective shrink with distance (far/near width ratio) | `2.200` | `2.256` |
| Fit of the whole depth motion to the exact `glm::lookAt`/`glm::perspective` formulas, with **no free scale parameter** | - | RMS error `9.0` px over `90` samples |
| Static frontal shape keeps its true proportions (width/height) | `0.7273` | `0.7316` |
| Widening the field of view from 45 deg to 90 deg shrinks the same object | ratio `2.414` | ratio `2.522` |

The small differences from prediction are consistent with pixel quantisation
and anti-aliasing on a triangle as small as `67` pixels wide, not with a wrong
formula: every ratio lands within a few percent, in the correct direction, with
zero fitted parameters beyond a single timing offset.

## What changed

| File | Change |
|---|---|
| `shaders/basic.vert` | Two new uniforms, `uView` and `uProjection`; `gl_Position` is now `uProjection * uView * uModel * vec4(aPosition, 1.0)` |
| `src/main.cpp` | New `CameraConfig` namespace: `EYE`, `TARGET`, `UP`, `FIELD_OF_VIEW_DEGREES`, `NEAR_PLANE`, `FAR_PLANE` |
| `src/main.cpp` | New `TriangleDepth` namespace: `DEPTH_AMPLITUDE`, `DEPTH_SPEED`, driving real world-space `z` motion |
| `src/main.cpp` | `SceneState` gained `view` and `projection`; `updateScene()` now also takes the framebuffer size and rebuilds both every frame |
| `src/main.cpp` | `renderScene()` uploads `uView` and `uProjection` alongside `uModel` |

## The one idea

Before this phase, `uModel`'s output went straight to `gl_Position`. That meant
every object lived in the same flat `-1..+1` box the screen shows directly,
with no real notion of a camera or of distance. Two more matrices fix that,
and they only make sense together:

```text
uModel        - where is the OBJECT, in the world? (Phases 4-6)
uView         - where is the CAMERA, and which way does it face?
uProjection   - how does distance make things look smaller, and what is
                even visible?
```

A vertex now passes through three matrices, read right to left as always:

```text
gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
                    3          2       1
```

## Why view and projection must arrive together

A view matrix alone moves everything into camera space, but a vertex in camera
space still has no perspective divide applied. Depending on where the camera
sits, the object can end up outside the `-1..+1` clip cube and simply vanish -
not shrink, vanish.

A projection matrix alone, with no view matrix, assumes the camera sits at the
world origin facing `-Z`. If the object being drawn is not already positioned
that way, the near plane can cut straight through it or the perspective divide
can produce a division by a near-zero value, which throws the vertex to
infinity.

Only having both together produces the picture this phase promises: **pushing
the triangle along `-Z` now makes it shrink, instead of vanishing or breaking**.
That is why this is a milestone phase in the plan and was never split further.

## `glm::lookAt`: three vectors instead of a recipe

```cpp
scene.view = glm::lookAt(CameraConfig::EYE, CameraConfig::TARGET, CameraConfig::UP);
```

Earlier matrices were built by combining `translate`/`rotate`/`scale`.
`lookAt` is different: it is handed three plain vectors -

- `EYE` - where the camera is,
- `TARGET` - the point it looks at,
- `UP` - which way "up" is, for that camera -

and constructs the matrix that re-measures every world position as if the
camera itself were sitting at the origin, looking down its own `-Z` axis. This
project's camera does not move yet - `EYE`, `TARGET`, and `UP` are constants -
so `view` does not depend on `now`. It is still rebuilt every frame here for
consistency with `projection`, and because Phase 12's orbit camera will make it
change.

## `glm::perspective`: the frustum

```cpp
scene.projection = glm::perspective(
    glm::radians(CameraConfig::FIELD_OF_VIEW_DEGREES),
    aspectRatio, CameraConfig::NEAR_PLANE, CameraConfig::FAR_PLANE);
```

This describes a **frustum**: a narrow pyramid of visible space with its point
at the camera. Four numbers define it:

| Parameter | What it controls |
|---|---|
| `FIELD_OF_VIEW_DEGREES` | How wide the pyramid opens. Wider = more visible, but everything looks smaller and more distorted near the screen edges |
| `aspectRatio` | Width divided by height of the window, so a square object on screen stays square |
| `NEAR_PLANE` | Anything closer than this is cut off |
| `FAR_PLANE` | Anything farther than this is cut off |

Only objects **inside** this pyramid are ever drawn. This project's triangle
moves between `2.5` and `5.5` units from the camera (see below), which sits
comfortably inside `NEAR_PLANE = 0.1` and `FAR_PLANE = 100.0`.

## Why the aspect ratio is read every frame, not once

```cpp
glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
updateScene(scene, clock.now, clock.deltaTime, framebufferWidth, framebufferHeight);
```

Before this phase, resizing the window only affected `glViewport`. The
triangle's own shape had no idea the window's proportions had changed, which is
exactly why it used to stretch (see below). Reading the framebuffer size every
frame and rebuilding `projection` from it means the window can be resized at
any time and the triangle's proportions stay correct.

A minimised window can report a height of `0`, and dividing by that is
undefined behaviour, so the code clamps the height used for the ratio to at
least `1`:

```cpp
const int safeHeight = std::max(framebufferHeight, 1);
```

## The stretching bug from Phase 5 and 6 is now fixed

Phase 5's explanation described the triangle stretching sideways as it spun,
because the flat `-1..+1` space mapped `640` pixels per unit horizontally but
only `360` vertically in this `1280 x 720` window. That correction is now
built into `uProjection`: it divides the horizontal term by the aspect ratio,
which exactly cancels the window's own width-to-height skew.

This was measured directly. With the triangle held still, not spinning, not
scaling, and not moving in depth, its on-screen width-to-height ratio was:

```text
predicted (true shape):  0.8 / 1.10 = 0.7273
measured on screen:      169 / 231  = 0.7316
```

Before Phase 7, the same shape would have measured close to `1.293` - stretched
by almost exactly the window's own `16:9` ratio. The fix was not to change the
triangle. It was to give the projection matrix the information it needed to
cancel the window's shape out.

## Real depth, for the first time

```cpp
const float offsetZ =
    TriangleDepth::DEPTH_AMPLITUDE * std::sin(TriangleDepth::DEPTH_SPEED * now);
const glm::mat4 slide =
    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, offsetX, offsetZ));
```

The triangle's world-space `z` now swings between `-1.5` and `+1.5`. The camera
sits at `EYE.z = 4.0` looking at the origin, so the triangle's actual distance
from the camera swings between `2.5` (nearest, looks biggest) and `5.5`
(farthest, looks smallest).

Before Phase 7, changing an object's `z` did nothing useful: with no view or
projection matrix, depth only decided whether a vertex was clipped away, never
how big it looked. Now the **same** triangle visibly grows and shrinks, because
`uProjection` performs a genuine perspective divide - the calculation real
lenses and real eyes perform, dividing `x` and `y` by depth.

This was measured against the exact formulas `glm::lookAt` and
`glm::perspective` use, with the real project constants and **no free scale
parameter** - only a single timing offset was fitted, because the capture
cannot know the exact instant the program's clock started relative to the
screenshot loop:

```text
RMS error across 90 samples: 9.0 pixels
measured far/near width ratio:   2.256
predicted far/near width ratio:  (4 + 1.5) / (4 - 1.5) = 2.200
```

## The field of view, measured

Widening the field of view should make the same object look smaller, because a
wider frustum spreads the same visible world over the same screen. This was
tested by holding the triangle still and rebuilding with two different fields
of view:

| Field of view | Measured width |
|---:|---:|
| `45 deg` | `169` px |
| `90 deg` | `67` px |

Ratio measured: `2.522`. Predicted from the cotangent term `glm::perspective`
uses internally, `f = 1 / tan(fov / 2)`: `f(45) / f(90) = 2.414`. The two agree
to within a few percent, in the correct direction: a wider lens shows more of
the world, so the same object takes up less of the screen.

**A mistake worth knowing about.** An early version of this measurement showed
a wildly wrong ratio, because the screen-capture tool briefly grabbed a
different window (in one case, an email inbox) instead of the simulator. The
program itself never crashed or misbehaved - the numbers were checked against a
direct, verified screenshot of the actual running window before being trusted.
This is worth remembering for any external tool measuring this project: verify
you are looking at the right window before trusting a number that looks
strange, because a capture tool can lie about what it saw even when the
program is completely correct.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `updateScene()` | Builds `slide` (now including depth), `spin`, `scaleMat`, `view`, and `projection` |
| `renderScene()` | Uploads all three matrices with `setMat4`, in any order - only the shader's multiplication order matters |
| `basic.vert` | Multiplies each vertex by `uProjection * uView * uModel`, unchanged in structure from Phase 4-6, just with two more matrices in the chain |

## Important changeable values

`CameraConfig` in `src/main.cpp`:

| Value | Current | What happens if it changes |
|---|---:|---|
| `EYE` | `(0, 0, 4)` | Where the camera sits. Moving it closer makes everything look bigger |
| `TARGET` | `(0, 0, 0)` | What the camera looks at |
| `FIELD_OF_VIEW_DEGREES` | `45` | Wider shows more but shrinks everything; narrower zooms in |
| `NEAR_PLANE` / `FAR_PLANE` | `0.1` / `100.0` | Anything outside this range is never drawn |

`TriangleDepth`:

| Value | Current | What happens if it changes |
|---|---:|---|
| `DEPTH_AMPLITUDE` | `1.5` | How far toward/away from the camera the triangle swings |
| `DEPTH_SPEED` | `0.8` | Radians per second of the depth swing |

## Likely teacher questions

### Why does a scene need a view matrix if the model matrix already places objects?

The model matrix places an object in the world. The view matrix answers a
different question: from where, and facing which way, is that world being
looked at? Without it, every scene only has one possible, fixed viewpoint,
sitting at the world origin facing `-Z`.

### Why does a scene need a projection matrix?

It performs the perspective divide that makes distant things look smaller, and
it defines the frustum: the region of space that is actually visible. Without
it, depth has no visual effect and nothing is ever clipped for being too far
away or too close.

### Why must they be added together, not one at a time?

A view matrix alone can move the whole scene outside the fixed `-1..+1` clip
cube the old code assumed, making everything vanish. A projection matrix alone
assumes the camera is at the origin facing `-Z`; if it is not, the divide can
send vertices to nonsensical positions. Only together do they produce a
correct, visible 3D scene.

### What is a frustum?

The pyramid-shaped region of space a camera can see, bounded by the field of
view on the sides and the near and far planes at the front and back. Only
objects inside it are drawn.

### Why does the aspect ratio matter?

Because the window is not necessarily square. Dividing the horizontal
projection term by `width / height` cancels the window's own shape out, so an
object's on-screen proportions match its true proportions, whatever the window
size is.

### Why is the aspect ratio recalculated every frame instead of once?

Because the window can be resized at any time, and `glfwGetFramebufferSize`
reports the CURRENT size. Computing it once at startup would leave the
triangle stretched after a resize.

### Why clamp the height to at least 1 before dividing?

A minimised window can report a height of `0`. Dividing by `0` is undefined
behaviour in C++, so a minimum of `1` is used only for that one division, never
stored back into the real framebuffer size.

### Why does the triangle finally shrink with distance now?

Because `uProjection`'s perspective divide scales `x` and `y` by roughly
`1 / distance`. Before this phase, `z` never reached that divide: it only
decided what fell inside or outside the flat clip cube.

### Is the depth motion pre-computed?

No, like every other motion in this project, `offsetZ` is `DEPTH_AMPLITUDE *
sin(DEPTH_SPEED * now)`, calculated fresh every frame from the clock.

### Why did the shader's multiplication order not change from Phase 6?

The new matrices are added to the LEFT of what was already there:
`uProjection * uView * (uModel)`. `uModel` still does exactly what it did in
Phase 6; it is simply no longer the last step before the screen.

## Simple viva modifications

- Zoom in: lower `FIELD_OF_VIEW_DEGREES` to `20`.
- Zoom out: raise it to `90` or higher, and watch the triangle shrink and the
  edges of the frustum distort more.
- Move the camera closer: reduce `EYE.z`, and everything grows.
- Cut off nearby objects: raise `NEAR_PLANE` above `2.5` and watch the triangle
  disappear when it swings toward the camera, past the near plane.
- Cut off distant objects: lower `FAR_PLANE` below `5.5` and watch the same
  thing happen on the far side.
- Make the depth swing bigger: raise `DEPTH_AMPLITUDE`, but keep it below `4.0`
  so the triangle never reaches the camera's own position.
- Prove the aspect fix: resize the window into a tall, narrow shape and a wide,
  short shape. The triangle's proportions should look the same in both.

## Checkpoint

Phase 7 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the triangle is visible, correctly proportioned, and moving in three
  dimensions;
- moving the triangle along `-Z` makes it shrink, not vanish or distort
  incorrectly;
- resizing the window keeps the triangle's proportions correct instead of
  stretching it;
- changing `FIELD_OF_VIEW_DEGREES`, `NEAR_PLANE`, or `FAR_PLANE` visibly and
  correctly changes what is drawn;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 7

No camera movement or mouse control yet - `EYE`, `TARGET`, and `UP` are fixed
constants. No depth testing beyond what Phase 1 already enabled, since there is
still only one object to draw. No indexed drawing and no real 3D mesh; the
triangle is still three raw vertices.

Phase 8 adds `GL_DEPTH_TEST` proved with two overlapping triangles at different
depths, which needs real depth-sorting to matter - the first time this project
draws more than one object at once.
