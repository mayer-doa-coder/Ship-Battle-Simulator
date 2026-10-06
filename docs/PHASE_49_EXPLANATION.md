# Phase 49 - Hull Planking: Alternating Strakes and Dark Belts, Painted in the Vertices

## Status

Phase 49 is complete and verified.

```text
Showcase : draws 26 | tris 7458 | verts 5078     (was 4176: the hull is 2033 vertices instead of 1131)
Gallery  : draws 28 | tris 4218 | verts 3480
Hull mesh: 2033 vertices, 2022 triangles (the SAME triangles - only the vertices are split)
```

No new mesh, material or shader. New in `src/Hull.h`: the planking colours and rules. New in `src/Mesh.h`:
a second pass in the hull builder. 35 new checks passed with zero failures (22 on the mesh, 13 on pictures),
the 51 hull checks of Phase 47 and the 182 earlier Stage D checks were re-run and pass, plus the build gate.

## What you see

The hull's side is **twelve strakes** of planking - six round the rounded bilge, six up the topsides - in
alternating light and dark shades, with **three dark belts** (wales) running the length of the ship: one at the
**waterline**, one at the **deck line** and the **gunwale rail** along the top. The cap (the deck between the
gunwales) is a lighter, drier wood, and the transom a darker one.

```text
strakes 1..12 from the keel:   L D L D [B] D L D [B] D L [B]       L light plank, D dark plank, B belt
```

## The one idea

**Paint is data in the vertices.** Phase 48 made the vertex colour a multiplier on the material. The hull
material is brown timber; each vertex now says *how much* of that brown: 1.12 for a light plank, 0.86 for a dark
one, 0.45 for a belt of heavy, tarred timber.

```cpp
const glm::vec3 LIGHT_PLANK(1.12f, 1.08f, 1.02f);
const glm::vec3 DARK_PLANK (0.86f, 0.82f, 0.78f);
const glm::vec3 BELT       (0.45f, 0.40f, 0.36f);
```

Nothing is a texture, and nothing reads an image. The planking costs 902 more vertices and not one more draw
call or triangle.

## The belts are placed by fraction, not by number

```cpp
constexpr float BELT_FRACTIONS[3] = { 0.40f, 0.75f, 1.00f };   // how far up the side: 0 = keel, 1 = gunwale
```

`hullBeltStrake(belt, strakes)` rounds that to a strake. With 12 strakes the belts land on 5, 9 and 12; with 16
they land on 6, 12 and 16 (tested), so changing the number of strakes - the viva exercise - moves the belts
along with it instead of leaving one off the end of the side. A single-strake side clamps them all onto it.

**The first belt really is on the waterline.** Amidships the waterline is `draft` above the keel line,
`y = -0.5 + 0.06/0.26 = -0.2692` in the unit hull. Reading the section curve there, that height lies inside
strake 5 - and strake 5 is the first belt. Tested, not asserted: a hull whose planking ignores its waterline
would fail.

## Crisp edges that the lighting cannot see

A vertex carries one colour. A row of the loft is shared by the plank below it and the plank above, which are
different colours - so a shared row would blend them into a soft gradient, not a plank edge. The fix is a second
pass in `buildHullGeometry()`:

1. **Pass one** builds the hull exactly as in Phase 47, with every ring-point shared by the quads around it, and
   computes `computeSmoothNormals()` over it (L9 slide 20). The hull shades as one smooth surface.
2. **Pass two** copies the triangles out, giving each *plank* its own copy of every ring-point it touches,
   with that plank's colour - and takes the **normal from pass one**. So the colour changes at the plank edge and
   the lighting does not: the edge is invisible to the illumination model and visible to the eye.

Tested: at all 429 interior plank edges on the right side there are exactly two vertices in one place with
**different colours and a bit-identical normal** (worst difference `0.0e+00`). The vertex count follows from
the construction and was worked out before it was run: `41 rings x 47 + the cap's 81 + the transom's 25 = 2033`,
and the program printed `vertices=2033`. Merging coincident positions, the hull is still a closed solid
(`V - E + F = 2`, no open edge), and it has the same triangles as the unpainted hull - only the vertices are split.

## What changed

