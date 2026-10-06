# Phase 52 - Railings: Posts at Equal Spacing, a Rail Along Their Tops

## Status

Phase 52 is complete and verified.

```text
Showcase : draws 108 | tris 8546 | verts 7254     (was 36 / 7682 / 5526: +72 draws, +864 triangles, +1728 vertices)
Gallery  : draws 110 | tris 5306 | verts 5656     (was 38 / 4442 / 3928)
Meshes   : still 10 - every post and every rail is the existing unit cube
```

New in `src/Ship.h`: `RailDimensions`, `RailRun` and the placement functions. New in `src/Material.h`: `RAIL_WOOD`. `src/main.cpp`
draws them. 88 CPU checks (with seven negative controls) and 15 picture checks passed with zero failures; the earlier Stage D suites and
the build gate pass.

## What you see

On both sides of the ship, a rail of square posts with a bar along their tops: along the waist's gunwale, and round the edge of the
quarterdeck, the poop and the forecastle.

## The one idea

**A railing is data derived from the decks, not a second set of numbers.** A *run* is a stretch of one deck's edge, and its ends are read from
the deck-level table of Phase 50:

| Run | Between | Stands on | Posts per side |
|---|---|---|---|
| waist | quarterdeck's forward end and forecastle's aft end | the hull's own cap, just inside the gunwale | 10 |
| quarterdeck | poop's forward end and quarterdeck's forward end (the part the poop does not cover) | the quarterdeck's slab | 3 |
| poop | its whole length | the poop's slab | 3 |
| forecastle | its whole length | the forecastle's slab | 4 |

So lengthening the forecastle or raising the sterncastle moves its railings with it (tested). Posts are placed at **equal spacing** from the
first to the last, with the spacing the nearest *whole number of gaps* to the configured `spacing = 0.075`: the run's length is fixed, so the
gap is `usable length / gaps`, not the configured number.

```cpp
const int gaps = std::max(1, lround((r.zTo - r.zFrom - postSize) / spacing));
z_k = first + (last - first) * k / gaps               // equal steps, end posts half a post inside the run
```

The end posts stand **half a post inside the run**, so their outer faces are flush with where a higher deck's wall begins. (My first design
put the quarterdeck's first post exactly at the end of its run, which is the poop block's front face: it would have been half buried in it.)

## Where each post stands

| Run | The post's foot is at | x |
|---|---|---|
| waist | the hull's height at that z (`hullGunwaleHeightAt`) | the gunwale's half-width at that z, less `sideGap` and half a post: **follows the hull's curve** |
| a level | the level's floor, which is the frame's origin | the block's half-width, less half a post: outer face flush with the block's side |

The post is a column from `embed` (0.004) *below* its foot to `postHeight` (0.05) above it: the foot sinks into the surface so there is never
a gap under a post, and `embed` is less than the slab's 0.03 thickness, so it ends inside the slab. A rail segment joins the tops of two
neighbouring posts: its own z axis runs from one top to the next, its x axis is horizontal (no roll), and its size is applied at the draw
call only (CLAUDE.md: scale only in the final `drawMesh`).

```cpp
shipRailPostModel(f, d, run, side, k)       // frame * T(foot + (0, (height - embed)/2, 0)) * S(post, height + embed, post)
shipRailSegmentModel(f, d, run, side, k)    // frame * [x y z axes, mid-point] * S(rail, rail, length)
```

