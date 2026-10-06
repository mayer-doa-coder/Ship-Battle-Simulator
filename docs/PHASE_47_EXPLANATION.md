# Phase 47 - `makeHull`: a Lofted Hull from a Table of Cross-Sections

## Status

Phase 47 is complete and verified. It is a **graded milestone** (`*`) and the first phase of Stage D3,
the hull.

```text
Showcase : draws 26 | tris 7458 | verts 4176    (Phase 46's 26 / 5448 / 3069, with the box hull's 12 triangles
                                                 replaced by the lofted hull's 2022)
Gallery  : draws 28 | tris 4218 | verts 2578    (its own objects unchanged; only the ship's hull is new)
Hull mesh: 1131 vertices, 2022 triangles, one draw call
Meshes   : 9
```

New files `src/Hull.h` (mathematics, no OpenGL) and a generator in `src/Mesh.h`. 104 new checks passed with
zero failures (50 on the hull, 38 on the size-scaling test as updated, 16 on pictures), and the 182 earlier
Stage D checks were re-run on the new hull (all pass; two assertions restated, see below), plus the build gate.

## Why a hull cannot be a scaled cube

A cube scaled to `(0.42, 0.20, 1.40)` is a barge. A galleon is narrow at the stem, full amidships,
rounded under the water, higher at the stern and bow than in the waist, and tapers again aft. None of that
is a *size* - it is a *shape* - and a scale matrix cannot make a shape. So this is the first of the three
generators the plan allows beyond the five original meshes (the others are the plank strip, Phase 51,
and the sail, Phase 60). Everything else in the ship stays on the five.

## The one idea: lofting

A hull is described the way a shipwright's plan describes it: by **cross-sections**. A *station* is the
outline of the hull cut across at one point along its length. Draw ten of them, stern to bow, fair a
surface through them, and that is the hull. Here a station is four numbers, all **fractions**:

| Number | Meaning |
|---|---|
| `t` | where along the hull: 0 at the stern, 1 at the stem |
| `halfBeam` | half the width, as a fraction of the widest point (1 = widest) |
| `keelRise` | how far the bottom of the section is lifted off the keel line, as a fraction of the depth |
| `sheer` | the gunwale's height, as a fraction of the depth |

```cpp
//   t     halfBeam keelRise sheer
{ 0.00f,  0.62f,   0.16f,   1.00f },   // the transom: narrower than the beam, keel lifted, gunwale at its highest
{ 0.38f,  1.00f,   0.00f,   0.66f },   // the widest part, keel on the keel line
{ 0.55f,  1.00f,   0.00f,   0.64f },   // the waist: the LOWEST gunwale - the main deck's level
{ 1.00f,  0.00f,   0.72f,   0.94f },   // the stem: width zero, forefoot raked up out of the water
```

The table says nothing about size. The mesh built from it is a **unit mesh** (it spans `[-0.5, 0.5]` on all
three axes) and the real hull is `scale(beam, depth, length)` at draw time - the Phase 16 rule, kept.

## How the surface is made

1. **Fair the table.** Ten stations are not enough rings for a smooth hull, so each of the three columns
   is interpolated along the length with **PCHIP** (monotone cubic Hermite). A plain Catmull-Rom spline
   through half-beams `0.72, 0.42, 0.20, 0.00` would swing *below* zero near the stem and give the bow a
   negative width; PCHIP never overshoots. Two properties are tested: values stay inside the table's range,
   and the faired sheer line's lowest point is exactly the table's lowest (so the deck level is exact).
2. **Cut 41 rings** (`HullShape::RINGS`) along the length, each read off the faired table.
3. **Draw each ring's section.** Round the bilge as a quarter-ellipse (6 strakes, spaced equally in angle),
   then up the topsides in a straight line that **leans in by 12%** (tumblehome - a real galleon's sides
   lean in above the water, which also narrows the deck) in 6 more strakes. A *strake* is one row of
   planking; Phase 49 will colour them in alternating shades.
4. **Join neighbouring points on neighbouring rings with quads**, two triangles each.

### Three surfaces, not one

Like the cylinder (whose wall and caps cannot share vertices), the hull is **three surfaces that meet at
creases**: the sides, the **cap** (the top between the two gunwales) and the **transom** (the flat stern).
A vertex carries one normal. The sides and the cap meet at the gunwale at roughly a right angle; sharing
those vertices would average the two normals and round the edge off. So the cap and the transom have their
*own* copies of the vertices along their edges - same positions, different meaning. Normals come from
`computeSmoothNormals()` (L9 slide 20), run over the whole mesh: within the sides they are blended across
neighbouring faces, and across a crease they are not, because no triangle touches both copies.

At the **stem** the width reaches zero, so the left and right gunwale points coincide. The last cap strip
would have two triangles, one of zero area; the builder keeps only triangles with an area, and serves both
points with one vertex so none is left unused (an unused vertex would have no normal to average).

## What changed

