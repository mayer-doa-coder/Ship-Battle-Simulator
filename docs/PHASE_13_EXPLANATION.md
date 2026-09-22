# Phase 13 - Mouse-Driven Orbit, and Real Limits

## Status

Phase 13 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed the
mouse correctly orbiting and zooming the camera, with both limits holding.
Nothing was printed to the error output, the loop stayed near `120 FPS`, and
the program exited with code `0`.

Every piece of this phase was driven with real simulated mouse input and
watched, not just wired up and assumed correct:

| Test | Result |
|---|---|
| Default view, no mouse used | Unchanged from Phase 12 |
| Left-drag 300 px sideways | A dramatic, correct orbit - about 86 degrees of yaw |
| Scroll forward 5 ticks | Zoomed in hard enough to fill the screen with one triangle - `radius` hit its `1.5` minimum |
| Scroll backward 40 ticks (far more than needed) | Zoomed out to a small, distant view - `radius` held at its `15.0` maximum instead of continuing to shrink |
| Drag straight up by 800 px (would be about 229 degrees unclamped) | The view stopped at a clean, sensible near-top-down angle - no flip |
| Drag straight down by 800 px | The same, symmetrically, from below |
| `D`, `W`, `W`, `O` tested after dragging and scrolling | All four still worked exactly as before, from the new camera position |

## What changed

| File | Change |
|---|---|
| `src/Camera.h` | `updateOrbitCameraFromKeys()` removed; two new GLFW callbacks added; pitch and radius are now clamped |
| `src/main.cpp` | Arrow-key camera call removed from `processInput()`; the now-unused `deltaTime` parameter removed with it |
| `src/main.cpp` | `main()` registers the camera on the window and connects the two new callbacks |

No shader changed. The camera's position still feeds `glm::lookAt` exactly
the way it did in Phase 12 - only how it is driven, and how far it can go, are
new.

## Why the driver had to be replaced, not just added to

Phase 12 read the arrow keys once per frame, inside `processInput()` - simple
polling, the same technique `D`, `W`, and `O` already used. A mouse drag does
not fit that pattern as naturally. GLFW does not report "the mouse is
currently at this position" as something to poll each frame; it reports "the
mouse just moved" and "the wheel just scrolled" as **events**, through
**callbacks** - functions GLFW itself calls, at the moment the event happens.
Using the mouse properly means writing those two callbacks and registering
them, which is a different mechanism from anything used so far.

## The problem with C-style callbacks, and GLFW's fix for it

```cpp
inline void handleOrbitCameraCursorMove(GLFWwindow* window, double xpos, double ypos)
{
    OrbitCamera* camera = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(window));
    ...
}
```

GLFW is a C library. Its callback functions are plain function pointers, and
a plain function pointer cannot "remember" anything about the C++ program
that registered it - it cannot capture `scene.camera` the way a C++ lambda
could. Every time GLFW calls this function, it hands back only the three
things it knows about: the window, and the new cursor coordinates.

GLFW's answer is `glfwSetWindowUserPointer` / `glfwGetWindowUserPointer`: a
single, free "sticky note" attached to the window itself, where a program can
leave a pointer to whatever it needs later. `main()` writes a pointer to the
camera onto the window once, and every callback that receives that same
window can read the note back out and get straight to the camera it needs.

```cpp
glfwSetWindowUserPointer(window, &scene.camera);
```

## Drag detection: polling a button from inside an event

The cursor-move callback fires on **every** mouse movement, whether or not a
button is held. Orbiting should only happen while the left button is
genuinely held down - the "click and drag" gesture, not "the camera follows
the mouse everywhere on screen".

```cpp
if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    return;
```

This mixes an **event** (the callback firing because the mouse moved) with a
**poll** (asking, right now, whether the button happens to be down). Both are
legitimate ways to read input, and there is nothing wrong with combining them
inside the same function - the callback answers "did something happen?", and
the poll answers "under what condition?".

## Why the cursor position is tracked even when not dragging

```cpp
const double dx = xpos - camera->lastCursorX;
const double dy = ypos - camera->lastCursorY;
camera->lastCursorX = xpos;
camera->lastCursorY = ypos;

if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    return;
```

`lastCursorX`/`lastCursorY` are updated **before** the early return, every
single time the mouse moves, dragging or not. If they were only updated while
dragging, the very first movement of a new drag would compare the current
position against a position from however long ago the *previous* drag ended -
potentially producing a huge, wrong jump the instant the button is pressed.
Keeping the tracked position always current means the first real movement of
any drag starts from an accurate, recent baseline.

The very first value these two fields ever hold matters for the same reason.
Left at their default of `0.0`, the first mouse-move event of the whole
program would measure a delta from the corner of the screen, not from
wherever the cursor actually started. `main()` fixes this by asking GLFW for
the real starting position before the loop begins:

```cpp
glfwGetCursorPos(window, &scene.camera.lastCursorX, &scene.camera.lastCursorY);
```

## The pitch clamp: fixing Phase 12's flip on purpose

```cpp
const float maxPitch = glm::radians(OrbitCameraConfig::MAX_PITCH_DEGREES);
camera->pitch = std::clamp(camera->pitch, -maxPitch, maxPitch);
```

