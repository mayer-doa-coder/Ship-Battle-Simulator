# Phase 45 - The Sky Dome, the Sun's Disc and the Distance Haze

## Status

Phase 45 is complete and verified.

```text
Showcase : draws 17 | tris 2548 | verts 1537     (15 + the dome + the sun's disc)
Gallery  : draws 28 | tris 2208 | verts 1471     (unchanged: no sky, no haze there)
Meshes   : 8  (the sky dome is the sphere generator again, built once with the sky's colours)
```

New file `src/Sky.h` (no OpenGL). 35 checks passed with zero failures: 19 on the sky's arithmetic
and 16 on rendered pixels, plus the build gate.

## The three ideas

### 1. The dome is the sphere, seen from inside, painted with a gradient

No cube map and no texture: it is the existing `makeSphere()` (the same generator as the ball and
the cannonball) built once with two colours. `makeSphere` blends them linearly from pole to pole
and the equator - the horizon - gets their midpoint. To put the **horizon** colour exactly on the
equator and the **zenith** exactly at the pole, the colours handed in are

```text
top    = zenith
bottom = 2 * horizon - zenith       so that (bottom + top) / 2 = horizon
```

(`bottom` has a red channel of 1.76. That is fine in a vertex - it is a float - and it is only seen
far below the horizon, behind the sea.) What is on screen is then
`horizon + (zenith - horizon) x elevation / 90 degrees`.

Three details that decide whether it works:

- **Seen from inside.** A sphere's triangles face outward, so from within they are all back faces
  and culling would discard the lot. `glCullFace(GL_FRONT)` culls the other set instead, then
  `GL_BACK` is restored.
- **Centred on the camera**, so its distance never changes: a sky does not parallax. Measured: the
  sun's disc is at pixel `(640.01, 119.78)` with the camera 8.96 units out and again at 14.
- **Writes no depth** (`glDepthMask(GL_FALSE)`), drawn first, so it can never hide anything.