| File | Change |
|---|---|
| `src/Hull.h` (new) | `HullStation`, `HullProfile`, `HullShape` constants, `hullPchip`, `hullStationAt`, `hullRings`, `hullSectionPoint`, and the readers `hullHalfBeamAt`, `hullGunwaleHalfBeamAt`, `hullGunwaleHeightAt`, `hullKeelHeightAt`, `hullLowestSheer` |
| `src/Mesh.h` | `buildHullGeometry()` and `makeHull()` |
| `src/Ship.h` | `ShipDimensions` gains `hullProfile` and `deckCenterZ`; the hull is `(0.36, 0.26, 1.40)`; the deck level comes from the hull's lowest sheer; `scaleShipDimensions` scales `deckCenterZ` and leaves the profile alone |
| `src/main.cpp` | a ninth mesh, `hullMesh`; `drawShip()` draws it instead of the cube |

### Re-basing the deck, masts and cannon

The old deck sat on "the top of the hull". A galleon's hull has no single top: the gunwale is highest at the
stern and lowest in the waist. The deck now rests at the **waist's** gunwale height, `depth x hullLowestSheer`,
which for the old box (every sheer 1) is the depth - the same formula as before. A rectangular slab can only
lie where the hull is wide, so it was resized to the waist: 0.26 x 0.62, centred at `deckCenterZ = -0.04`.
The masts moved with it (fore at +0.20, main at -0.12 from the deck's centre, which is `+0.16` and `-0.16`
from the hull's), the main yard is 0.46 long and its sail 0.42 wide (the hull is narrower, 0.36 against 0.42),
and the cannon's mount is at `(0.07, 0)`. All of it is checked against the hull's outline:

```text
the deck slab lies inside the gunwale outline along its whole length with 0.021 to spare on each side
its underside is 0.1664 above the keel = depth x lowest sheer, and never above the hull under it
mast 0 and mast 1 stand 0.11 and 0.19 clear of the slab's ends, where the gunwale half-beam is 0.157
the cannon's mount is entirely on the slab, and 0.038 inside the gunwale
```

## Evidence

### The hull against its table (50 checks, the real generator)

```text
PLAN view  - the widest point of each of the 41 rings is the table's half-beam:       worst error 0.0e+00
SIDE view  - the gunwale of each ring is the table's sheer:                           3.0e-08
SIDE view  - the keel of each ring is the table's keel rise:                          0.0e+00
5 of the 10 stations fall exactly on a ring; on those the mesh equals the TABLE ENTRY: 0.0e+00
between rings the plan outline is within 0.0009 of the faired curve (hull 1.4 long)
the unit mesh spans exactly [-0.5, 0.5] on all three axes
```

**Proportions, measured on the lofted hull:** length 1.4000, beam 0.3600, **length : beam = 3.889**, inside
the plan's 3.6 to 4.4. The last 5% of the length is 0.031 wide against a beam of 0.18: a pointed bow.

**A closed solid.** Positions that coincide are merged and every edge is counted:

```text
V = 1013, E = 3033, F = 2022, V - E + F = 2       (Euler's formula for a closed sphere-like surface)
0 edges with one triangle (no hole); 0 with three or more (no fold)
0 degenerate triangles; signed volume +0.493 of the unit box: the winding is outward
2022 triangles = sides 1920 + cap 79 + transom 23    (worked out from the construction before running it)
```

Also: all 1131 normals are finite and unit length; every triangle's winding agrees with its vertices' averaged
normals; the cap and sides share positions but their normals are at least 78 degrees apart at every ring
(the gunwale is a crease, not rounded); the transom's normals point exactly aft; the hull is exactly
symmetric about the centreline; and editing two table entries fills out the bow (width there 0.40 to 0.69
of the box) with no other change to the code and the edited hull still closed.

### Pictures (16 checks)

```text
gallery, ship hidden, vs the real Phase 33 : 10 views byte-identical (Demo A and Demo B untouched)
showcase -> G -> G -> G                    : the gallery, byte for byte
gallery with the ship vs Phase 37          : 94,569 pixels differ INSIDE the ship's region, 0 outside it
```

(With the ship shown the gallery can no longer equal Phase 37 - the hull is a new shape - so the claim is the
precise one: the new hull changes pixels only where the ship is.)

**No hole from any side.** Every pixel covered by a projected hull triangle (eroded by 2 px) must be
painted. Seven viewpoints, including two from **below the waterline** to see the keel and bilge, plus the
normals view from two sides:

```text
F1  32,256 interior pixels, 0 holes     F2  22,973, 0     F3  46,751, 0     F4  29,646, 0     F5  30,567, 0
below (yaw 35, pitch -35)  50,096, 0    below (yaw 215, pitch -50)  66,351, 0
normals view: 0 black pixels (no NaN or zero normal)
```

**Negative controls.** With the *cap* removed the see-through test reports **24,227 holes** from above and the
CPU test reports 81 open edges and Euler's formula broken. With the winding *reversed* the CPU test fails 5
checks (signed volume `-0.49`) - **but the see-through test does not notice**: an inside-out hull still paints
the far inside wall, so no pixel is empty. That is stated here because it matters: the picture test finds
*missing* faces, the CPU volume and normal-agreement tests find *inverted* ones, and both are needed.

## Earlier Stage D tests on the new hull

The plan says every earlier Stage D test passes on the new hull. All 182 checks (Phases 34 to 37: rigidity,
the hierarchy, the cannon chain, the `H` proof at eight angles) pass, with **two assertions restated** in
Phase 34's test because their premise was the box:

| Was | Now |
|---|---|
| the deck's underside meets "the top of the hull" (hull centre + depth/2) | it meets the hull's **lowest gunwale** (keel + depth x lowest sheer) - the same number for a box |
| the deck is centred over the hull | it is centred across the hull, and sits at `deckCenterZ` along it |

The hierarchy checks are untouched: `H` still gives the level ship byte for byte, and the 17 parent-child
links still hold.

**Phase 38's size-scaling test also had to change**, and it is worth saying why. It read the dimensions as an
array of floats and required the untouched ones to be exactly the two yard fractions. The hull profile is 40
more floats that must *not* scale, and several are zero, where "scaled" and "left alone" are the same number.
It now probes with a copy in which every float is a distinct nonzero value, treats the profile as one block
that must be bit-for-bit unchanged, and requires everything else to scale or be a yard fraction. Its negative
control - the cannon mount's `z` not scaled - fails. That control was a *zero* in the real table (I set the
mount's `z` to 0), which the old form of the test could not have caught.

## What you see, and what is still missing

The ship now has a curved, pointed hull with a rounded bilge, a sweeping sheer line and a raised stern. It
reads as a boat with two masts, not yet a galleon: there is no sterncastle or forecastle (Phase 50), the whole
hull is one brown (Phases 48 and 49), no gunports or cannons along the sides (53, 54), no bowsprit or
figurehead (57, 58). The preset framing test (Phase 39) still passes, with the ship a little smaller in frame
(60% of the window's height in `F1`, from 73%).

## Likely teacher questions

### Why not scale a cube?
A scale matrix changes size, not shape. A hull's width changes along its length; a cube's cannot.

### What is lofting?
Describing a surface by cross-sections and fairing a surface through them - how a shipwright's lines plan works.

### Why PCHIP?
Because a plain spline overshoots: through widths `0.42, 0.20, 0.00` it would go negative before the stem. PCHIP
stays between neighbouring values and reproduces a minimum exactly.

### Why is the cap a separate surface?
A vertex has one normal. Sharing the gunwale's vertices would average the side's and the deck's normals and
round the edge. Separate vertices keep the crease sharp (the same lesson as the cube in Phase 10).

### Is it watertight?
Yes: Euler's formula gives 2, no edge has one triangle, none has three.

### How is the table a "viva exercise"?
Edit two numbers and the bow fills out, measured: 0.40 to 0.69 of the box, still closed, no other code changed.

### Why does the deck sit at the waist and not at the top?
There is no single top: the gunwale is highest at the stern. The deck rests at the lowest gunwale.

## Simple viva modifications

- **The named exercise: sharpen the bow or widen the beam by editing the station table.**
  `ShipConfig::DEFAULT_DIMENSIONS` in [src/Ship.h](../src/Ship.h): lower the bow stations' `halfBeam` for a
  sharper bow; raise the stern's `sheer` entries for a higher poop; or change `hullSize.x` to widen the whole ship.
- **More or fewer planks:** `HullShape::BILGE_STRAKES` and `TOPSIDE_STRAKES` in [src/Hull.h](../src/Hull.h).
- **Lean the sides in more:** `HullShape::TUMBLEHOME`.
- **Smoother hull:** `HullShape::RINGS`.
- **See the shape:** press `N` for the normals view, or `W` for wireframe: 41 rings of 25 points.

## Checkpoint

Phase 47 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the hull's outline from above and from the side matches the station table (worst error `0` and `3e-08`);
- length to beam is inside 3.6 to 4.4 (3.889), measured on the mesh;
- the mesh is a closed solid (Euler 2), has no degenerate triangle, is wound outward, and has finite unit normals;
- every earlier Stage D test passes (182 checks, two restated for the new premise);
- the rendered hull has no hole from seven viewpoints, including from below;
- the gallery with the ship hidden is byte-identical to Phase 33, and the new hull changes nothing outside the ship;
- the showcase is 26 draws / 7458 triangles / 4176 vertices.

## What is not part of Phase 47

No colour variation (Phase 48), no planking bands (49), no deck levels or castles (50), no railings, gunports or
cannons along the sides, no bowsprit. The deck slab and the masts are still the prototype's parts, re-placed to
fit; Phases 50 and 59 replace them.

**Phase 48 is next**: vertex-colour albedo - the per-vertex colour, unread since Phase 29, returns as a
multiplier on the diffuse and ambient colours, behind a switch that is off for everything that exists.
