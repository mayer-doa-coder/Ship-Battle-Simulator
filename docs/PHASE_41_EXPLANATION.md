# Phase 41 - The Keyboard Camera: `,` `.` `PageUp` `PageDown` `Q` `E`

## Status

Phase 41 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged)
Keys added: ,  .  turn      PageUp  PageDown  tilt      E  Q  zoom in / out
```

No new mesh, shader or draw call. 46 checks passed with zero failures: 28 on `applyCameraKeys()`
(the real `Camera.h`) and 18 on rendered frames driven through the project's own `processInput()`.

## The one idea

**A held key is a level, not an event - so it needs the frame's duration to become motion.**

Every key before this one is a *toggle*: it acts once, on the frame it goes down, and the program
remembers whether it was already down so a held key does not flip it 60 times a second. The
camera keys are the opposite. They must keep acting for as long as they are held, and how far the
camera has moved by then depends on how long the key was down - not on how many frames that was.

```cpp
camera.yaw    += (right - left) * KEY_YAW_RATE   * deltaTime;
camera.pitch  += (up - down)    * KEY_PITCH_RATE * deltaTime;       // then clamped to +-89 degrees
camera.radius *= exp((out - in) * KEY_ZOOM_RATE  * deltaTime);      // then clamped to [1.5, 15]
```

`processInput()` records which keys are down in a `CameraKeys` struct - with **no** "was down last
frame" memory, deliberately - and `updateScene()`, which has `deltaTime`, applies them.

## What changed

| File | Change |
|---|---|
| `src/Camera.h` | `CameraKeys`, `applyCameraKeys()`, and the rates `KEY_YAW_DEGREES_PER_SECOND` 60, `KEY_PITCH_DEGREES_PER_SECOND` 40, `KEY_ZOOM_RATE` 0.9 |
| `src/main.cpp` | `SceneState::cameraKeys`; six `glfwGetKey` reads in `processInput()`; one `applyCameraKeys()` call in `updateScene()`, before the ease update |

## The keys

| Key | Does | Why this key |
|---|---|---|
| `,` | yaw left (decreases yaw) | `<` is on it |
| `.` | yaw right (increases yaw; the eye moves towards `+x`) | `>` is on it |
| `PageUp` | tilt up (raises the eye) | the obvious name |
| `PageDown` | tilt down | |
| `E` | zoom in | |
| `Q` | zoom out | |

They were picked to leave every key the ship and cannon need: `W` `A` `S` `D` steer (Phase 79) and
the arrow keys aim (Phase 85). The mouse still works alongside.

### Zoom multiplies; it does not add

A fixed number of units per second would crawl when the camera is far from the ship and lurch
when it is close. Multiplying the radius by `exp(rate x dt)` changes it by the same *proportion*
per second at any distance: at 0.9 the distance changes by a factor of 2.46 per second held. It
also composes exactly - two frames of `dt/2` multiply to one frame of `dt` - which is why the
frame rate cannot matter.

## The same speed at every frame rate

One second of a held key, simulated at four frame rates:

```text
  30 FPS: '.' turned 60.0000 deg   PageUp raised 40.0000 deg   Q scaled the radius by 1.56831
  60 FPS: '.' turned 60.0001 deg   PageUp raised 40.0000 deg   Q scaled the radius by 1.56831
 144 FPS: '.' turned 60.0001 deg   PageUp raised 39.9999 deg   Q scaled the radius by 1.56831
 240 FPS: '.' turned 59.9997 deg   PageUp raised 40.0000 deg   Q scaled the radius by 1.56830