Phase 12 demonstrated, deliberately, what happens with no limit on `pitch`:
past 90 degrees the camera passes directly over its target and the picture
flips. The fix here is exactly one line, applied every time the mouse
changes `pitch`: clamp it to `89` degrees each way, just short of that pole.
`std::clamp` (from `<algorithm>`, already used elsewhere in this project for
`deltaTime`) returns a value forced into a range - here, `pitch` can get
arbitrarily close to straight up or straight down, but never far enough to
tip over.

This was tested against the exact scenario Phase 12 used to demonstrate the
bug: an equivalent drag of about `229` degrees of pitch. Where Phase 12
showed an inverted, backwards-pointing scene, this phase's build showed a
clean, valid near-vertical view instead - the flat quad correctly collapsed
edge-on, exactly what a plane looks like viewed almost along its own surface,
with nothing flipped or reversed.

## The radius clamp: stopping the zoom from breaking the scene

```cpp
camera->radius = std::clamp(
    camera->radius, OrbitCameraConfig::MIN_RADIUS, OrbitCameraConfig::MAX_RADIUS);
```

Without a limit, scrolling in far enough would let the camera pass through
`NEAR_PLANE` and clip straight through the middle of an object, and scrolling
out far enough would shrink everything to invisible specks. `MIN_RADIUS` and
`MAX_RADIUS` stop both. This was tested at each extreme: `5` scroll ticks
forward reliably reached the `1.5` minimum, and `40` ticks backward - far more
than the roughly `22` needed to reach the limit from the default `4.0` - still
stopped exactly at `15.0` rather than continuing to shrink the scene forever.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `handleOrbitCameraCursorMove()` (Camera.h) | Called by GLFW on every mouse move; updates `yaw`/`pitch` only while dragging, then clamps `pitch` |
| `handleOrbitCameraScroll()` (Camera.h) | Called by GLFW on every scroll; updates and clamps `radius` |
| `main()` | Registers the camera pointer and both callbacks once, before the loop starts |

## Likely teacher questions

### Why can a GLFW callback not just use a C++ lambda that captures the camera?

GLFW's callback types are plain C function pointers, and only a lambda with
an **empty** capture list can convert to one. A lambda that captured
`scene.camera` by reference could not be used here at all - `main()` uses a
plain function plus the window's user pointer instead.

### What is `glfwSetWindowUserPointer` actually for?

It is a single slot on the window object where a program can store one
pointer, retrievable later by anything holding that same window - most
usefully, from inside a callback that GLFW itself invokes and that otherwise
has no way to reach the rest of the program's data.

### Why check the mouse button state inside the cursor-move callback instead of in `processInput()`?

Because the check needs to happen at the exact moment a movement event
arrives, and only a movement event tells this program a new position exists
at all. Polling the button separately, once per frame, would work almost as
well, but checking it right where the position is used keeps the two ideas -
"did the mouse move" and "is the button down" - together in one place.

### Why track the cursor position even when the button is not held?

So that the moment a drag actually starts, the position it is compared
against is recent and accurate, instead of being however old the last drag's
final position was.

### What exactly does the pitch clamp prevent?

The camera passing directly over its target and coming out the other side
upside down, relative to how it looked a moment before - the flip Phase 12
demonstrated on purpose.

### Why choose 89 degrees instead of a full 90?

At exactly 90 degrees the camera would sit directly on the vertical axis it
orbits around, where "which way is forward" briefly stops being well
defined. Stopping one degree short keeps the camera always looking at the
target from a genuine angle, however steep.

### What stops the camera from zooming through an object, or out to nothing?

`MIN_RADIUS` and `MAX_RADIUS`, applied by `std::clamp` every time the scroll
wheel changes `radius`. Neither limit can be exceeded no matter how much or
how fast the wheel is scrolled.

### Why did `deltaTime` get removed from `processInput()` in this phase?

Because its only user was the arrow-key driver, which this phase deletes.
Mouse callbacks fire from real events, not from a per-frame poll, so they
have no need for a frame's elapsed time at all.

## Simple viva modifications

- Change the drag sensitivity: edit `OrbitCameraConfig::MOUSE_SENSITIVITY`.
- Change the zoom limits: edit `MIN_RADIUS` or `MAX_RADIUS` - the viva
  modification this phase is named for in the plan.
- Change how fast scrolling zooms: edit `ZOOM_SPEED`.
- Loosen or tighten the pitch limit: edit `MAX_PITCH_DEGREES`, and try setting
  it to `90.0f` or above to see the flip return.
- Demonstrate the clamp directly: drag far enough in one direction to reach
  the limit, keep dragging further, and show the view holding steady instead
  of continuing to turn.

## Checkpoint

Phase 13 passes when:

- Debug and Release builds succeed with no compiler warnings;
- all shaders compile and link, with no missing-uniform warning;
- the default view, with no mouse input, looks identical to Phase 12's;
- left-drag visibly and smoothly orbits the camera;
- scrolling visibly zooms in and out, and holds at fixed limits at each
  extreme instead of continuing past them;
- dragging far enough in pitch never flips the view;
- the earlier triangles, quad, cube, and the `D`, `O`, and `W` keys still
  behave exactly as before, from any camera position;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 13

No reusable mesh class yet - the triangle, quad, and cube still each have
their own near-identical `create`/`destroy` functions and GPU-handle structs,
written out by hand. No lighting. No ships.

Phase 14 introduces `src/Mesh.h`: one `Vertex` struct and one `Mesh` type that
owns its own VAO/VBO/EBO and can draw itself, replacing the three separate,
repeated versions of that same idea this project has built up so far.
