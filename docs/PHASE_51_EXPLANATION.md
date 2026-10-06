# Phase 51 - Deck Planks: One Sheet of Strips, Laid Over Every Deck

## Status

Phase 51 is complete and verified.

```text
Showcase : draws 36 | tris 7682 | verts 5526     (was 32 / 7530 / 5222: +4 draws, +152 triangles, +304 vertices)
Gallery  : draws 38 | tris 4442 | verts 3928     (was 34 / 4290 / 3624)
Meshes   : 10 (the plank sheet is the new one: 76 vertices, 114 indices, 38 triangles)
```

New: `src/Planks.h` (the strips as data, no OpenGL), the second of the plan's three extra generators in `src/Mesh.h`
(`buildDeckPlankGeometry`, `makeDeckPlanks`), the `DECK_PLANK` material, and two model functions in `src/Ship.h`. 97 CPU
checks (with six negative controls) and 19 picture checks passed with zero failures; the earlier Stage D suites and the
build gate pass.

## What you see

Every deck - the main deck, the quarterdeck, the poop and the forecastle - is laid in planks that run along the ship: nine
planks across the beam, light and dark in turn, with a thin dark seam between each pair.

## The one idea

**A deck is one flat sheet of strips, stretched to fit.** The mesh is a 1 x 1 unit sheet in the xz plane (the same unit-mesh
rule as every other mesh): a seam, a plank, a seam, a plank ... a seam, 9 planks and 10 seams, each strip its own quad with
its own four vertices so the colour changes at a seam *edge* and not by blending (a vertex has one colour; the same reason as
Phase 49's second pass).

```text
x = -0.5                                                        x = +0.5
 |s| plank 0 |S| plank 1 |S| plank 2 |S| ...  |S| plank 8 |s|        S = a full seam, s = half a seam
```

The two outer seams are half-width, clipped to the sheet, so all nine planks are exactly the same width
(`pitch x (1 - 0.14) = 0.09556` of the sheet) and the pattern is symmetric. **The count is odd on purpose**: with an even
count the light-dark alternation would not read the same from both sides; the CPU test catches it (negative control 6).

Colours are vertex colours read as a multiplier on `DECK_PLANK` (`DECK_WOOD`'s four numbers with the vertex-colour switch on,
Phase 48): 1.10 for a light plank, 0.88 for a dark one, 0.30 for a seam.

## How a sheet sits on a slab

The sheet is laid over each slab's whole top face, **2% of the slab's thickness above it**, applied at draw time:

```cpp
shipDeckPlankModel(f, d)         // main deck: f.deck * T(0, thickness/2 + lift, 0) * S(deckSize.x, 1, deckSize.z)
shipDeckLevelPlankModel(f, d, i) // a level: f.level[i] * T(0, lift, 0) * S(slab width, 1, slab length)
```

The scale in y is exactly 1: it is a sheet, not a box. The lift is the one number to understand. A sheet exactly on the slab
would fight it for the depth buffer: both would be at the same depth and the pixel would flicker between them. A lift that is
too big is a visible gap at a grazing angle. 2% of 0.12 (the slab at the showcase scale) is 0.0024.

## What changed

| File | Change |
|---|---|
| `src/Planks.h` (new) | `DeckPlanking` (9 planks, seam 0.14 of a pitch, the three shades, the lift), `DeckStrip`, `deckPlankColor`, `deckStrips()` |
| `src/Mesh.h` | `buildDeckPlankGeometry()`, `makeDeckPlanks()` |
| `src/Material.h` | `DECK_PLANK` |
| `src/Ship.h` | `shipDeckSlabLip()` (the slab's lip, now one definition instead of two), `shipDeckPlankLift`, `shipDeckPlankModel`, `shipDeckLevelPlankModel` |
| `src/main.cpp` | `planksMesh` created, passed to `drawShip()`, destroyed; four sheet draws; title and banner say Phase 51 |

## Evidence

### The CPU checks (97)

```text
strips   : 19 = 9 planks + 10 seams, alternating seam/plank, contiguous (worst gap 0.0), covering -0.5 .. +0.5 exactly;
           every plank 0.09556 wide, interior seams 0.0156, the outer two exactly half; symmetric in position AND colour
colours  : all 8 pairs of neighbouring planks differ in luminance by more than 0.1, light first; the brightest seam (0.274)
           is under half of the darkest plank (0.846)
mesh     : 76 vertices and 38 triangles, worked out before running (4 and 2 per strip x 19); every vertex at y = 0, every
           normal exactly (0, 1, 0), every triangle wound counter-clockwise from above, areas add up to 1.000000
decks    : on all four decks, at scale 1 and at scale 4 rolled 6 and pitched 3 degrees, the sheet is flat, covers exactly the
           slab's top face (error <= 2.4e-7), floats exactly 2.0% of the slab's thickness above it, and has scale 1 in y
general  : the same checks pass for 5, 7 and 11 planks
```

### The pictures (19 checks)

```text
with the four sheet draws switched off, the showcase is byte-identical to Phase 50 from all five presets and the gallery
gallery with the ship hidden: all 10 views byte-identical to the real Phase 33
```

**Each strip is the bare slab times its multiplier.** Rendered with only the sheets, and with only the unplanked slabs, both at
ambient + diffuse (`-terms 3`) from the same camera, at the middle of every plank and every interior seam wide enough to
test (2.5 px or more):

```text
planked = (bare - f H) x m + f H         f = the Phase 45 haze fraction, H the horizon colour, m the strip's multiplier
yaw 90 pitch 80: 52 strips tested over the four decks, worst error 1 of 255
yaw 60 pitch 76: 45 strips, worst 1          yaw 135 pitch 70: 46 strips, worst 1
```

**The planks are counted in the picture.** A line walked across the main deck's beam (about 500 samples) finds **9 runs of
plank and 8 of seam** in all three views.

**No fighting.** Over a grid of points on every sheet, a pixel of the full picture that equals the bare slab's colour where the
sheet's own pixel differs would be the slab showing through. Five views, all within the camera's range (the radius is clamped
to 15): top-down at 14, grazing 12 degrees at 12 and at 15, 8 degrees at 15, oblique 35 degrees at 15: between 5065 and
6941 samples each, **0 show the slab through**.

### Negative controls

```text
CPU 1  painted plank 3 light like its neighbour           -> fails the alternation and symmetry checks
CPU 2  a gap opens between two strips                     -> fails contiguity, the width sum, the plank widths
CPU 3  the sheet lies exactly ON the slab                 -> fails the lift check on every deck
CPU 4  first triangle wound the wrong way                 -> fails the winding check
CPU 5  left outer seam sticks out past the sheet          -> fails contiguity, the sum and the seam widths
CPU 6  an even plank count (8)                            -> fails symmetry
picture: light and dark swapped in the model              -> the strip test fails
picture: a build with 7 planks (the test expects 9)       -> the count finds 7 and fails
picture: a build with the lift set to 0                   -> the slab shows through at 1912 to 4141 samples per view
```

That last one is what makes the zero above mean something: the test *can* see z-fighting.

## Mistakes, and what they taught

1. **A bug caught on reading, before running.** The first draft of `deckStrips()` ended the last plank at 0.5 instead of where
   the last seam begins. The contiguity check would have caught it; a re-read did first.
2. **The z-fight detector cried wolf first.** Its first version reported 80, 144 and 372 samples of slab "showing through". I did
   not accept that or wave it away: printing the positions showed they were the *side face of the poop's slab* (also
   `DECK_WOOD`, so the colour matched the bare slab) and pixels on the sheet's own edge. A sheet lift three times larger (0.06)
   did not remove them (10, 19 and 1 samples remained in three of the views): a depth fight would have gone. The detector now skips samples that another deck's slab
   (fattened by 3 px) hides, and samples within 3 px of a sheet's edge, and counts both; the numbers above are with it, and the
   lift-0 build still shows thousands.
3. **The plank counter's first thresholds were wrong**: they used the brightest sample on the line, which was a bright edge pixel
   at the end, so every plank was read as a seam. It now uses percentiles (the 5th for the seam level, the median for the plank
   level) and ignores runs under a third of a pixel.
4. **One view was badly chosen**: from `yaw 0` the beam is foreshortened and half the strips were under 2.5 px.

## Limits, reported

- **The lift is sized for the camera's range.** The radius cannot exceed 15. Beyond it I looked: at radius 25 there were 0 samples
  showing through; at radius 40, grazing, 9 of 2653. If a later phase lets the camera go farther, raise `LIFT_FRACTION` or use
  `glPolygonOffset`.
- **Plank widths differ between decks.** The sheet always has nine planks, so a plank is 0.0248 wide on the main deck, 0.0196 on the
  quarterdeck and poop, and 0.0154 on the forecastle (prototype units; 4x at the showcase scale). Real decks have equal planks and
  differing counts. A per-deck count would need a mesh per count; this is the cost of the one-sheet idea.
- **No butt joints.** The planks run the full length of each deck with no ends. Cross seams are a separate idea.
- `p38/scale.cpp` (the Phase 38 scale test) still has its two known failures from Phase 47; `scale47.cpp` supersedes it and passes.

## Likely teacher questions

### How are the planks made?
One flat mesh of 19 coloured strips, drawn four times at four sizes. No texture: the colours are vertex colours.

### Why is the number of planks odd?
So the light-dark pattern is symmetric about the centreline.

### Why float the sheet above the slab?
Two surfaces at the same depth fight for the depth buffer. A lift of 2% of the slab's thickness is too small to see and large
enough to win.

### Why one mesh for four decks?
The unit-mesh rule: the mesh is 1 x 1 and its real size is a draw-time scale. That is also why the planks are not the same width
on every deck.

### What does it cost?
Four draws, 152 triangles and 304 vertices.

## Simple viva modifications

- **The named exercise: change the plank width.** `DeckPlanking::PLANKS` in [src/Planks.h](../src/Planks.h) (keep it odd): 7 gives
  wider planks, 13 narrower. Or `SEAM_FRACTION`, the seam's share of a plank's pitch.
- **Change the shades:** `LIGHT_PLANK`, `DARK_PLANK`, `SEAM_COLOR`.
- **Lay planks across the ship instead of along it:** turn the sheet a quarter turn about y in the two model functions.

## Checkpoint

Phase 51 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the plank count across the beam equals `DeckPlanking::PLANKS` in the CPU test and in a rendered picture (9 and 9);
- every seam is darker than every plank, and neighbouring planks alternate;
- each strip's pixel is the bare slab's times its multiplier, to within 7 levels (measured: 1);
- no slab shows through any sheet in five views, and the lift-0 build proves the test can see it;
- with the sheets off the picture is byte-identical to Phase 50;
- the showcase is 36 draws / 7682 triangles / 5526 vertices, and there are 10 meshes.

## What is not part of Phase 51

No railings (Phase 52), no gunports (53), no butt joints, no per-deck plank counts, no planking on the castles' vertical sides.

**Phase 52 is next**: railings and supports along the gunwale.
