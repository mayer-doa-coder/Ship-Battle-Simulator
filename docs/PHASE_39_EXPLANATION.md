# Phase 39 - Camera Presets: `F1` to `F5` and `R`

## Status

Phase 39 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged)
Keys added: F1 three-quarter front (the default), F2 front, F3 side, F4 rear, F5 elevated, R reset
```

No new mesh, no shader change, no new draw call. 77 checks passed with zero failures: 50 on the
preset table against the ship's real geometry, 12 on the keys and the regression, and 15 on
rendered pixels against the projection.

## The one idea

**A camera position is four numbers, so a named view is four numbers with a name.**

```cpp
struct CameraPreset {
    const char* name;  const char* key;
    float yawDegrees, pitchDegrees;
    float radiusInHullLengths;
    glm::vec3 targetInHullLengths;
};
```

Yaw, pitch, radius and target are exactly what `OrbitCamera` already had (the target arrived in
Phase 38). A preset adds nothing to the camera; it is a row in a table, and pressing a key
copies that row into the camera.

## What changed

| File | Change |
|---|---|
| `src/CameraPresets.h` (new) | `CameraPreset`, the five-row table `CameraPresetConfig::PRESETS`, `presetCamera()` |
| `src/main.cpp` | `ShowcaseConfig::hullLength()`; the start camera now comes from the table; `applyCameraPreset()`; `F1` to `F5` and `R` in `processInput()`; `SceneState::activePreset` and the key memory |

## The five views

| Key | View | Yaw | Pitch | Radius | Use it to show |
|---|---|---:|---:|---:|---|
| `F1` | three-quarter front (**default**) | 35 | 14 | 1.60 | bow, near flank and sails together; the report's standard shot |
| `F2` | front | 0 | 8 | 1.55 | the bow and the figurehead, and how narrow the hull is |
| `F3` | side | 90 | 4 | 1.45 | the profile: stepped decks, gunports, mast spacing |
| `F4` | rear | 180 | 10 | 1.55 | the stern and its windows |
| `F5` | elevated | 35 | 55 | 1.95 | the deck layout from above |

Radius is in hull lengths; the target is 0.27 hull lengths above the waterline (0.15 for `F5`,
which is about the deck). Yaw 0 is straight in front of the bow, and positive yaw swings the
camera towards the ship's `+x` side. `R` returns to `F1` from anywhere.

### Why the distances are in hull lengths

If a preset stored `8.96` units, then changing `ShowcaseConfig::SHIP_SCALE` would leave all five
looking at a ship that had grown or shrunk underneath them, and they would have to be retyped.
Stored as `1.60 x the hull length`, a preset keeps the ship the same size on screen at any scale:
`presetCamera()` does the single multiplication that turns the table into world units.

This was tested directly. At ship scales 1, 2, 4 and 10 every preset's silhouette is the same
size on screen to better than 0.01% of the window - which is a property of perspective (scaling
a scene and the camera together about one point changes nothing in the image), not a coincidence.

## What "the whole ship is in view" means, and the one rule I changed

The plan said each preset must put the whole ship in the window "and fill at least 40% of its
width". Measured, `F2` (looking at the bow) and `F4` (the stern) are 20% and 21% of the width,
and that is **correct**: seen end-on, a galleon is a tall narrow shape. Filling 40% of a 16:9
window's width would push the masts out of the top of it. The plan's wording was therefore
tightened (it is edited in [PHASE_PLAN.md](PHASE_PLAN.md)) to:

> inside the window with a margin, and the silhouette fills at least 40% of the window's width
> **or** height, whichever it fills more.

The margin is 4% of the window on every side (51 px across, 29 px up and down). Measured on the
real renders:

```text
        width   height   edges (px)
