# Phase 30 - The Second Light, and Attenuation

## Status

Phase 30 is complete and verified. Debug and Release build with no warnings, and
15 checks passed with zero failures.

```text
Ship Battle Simulator - Phase 30 | draws 14 | tris 1840 | verts 1045
```

Counts unchanged from Phase 29. A second light adds no geometry.

### THE CHECKPOINT: the point light falls off, and `L` isolates each light

The floor was rendered on its own under each light and compared against an
**independent re-implementation** of the whole illumination model, evaluated at the
real world position of every floor pixel:

```text
lights        | pixels | mean measured | mean predicted | mean error | worst
sun only      |  60884 |         17.33 |          17.38 |       0.26 |   0.3
point only    |  60884 |         24.56 |          24.59 |       0.25 |   0.5
both          |  60884 |         39.80 |          39.83 |       0.25 |   0.5
```

**A quarter of one level out of 255**, across sixty thousand pixels. That is
rounding, not disagreement.

And the test has teeth - predicting with a pure `1/d²` falloff instead of the
three-term law gives a mean error of **20.49 levels**, eighty times worse.

The simplest picture of the difference between the two lights:

```text
floor, sun only   : mean 15.92   max  15.92    max/mean =  1.0
floor, point only : mean 25.23   max 255.00    max/mean = 10.1
```

The sun lights a flat floor **perfectly evenly** - the brightest pixel and the
average are the same number. It has no position, so there is no distance, so there
is nothing to fall off over. The point light makes a pool.

## What changed

| File | Change |
|---|---|
| `shaders/basic.frag` | New `lightContribution()` function; `uPointPosition`, `uPointColor`, `uPointIntensity`, `uAttenuation`, `uLightMask`; main body restructured into one call per light |
| `src/main.cpp` | Point-light constants and the attenuation triple in `LightConfig`; `MASK_*` flags; `SceneState::lightMask`; the `L` key; five more uniforms |

## The one idea

**A light with a position has a distance, and distance makes light fall off.**

That is the entire difference between the two lights in this project, and it is why
there is exactly one of each kind.

| | Sun (Phase 27) | Point light (Phase 30) |
|---|---|---|
| What it has | a direction only | a **position** |
| Distance to a surface | undefined - it is infinitely far away | a real number |
| Falloff | none at all (L8 s19) | `1/(a0 + a1·d + a2·d²)` (L8 s21) |
| On a flat floor | identical brightness everywhere | a pool, brightest underneath it |
| Becomes | the sun | the muzzle flash (Phase 90) |

Having one of each makes the difference something you can **see** rather than
something you have to be told.

## The attenuation law, term by term

```glsl
float attenuation = 1.0 / (uAttenuation.x
                         + uAttenuation.y * d
                         + uAttenuation.z * d * d);
```

```cpp
const float ATTENUATION_CONSTANT  = 1.000f;   // a0
const float ATTENUATION_LINEAR    = 0.090f;   // a1
const float ATTENUATION_QUADRATIC = 0.032f;   // a2
```

Each term does a different job:

- **`a0 = 1.0`, the constant term.** It stops the division exploding as `d`
  approaches zero. Without it, a surface touching the light would be divided by
  nearly nothing and come out infinitely bright. It also sets the brightness right
  at the light.
- **`a1 = 0.09`, the linear term.**
- **`a2 = 0.032`, the quadratic term.** Real light falls off as `1/d²` because it
  spreads over the surface of an expanding sphere. This is the term that models
  that, and it dominates at long range.

So why have `a0` and `a1` at all, if physics says `1/d²`? Because a purely quadratic
falloff looks harsher than real light does - partly for the same reason there has to
be a global ambient term at all. Real rooms are full of light that has bounced off
other surfaces, and a **local** illumination model cannot compute any of it
(L8 s10-11).

Over this scene's scale:

```text
attenuation at d = 0.6 is 0.9385
attenuation at d = 2.4 is 0.7141
so the light is 1.31x brighter close up, purely from distance
```

## One function, called once per light

This is the structural change that matters, and it is what makes a second light
cheap to add:

```glsl
vec3 lightContribution(vec3 N, vec3 V, vec3 L, vec3 lightColor, float attenuation)
{
    float lambert = max(dot(N, L), 0.0);
    float rv = max(dot(reflect(-L, N), V), 0.0);
    float specular = (lambert > 0.0) ? pow(rv, uShininess) : 0.0;
    return attenuation * lightColor * (uKd * lambert + uKs * specular);
}
```