It is pure emission: a new material `SKY_DOME` with `k_a = k_d = k_s = 0` and `ke` white, plus one
new flag `keFromVertexColor` (Phase 44's emission, multiplied by the vertex colour) so a single
material can glow a different colour at every point. A sky is not a surface the sun shines on; it is
where the sun is.

### 2. The sun's disc is where the light says

A small sphere at `SUN_DISTANCE` (80) from the camera along `L = -normalize(sunDirection)` - the
direction the sunlight comes **from**. That one negation is the whole connection between "the sun
that lights the ship" and "the sun in the sky". Its diameter is set so it *covers* 3 degrees:
`D = 2 d tan(1.5 degrees) = 4.19`.

### 3. Distance haze

The plan's own checkpoints ("continuous with the sea's haze", Phase 46's "hazed silhouettes")
presuppose a haze, and no earlier phase supplied one - so this phase does:

```glsl
fraction = 1 - exp( -(density * distance)^2 )
lit      = mix(lit, uHazeColor, fraction)       // BEFORE the emission is added
```

- **Squared**, so things near the eye are left alone and the horizon is lost completely. At density
  0.03: 9 units away is 7.0% hazed, 13 units 14.1%, 60 units 96.1%, 80 units 99.7%. A plain
  `1 - exp(-density x distance)` would wash out the ship to hide the sea's edge.
- **Before the emission**, so a lantern or the sun is not dimmed by the air in front of it. And the
  sky is drawn with density 0, or the dome (90 units away) would be 100% haze and come out one flat
  colour.
- **Density 0 means no haze, exactly**: the exponent is 0, the fraction is exactly 0, and
  `mix(x, y, 0)` returns `x`. The Stage C profile has density 0.
- Haze colour = the horizon colour, so far water fades into the colour the sky starts from.

## What changed

| File | Change |
|---|---|
| `src/Sky.h` (new) | `SkyConfig` (radius 90, 24 x 36, sun at 80 units, 3 degrees), `skyDirectionToSun`, `skySunPosition`, `skySunDiscDiameter`, `skyDomeBottomColor`, `skyColorAtElevation` |
| `src/Lighting.h` | `uKeFromVertexColor`, `gVertexColor`, `uHazeColor`, `uHazeDensity`; the haze and the vertex-colour emission in `computeLighting()` |
| `shaders/basic.vert`, `basic.frag` | each sets `gVertexColor` before calling `computeLighting()`; the fragment shader reads `vColor` again |
| `src/Material.h` | `keFromVertexColor`; `SKY_DOME`, `SUN_DISC` |
| `src/main.cpp` | `LightProfile` gains `hazeColor`, `hazeDensity`, `hasSky`, `zenithColor`; `makeSkyMesh()`; `drawSky()`; the sky mesh created, drawn and freed; `ShowcaseConfig::SEA_SIZE` 120 to 160 |

The sea is 160 across now (it was 120). At 120 its edge was 60 units out, where the haze is 96%, and
a faint seam showed against the sky; at 160 the edge is 80 units out and 99.7% haze.

## Evidence

### The sky's arithmetic (19 checks, the real `Sky.h` and the real sphere generator)

```text
from 4 different eyes the direction to the disc equals L = -normalize(sunDirection): worst 0.0000 degrees
it is always exactly 80 units away; it stands 19.4 degrees up, as the light does
a sphere of diameter 4.190 at 80 units covers 3.0000 degrees
all 830 dome vertices carry the formula's colour at their elevation (worst error 1.7e-07)
the 36 vertices of the equator are exactly the horizon colour (error 0); the top vertex is exactly the zenith
the dome (90) is inside the far plane (100); the disc is inside the dome; the ship is within 13.6 units
  of the camera in all five presets - under a quarter of the dome's radius
```

### The rendered pixels (16 checks)

```text
sky, rendered vs the formula for the direction each pixel looks in, 5 pixels, 3 shading modes :
    worst difference 1 level of 255 (e.g. elevation 15.75 deg: expected 200 146 98, rendered 200 146 98)
the horizon, F3 and F1: the largest change between adjacent rows is 3 and 4 levels (no seam)
the sun, viewed from the far side of the ship: centroid (640.0, 119.8) vs predicted (640.5, 120.5),
    width 47 px vs predicted 47.2, round, colour 255 229 158
haze on the ship, 3 sail points 9.25 units away: lit (149,108,75) -> expected (156,112,77),
    rendered (156,113,77), worst difference 1
gallery with the ship hidden: 10 views byte-identical to the real Phase 33
gallery with the ship shown (no-sample window): 10 views byte-identical to Phase 37
showcase -> G -> G -> G: the Stage C picture, byte for byte; G, G: the sky returns, byte for byte
```

### Negative controls

| Broken on purpose | Result |
|---|---|
| the sun's direction not negated, in a copy of `Sky.h` | 3 failures, the first reading **180.0000 degrees** |
| haze density 0 | the seam test fails: adjacent rows differ by **204 levels** (against 3); the haze test fails by 7 |

## A mistake the negative control caught

The first version of the sun-direction test **passed when the sun was pointing the wrong way**.
The failing control reported 3 failures where it should have reported 4: the angle check had passed
at "worst 0.0000 degrees" for a disc exactly 180 degrees out. The cause: `acos` of a dot product that
float rounding had left at `-1.0000001` is `NaN`, and `std::max(0, NaN)` returns 0, so the worst
angle stayed 0. The dot product is now clamped to `[-1, 1]` before the `acos`; with the clamp the
broken copy reads 180 degrees and fails. A test that was never made to fail would have kept passing
for ever.

(Two smaller test-writing slips: the no-parallax check first rendered the camera at 4 units, where the
ship covers the sun; and a ring-ordering inequality was written backwards. Neither touched the code.)

## What you see - and a thing to know about the presets

The sun is at the **same side of the ship as the F1 camera** (that is what lights the visible faces),
so in all five presets it is behind the camera and not on screen. To see it, swing the camera round to
the far side: hold `.` for about 3.3 seconds (60 degrees a second from yaw 35 to about 230). The ship
becomes a silhouette against the sun, and the sea under the sun gets a bright **specular glitter**
(visible at the bottom of that picture) - Demo A's highlight in the showcase. A different choice would
have put the sun in front of the F1 camera at the price of lighting the ship from behind.

## Known limits

- **Gouraud haze is per vertex.** The sea is 8 x 8 large triangles, so in Gouraud the haze is
  interpolated linearly between corners. The default Phong mode evaluates it per pixel. (Demo A's whole
  point is that Gouraud misses what falls between vertices.)
- The sea is still the unmodified `OCEAN` material and still flat: waves arrive in Phase 81.
- The `W` wireframe key shows the dome's lines; the `N` normals view paints the dome's normals. Both
  are debug views doing what they say.

## Likely teacher questions

### Is this a skybox?
No. A skybox is a cube of six textures. This is a sphere with a colour blend between two colours
painted into its vertices; there is no image anywhere in the project.

### Why does the horizon not show a line?
The dome starts from the horizon colour, the haze fades the far sea to the same colour, and the sea is
large enough that its edge is 99.7% haze. Measured: adjacent rows differ by at most 3 levels.

### How do you know the sun is where the light is?
The disc's position is `eye + L x 80` and `L` is `-normalize(sunDirection)` - the same vector the
lighting equation uses. Tested to 0.0000 degrees from four eyes, and in the picture the disc's centroid
is within 0.7 px of the projection.

### Why is the haze squared?
So the ship, 9 to 13 units away, is only 7% to 14% hazed while the sea 60 units out is 96%.

### Why is the dome not hazed?
It is drawn with density 0. At 90 units it would otherwise be all haze, one flat colour.

### Why does emission skip the haze?
So a lantern, the sun or the sky is not dimmed by air in front of it. The haze is applied to the lit
colour, then the emission is added.

## Simple viva modifications

- **The named exercise: change the zenith colour.** `GOLDEN_HOUR_PROFILE.zenithColor` in
  [src/main.cpp](../src/main.cpp). `(0.02, 0.02, 0.10)` is a night sky; `(0.5, 0.7, 1.0)` is noon.
- **Move the sun:** change `GOLDEN_HOUR_PROFILE.sunDirection` - the disc and the lighting follow together.
- **Make the sun bigger:** `SkyConfig::SUN_ANGULAR_DIAMETER_DEGREES` in [src/Sky.h](../src/Sky.h).
- **More or less haze:** `hazeDensity`. 0 removes it exactly; 0.08 fogs the ship.
- **Show it is not a texture:** the sky mesh is 830 vertices and 1656 triangles; there is no image file.

## Checkpoint

Phase 45 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the dome's pixels match `horizon + (zenith - horizon) x elevation / 90` to within 1 level, in Flat,
  Gouraud and Phong;
- the horizon is continuous: adjacent rows differ by at most 8 levels (measured 3 and 4);
- the sun's disc lies exactly along the negated sun direction (0.0000 degrees) and covers 3 degrees;
- with haze density 0 and no sky, every earlier render is byte-identical (the gallery to Phases 33 and
  37, and the key round-trips);
- the showcase is 17 draws / 2548 triangles / 1537 vertices and the gallery is unchanged.

## What is not part of Phase 45

No clouds, stars or moon. No change to the sea material. No shadows. The dome's colours are built once
at startup; the environment modes (Stage M) will rebuild them.

**Phase 46 is next**: distant islands and rocks, hazed into dark silhouettes at the horizon.
