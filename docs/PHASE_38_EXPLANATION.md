# Phase 38 - The Showcase: the Ship at Galleon Scale, and `G` for the Gallery

## Status

Phase 38 is complete and verified. It is a **graded milestone** (`*` in the plan) and the first
phase of Stage D2 (showcase, camera and atmosphere).

```text
Showcase : draws 15 | tris 496  | verts 507      (1 sea + 14 ship parts)
Gallery  : draws 28 | tris 2208 | verts 1471     (exactly Phase 37)
```

The program now **opens on the showcase**: a sea 120 across and the ship at 4 times its
prototype size, with the camera looking at the ship. `G` swaps in the whole Stage C test
gallery and back. No new mesh and no new shader code.

70 checks passed with zero failures: 36 on the ship's numbers and frames, 34 on images.

## The one idea

**Make the ship bigger by changing the numbers it is built from, never by scaling a matrix.**

```cpp
ShipDimensions big = scaleShipDimensions(ShipConfig::DEFAULT_DIMENSIONS, 4.0f);
ShipFrames frames  = buildShipFrames(root, big);
```

A matrix scale on the root would reach every child (Phase 34's rule): the masts would be
stretched along with the hull and no frame below the root would be a rigid motion any more.
`scaleShipDimensions()` multiplies the *lengths* - hull size, deck thickness, mast heights, the
cannon's mount - and `buildShipFrames()` turns them into translations as it always did. Every
frame that comes out is still a rotation plus a translation, at any `k`.

`yardHeightFraction` is the one field it must **not** multiply: it is a fraction of the mast's
height, so the yard already rises with a taller mast, and multiplying it would put the yard
through the masthead.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `scaleShipDimensions(d, k)` |
| `src/Camera.h` | `OrbitCamera::target` (default `(0,0,0)`) and `orbitCameraEye()` |
| `src/main.cpp` | `ShowcaseConfig` (`SHIP_SCALE`, `SEA_SIZE`, the start camera); `SceneState::galleryVisible`, `otherCamera`, `shipDims`; `setGalleryVisible()`; the `G` key; `drawShip()` takes its dimensions as an argument; draws 2 to 11 and the three lighting spheres only run in the gallery |

The camera's target used to be a constant, `CameraConfig::TARGET`, fixed at the origin. The ship
does not stand at the origin, so the target moved *into* the camera. Its default is still
`(0, 0, 0)` and `orbitCameraEye()` adds it to the orbit offset, so nothing in the gallery moves
by a single float.

## Two scenes, one program

| | Showcase (start) | Gallery (`G`) |
|---|---|---|
| Sea | the grid mesh drawn **120** across | the same mesh drawn **4** across |
| Test objects | skipped | all of them, in Phase 37's order |
| Ship | `SHIP_SCALE` 4, at the sea's centre, heeled 6 degrees | prototype size, at the back, rolled 45 degrees |
| Camera | 9 units from the ship, yaw 35, pitch 14 | 4 units from the origin, as in every phase to 37 |
| Point light | off | on |

`G` calls `setGalleryVisible()`, which **swaps** the two cameras' views (radius, yaw, pitch,
target) rather than resetting them. Each scene keeps wherever you left its camera, and `G`, `G`
returns to exactly the picture you started from. It does not swap the cursor memory, which
describes the mouse and not the view: swapping it would make the first drag after a toggle jump.

The gallery is not a second build. It is the *same* draw calls, skipped in the showcase and
unchanged in the gallery - which is what lets the regression below be exact.

### Decisions that are easy to miss

- **The showcase heel is 6 degrees, not 45.** The gallery's 45 is the Phase 37 proof and is
  deliberately violent; as a centrepiece it would look like a wreck. `H` still clears the heel
  and every part still follows the root. Phase 83 replaces the constant with the real roll.
- **The point light is forced off in the showcase.** Phase 30's test light is parked 0.95 above
  the gallery's floor; at the showcase's scale that spot is *inside* the hull and would light the
  timbers from within. `L` still cycles its mask, but in the showcase only the sun bit has any
  effect (checked: sun-only and both-lights give the same picture there, and different pictures
  in the gallery). Phase 43 moves this decision into the lighting profile; the real point light
  is the muzzle flash, Phase 90.
- **`SHIP_SCALE` is 4.** It makes the hull 5.6 long. Nothing else depends on it yet; Phase 39
  stores camera distances in hull lengths so they will follow it.

## Evidence

### The ship's numbers (36 checks, the real `Ship.h`)

