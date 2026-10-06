# Phase 46 - Distant Islands and Sea Stacks

## Status

Phase 46 is complete and verified. It is an optional phase (`(o)`) in the plan; it was done because
it is cheap and it finishes the horizon.

```text
Showcase : draws 26 | tris 5448 | verts 3069    (Phase 45's 17 / 2548 / 1537 + 9 islands)
Gallery  : draws 28 | tris 2208 | verts 1471    (unchanged: no islands there)
```

New file `src/Islands.h` (no OpenGL), one new material, nine draws, **no new mesh**. 41 checks passed
with zero failures: 18 on the placement table and 23 on rendered pictures, plus the build gate.

## The one idea

**Scenery is a table.** An empty horizon looks like a render; a faint dark coastline on it looks like
a place. Nine shapes are listed in a table by compass bearing, distance, width and height, and drawn
with meshes that already exist:

| Kind | Mesh | Drawn as |
|---|---|---|
| `HILL` (7 of them) | the unit sphere | scaled `(width, 2 x height, width)` and centred **on** the waterline: its upper half rises `height`, the lower half is under the sea |
| `ROCK` (2) | the unit cylinder | scaled `(width, height + 0.3, width)` and lifted so its top is `height` above the water and 0.3 of it is below |

```cpp
{ IslandKind::HILL, 190.0f, 46.0f, 14.0f, 2.6f },    // bearing, distance from the ship, width, height
```

A **bearing and a distance** read better than an `x` and a `z`: bearing 0 is straight ahead of the bow
(`+z`) and it turns towards `+x`, the same convention the camera's yaw and the sky use.

## Why they are "dark hazed silhouettes" without any code that says so

The islands are 38 to 56 units from the ship. The Phase 45 haze is 76% to 93% complete over that range.
Whatever the island's colour, most of what reaches the eye is the horizon's colour, and the island
shows only as a slightly darker, slightly bluer shape against it. The material `ISLAND_ROCK` is dark,
cool and matt (`k_d` about 0.1, `k_s` 0.03, `n_s` 6) so that the low sun puts no glint on a hill that
is a quarter of a mile away. No haze-specific code was written in this phase.

That claim was tested, with a control. In the F1 view, over the 64,320 pixels the islands cover:

```text
with the haze    : an island changes a pixel by 6.6 of 255 on average, 39 at most   (faint)
haze density = 0 : an island changes a pixel by 38.4 of 255 on average, 239 at most (solid black shapes)
```

## What changed

| File | Change |
|---|---|
| `src/Islands.h` (new) | `Island`, `IslandKind`, the nine-row `IslandConfig::ISLANDS` table, `islandOffsetFromShip()`, `islandRadius()`, `islandModel()` |
| `src/Material.h` | `ISLAND_ROCK` |
| `src/main.cpp` | nine `drawMesh()` calls in the showcase, from the sphere and the cylinder already on the card |

## Evidence

### Placement (18 checks, the real `Islands.h`)

```text
every island is exactly its stated distance (3.8e-06) and bearing (0) from the ship;
    bearing 0 is +z, bearing 90 is +x
the ship reaches 2.92 units from its root; the nearest island's shore is 34.28 units beyond that
    (more than four ship-reaches)
the closest pair of islands has 5.7 units of open water between them
every island stands on the sea (farthest extent 62.6, the sea reaches 80)
with the camera zoomed all the way out (15), every island is within 81.0 of the eye:
    inside the dome (90) and the far plane (100)
every island's top is exactly `height` above the waterline (2.4e-07); none floats
F1 has 3 islands well inside its field of view, F2 has 3, F3 has 2, F4 has 2
```

**"Never crossing the ship"** is read as a 3D statement - no island is anywhere near the ship - and
checked that way. Seen from the camera an island can of course sit *behind* the ship's silhouette; the
ship is in front of it and the depth test draws it correctly.

### Pictures (23 checks)

```text
gallery with the ship hidden vs the real Phase 33     : 10 views byte-identical
gallery with the ship shown, no-sample window, vs Phase 37 : 10 views byte-identical
showcase -> G -> G -> G                               : the Stage C picture, byte for byte
draw calls at detail levels -2 .. +3                  : 26 26 26 26 26 26
```