```glsl
vec3 total = uGlobalAmbient * uKa;

if ((uLightMask & 1) != 0)
    total += lightContribution(N, V, -normalize(uSunDirection), uSunColor, 1.0);

if ((uLightMask & 2) != 0) {
    vec3 toLight = uPointPosition - vWorldPos;
    float d = length(toLight);
    float attenuation = 1.0 / (uAttenuation.x + uAttenuation.y * d + uAttenuation.z * d * d);
    total += lightContribution(N, V, toLight / d, uPointColor * uPointIntensity, attenuation);
}
```

Adding the second light was **three lines**, not a copy-paste of twenty. More
importantly, the two lights provably run *identical maths* - so any difference you
see between them comes from what is passed in, and not from a typo in one of two
near-identical copies.

Notice how the sun passes `attenuation = 1.0`. It is not a special case in the
function at all; it simply never falls off.

**Phase 31 moves this function into a string shared by the vertex and fragment
shaders**, which is how Gouraud and Phong end up running the same lighting code.
Structuring it this way now is what makes that possible.

### The ambient term is outside the loop

`uGlobalAmbient * uKa` is added once, before any light. It belongs to the scene, not
to a light - it is the stand-in for all the bounced light that this model cannot
compute. Putting it inside the loop would double it when both lights are on.

## The `L` key, and why isolating a light is necessary

```text
L -> BOTH   the sun and the point light, summed per fragment
L -> SUN    directional, no falloff anywhere
L -> POINT  watch it fall off with distance
```

Once two lights have been summed into one number per pixel, the eye is very bad at
separating them again - and very good at spotting what changed when one is removed.
That is the whole value of this key.

Measured over the full scene:

```text
sun only   : mean 50.62
point only : mean 26.88
both       : mean 69.68
```

And the summing was checked directly, pixel by pixel: across 430,058 unclipped
pixels, `both` **never exceeds `sun + point` by more than 1.00 level**. The lights
add, and nothing else happens. (It does not exactly equal the sum, because each
isolated image also contains the shared ambient term, so adding them counts the
ambient twice.)

## Where the point light sits, and why it sits still

```cpp
const glm::vec3 POINT_POSITION(0.0f, -0.95f, 0.60f);
const glm::vec3 POINT_COLOR(1.00f, 0.78f, 0.45f);
const float POINT_INTENSITY = 3.2f;
```

In the finished project this light becomes the **muzzle flash**: it will sit at the
cannon's muzzle and be alive for about 0.15 seconds after firing (Phase 90). Here it
sits still, just above the floor, so that its falloff can be studied in a single
frame rather than in a fifteenth of a second.

It is warm orange because it becomes a flame. That also helps tell the three
contributions apart: the ambient is faintly blue, the sun is faintly warm white, and
this is strongly orange, so the `L` key's comparison is easy to read.

`POINT_INTENSITY` is greater than 1 deliberately. A light that falls off has to
start strong to be worth anything at a distance.

## Likely teacher questions

### What is the difference between your two lights?

The sun is directional: it has a direction and no position, so there is no distance
and no falloff (L8 s19). The point light has a position, so it has a distance to
every surface and its light falls off as `1/(a0 + a1·d + a2·d²)` (L8 s21).

### Show me the falloff.

Press `L` to the point light alone and look at the floor. Measured, its brightest
floor pixel is **10.1 times** its average - a pool. Under the sun alone the floor's
brightest pixel and its average are the **same number**, because nothing falls off.

### What are the three attenuation constants for?

`a0` stops the division exploding when the distance approaches zero - without it a
surface touching the light would be infinitely bright. `a2` is the physical
inverse-square term, because light spreads over an expanding sphere. `a1` is linear.
The constant and linear terms are there because a purely quadratic falloff looks
harsher than real light, which is full of bounced light this local model cannot
compute.

### How do you know your attenuation is actually that formula?

The floor's pixels were compared against an independent re-implementation of the
model, evaluated at each pixel's real world position. Mean error: **0.25 of 255
levels** over 60,884 pixels. Predicting with a pure `1/d²` law instead gives 20.49 -
eighty times worse - so the test can tell the two apart.

### Why is the lighting in a function?

So that both lights run exactly the same maths. Adding the second light was three
lines, and no difference between the two lights can come from a typo in one of two
copies. Phase 31 moves that same function into a string shared by the vertex and
fragment shaders, which is how Gouraud and Phong end up identical apart from where
they run.

### Why does the sun pass `attenuation = 1.0`?

Because it has no position to fall off from. It is not a special case in the
function - it just never attenuates.

### Why is the ambient term outside the per-light code?

Because it belongs to the scene, not to a light. Inside the loop it would be counted
twice when both lights are on.

### What does `L` prove?

That there are two separate light sources being summed per fragment. Measured across
430,058 pixels, `both` never exceeds `sun + point` - the contributions add.

### Why only two lights?

