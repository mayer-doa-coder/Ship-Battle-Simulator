# Playable build - sail, aim, fire

Added after Phase 53, on top of the ship as it stands, at the student's request. The ship's own phases (Stage D) are paused at Phase 53;
this is a separate step that makes the showcase playable. All of it lives in [src/main.cpp](../src/main.cpp) (search for `PlayConfig`,
`updatePlay`).

## Controls

| Input | Action |
|---|---|
| Arrow keys, or `W` `A` `S` `D` | forward, back, turn left, turn right |
| Mouse (just move it) | the point on the sea under the cursor is the target; the barrel turns and rises so the ball lands there |
| `SPACE` | fire one ball (one per press, 0.6 s reload) |
| Left-drag, scroll, `,` `.` `E` `Q`, `F1`-`F5`, `R` | camera, as before; the camera now follows the player's ship |
| `ESC` (pause + options menu), right mouse button, `I` `C` `T`, `F11`, `F12` | added by the environment build - see [ENVIRONMENT_BUILD.md](ENVIRONMENT_BUILD.md) (scroll now zooms out to 30 units) |

The two debug toggles that used `W` and `D` moved: wireframe is **`X`**, depth test is **`V`**. Everything else is unchanged. `G` still
swaps to the test gallery, where the ship is not driven.

The dots are the arc the next ball will fly (the big one is the impact point). The enemy is a second copy of the ship lying at
`PlayConfig::ENEMY_POSITION`, bow towards where you start. A ball that enters its hull prints `[combat] HIT`; one that reaches the water
prints `[combat] splash`.

## How it works

- **Ship movement is state** (position, heading, speed), advanced by the frame's duration `dt`. Speed eases towards the key's request;
  heading turns at a fixed rate. A positive heading turns the bow (+z) towards +x, the ship's left.
- **Aiming**: the cursor becomes a ray through last frame's camera, cut by the plane `y = sea level`. The elevation that lands a ball
  there is found by scanning angles (`playSolveElevation`, -20 to +60 degrees), made a world direction, taken into the ship's space with the
  transpose of the root's rotation, and read off as azimuth and elevation for `buildShipFrames`. The muzzle moves as the barrel turns, so
  this runs three passes. A cursor above the horizon aims as far out as the gun reaches.
- **The flight** is the closed form `p = p0 + v0 t + 1/2 g t^2` of the ball's age, not a stepped integration; the position and direction
  of the muzzle come from `shipMuzzlePosition` (w = 1) and `shipMuzzleDirection` (w = 0).
- **The enemy** is `buildShipFrames` called a second time and `drawShip` a second time: the same function, the same meshes.

## Measured (headless, with the project's own `updateScene` and a scripted keyboard and mouse)

```text
UP held 120 frames   : z = 3.984 (hand calculation 3.96), speed 3.00;  DOWN: z = -2.44, speed -1.50;  W and the arrow give the same result
LEFT / A 60 frames   : heading +0.754 rad (= 45 deg/s x 0.96 s);  RIGHT / D: -0.754;  camera target follows the ship exactly
reload               : SPACE at frames 30, 35, 80 fires shots at 30 and 80 only;  SPACE held 100 frames fires once
aim accuracy         : 7 cursor positions, landing point of the ball's own p0 and v0 against the cursor's target worked out
                       independently with glm::unProject: 0.002 to 0.185 units (targets from 2.5 to 36 units away)
shot at the enemy    : cursor on the enemy, 1 shot, [combat] HIT
build                : Debug, Release and strict /W4 clean; 282 draws, 18324 triangles, 16201 vertices (worked out by hand first)
```

## Limits

- The nearest the ball can land is limited by the barrel's 20 degree depression; the farthest by `MUZZLE_SPEED^2 / GRAVITY` (about 69 units
  from a muzzle at water level, less from the deck).
- Only the enemy's hull box is hit-tested; its masts, sails and castles are not. The enemy does not move, fire back or sink.
- The ship's roll is still the fixed 6 degree heel; waves do not move it (Phase 83 in the plan).
- A ball in flight when `G` is pressed is frozen until you return.
