# Phase 50 - Deck Levels: Quarterdeck, Poop and Forecastle as Frames

## Status

Phase 50 is complete and verified.

```text
Showcase : draws 32 | tris 7530 | verts 5222     (was 26 / 7458 / 5078: +6 draws, +72 triangles, +144 vertices)
Gallery  : draws 34 | tris 4290 | verts 3624     (was 28 / 4218 / 3480)
Meshes   : still 9 - the levels are the existing unit cube at six sizes
```

New in `src/Ship.h`: `DeckLevelDimensions`, three table entries, three frames, and the block and slab models. New in
`src/Material.h`: `CASTLE_WOOD`. `src/main.cpp` draws the six parts. 121 new CPU checks and 15 picture checks passed
with zero failures, six negative controls fail as they must, and the earlier Stage D suites and the build gate pass.

## What you see

A galleon's stern is a tall stepped castle and its bow carries a lower one; between them the deck sinks into the waist.
From the side the ship now has that stepped profile:

```text
level        floor height   z span (from the hull centre, +z = bow)   stands on
poop         0.38           -0.66 .. -0.50                            the quarterdeck
quarterdeck  0.30           -0.66 .. -0.36                            the main deck
forecastle   0.28            0.32 ..  0.54                            the main deck
waist        0.1964         (gunwale 0.1664 + the 0.03 deck slab)
```