Two is enough to show that contributions sum, which is the lecture's point, and few
enough to keep the shader readable. One directional and one point means each kind is
represented, so the difference between them is visible rather than described.

## Simple viva modifications

- **The named exercise: change the attenuation constants.** `ATTENUATION_LINEAR` and
  `ATTENUATION_QUADRATIC` in [src/main.cpp](../src/main.cpp). Set both to 0 and the
  light stops falling off entirely - it becomes a positioned light with no distance
  behaviour. Set `ATTENUATION_QUADRATIC` to 0.5 and the pool shrinks hard.
- **Delete `a0`:** set `ATTENUATION_CONSTANT` to 0 and move the light close to the
  floor. The division explodes and the pool blows out to white.
- **Move the light:** change `POINT_POSITION` and watch the pool follow it. Put it at
  `(0, 0.5, 0)` and it lights the cubes from below instead.
- **Change its colour or intensity:** `POINT_COLOR`, `POINT_INTENSITY`.
- **Prove each light separately:** press `L` three times and compare.
- **Break the ambient placement:** move `uGlobalAmbient * uKa` inside
  `lightContribution` and watch the scene get brighter when both lights are on, for
  the wrong reason.

## Checkpoint

Phase 30 passes when:

- Debug and Release build with no compiler warnings;
- both shaders compile and link with no missing-uniform warning;
- a second light exists, with a **position**, and its contribution falls off as
  `1/(a0 + a1·d + a2·d²)`;
- moving the point light near an object visibly brightens it - measured, its
  brightest floor pixel is 10.1x its average, while the sun's floor is perfectly
  uniform;
- the floor matches an independent re-implementation of the model to within 0.26 of
  255 levels, under each light alone and under both;
- a pure `1/d²` law demonstrably does **not** fit, so the above is real evidence;
- `L` cycles both / sun only / point only, and the two contributions **add** -
  measured, `both` never exceeds `sun + point`;
- the per-light maths lives in one function called once per light;
- every earlier object and key still behaves as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 30

No third light, ever. No shadows - a local model cannot compute them, which is the
same reason ambient exists. No flicker or timing on the point light: it becomes the
muzzle flash in Phase 90, where it gets a 0.15 second life tied to firing.

No shading-mode comparison yet. Phase 31 is the **graded milestone** of Stage C:
`uniform int uShadingMode` with one branch in **one** program, giving Flat, Gouraud
and Phong on keys `1`/`2`/`3`, with `computeLighting` living in a single C++ string
prepended to both shader stages.

---

## Note: verification

### 15 checks, no failures

The floor was isolated and rendered under each light separately, then every floor
pixel was unprojected back to its world position by intersecting the camera ray with
the grid's plane, and the **entire illumination model** was re-implemented
independently and evaluated there. Mean error 0.25 levels out of 255.

That approach verifies the attenuation law, the sun's lack of falloff, the material
values, the specular term and the summing, all in one measurement.

### Three faults in my own testing, all corrected

This phase's verification was wrong three times before it was right, and each wrong
version produced a confident-looking number.

1. **Fitting only the attenuation.** The first test divided the measured brightness
   by the predicted attenuation and Lambert term and checked the result was
   constant. It was not - it varied by 86%. The reason was not a broken attenuation
   law: the floor's material is `OCEAN`, whose `k_s` is 0.95 at `n_s` 160, so much of
   its brightness is **specular**, which that fit never accounted for. The measured
   luminance went *down* and then back *up* with distance, which looked like a bug
   and was really a sun glitter.

2. **Assuming "even brightness" means "no falloff".** The test asserted the sun
   would light the floor evenly and it appeared not to - again because of the
   ocean's specular streak, not because of any distance term. Both of these were
   fixed by predicting the whole model instead of one term of it.

3. **A patch block placed after the file was written.** The "draw only the grid"
   patches were inserted into the harness script *below* the line that writes the
   patched source to disk, so they modified a string that had already been saved. The
   harness silently ignored `-onlygrid`, and `grid_m1.ppm` and `full_m1.ppm` came out
   **byte-identical** - which is what gave it away. Until then the test was
   predicting floor lighting for pixels belonging to spheres and cubes, and
   reporting a 14-level error.

The third one is the instructive one. A per-pixel sample dump showed measured
`8, 18, 26` against predicted `8.1, 18.4, 25.7` - a perfect match - while the overall
mean error was 14.5. Those two facts together could only mean the test was measuring
the wrong pixels, and comparing the file hashes confirmed it in seconds.

### Earlier phases still correct

14 draws, 1840 triangles, 1045 vertices - unchanged from Phase 29, as expected. The
`N` view, the `M` toggle, the `+`/`-` detail levels, the three material spheres and
every Stage B key behave as before.
