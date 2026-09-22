# Phase 12 - The First Orbit Camera

## Status

Phase 12 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed the
camera correctly orbiting when the arrow keys were held. Nothing was printed
to the error output, the loop stayed near `120 FPS`, and the program exited
with code `0`.

The camera was actually driven and watched, not just wired up and assumed
correct:

| Test | Result |
|---|---|
| Default view, no keys pressed | Pixel-for-pixel identical to Phase 11 - the new camera's starting position matches the old fixed one exactly |
| `RIGHT` held for 1 second | The whole scene visibly swung around, as if walking around it - about a 69 degree turn |
| `UP` held for 1 second | A visibly different change from `RIGHT` - the view tilted instead of turning |
| `UP` held for 2.5 seconds (about 172 degrees) | The view passed over the top and came out looking at the scene from the far side, upside-down relative to before - the flip this phase's explanation predicts |
| `D`, `W`, `W`, `O` pressed after orbiting | All four still worked exactly as before, from the new camera angle |

## What changed

| File | Change |
|---|---|
| `src/Camera.h` | **New file.** `OrbitCamera`, `orbitCameraPosition()`, `updateOrbitCameraFromKeys()` |
| `src/main.cpp` | `CameraConfig::EYE` removed; `SceneState` gained a real `OrbitCamera` |
| `src/main.cpp` | `processInput()` gained a `deltaTime` parameter and now drives the camera every frame |
| `src/main.cpp` | `updateScene()` computes the camera's position fresh each frame instead of reading a constant |

No shader changed. The camera's position feeds into `glm::lookAt`, exactly the
way the fixed `EYE` did in Phase 7 - only where that position comes from is
new.

## Why this earned its own file

Every file so far has been `main.cpp`, `Shader.h`, and nothing else. A camera
is different: it is a real, reusable idea on its own, and every phase from
here on needs the camera to be able to move, not just sit at a fixed spot.
Putting it in `src/Camera.h` keeps that idea in one place instead of letting
`main.cpp` grow a second, unrelated topic inside it.

## The one idea: three numbers, one position

An orbit camera never needs to store a raw `(x, y, z)` position directly.
Instead it stores:

- **`radius`** - how far away it is from whatever it is looking at;
- **`yaw`** - how far it has turned left or right;
- **`pitch`** - how far it has tilted up or down.

and works out the real position fresh, every frame, from those three:

```text
x = radius * cos(pitch) * sin(yaw)
y = radius * sin(pitch)
z = radius * cos(pitch) * cos(yaw)
```

This is the same trick Phase 5's rotation matrix used - turning an angle into
a position - just aimed at a point circling a sphere instead of an object
spinning in place. Picture a satellite: however far along its orbit it has
travelled, and however high above or below the equator it sits, those two
angles plus the one distance describe exactly where it is.

At `yaw = 0` and `pitch = 0`, the formula gives `(0, 0, radius)`. With
`radius = 4.0` as the starting value, that is `(0, 0, 4)` - the exact spot
Phase 7's fixed camera sat at. This is why the very first screenshot of this
phase looked identical to Phase 11's: nothing about the picture changes until
an arrow key is actually pressed.

## Held keys, not toggles

```cpp
if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    camera.yaw -= step;
```

The `D`, `W`, and `O` keys from earlier phases are all **toggles**: one press
should flip a setting once, which is why they all need the "was it already
down" edge-detection check. The arrow keys are the opposite. **Holding** an
arrow key is supposed to keep turning the camera for as long as it is held,
the same way a real joystick or drag gesture works. So there is no edge
detection here on purpose - every single frame the key is down, the camera
turns a little further.

`step = ORBIT_SPEED * dt` is the same dt-scaling idea Phase 1 introduced for
the clock: multiplying a speed by the time since the last frame means the
camera turns at a steady rate in real seconds, whatever the frame rate
happens to be.

## Why `deltaTime` had to travel further this phase

`updateScene()` has taken `deltaTime` as a parameter since Phase 4, but never
used it - every motion in this project until now has been a formula of `now`,
the absolute clock, not of how much time passed since the last frame.

Reading a held key is different: it genuinely needs to know how much time
passed, to know how far to turn the camera this frame. Since keys are read in
`processInput()`, not `updateScene()`, `processInput()` needed its own new
`deltaTime` parameter, and the loop in `main()` now passes `clock.deltaTime`
to it. This is the first real, working use of input-driven, dt-scaled motion
in the whole project - the same technique a future player-controlled ship
will use to steer.

## The flip, on purpose

The formula above has no limit on `pitch`. Push it far enough - past
90 degrees - and the camera does not stop; it keeps climbing, passes directly
over the top of whatever it is looking at, and starts coming back down the
far side. From the camera's point of view, "up" and "down" swap, so the
picture appears to flip.

