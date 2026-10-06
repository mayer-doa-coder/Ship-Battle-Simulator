# Phase 44 - The Emission Term: `uKe`

## Status

Phase 44 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged - and so is every pixel of it)
Material : one new field, ke, default (0, 0, 0)
Shader   : one new uniform, uKe, and one changed line:  return total + uKe;
```

Nothing in the program looks different yet, **on purpose**: this phase adds a capability and the
plan's checkpoint is that adding it changes no earlier picture. Lanterns (Phase 72), cabin windows
(55) and the sky (45) are what will use it. 10 image checks plus the build gate passed with zero
failures.

## The one idea

**Some surfaces are bright because they give out light, not because light hits them.**

The model so far has four numbers per material, and every one describes how a surface treats
light that *arrives*: `k_a`, `k_d`, `k_s`, `n_s`. A lantern's glass or the sun's disc is different
in kind. Lit by nothing at all, it is still bright. A fifth number says so:

```glsl
return total + uKe;      // at the very end of computeLighting()
```

It is added after both lights, outside the term mask (`K`) and the light mask (`L`), and it is
multiplied by nothing - not `k_d`, not the light's colour, not `N . L`. It is **not a light
source**: it brightens only its own surface and illuminates nothing else (that would be global
illumination, which this local model cannot do - L8 s10-11). The project still has exactly two
lights.

## What changed

| File | Change |
|---|---|
| `src/Material.h` | `Material::ke`, last, with a default of `(0, 0, 0)` |
| `src/Lighting.h` | `uniform vec3 uKe;` and `return total + uKe;` in `computeLighting()` |
| `src/main.cpp` | `drawMesh()` uploads `uKe` with the other four numbers |

Because `computeLighting()` is the one function Flat, Gouraud and Phong all call, **all three
honour the emission by construction**: Gouraud adds it per vertex and the rasteriser carries it
across, the other two add it per fragment. There is no second copy of the model to keep in step.

`ke` is last and has a default, so every material above it - the L8 slide 60 values included - is
untouched and glows not at all. No existing line of `Material.h` was edited.

### A uniform keeps its last value

`drawMesh()` sets `uKe` on *every* draw, including for the many materials where it is zero. If it
set it only when non-zero, the draw after a glowing object would inherit the glow. This is stated in
a comment, and it is tested - see below.

## Evidence

No existing material has emission, so the test harness overrides one: `-kemat sailcloth -ke r g b`
gives the sail cloth an emission, and `-kefull` also zeroes its `k_a`, `k_d`, `k_s` so the colour
is emission alone. The tests read rendered pixels at three interior points of the main sail.

### 1. With `uKe = 0` nothing changes

```text
gallery, ship hidden vs the real Phase 33      : all 10 views byte-identical
gallery, ship shown, no-sample window vs Phase 37 : all 10 views byte-identical
the golden-hour showcase vs Phase 43's picture : byte-identical
F1 and F4 in Flat, Gouraud and Phong vs Phase 43 : all six byte-identical
```

`total + vec3(0)` adds exactly zero, so this holds exactly, not approximately.

### 2. A purely emissive surface ignores everything

The sail is made purely emissive, `ke = (204, 102, 51) / 255`, and rendered under every combination
of:

```text
3 shading modes (Flat, Gouraud, Phong)  x  3 light states (both, sun only, no light at all)
x  4 term masks (all, specular only, ambient only, ambient + diffuse)  x  2 specular formulas
```

**216 pixels read, every one exactly `(204, 102, 51)`. Worst difference: 0.** Emission ignores the
sun, the `L` key, the `K` mask, the shading mode and Blinn-Phong.

The **control** shows the test could have failed: the same sail, *not* emissive, takes three
different colours over those states - lit `(149, 108, 75)`, ambient-only `(15, 17, 23)`, and black
under specular-only.

(Two details worth knowing. In the showcase the golden-hour profile switches the test point light
off, so "both lights" and "sun only" are the same picture there; the three light states still give
two distinct ones. And the `K` key's first press goes to *specular only*, not to ambient plus
diffuse - the cycle is the L8 slide 54 order, not counting upward. My first test script named the
states wrongly; the colours it measured were right.)

### 3. It is added, and it does not depend on the light

With a small emission `(26, 51, 77) / 255` on an ordinary sail:

```text
sunlit face : (149, 108,  75) -> (175, 159, 152)    added (26, 51, 77)
shaded face : ( 15,  17,  23) -> ( 41,  68,  99)    added (26, 51, 77)
```

The same amount on a bright face and a dark one, to within 1 of rounding.

### 4. The reset is real

Making the sails glow in a full render changes **33,020 pixels, all of them where sails are, and
0 anywhere else** - the cannon, mount, hull and deck are drawn after the sails and are not touched.

**Negative control:** with `drawMesh()` changed to upload `uKe` only when it is non-zero, the same
test shows **6,629 changed pixels where there are no sails** - the glow leaks onto the objects drawn
next. So the comment in `drawMesh()` is not decoration.

## Likely teacher questions

### What is the emission term for?
Surfaces that are bright by themselves: lantern glass, windows, the sun's disc, the sky. No amount
of lighting explains them, and no lighting should dim them.

### Is an emissive object a light?
No. It brightens itself and nothing else; a local model cannot light its neighbours. The project
has two lights and the lanterns will not change that.

### Why is it not multiplied by anything?
Because it is not light *received*. `k_d` and `k_s` scale incoming light; emission is outgoing.

### Why does `L` not affect it?
`L` switches lights. An emissive surface needs none. Tested at all light states: 216 exact pixels.

### How do Flat, Gouraud and Phong all get it?
They all call the one `computeLighting()`. The line is added there, once.

### Why set `uKe` for every draw?
A uniform keeps its value until it is changed. Without the reset the next object inherits the
glow - measured: 6,629 wrong pixels.

## Simple viva modifications

- **The named exercise: make a material glow.** In [src/Material.h](../src/Material.h) add a
  fourth number to a material, e.g. give `FLAG_CLOTH` an emission after its name:
  `"flag cloth", { 0.3f, 0.0f, 0.0f }` - the flag keeps glowing red under `L` and `K`.
- **Make the sails glow like paper lanterns:** `SAILCLOTH` with `{ 0.2f, 0.18f, 0.12f }`.
- **Show it ignores the lights:** press `L` and `K` and watch the glowing object not move.
- **Break the reset on purpose:** make `drawMesh()` upload `uKe` only when non-zero, give one
  material a glow, and watch the next object glow too.

## Checkpoint

Phase 44 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind (a `Material` aggregate with a
  defaulted last member needs no edits to the materials above it);
- with `uKe` zero every earlier picture is byte-identical - the gallery to Phase 33 and Phase 37,
  the showcase to Phase 43, in all three shading modes;
- an emissive surface is exactly its emission under every combination of mode, light state, term
  mask and specular formula tested (216 pixels, difference 0);
- the emission adds the same amount to a lit face and a shaded face;
- it does not leak to the next draw;
- the showcase is still 15 draws / 496 triangles / 507 vertices.

## What is not part of Phase 44

Nothing uses emission yet. There is no glow, bloom or halo - those are post-processing, which the
project does not do. Emission does not feed the vertex colour (the sky gradient in Phase 45 needs
that, and adds it then).

**Phase 45 is next**: the sky dome, and the distance haze its checkpoint presupposes.