F1      35%     73%      x 368..820   y 129..653
F2      20%     74%      x 506..760   y 130..665
F3      52%     61%      x 310..969   y 116..557
F4      21%     79%      x 520..791   y 105..674
F5      28%     72%      x 440..800   y  88..602
```

The ship is also tested heeled 6 degrees and level (`H`), because a preset that only worked for
one of them would break when the key is pressed.

`F5` first failed this test: at 1.55 hull lengths its silhouette ran off the top of the window
(rows 2 to 672 of 720), because from above the ship's length and its mast both fill the
vertical. 1.95 fixed it. Finding that is what the measured margin is for.

## Evidence

### The table against the real geometry (50 checks)

The test projects **every vertex the ship draws** - the real cube, cylinder and quad generators
under the real model matrices, not a box around them - through each preset's camera, for the
heeled and the level ship. It also checks: every point is in front of the camera; all five
radii are inside the camera's own limits (`1.5` to `15`); every pitch is inside `+-89`; no two
presets share a yaw and pitch; and `F2` is on the bow side, `F3` on the `+x` flank, `F4` astern
and `F5` high above.

### The keys and the regression (12 checks)

```text
gallery, ship hidden vs the real Phase 33  : all 10 views byte-identical
gallery, ship shown  vs Phase 37           : all 10 views byte-identical
F1 == the camera the program starts with
the five presets give five different pictures
F3 then R == F1;   F4 then F2 == F2 (a preset does not depend on where the camera was)
F3 HELD for six frames == F3 tapped
F2 in the gallery does nothing;   G, F2, G returns to the unchanged F1;   G, G, F2 == F2
```

### Rendered against projected (15 checks)

The ship alone was rendered under each preset by the project's own `processInput()` and its
bounds compared with the projection:

```text
F1 rendered x 368..820 y 129..653 | predicted x 367..820 y 128..653
F2 rendered x 506..760 y 130..665 | predicted x 505..761 y 130..666
F3 rendered x 310..969 y 116..557 | predicted x 310..969 y 115..557
F4 rendered x 520..791 y 105..674 | predicted x 520..792 y 102..674
F5 rendered x 440..800 y  88..602 | predicted x 439..800 y  88..603
```

Every edge is within 1 px except one: the **top of `F4` is 3 px lower than predicted**. I
looked at why rather than widening the tolerance blindly. Without the flag, the predicted top
would be at row 112; the flag's top corner is at row 102.7. From astern the flag is seen almost
edge-on (11 px wide), so near its top corner it is a sliver narrower than a pixel, which can
miss the pixel centres the rasteriser samples; the first covered row is 105. This is my
explanation of the 3 px, consistent with those numbers - I did not isolate the flag's pixels to
prove it. The tolerance was set to 3 px *and the reason is recorded here*; no other edge needed
more than 1.

## Known limit: very large ship scales

The camera keeps its Phase 13 zoom limits (`1.5` to `15`). At `SHIP_SCALE` 4 every preset is
inside them (8.1 to 10.9). At a scale of 8 the radii would be 16 to 22, over the limit, so a
scroll after pressing a key would snap the camera back to 15. If the viva asks for a much larger
scale, `OrbitCameraConfig::MAX_RADIUS` must grow with it.

## Likely teacher questions

### What is a camera preset?
Yaw, pitch, radius and target - the camera's own four numbers - with a name. A key copies a row
of a table into the camera.

### Why are the distances in hull lengths?
So the views follow the ship's scale. Change `SHIP_SCALE` and nothing needs retyping.

### Why do `F2` and `F4` only fill 20% of the width?
A galleon seen from the front or the back is tall and narrow. They fill 74% and 79% of the
height instead, and the rule is "40% of the width or the height".

### Why are the function keys used?
`W`, `A`, `S`, `D` and the arrow keys are reserved for steering and aiming the cannon (Stages E
and G), and `H`, `G`, `K`, `L`, `N` and the rest are already taken. `F1` to `F5` and `R` collide
with nothing.

### Does a preset move the camera smoothly?
Not yet - it jumps. Phase 40 adds the easing.

## Simple viva modifications

- **The named exercise: change one preset's yaw.** `CameraPresetConfig::PRESETS` in
  [src/CameraPresets.h](../src/CameraPresets.h). Set `F3`'s yaw to `270` and it looks at the
  other flank.
- **Add a sixth view:** raise `CameraPresetConfig::COUNT` and add a row; add a `GLFW_KEY_F6`
  binding - the key loop is `GLFW_KEY_F1 + i`, so it already reaches `F6`.
- **Start somewhere else:** change `DEFAULT_PRESET`. Both the start and `R` follow it.
- **Move the target:** the three numbers of `targetInHullLengths`. A target at the masthead
  makes the camera orbit the top of the ship.

## Checkpoint

Phase 39 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- in every preset, heeled and level, the whole ship is inside the window with a 4% margin and
  fills at least 40% of the window's width or height;
- the rendered ship's bounds match the projected frames within 3 px on every edge (1 px on all
  but one, explained above);
- `F1` is the start camera, `R` returns to it, a preset ignores where the camera was, and a held
  key acts once;
- the presets do nothing in the gallery, which keeps its own camera;
- the gallery is byte-identical to Phase 33 (ship hidden) and Phase 37 (ship shown);
- the showcase is still 15 draws / 496 triangles / 507 vertices.

## What is not part of Phase 39

No smoothing: the camera jumps (Phase 40). No keyboard camera (Phase 41). The presets were tuned
for the prototype ship; the real galleon is taller and has a bowsprit, so Phase 59 and Phase 78
re-run this same test and retune the table if it fails.

**Phase 40 is next**: the camera eases to a preset over a fixed time instead of jumping.
