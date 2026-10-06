# Phase 40 - Smooth Camera: Gliding to a Preset

## Status

Phase 40 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged)
Easing   : 0.8 s, smoothstep, the short way round, cancelled by a drag or a scroll
```

No new mesh, shader or draw call. 45 checks passed with zero failures: 28 on the easing
functions (the real `Camera.h`, driven with explicit frame times) and 17 on rendered frames.

## The one idea

**Store where a move started, where it ends, and how much time has passed; work the camera out
from those three things each frame.**

```cpp
struct CameraEase {
    bool  active;  float elapsed, duration;
    float fromYaw, fromPitch, fromRadius;  glm::vec3 fromTarget;
    float toYaw,   toPitch,   toRadius;    glm::vec3 toTarget;
};
```

This is *state*, not a recording. Nothing holds a path or a table of positions, so it is not a
pre-computed animation (Requirement 12): the camera at any moment is a closed form of
`elapsed / duration`.

## What changed

| File | Change |
|---|---|
| `src/Camera.h` | `CameraEase`, `OrbitCamera::ease`, `OrbitCameraConfig::EASE_SECONDS`, `wrapAngle()`, `startCameraEase()`, `updateCameraEase()`; a drag and a scroll cancel a move |
| `src/main.cpp` | `applyCameraPreset()` starts a move instead of assigning; `updateScene()` advances it once per frame, before the view matrix is built; `G` cancels a move |

`deltaTime` had been passed into `updateScene()` since Phase 1 and ignored (there was a
`static_cast<void>(deltaTime)` with a comment promising it for "a later phase"). This is the
first use, and the cast is gone.

## The three ideas inside it

### 1. Smoothstep, so it starts and stops gently

```cpp
const float s = t * t * (3.0f - 2.0f * t);      // t = elapsed / duration, 0 to 1
camera.yaw = fromYaw + (toYaw - fromYaw) * s;   // and the same for pitch, radius, target
```

`s` runs from 0 to 1 as `t` does, and its slope is zero at both ends. Measured, over 1000 steps
of a unit move: the first step is `3.0e-06` and the last `3.0e-06` against an average step of
`1.0e-03`, and the middle step is `1.50e-03` - exactly 1.5 times the average, which is
smoothstep's steepest slope.

### 2. The same real time at every frame rate

`updateCameraEase()` adds the frame's `deltaTime` to `elapsed`. The move finishes after 0.8 s of
real time whether that took 24 frames or 115:

```text
dt 0.0333 (30 FPS)  : finished after 0.800 s
dt 0.0167 (60 FPS)  : finished after 0.817 s
dt 0.0069 (144 FPS) : finished after 0.806 s
dt 0.1000 (the engine's clamp) : finished after 0.800 s
```

(Each is within one frame of 0.8 s, because a frame cannot end part-way.) At 0.4 s a 100 FPS
camera and a 20 FPS camera agree to `7e-07`; through the real renderer the two pictures differ
by **2 bytes** out of 2.7 million.

### 3. The short way round

A camera's yaw is just a number that mouse drags keep adding to, so it can be 350 degrees or
`7 * 360 + 17`. Easing "towards 35" by plain subtraction from 350 swings 315 degrees the long
way; from seven turns up it would spin seven times backwards. `wrapAngle()` folds any angle into
`[-pi, pi)`, and `startCameraEase()` first replaces the camera's yaw by its nearest equivalent to
the destination - a whole number of turns added or removed, identical on screen - so the move
is a straight line between two numbers that is also the short way:

```text
350 -> 35 degrees : travels 45.0 degrees in total   (not 315)
seven turns of accumulated drag : travels 0.200 rad  (not 44 rad)
```

`wrapAngle()` is the same function Phase 87 needs for the cannon's turret ("without it the
turret swings 357 degrees the wrong way every time the bearing crosses the branch cut"). It is
written here, tested here, and reused there.

## Arriving exactly

On the frame the time runs out, the camera is set to the preset's numbers *directly* - not to
whatever the formula gives at `t = 1`. So the arrival is not merely close:

```text
100 runs (5 x 5 presets x 4 frame rates): yaw, pitch, radius and target are EQUAL (==) to the
preset's in every one, and the ease has finished
```

and through the renderer, after 0.96 s a move to each of `F1` to `F5` produces **the Phase 39
picture with 0 differing bytes**. The easing changes how the camera gets there and nothing
about where.

## No jump

The bound is the formula's own: smoothstep's steepest slope is 1.5 times the average, so no
frame can move any quantity by more than `1.5 x (total change) x dt / duration`.

```text
100 runs: the largest step of any quantity, as a fraction of that bound : 0.999
at 30 FPS or faster the largest yaw step in any run : 11.22 degrees (a 180-degree move)
```

A move that is interrupted starts from where the camera *is*: pressing a second preset 25 frames
in moves the camera by `0.00` and then heads for the new destination.

## A drag or a scroll takes control back

```text
cursor moved with the button UP        : the move carries on
a drag                                 : cancelled; the drag continues from where the camera had
                                         got to (yaw moved by exactly the drag, pitch likewise)
a scroll                               : cancelled (the zoom and the move would both drive radius)
later frames after a cancel            : leave the camera alone - no tug-of-war
G during a move                        : cancelled, and the move does NOT follow the swap
```

Pressing `G` twice mid-move returns to exactly the position the move had reached (checked
against a render of that frame count), not to the destination: a cancelled move is not resumed.

## Negative controls

A test that cannot fail proves nothing, so each of the two ideas above was removed in turn from
a copy of `Camera.h`:

| Removed | Result |
|---|---|
| the fold to the short way (`wrapAngle` line in `startCameraEase`) | 2 failures: 350 -> 35 travels **315.0** degrees; seven turns travels **43.8** rad |
| smoothstep, replaced by `s = t` | 2 failures: first and last steps are `1.0e-03`, not `3.0e-06`; the middle step is not 1.5x |

## Likely teacher questions

### Is this a pre-computed animation?
No. Only the start, the end and the elapsed time are stored. The camera at any moment is
computed from `elapsed / duration`.

### Why smoothstep and not a straight line?
A straight line starts and stops at full speed; the camera lurches. Smoothstep's slope is zero at
both ends. Measured: its first step is 0.3% of the average step.

### What if the frame rate changes?
Nothing changes. The move is driven by `deltaTime`, so 30 FPS and 144 FPS take the same 0.8 s.

### Why does 350 -> 35 not swing the long way?
`wrapAngle()` folds the difference into `[-pi, pi)`. Without it the camera travels 315 degrees;
the test proves it by removing the line.

### Why not let the move finish when the user drags?
The two would fight over the same numbers and the view would lurch. The drag wins.

## Simple viva modifications

- **The named exercise: change the easing time.** `OrbitCameraConfig::EASE_SECONDS` in
  [src/Camera.h](../src/Camera.h). 0.2 is a snap; 3.0 is a slow pan.
- **Change the shape:** replace `s = t*t*(3 - 2t)` with `s = t` (linear - watch it lurch) or
  `s = t*t` (starts slowly, stops abruptly).
- **Ease something else:** the fields are plain; field of view could be added to `CameraEase` the
  same way.

## Checkpoint

Phase 40 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- no frame-to-frame step exceeds the formula's bound, at 30, 60 and 144 FPS and at the engine's
  0.1 s clamp, and every run arrives on the preset with **equality**;
- a preset reached by easing is byte-identical to the Phase 39 jump to it;
- the move is monotone towards the destination, away from the start, and barely starts in the
  first two frames;
- a drag or scroll cancels it, `G` cancels it, a second preset restarts from the current place;
- the gallery is still byte-identical to Phase 33 (ship hidden) and Phase 37 (ship shown).

## What is not part of Phase 40

No keyboard camera (Phase 41). No easing of the field of view or of anything but the four view
numbers. The easing time is one constant for all presets.

**Phase 41 is next**: `,` `.` for yaw, `PageUp` `PageDown` for pitch, and `Q` `E` for zoom.