The triangle and vertex totals were worked out before running: 7 hills of 396 triangles and 200
vertices plus 2 rocks of 64 and 66 give `2548 + 2900 = 5448` and `1537 + 1532 = 3069`, and the counters
read exactly that. The **draw count is the same at every detail level** because detail changes how
finely the same nine objects are divided, never how many objects there are (the Phase 25 argument, again).

The coast in each preset, measured as the difference between the full picture and the same picture
drawn with `-skipmat island`, over the pixels the islands occupy:

```text
view   island pixels   mean change   largest   mean luminance change
F1        64,320          6.57          39        -7.00   (darker)
F2        39,214          5.90          40        -6.30
F3        71,931          6.54          29        -6.94
F4        31,008          6.26          33        -6.69
```

Faint (a few levels of 255), visible, and darker than the horizon it dissolves into, in all four
presets that face one. `F5` looks straight down and has no horizon to show.

The islands disturb nothing else: the Phase 45 sky-gradient test and ship-haze test still pass, and the
ship alone is byte-identical with and without them.

## Two harness switches this phase needed

`-skipmat NAME` (draw everything except materials starting with `NAME`) is new; `-onlymat NAME` already
existed. Together they let a test find exactly which pixels an object owns, and what they would have been
without it. They live in the test harness, not in the project.

(While running these checks the disk filled up: each phase's pictures and Debug/Release/strict builds
add up to about 150 MB. The pictures and builds of finished phases were deleted from the scratch folder
and the checks re-run. One reference picture was lost in the process, so the Phase 45 haze check now takes
the sail's measured lit colour, `149 108 75`, as a constant instead of reading it from the old file.)

## Likely teacher questions

### Are the islands new geometry?
No. They are the sphere and the cylinder the project has had since Phase 22 and Phase 20, scaled at draw
time - the unit-mesh rule (Phase 16) once more.

### Why do they look faint?
They are 38 to 56 units away, where the Phase 45 haze is 76% to 93% complete. With the haze switched off
they become solid black shapes (largest change 239, against 39).

### Do they cost much?
Nine draw calls, fixed. The triangles follow the `+` and `-` detail level but the draws do not.

### Can the ship hit one?
No - nothing collides with scenery - and the nearest shore is 34 units from the ship. If the ship sails
(Stage E) the sea bounds keep it far from them.

### Why a table?
So "move one island" is one number, and so a test can check spacing and containment exactly instead of
trusting the picture.

## Simple viva modifications

- **The named exercise: move one island.** `IslandConfig::ISLANDS` in
  [src/Islands.h](../src/Islands.h): change a bearing, e.g. the first row from `190.0f` to `230.0f`.
- **Add an island:** add a row and raise `COUNT`.
- **Make the coast clearer:** shrink a distance - at 30 units the haze is only 55% and the island is darker.
- **Make it a dead calm lagoon:** set every `height` to `0.5`.
- **See the haze doing it:** set `hazeDensity` to `0` in the golden-hour profile.

## Checkpoint

Phase 46 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- a faint dark coastline is on screen in `F1`, `F2`, `F3` and `F4`: visible (more than 3 levels), faint
  (under 60), and darker than the background;
- the cost is a fixed 9 draws at every detail level;
- every island is at its stated place, far from the ship, apart from the others and inside the sea,
  the dome and the far plane;
- the sky and the ship are untouched;
- the gallery is byte-identical to Phase 33 (ship hidden) and Phase 37 (ship shown);
- the showcase is 26 draws / 5448 triangles / 3069 vertices.

## What is not part of Phase 46

No collision, no landing, no trees or buildings, no animation, no islands in the gallery. They do not
move when the ship does; the table is measured from a fixed ship position, and Stage E decides whether
that should change.

**This ends Stage D2, the showcase.** The scene has a stage - a large sea, a sky, a sun and a coast,
cameras that glide and a keyboard to drive them, smooth edges and warm light. **Phase 47 is next** and
begins the ship itself: `makeHull`, the first of the three generators the plan allows beyond the five
meshes.
