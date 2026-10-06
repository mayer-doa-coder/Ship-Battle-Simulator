# Phase 48 - Vertex-Colour Albedo: the Mesh Chooses Its Own Colour, Point by Point

## Status

Phase 48 is complete and verified.

```text
Showcase : draws 26 | tris 7458 | verts 4176     (unchanged - and so is every pixel of it)
Gallery  : draws 28 | tris 4218 | verts 2578     (unchanged)
```

No new mesh, no new draw call. One new uniform (`uVertexAlbedo`), one new material field
(`Material::vertexAlbedo`), one new material (`HULL_TIMBER`). 9 image checks plus the build gate passed
with zero failures, with three negative controls.

## The one idea

**The vertex colour goes back to work - as a multiplier on the material, not a replacement for it.**

Every mesh has carried a colour per vertex since Phase 2. It was an object's whole appearance until
Phase 29 replaced it with a *material*, and since then nothing has read it. A hull needs it back: a
single material (timber) that is a darker or lighter shade at different points - alternate planks, dark
belts of heavy timber round the waterline. A material alone cannot do that; it has one `k_d` for the
whole mesh.

```glsl
gAlbedo = (uVertexAlbedo != 0) ? gVertexColor : vec3(1.0);
total  += uGlobalAmbient * uKa * gAlbedo;          // ambient scaled
result += uKd * gAlbedo * lambert;                 // diffuse scaled
result += uKs * specular;                          // specular is NOT scaled
```

The **specular** term is deliberately left alone. A highlight is the colour of the *light*, not of the
surface (Phase 28): a red ball has a white highlight, and a stripe of paint does not tint the glint on it.

## Why nothing else changes

- **The switch is per material and off by default.** `Material::vertexAlbedo` has a default of `false`
  and sits last, so no line of any existing material was edited.
- **Multiplying by exactly 1.0 returns the number unchanged.** When the switch is off, `gAlbedo` is
  `vec3(1.0)`, and `uKd * 1.0 * lambert` is `uKd * lambert` to the last bit.
- **The hull is on a new material, not on `HULL_WOOD`.** The masts and yards also use `HULL_WOOD`, and
  their mesh - the cylinder - still carries Phase 20's *silver gradient* in its vertices. If the switch were
  on for `HULL_WOOD` the masts would be tinted silver. `HULL_TIMBER` has exactly `HULL_WOOD`'s four numbers
  with the switch on, and the hull's vertices are now white, so it looks identical.
- **The uniform is reset on every draw**, like `uKe` and `uKeFromVertexColor` before it: a uniform keeps
  its last value, and the draw after the hull must not inherit the switch.

## What changed

| File | Change |
|---|---|
| `src/Lighting.h` | `uniform int uVertexAlbedo`, `gAlbedo`; ambient and diffuse multiplied by it |
| `src/Material.h` | `Material::vertexAlbedo` (default false); `HULL_TIMBER` |
| `src/main.cpp` | `drawMesh()` uploads `uVertexAlbedo`; the hull is drawn with `HULL_TIMBER`; its vertices are white |

## Evidence

### 1. With the switch off for everything old, nothing changes

```text
gallery, ship hidden vs the real Phase 33                 : 10 views byte-identical
gallery WITH the ship vs Phase 47 (a build kept for this) : 10 views byte-identical
the showcase from F1..F5 in Flat and in Phong             : byte-identical to Phase 47
the showcase at the finest detail level                   : byte-identical to Phase 47
```

That includes the hull itself, now on `HULL_TIMBER` with white vertices. The comparison is against
the Phase 47 build, not against a stored picture, so it is a check on exactly what this phase touched.

### 2. A striped test mesh shows its stripes

The test harness adds a quad with four vertical stripes whose vertices carry colours A, B, A, B, where
A is white and B is `(0.5, 0.25, 0.75)`. Vertices are duplicated at every stripe boundary, so inside a
stripe the colour is constant. It is drawn through the project's own `drawMesh()`, so it exercises exactly the
uniforms and shader the ship does. With the switch **on**:

```text
ambient + diffuse (all three shading modes) : A = 151 129 108,  B =  76  32  81   ->  B/A = 0.503 0.248 0.750
ambient only      (all three shading modes) : A =  11  11  14,  B =   6   3  10
```