The frame is the hull's for the waist and the level's for a level. That is the hierarchy at work: a level's posts hang from that level's
frame, so they ride with it.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `RAIL_*` indices, `RailDimensions` (six numbers, all scaled by `scaleShipDimensions`), `ShipDimensions::rails`, `RailRun`, `shipRailRun`, `shipRailFrame`, `shipRailPostCount`, `shipRailPostZ`, `shipRailPostBase`, `shipRailPostModel`, `shipRailPostTop`, `shipRailSegmentModel` |
| `src/Material.h` | `RAIL_WOOD` (`HULL_WOOD`'s numbers under their own name, like `CASTLE_WOOD`) |
| `src/main.cpp` | a loop in `drawShip()` draws 40 posts and 32 rails; title and banner say Phase 52 |

No shader, no mesh, no light.

## Evidence

### The CPU checks (88, over four configurations)

On the prototype, the showcase ship (scale 4, rolled 6 and pitched 3 degrees), a hull with the beam 1.5x, and a ship with edited decks:

```text
counts   : 10 / 3 / 3 / 4 posts per side (worked out by hand first: (0.67, 0.13, 0.15, 0.21) / 0.075 = 8.9, 1.7, 2.0, 2.8 gaps), 40 posts, 32 rails
spacing  : equal within 3e-8 on every run and side: 0.0744 (99% of 0.075), 0.065 (87%), 0.075 (100%), 0.070 (93%) -
           each the nearest whole number of gaps, checked against the alternatives with one gap more or fewer
standing : every foot exactly `embed` below the surface under it and every top exactly `postHeight` above it (worst error 7.5e-9);
           every level post's whole footprint on its slab
inside   : every post across its whole width and every rail along its whole length inside the gunwale outline (least margin 0.0030)
mirror   : the 20 port posts are the 20 starboard posts reflected in the centre plane (worst 0.0)
rails    : each end is exactly a post top (worst 6e-8), 0.012 square, orthogonal axes, no roll
clear    : no two of 40 posts overlap (780 pairs); none inside a castle block, the main deck or the cannon's mount; none at a mast
scaling  : the same counts at scale 4, and every post exactly 4x further from the centre
general  : widening the beam 1.5x moves the waist's posts out (0.229 against 0.149) and every check passes; editing the decks does too
```

### The pictures (15 checks)

```text
with the railings not drawn, the showcase is byte-identical to Phase 51 from all five presets and the gallery
gallery with the ship hidden: all 10 views byte-identical to the real Phase 33
```

**A whole-picture comparison, not a few probes.** Rendered with only `rail wood`, ship level, in five views (side, three-quarter, from
astern, from above, a close bow quarter). Each of the 72 cubes is carried through the real model functions and the camera; the convex hull of
its 8 projected corners is its exact silhouette. A pixel is *predicted* covered if it lies in any hull. Prediction and picture are compared at
every one of 921,600 pixels:

```text
side         : 7477 predicted, 8628 in the picture, 1151 disagreements all within 2 px of an edge, 0 elsewhere
three-quarter: 8878 / 9829, 951 forgiven, 0 errors      from astern: 4618 / 5082, 464, 0      from above: 3912 / 4603, 691, 0
close bow    : 17033 / 18292, 1259, 0
```

(The picture is always a little bigger than the prediction: anti-aliasing blends the edge pixels, which is what the 2 px band is for.)

### Negative controls

```text
CPU 1  a waist post moved 0.01 along the ship        -> fails equal spacing, standing, mirror
CPU 2  a forecastle post lowered 0.02                -> fails standing, mirror, rail ends
CPU 3  a waist post moved 0.03 outward               -> fails inside-the-hull, mirror, rail ends
CPU 4  a starboard post pulled 3% inward             -> fails mirror, rail ends
CPU 5  a quarterdeck post moved aft into the poop    -> fails spacing, mirror, rail ends AND "no post inside a castle block"
CPU 6  a rail's end moved off its post               -> fails rail ends
CPU 7  one rail's thickness 25% wrong                -> fails the size check
picture, one post missing from the prediction        -> 67 errors   (one post out of 40)
picture, all models 0.03 too high                    -> 5461 errors
picture, the two sides 0.02 too far apart            -> 6209 errors
```

## Mistakes, and what they taught

1. **A test tolerance that was wrong, not the code.** I first required each run's actual spacing to be within 10% of the configured one. The
   quarterdeck's short run (0.13 usable) has two gaps of 0.065, which is 87%, and the test failed an honest ship. The right statement is the one
   in the code: the spacing is the *nearest whole number of gaps*. The check now compares against the alternatives with one gap more and one fewer.
2. **A number on a rounding knife edge.** `spacing = 0.08` made the waist `8.5` gaps, where `lround` could go either way depending on float
   noise. `0.075` puts every run well away from a half.
3. **A comment I damaged in Phase 50 and found now.** In the Phase 50 edit script I passed a replacement text and an extra `+ "..."` as separate
   command-line arguments; PowerShell dropped the extra, and the first line of the `ShipDimensions` comment ("How big the parts of the ship are...")
   went missing, leaving its second line dangling. Found because this phase's anchor was not there, restored in this phase. No code was affected.

## Limits, reported

- **The railings are side runs only.** There is no rail across the back of the poop, none across the front of the castles, none at the bow:
  open ends. The stern gets one with the stern cabin (Phase 55), the others as the bow and the castles are dressed.
- **One bar, square posts.** A real rail has a lower bar and turned balusters; this is the unit cube, which keeps the triangle cost at 12 each.
- **72 draw calls for the railings alone.** Every post and every rail is its own draw. The plan batches the static ship at Phase 76; the draw
  count is recorded here so that phase has a number to beat.
- **The player's cannon can still aim its barrel through a rail**, as it passed through the open gunwale before. Gunports and the broadside array
  (Phases 53-54) deal with that.
- A ledge of bare hull cap is visible between the castles' blocks and the hull's gunwale (the blocks are narrower than the hull at the bow end
  of each span); the rails stand at the block's edge, not at the gunwale, there.

## Likely teacher questions

### How are the posts placed?
At equal spacing along a run whose ends come from the deck-level table; the count is the nearest whole number of gaps to the configured spacing.

### Why do they follow the hull?
On the waist a post's x is the gunwale's half-width at its z, read from the same station table that builds the hull.

### How does a post know where the deck is?
On a level, the level's frame *is* the floor: the post is a child of that frame. On the waist, it is the hull's gunwale height at that z.

### What does it cost?
72 cubes, 72 draws, 864 triangles.

### What happens to the railings when the ship rocks, or when `H` is pressed?
They are children of the hull's and the levels' frames, so they rock with the hull; `H` clears the root's roll and pitch and they follow.

## Simple viva modifications

- **The named exercise: change the post spacing.** `rails.spacing` in `DEFAULT_DIMENSIONS`, [src/Ship.h](../src/Ship.h): 0.05 gives about 15 posts
  along the waist, 0.15 gives 5. The posts stay equally spaced and stay on the decks.
- **Taller rails:** `rails.postHeight`. Thicker bars: `rails.railSize`.
- **Move the rail in from the gunwale on the waist:** `rails.sideGap`.

## Checkpoint

Phase 52 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- post spacing is equal on every run and side, and is the nearest whole number of gaps to the configured one;
- every post stands on the deck level beneath it (foot `embed` below, top `postHeight` above the surface);
- no post or rail lies outside the hull's outline, none overlaps another or a castle block;
- port and starboard are mirror images;
- each rail's two ends are exactly the tops of the posts it joins;
- in five rendered views the picture agrees with the projected models at every pixel but the 2 px edge band (0 errors);
- with the railings off the picture is byte-identical to Phase 51;
- the showcase is 108 draws / 8546 triangles / 7254 vertices.

## What is not part of Phase 52

No gunports (Phase 53), no broadside cannons (54), no stern cabin or transom rail (55), no gilded trim (56), no bowsprit (57), no figurehead (58).

**Phase 53 is next**: gunports - dark recessed openings in a row along both flanks.