Each level is a **block** of timber (`CASTLE_WOOD`, the hull's brown) from the level below up to the underside of the
**slab**, a thin planked top (`DECK_WOOD`) that overhangs the block by a small lip so its edge reads as a separate board.

## The one idea

**A level is a frame at a named height; its size is only a draw-time scale.** The table says where a level lies and how
high its floor is:

```cpp
struct DeckLevelDimensions { float zFrom, zTo, floorHeight, inset; };
//                                zFrom   zTo    floor  inset
{ -0.66f, -0.36f, 0.30f, 0.012f },   // quarterdeck
{ -0.66f, -0.50f, 0.38f, 0.012f },   // poop
{  0.32f,  0.54f, 0.28f, 0.012f },   // forecastle
```

`buildShipFrames()` turns each row into `f.level[i]`, the centre of that level's floor: a hull frame times one translation.
The poop is the quarterdeck's **child** - its offset is the *difference* of the two floors (`0.38 - 0.30`), not an
absolute height - because the poop stands on the quarterdeck. Nothing in a frame carries a size (CLAUDE.md, parent scale
must not reach children): `shipDeckBlockModel()` and `shipDeckSlabModel()` apply the scale, at the draw call, and nowhere
else. Everything that will stand on a level later - railings, the helm - can hang from that frame.

## The width is read from the hull, not typed in

A level is as wide as the hull allows at the *narrowest* point under it, less `inset` each side:

```cpp
inline float shipDeckLevelWidth(const ShipDimensions& d, int i)   // samples hullGunwaleHalfBeamAt along the span
```

so editing the hull's station table (the Phase 47 exercise) changes the castles with it. Tested: with the beam 1.5x
the quarterdeck is 0.302 wide against 0.193, and every check still passes.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `DECK_*` indices, `DeckLevelDimensions`, `ShipDimensions::deckLevels[3]`, their scaling in `scaleShipDimensions`, `ShipFrames::level[3]`, `shipDeckLevelBaseHeight`, `shipDeckLevelWidth`, `shipDeckBlockModel`, `shipDeckSlabModel` |
| `src/Material.h` | `CASTLE_WOOD`: `HULL_WOOD`'s four numbers under a name of its own (so a picture test can draw the blocks alone, and the trim of Phase 56 can tell them apart) |
| `src/main.cpp` | `drawShip()` draws a block and a slab per level; title and banner say Phase 50 |

No shader, no new mesh, no new light.

## Evidence

### The CPU checks (121 over four configurations)

Run on the prototype (scale 1), the showcase ship (scale 4, rolled 6 and pitched 3 degrees, heading 0.7 rad), and a hull
with the beam 1.5x - 39 checks each - plus the scaling checks:

```text
frames   : each is the parent's rotation (difference 0.0) plus a translation; at x = 0, the table's height and span middle
heights  : poop 0.3800 > quarterdeck 0.3000 > forecastle 0.2800 > the waist's walking surface 0.1964
           the sterncastle is 2.28x the waist's gunwale height (the plan wants at least 1.4x); forecastle lower than quarterdeck
fit      : every block is inside the gunwale at 65 sampled stations with at least its 0.0120 inset (least margin 0.0120);
           every slab with its lip too (0.0060); block tops 0.2700 / 0.3500 / 0.2500 clear the hull's highest gunwale under
           them (0.2545 / - / 0.2104), so the rail cannot show through a floor
stacking : each slab's underside is exactly its block's top, slab top = floor; the poop block starts exactly at the
           quarterdeck's floor (0.3000) and lies within the quarterdeck's footprint
clear    : neither mast nor the cannon's mount stands inside any level's span; quarterdeck and forecastle leave a 0.68 waist
profile  : top of the ship over the poop / quarterdeck / waist / forecastle = 0.3800 / 0.3000 / 0.1964 / 0.2800, as tabled
scaling  : all four fields of all three levels are 4x at scale 4; all 96 block and slab corners are exactly 4x (worst 0.0)
```

### The pictures (15 checks)

```text
with the six level draws switched off, the showcase is byte-identical to Phase 49 from all five presets and the gallery
gallery with the ship hidden: all 10 views byte-identical to the real Phase 33
```

**The stepped profile, measured.** Rendered with only `deck wood` (then only `castle wood`) drawn, ship level, from the
side. At a screen column over each part the topmost non-background pixel is compared with where the real models'
silhouette is (the minimum over the 12 projected box edges, interpolated - a straight edge projects to a straight line):

```text
yaw 90 pitch 2 radius 9, deck wood      poop 380.07 -> 380    quarterdeck 409.53 -> 409    waist 446.21 -> 446    forecastle 417.71 -> 417
yaw 90 pitch 2 radius 9, castle wood    tops 391.28 / 420.79 / 429.09 -> 391 / 420 / 429;  bottoms 470.42 / 470.42 / 468.99 -> 471 / 471 / 469
```

Three views (both sides of the ship, two pitches): worst error **0.85 px** against the 2.5 px allowed. The block bottoms
are checked too, so the height of the level *below* each block is proved, not just its top.

### Negative controls

```text
1  poop offset from the quarterdeck uses absolute numbers instead of the difference   -> fails frame position, poop height, z span, top, slab
2  the quarterdeck's frame carries a scale                                           -> fails the rigid-frame and fit checks
3  forecastle floor above the quarterdeck                                            -> fails the height order, profile and "lower" checks
4  negative inset (blocks wider than the hull)                                       -> fails the slab fit and the positive-inset check
5  quarterdeck extended forward to z = 0.25                                          -> fails aft-of-midships, the mast, the cannon and the waist profile
6  scaleShipDimensions forgets the inset                                             -> fails the "all four fields are 4x" and corner checks
picture test with the six draws switched off                                         -> both the deck and the castle profile tests fail
```

## One thing a test cannot see

My first version of control 1 built the poop frame as a child of the **hull** with the right numbers. It passed all the
checks - correctly: while every frame is a pure translation, `hull * T(a)` and `quarterdeck * T(b)` are the same matrix.
Parenthood is a fact about the code, not about the numbers, until a frame moves. It matters from Phase 52, when things
are attached to a level; the useful control is the *offset* bug (control 1 above), which is the way the mistake really
happens.

## Also reported

- `p38/scale.cpp` - the Phase 38 scale test - has failed two of its checks since Phase 47: it reads the hull profile's 40
  fractions as unscaled lengths and counts the zeros in the profile. Its successor `scale47.cpp` probes every float with a
  distinct nonzero number and treats the profile as one block; it passes (38 checks), and covers the new level fields.
  Nothing in the ship is wrong.
- The blocks are plain boxes with vertical sides; the hull leans in above the waterline. The ledge that results is real
  and is hidden by the gunwale rail in Phase 52 and the gunports in Phase 53.

## Likely teacher questions

### How does the ship get a raised stern?
Three rows in a table and three frames. Each level is a block and a slab, both the unit cube scaled at draw time.

### Why is the poop a child of the quarterdeck?
It stands on it. Its frame is the quarterdeck's frame plus the difference of the two floor heights, so raise the quarterdeck and
the poop rises with it.

### Why is the width not in the table?
It is the hull's own width at the narrowest point under the level, so changing the hull's shape changes the castles too.

### What is the cost?
Six draws of an existing mesh: 72 triangles. No new geometry.

## Simple viva modifications

- **The named exercise: raise the sterncastle.** `deckLevels[DECK_QUARTERDECK].floorHeight` and `deckLevels[DECK_POOP].floorHeight`
  in `DEFAULT_DIMENSIONS`, [src/Ship.h](../src/Ship.h). The floors must stay in order and the poop above the quarterdeck.
- **Lengthen the forecastle:** `deckLevels[DECK_FORECASTLE].zTo`. Keep it clear of the foremast at z = 0.20.
- **Change the slab's lip:** `lip` in `shipDeckSlabModel()`.

## Checkpoint

Phase 50 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- each level's frame is the parent's frame plus a translation, with no scale;
- the floors step poop > quarterdeck > forecastle > waist, the sterncastle at least 1.4x the waist's gunwale;
- every block and slab lies inside the hull's gunwale and above its highest gunwale;
- the rendered side profile matches the models to within 2.5 px (measured: 0.85);
- with the six draws off the picture is byte-identical to Phase 49;
- the showcase is 32 draws / 7530 triangles / 5222 vertices.

## What is not part of Phase 50

No railings (Phase 52), no gunports (Phase 53), no stern cabin windows (Phase 55), no deck planks on the slabs (Phase 51),
no gilded trim (Phase 56). The masts are not yet on the new levels; the cannon is still the one mounted amidships.

**Phase 51 is next**: deck planks - a plank-strip mesh with alternating colours and dark seams.
