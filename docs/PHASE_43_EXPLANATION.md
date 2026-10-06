# Phase 43 - Golden-Hour Lighting: a Profile per Scene

## Status

Phase 43 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged)
Showcase lighting : sun 19.4 degrees above the horizon, colour (1.15, 0.80, 0.50);
                    ambient (0.30, 0.36, 0.52); empty pixels (0.93, 0.66, 0.38)
Gallery lighting  : Stage C's, byte for byte
```

No new mesh, shader or draw call. 18 image checks plus the build gate, zero failures.

## The one idea

**What decides the *look* of a scene is a handful of numbers. Gather them into a struct, and let each scene pick its own.**

```cpp
struct LightProfile {
    const char* name;
    glm::vec3 sunDirection, sunColor, ambient, clearColor;
    bool pointLightEnabled;
};
```

A profile is only data. The shader, the two lights and the way they are added are untouched; the
same two uniforms are given different values. That is the answer to "why not a second shader for
sunset?": it is the same illumination equation with different inputs (and the project's rule is
one shader program).

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | `LightConfig::LightProfile`, `STAGE_C_PROFILE` (assembled from the existing constants), `GOLDEN_HOUR_PROFILE`; `activeLightProfile()`; `renderScene()` reads the clear colour, sun, ambient and point-light switch from it |

Phase 38's one-off rule "force the test point light off in the showcase" was written inside
`renderScene()`. It now lives where it belongs, as `pointLightEnabled = false` in the golden-hour
profile, and Stage C's profile has it `true`.

## The two profiles

| | Stage C (gallery) | Golden hour (showcase) |
|---|---|---|
| Sun travels | `(-0.4, -0.35, -0.5)` | `(-0.65, -0.30, -0.55)` |
| Sun elevation | 28.7 degrees | **19.4 degrees** |
| Sun colour | `(1.00, 0.96, 0.90)` near-white | `(1.15, 0.80, 0.50)` warm, over-bright |
| Ambient | `(0.15, 0.15, 0.18)` | `(0.30, 0.36, 0.52)` sky-tinted |
| Empty pixel | `(0.82, 0.66, 0.04)` golden | `(0.93, 0.66, 0.38)` horizon glow |
| Test point light | on | off |

- **Low sun.** Long grazing light across hull and sails. From the ship's `+x` side and a little
  ahead, so the visible flank and the sail faces are lit and the opposite faces are not: the form
  has a bright side and a dark side.
- **Warm and over-bright.** A sun near the horizon has passed through a lot of air, which
  scatters the blue away. Red 1.15, blue 0.50.
- **Sky-coloured ambient.** In a real scene the shadows are filled with light from the sky, so
  shaded faces take a blue cast. (It is multiplied by each material's `k_a`: a brown material
  stays brownish in shadow, a neutral one - the sails - goes visibly blue.)
- **Horizon-coloured clear.** Phase 45's sky dome starts from this same colour, so the sky and
  the empty background will meet without a seam.

## It is a measurement, not a feeling

The plan's checkpoint is "sunlit faces measure warm (red above blue) and shaded faces measure
cool". A brown hull is warm under *any* light, so it would prove nothing. The test points are on
the **main sail**, which is nearly neutral cloth, so its colour is the light's colour. Three
points on it were projected through the real `F1` and `F4` cameras and the rendered pixels read:

```text
Sunlit  (F1, the sail's +z face) : (149, 108, 75)   red is 1.99 x blue
Shaded  (F4, the sail's -z face) : ( 15,  17, 23)   blue is above red
```

and this holds in **Phong, Gouraud and Flat** (nine checks in each). The shading mode changes how
the lighting is evaluated, not what colour the light is.

### Negative control

With the sun made `(1, 1, 1)` and the ambient `(0.30, 0.30, 0.30)`, the same test fails 6 of 9:
the lit sail is `(132, 128, 119)`, only 1.11 times as red as blue, and the shaded one is
`(15, 15, 13)`, with red above blue. Worth noticing: "red above blue" alone *passes* for a neutral
light (the cloth's own `k_d` is slightly warm, 0.75 against 0.68) - which is exactly why the test
also demands a ratio of at least 1.5.

## The Stage C numbers return exactly

```text
gallery, ship hidden, vs the real Phase 33       : all 10 views byte-identical
gallery, ship shown, no-sample window, vs Phase 37 : all 10 views byte-identical
showcase -> G -> G -> G                          : the Stage C picture, byte for byte
showcase -> G -> G                               : the golden-hour picture, byte for byte
the gallery's empty pixels are still (209, 168, 10)
```

The profile in the gallery holds the *same floats* as the old constants, so nothing there moved.

## What I got wrong first

My first golden-hour sun came from the ship's `+x` side and slightly *astern*, `(-0.85, -0.30,
0.15)`, with an ambient of `(0.20, 0.25, 0.38)`. Rendered, the default `F1` view had **black sails
on a black sea**: every `+z`-facing surface faced away from the sun, and a shaded sail under a dim
ambient was too dark to see against dark water. The numbers were "correct" and the picture was
unreadable.

The fix was to move the sun *ahead* of the beam (`-0.55` in `z`) so the bow-facing surfaces are
lit, and to raise the ambient to `(0.30, 0.36, 0.52)` so shadows read as blue rather than as
holes. That is a real trade-off and it is worth knowing: **the stern (`F4`) is now the shaded
view.** The rear sail is a dark blue-grey, not black, which is what "cool shaded" should look
like, but the stern will be darker than the bow until the gilded trim and the lanterns
(Phases 56 and 72) give it its own light.

## Known limits

- The **sea is nearly black.** It is the unmodified `OCEAN` material (Demo A depends on it), lit
  by a grazing sun and a small ambient. It becomes a reflecting surface when the wave normals
  arrive (Phase 82) and gets a horizon to meet in Phase 45.
- The **sun cannot throw a glitter streak** in these views: it is on the same side as the camera,
  so its mirror image is behind the viewer. That is a property of the chosen angle, not a defect.

## Likely teacher questions

### Why not a second shader for the sunset?
It is the same equation with different inputs. One program, two sets of uniform values; switching
mode costs nothing.

### Why does the sun colour have a value above 1?
Light colours are multiplied by reflectances below 1 (the sail's `k_d` is 0.75) and by `N . L`, so
the lit sail face measures red 149 of 255, about 0.58. Only the final colour is clamped to 1.

### How did you prove "warm" and "cool"?
By reading rendered pixels on neutral sail cloth, not by looking. Sunlit red is 1.99 times blue;
shaded blue is above red; in all three shading modes; and a neutral light fails the same test.

### Why is the gallery not golden hour too?
It is Stage C's evidence. Demo A and Demo B were measured under that lighting and every phase
since proves it unchanged by comparing bytes.

### Where did the test point light go?
Off in the showcase, as a field of the profile. The real point light is the muzzle flash, Phase 90.

## Simple viva modifications

- **The named exercise: change the sun's colour.** `GOLDEN_HOUR_PROFILE.sunColor` in
  [src/main.cpp](../src/main.cpp). `(0.6, 0.7, 1.0)` is moonlight, which is Phase 104's mode.
- **Raise the sun:** make the second sun component more negative, e.g. `-0.8`; the elevation
  rises and the light flattens.
- **Make shadows warmer or colder:** the `ambient` triple.
- **Move the horizon:** `clearColor`.
- **Put the sun behind the ship:** negate the sun's `x` and `z`; the bow and sails go into shadow
  and the sea may start to glitter.

## Checkpoint

Phase 43 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- sunlit sail cloth measures warm (red at least 1.5 x blue) and shaded sail cloth measures cool
  (blue above red) in Flat, Gouraud and Phong;
- with `G` on, the Stage C numbers return exactly: ten views byte-identical to Phase 33 (ship
  hidden) and to Phase 37 (ship shown, no-sample window), and the key round-trips byte for byte;
- the sun is low (19.4 degrees), warm, with a sky-tinted ambient and a horizon-coloured clear;
- the test point light is a field of the profile, off in the showcase, on in the gallery;
- the showcase is still 15 draws / 496 triangles / 507 vertices.

## What is not part of Phase 43

No emission term (Phase 44), no sky (Phase 45), no horizon haze, no shadows (never: the position
is that this pipeline is local illumination, L8 s10-11). The environment modes sun, moonlight,
rain and winter (Stage M) will use this same struct.

**Phase 44 is next**: the emission term `uKe`, so a surface can glow without being lit.