This was tested directly. Holding `UP` for `1` second (about `69` degrees)
gave an ordinary, if dramatic, tilt. Holding it for `2.5` seconds (about
`172` degrees) produced a picture that had genuinely turned inside out: the
triangle pointed the opposite way, and both the cube's top and bottom faces
were visible together in an arrangement that never appears from a normal
angle.

Nothing here is broken. The maths is doing exactly what it was asked. The
flip is simply what an unconstrained orbit does once it passes over a pole,
and it is deliberately left in for this phase so the fix - a hard limit on
`pitch` - has something real to fix. That limit, "never more than 89 degrees
up or down", is Phase 13's job.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `updateOrbitCameraFromKeys()` (Camera.h) | Reads the arrow keys and adjusts `yaw`/`pitch`, scaled by `dt` |
| `orbitCameraPosition()` (Camera.h) | Turns `radius`/`yaw`/`pitch` into a world position |
| `processInput()` | Calls the key-reading function every frame |
| `updateScene()` | Calls the position function and hands the result to `glm::lookAt` |

## Likely teacher questions

### Why store radius, yaw, and pitch instead of just an (x, y, z) position?

Because a position alone cannot answer "turn this camera 10 degrees to the
left" without redoing trigonometry from scratch every time. Storing the
angles directly means turning the camera is as simple as adding to one
number.

### Why does the arrow-key camera control not need edge detection, unlike D, W, and O?

Because it is not a toggle. A toggle should flip once per press; an orbit
control should keep turning for as long as the key stays down. Those are
opposite requirements, so they need opposite code.

### Why did `deltaTime` finally get used in this phase?

Because reading a held key needs to know how much real time has passed to
decide how far to turn the camera. Every earlier motion in this project was a
formula of the absolute clock instead, which never needed that value.

### Why does the camera flip when pitch goes past 90 degrees?

The formula for the camera's position has no limit built into it. Once pitch
passes 90 degrees, the camera has climbed directly over the target and begun
descending the other side, and "up" and "down" swap from its point of view.
The fix is a limit on pitch, not a change to the formula itself.

### Why is this camera not clamped yet, if the flip is a known problem?

Because seeing the actual problem, unfixed, is worth more than being told
about it. Phase 13 adds the `89` degree limit as its own real change, once
the reason for it has been demonstrated directly.

### Why does `src/Camera.h` need `inline` on its functions?

Because it is a header, and a header can end up included from more than one
`.cpp` file in a larger project. Without `inline`, a plain function
definition included twice would be compiled twice, and the linker would
refuse to combine the two copies. Shader.h never needed this because its
functions are defined inside the class body, which C++ treats as `inline`
automatically.

### Does moving the camera change how far away the triangle's depth motion actually is?

Yes, and this is worth being precise about. Phase 7's `2.5` to `5.5` unit
range was measured from the camera's *original* fixed position. Now that the
camera itself can move, those two numbers describe the *starting* view only -
orbiting away changes the real distance, even though the triangle's own
motion formula has not changed at all.

## Simple viva modifications

- Turn faster or slower: change `OrbitCameraConfig::ORBIT_SPEED`.
- Start from a different angle: give `OrbitCamera`'s `yaw` or `pitch` a
  non-zero default value.
- Start farther away or closer: change `OrbitCamera`'s default `radius`.
- Demonstrate the flip on purpose: hold `UP` or `DOWN` for several seconds
  and describe what "up" and "down" mean to the camera once it has passed
  over the pole.
- Prove nothing else broke: orbit away from the default view, then press
  `D`, `W`, and `O` and show each one still works exactly as before.

## Checkpoint

Phase 12 passes when:

- Debug and Release builds succeed with no compiler warnings;
- all shaders compile and link, with no missing-uniform warning;
- the default view, with no keys pressed, looks identical to Phase 11's;
- holding `LEFT`/`RIGHT` visibly orbits the whole scene;
- holding `UP`/`DOWN` visibly tilts the view, distinctly from `LEFT`/`RIGHT`;
- pushing `UP` or `DOWN` far enough demonstrates the unclamped flip;
- the earlier triangles, quad, cube, and the `D`, `W`, and `O` keys still
  behave exactly as before, from any camera angle;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 12

No mouse control yet - dragging does nothing, and the scroll wheel does
nothing. No pitch limit, so the flip demonstrated above is still possible. No
zoom control - `radius` never changes at runtime yet.

Phase 13 replaces the arrow-key driver with mouse drag and a scroll-wheel
zoom, adds the `89` degree pitch clamp that stops the flip, and clamps
`radius` to a sensible minimum and maximum distance.