```text
the struct read as 33 floats: every one is multiplied by exactly k, or left untouched,
                              and the untouched ones are EXACTLY the 2 yardHeightFractions
k = 1                       : bit-for-bit the original
k = 0.25 ... 10             : all 18 frames rigid (worst basis error 1.5e-08), every
                              orientation unchanged, every position exactly k times the original
14 model matrices at k=0.5,2,4,10 : each side exactly k times its k=1 length (to 5e-06), each
                              centre exactly k times its k=1 position
a posed ship at k=4         : equals  root * (the ship at the origin)  for all 18 frames (4.8e-07),
                              muzzle position follows the root, muzzle DIRECTION ignores k
```

**The test has a negative control.** With the line `c.muzzleZ *= k;` deleted from a copy of
`Ship.h`, the same test fails 14 checks - including the "untouched floats are exactly the
fractions" one, which is the check that exists to catch a forgotten length. With the real file it
passes all 36.

One thing the test taught: a quad (sail, flag) has no thickness, so its z scale factor is 1 at
every `k` and its third column does *not* grow. My first version compared all three columns and
failed on the six quad parts. The code was right; the test now compares a quad's two real sides
and requires its third to stay 1.

### The images (34 checks, the real `processInput()` and renderer)

```text
gallery, ship hidden   vs the real Phase 33 (git HEAD)  : 10 views, 0 differing bytes in each
gallery, ship shown    vs Phase 37 (the sources saved before the 5 October renumbering) : 10 views, 0 differing bytes in each
G once == gallery;  G twice == showcase;  G three times == gallery  (byte-identical)
G held 5 frames toggles ONCE;  an unrelated key changes nothing
H in the showcase == the level ship;  sun-only == both lights in the showcase
the ship in the showcase: x 369..819, y 129..650 of 1280 x 720 (35% of the width, 72% of the height)
```

The ten views are the default, from above, Flat, Gouraud, lowest detail, highest detail, from
behind, the normals view, specular-only, and Blinn-Phong. Because the gallery is byte-identical to
Phase 37 *with* the ship and to Phase 33 *without* it, neither Demo A nor Demo B nor any Stage C
or hierarchy result can have moved.

## Likely teacher questions

### Why not just scale the ship's root matrix?
Because a scale in a parent is inherited by every child. The mast would be stretched with the
hull, and no frame under the root would be a rigid motion. Scaling the *dimensions* instead
leaves the root alone: the builder is simply given bigger numbers.

### Why is `yardHeightFraction` not scaled?
It is a fraction of the mast's height, already relative. A taller mast carries its yard up with
it. Multiplying it by `k` would put the yard through the masthead.

### How do you know no length was forgotten?
The test reads the struct as an array of floats and requires that exactly the fractions are left
alone. Deleting one multiplication makes it fail - shown with a negative control.

### Is the gallery a separate program?
No. It is the same draw calls. In the showcase they are skipped; in the gallery they run in the
original order, which is why the output is byte-identical to Phase 33 and Phase 37.

### Why did the camera get a target?
Every view before this one looked at the origin. The ship is not there. The target is part of
the camera now; its default is the origin and adding `0.0f` changes no float, so nothing moved.

## Simple viva modifications

- **The named exercise: change `k`.** `ShowcaseConfig::SHIP_SCALE` in
  [src/main.cpp](../src/main.cpp). Try 1 (prototype size) or 8 (the hull is 11.2 long, so
  raise `CAMERA_RADIUS` as well to keep it in frame). Nothing stretches.
- **Make the sea bigger or smaller:** `ShowcaseConfig::SEA_SIZE`.
- **Change the heel:** `ShowcaseConfig::DISPLAY_ROLL`; `H` clears it.
- **Start in the gallery:** set `SceneState::galleryVisible` to `true` and swap the two cameras
  in `SceneState` - or just press `G`.
- **Show the scale is data:** print `ShipConfig::DEFAULT_DIMENSIONS.hullSize` and
  `scene.shipDims.hullSize` - 1.4 and 5.6 - then print `inverse(deck) * mount` in both: only the
  numbers inside it differ, not its form.

## Checkpoint

Phase 38 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- with the gallery on and the ship hidden, ten renders are byte-identical to the real Phase 33;
- with the gallery on and the ship shown, the same ten are byte-identical to Phase 37;
- with the gallery off the whole ship is inside the window with a margin and fills most of it;
- every frame is rigid at `k` = 0.25, 0.5, 1, 2, 4 and 10, and every model matrix is uniformly
  `k` times its original size;
- `G` toggles once per press and survives being held;
- the showcase is 15 draws / 496 triangles / 507 vertices and the gallery is 28 / 2208 / 1471.

## What is not part of Phase 38

No new geometry (the hull is still a box), no camera presets (Phase 39), no lighting change
(Phase 43), no horizon or sky: the sea ends in a visible edge against the golden clear colour
and the far corners are cut off by the far plane. Those are Phases 43 to 46, and the picture is
meant to look unfinished until then.

**Phase 39 is next**: five named camera presets on `F1` to `F5`, and `R`.