```

Through the real renderer, a `.` held for 1 s at 30 FPS and at 144 FPS give pictures that differ
by 21 bytes out of 2.7 million, and each is within 16 bytes of setting yaw to 95 directly (35 + 60).
PageUp for 1 s is 3 bytes from pitch 54; `Q` for 0.5 s is 7 bytes from radius 14.05.

## The limits hold

The camera keeps its Phase 13 limits - pitch `+-89` degrees, radius `[1.5, 15]` - and the keys
respect them **exactly and at every frame**, not just eventually:

```text
PageUp / PageDown held for ages at dt 1/30, 1/144 and 0.1 : stop at exactly +-89 degrees
E / Q held for ages at the same three : stop at exactly 1.5 and 15.0
at no frame did the pitch pass the limit, not even for one frame
```

The 0.1 s case is the engine's own `MAX_DELTA_TIME` clamp - the biggest step a frame can take, and
the one a limit written as "stop when it has passed" would overshoot. The clamp is applied after
the step, so it cannot. Through the renderer: PageUp held for 4 s is the same picture as pitch 89,
and `E` for 6 s and `Q` for 10 s are **byte-identical** to radius 1.5 and radius 15.

Yaw is not clamped: 10 s of `.` turned the camera 600 degrees.

## Combinations

- Two opposite keys cancel exactly, and `applyCameraKeys()` reports that nothing moved.
- Yaw, pitch and zoom work together.
- With no key held the camera is bit-for-bit unchanged; a frame of zero length moves nothing.
- A key that *really* moves the camera cancels a preset move in progress (Phase 40), for the same
  reason a drag does - they would drive the same numbers - and the key takes over from exactly
  where the move had got to. Two opposite keys move nothing and so cancel nothing. After a
  cancelled move, a preset key works normally.
- The keys work in the gallery as well: `Q` for 1 s there is 1 byte from radius `4 x e^0.9`.

## Negative controls

| Broken on purpose, in a copy of `Camera.h` | Result |
|---|---|
| yaw uses a fixed `0.016` instead of `deltaTime` | 3 failures: the error is **2.97 rad** at the wrong frame rate; 30 and 144 FPS differ by 1.9 rad |
| the pitch clamp removed | 4 failures: pitch passes 89 degrees, at all three frame times |

## Mistakes I made while testing (the code was right each time)

1. **A test duration that was not a whole number of frames.** The first run measured PageUp for
   0.25 s - which is 7.5 frames at 30 FPS, so the simulation rounded to 7 and "failed" with an
   error of 0.0116 rad. The camera was behaving correctly; the test was asking for half a frame.
   All durations are now whole numbers of frames at every rate tested.
2. **An expected radius typed to four digits.** I wrote 5.7129 for `8.96 x e^-0.45`; the value is
   5.71316. Up close a 0.0003 error is 3380 differing bytes. The expected values are now computed.
3. **A test that pressed `W`.** `W` is still the wireframe toggle until Phase 79, so the "unrelated
   keys change nothing" check failed on a picture that had correctly gone to wireframe. It now
   presses `F`, `A` and `S`.

## Likely teacher questions

### Why does the key code need `deltaTime`?
Because a held key acts every frame, and frames have different lengths. Rate x time is distance;
rate x frames is a number that changes with the computer.

### Why is zoom multiplied and not added?
So it feels the same near and far, and so it composes exactly across frame rates.

### Why are the keys read in `processInput` but applied in `updateScene`?
`processInput` reads the keyboard and knows nothing of time; `updateScene` has the frame's
`deltaTime`. The `CameraKeys` struct is the message between them.

### Why not WASD?
They are reserved for steering the ship (Stage E); the arrow keys are reserved for the cannon.

### What happens at the limit?
The value is clamped after the step. It stops at exactly 89 degrees or exactly 1.5 / 15, even for
the biggest frame the engine allows.

## Simple viva modifications

- **The named exercise: change the keyboard turn speed.**
  `OrbitCameraConfig::KEY_YAW_DEGREES_PER_SECOND` in [src/Camera.h](../src/Camera.h). 15 is a slow
  pan; 180 is half a turn per second.
- **Swap the zoom keys:** exchange `zoomIn` and `zoomOut` in `processInput()`.
- **Make zoom faster:** `KEY_ZOOM_RATE`. Double it and the same key press zooms twice as far.
- **Break frame-rate independence on purpose:** delete `* deltaTime` from the yaw line and hold
  `.` - it now turns twice as fast on a 120 Hz screen as on a 60 Hz one.

## Checkpoint

Phase 41 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- a held key turns, tilts or zooms the camera at the same rate at 30, 60, 144 and 240 FPS;
- the pitch clamp and the zoom limits hold at every frame, including at `dt` = 0.1;
- through the real `processInput()` and update loop, each key's picture matches the same camera
  set directly, and the limits give byte-identical pictures;
- a key that moves the camera cancels a preset move; opposite keys do not;
- the gallery is byte-identical to Phase 33 (ship hidden) and Phase 37 (ship shown);
- the showcase is still 15 draws / 496 triangles / 507 vertices.

## What is not part of Phase 41

The mouse is unchanged. No smoothing or acceleration of the keys. No camera collision with the
sea. The keys do not yet appear in a HUD (Phase 102).

**Phase 42 is next**: 4x multisample anti-aliasing, with a fallback if the driver refuses it.