| File | Change |
|---|---|
| `src/Hull.h` | `HullPlanking` (the three shades, the belt fractions, the cap and transom colours), `hullBeltStrake`, `hullStrakeColor`, `HullColors`, `hullPlankColors`, `hullPlainColors`, `hullStrakeOfQuad`, and `hullSectionPointAt` (the section curve at a fractional position, so a point can be taken ON a plank's arc) |
| `src/Mesh.h` | `buildHullGeometry()` takes a `HullColors` and has the second pass; `makeHull()` likewise |
| `src/main.cpp` | the hull is built with `hullPlankColors()` |

## Evidence

### The mesh (22 checks)

```text
colour table   : LDLDBDLDBDLB. 12 strakes, 3 belts (5, 9, 12), the last belt is the gunwale rail
alternation    : all 6 pairs of neighbouring planks differ in luminance by more than 0.1
belts          : the brightest belt is 0.408, the darkest plank 0.826 - every belt darker than every plank
520 ring-points on the right side: each has a vertex in the colour of every plank that touches it
the cap's 81 vertices are the cap colour, and all face upward; the transom's 25 are the transom colour
```

### The pictures (13 checks)

```text
the showcase drawn WITHOUT the hull is byte-identical to Phase 48 from all five presets
the gallery drawn without the hull is byte-identical to Phase 48;  with the ship hidden, to the real Phase 33
no hole in the painted hull from yaw 35/pitch 14, from above (55) and from below the waterline (-35)
```

**Each plank is the plain hull times its multiplier.** The same view is rendered from the Phase 48 build (white
vertices, so identical lighting) and from this one, and at the middle of each plank's arc - not its chord, which
for the rounded bilge lies inside the hull - the planked pixel is predicted by

```text
planked = (plain - f H) x m + f H        f = the Phase 45 haze fraction at that distance, H the horizon colour
```

(diffuse and ambient scale by `m`; the haze mix and the specular do not). Three camera views, close in:

```text
side view          strakes 5..12 : worst error 4 of 255      (4 strakes face downward and are hidden from here)
three-quarter view strakes 6..12 : worst error 1             (5 hidden)
from below         strakes 2..12 : worst error 1             (strake 1's sample point is off screen)
```

Only a plank that can be seen is tested - the surface must face the camera, and its pixel must be on the hull -
and the report says which were skipped and why. Between them the views verify planks 2 to 12 against pixels;
**strake 1, the lowest, is verified on the mesh but not in a picture.**

### Negative control

With the quad-to-strake mapping off by one (every plank painted the colour of the one below), the planking test
fails at 280 ring-points, at the plank edges, and on the vertex count.

## Two mistakes in my test, not in the hull

1. **The first plank test sampled the chord midpoint of each plank**, the straight line between two rows - which for
   the rounded bilge is *inside* the hull. It reported errors of 24 to 30 at strake 1 and a 93-level error at strake 5
   where the pixel turned out to be the background. The fix was a function that reads the section curve at a
   fractional strake position (`hullSectionPointAt`), a facing test, and an on-hull test.
2. **A threshold written wrongly**: `groups > 1000` for a quantity that is `39 x 11 = 429`. It failed an honest hull.

## Likely teacher questions

### How are the planks made?
They are not geometry. Twelve rows of the loft carry twelve vertex colours; the triangles are the same 2022 as
before.

### Why do the vertices have to be duplicated?
A vertex has one colour. Two planks meet at a row; to have a crisp edge each needs its own vertices there. The normals are
copied from the shared mesh so the lighting stays smooth.

### Why is a belt a fraction and not a strake number?
So changing the number of strakes keeps the belts at the same heights on the hull.

### Is the waterline belt really at the waterline?
Tested: the strake that holds the waterline amidships is the first belt.

### Does it cost anything?
902 more vertices (2033 against 1131). No extra draw call, no extra triangle.

## Simple viva modifications

- **The named exercise: change the number of strakes.** `HullShape::BILGE_STRAKES` and `TOPSIDE_STRAKES` in
  [src/Hull.h](../src/Hull.h): 8 and 8 gives sixteen thinner planks, with the belts moved to 6, 12 and 16.
- **Change a shade:** `HullPlanking::LIGHT_PLANK`, `DARK_PLANK`, `BELT`.
- **Move a belt:** `BELT_FRACTIONS`.
- **Make the hull plain again:** build it with `hullPlainColors(glm::vec3(1.0f))` in `createMeshes()`.

## Checkpoint

Phase 49 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the side has 12 strakes, three belts, and neighbouring planks alternate in luminance;
- every belt is darker than every plank;
- the first belt is the waterline strake;
- the plank edges are crisp in colour with identical normals on both sides;
- the mesh is the same closed solid with the same 2022 triangles;
- each visible plank in a rendered picture is the plain hull times its multiplier, to within 7 levels (measured: 4);
- the rest of the ship is byte-identical to Phase 48;
- the showcase is 26 draws / 7458 triangles / 5078 vertices.

## What is not part of Phase 49

No gunports, so the deck-line belt has nothing to frame yet (Phase 53). No gilded trim along the belts (Phase 56).
The palette is still the one in this file, audited in Phase 75. The deck slab and the cap are not planked: Phase 51
lays real deck planks.

**Phase 50 is next**: deck levels - the main deck, the raised sterncastle (quarterdeck and poop) and the forecastle.