`0.503, 0.248, 0.750` against the vertex colours `0.5, 0.25, 0.75`: Flat, Gouraud and Phong give the
same stripes. (The ambient-only values are tiny, so they are compared in levels of 255 - stripe B is stripe A
times the vertex colour to within 1 level - not as a ratio, where one level of rounding on a value of 11 is a
9% "error". My first version used a ratio and failed for that reason alone.)

### 3. The highlight is not tinted

The highlight varies across the quad (the view direction changes with position), so the four stripes cannot
be compared with each other. Each is compared with itself: the same mesh with the switch on and off, specular
term alone, under Blinn-Phong so that a real highlight is on the quad:

```text
switch ON  : 3 3 3 | 9 9 8 | 20 19 18 | 35 33 31      (Flat and Phong; Gouraud 4 | 11 | 23 | 38)
switch OFF : 3 3 3 | 9 9 8 | 20 19 18 | 35 33 31      -> the two pictures are byte-identical
```

(An earlier version compared stripe A with stripe B directly, found them different, and was wrong to: the
difference was position, not paint. It also first measured a specular value of 0 under Phong's `(R . V)^n`,
where the quad is nowhere near the mirror direction - a check that passed because both sides were zero. It
now uses Blinn-Phong and requires the highlight to be at least 5 levels.)

### 4. The control: the same mesh with the switch off

All four stripes come out `151 129 108` - to the byte, in all three shading modes. The vertex colours are
ignored, exactly as for every material before this phase.

### Negative controls

| Broken on purpose, in a copy of `Lighting.h` | Result |
|---|---|
| the switch is ignored (`gAlbedo = gVertexColor` always) | the three gallery views checked each differ from Phase 47 by **over a million bytes** (every mesh takes its own vertex colours); and the switch-OFF control shows the stripes |
| paint tints the specular term too | specular alone, ON vs OFF: **61,504 pixels differ**, largest 43 |

The harness also showed the project's own safety net working: in the first variant the compiler removed the
now-unused uniform and the program printed `uniform 'uVertexAlbedo' not found or unused` - the warn-once
mechanism from Phase 3.

## Likely teacher questions

### Why a multiplier and not a replacement?
A replacement would throw away the material. A multiplier keeps its `k_d`, `k_a` and shininess and varies
only the colour: white means "as the material says", a darker value is a darker patch of the same wood.

### Why is the specular not multiplied?
A highlight is reflected light, the colour of the source (Phase 28). Tinting it would make a painted stripe
glow its own colour.

### Why a new material, `HULL_TIMBER`?
The masts share `HULL_WOOD` and their cylinder mesh has silver in its vertices. The switch is per material so
that only the hull takes its colours from its vertices.

### How can you be sure nothing else changed?
Multiplying by exactly 1.0 is exact, and the comparison is byte-for-byte against the previous phase's build
across the gallery and five camera presets in two shading modes.

### Do all three shading modes agree?
Yes: stripe ratios `0.503 0.248 0.750` in Flat, Gouraud and Phong, because the vertex shader (Gouraud) and the
fragment shader (Flat, Phong) both set the same `gVertexColor` before calling the one `computeLighting()`.

## Simple viva modifications

- **The named exercise: change a stripe colour.** In the hull builder, or - for the test - the two colours in
  the harness; Phase 49 puts real plank shades there. Try making the hull's vertices `(1.0, 0.6, 0.6)` in
  `createMeshes()`: the whole hull takes a red cast.
- **Turn it off:** set `vertexAlbedo` to `false` on `HULL_TIMBER` - the hull ignores its vertices again.
- **Break the rule on purpose:** make the specular term multiply `gAlbedo` and press `K` to see specular alone:
  the highlight changes colour with the paint.

## Checkpoint

Phase 48 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- with the switch off every earlier render is byte-identical (the gallery to Phase 33 and to Phase 47, the
  showcase to Phase 47);
- the striped test mesh shows its stripes, `B = A x (0.5, 0.25, 0.75)` to within 1 level, in Flat, Gouraud and
  Phong, with ambient+diffuse and with ambient alone;
- the specular term is byte-identical with the switch on and off;
- with the switch off the stripes vanish, to the byte;
- the showcase is still 26 draws / 7458 triangles / 4176 vertices.

## What is not part of Phase 48

No planks and no belts (Phase 49). The hull's vertices are all white. The hull builder still takes one
colour; Phase 49 changes it to take a colour per strake.

**Phase 49 is next**: hull planking - alternating shades per plank strip and dark timber belts at the
waterline, the gunwale and the deck line, all from the vertex colours this phase switched on.
