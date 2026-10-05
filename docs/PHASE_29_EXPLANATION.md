# Phase 29 - Named Materials from L8 Slide 60

## Status

Phase 29 is complete and verified. Debug and Release build with no warnings, and
17 image checks passed with zero failures.

```text
Ship Battle Simulator - Phase 29 | draws 14 | tris 1840 | verts 1045
```

Three more draws than Phase 28 - the three comparison spheres - and **no new mesh
on the graphics card**, because they are the same sphere drawn three times.

### THE CHECKPOINT: three spheres, obviously different materials

This is the screenshot the report needs. Measured on each ball alone:

| material | `n_s` | mean RGB | mean lum | max lum | highlight size |
|---|---:|---|---:|---:|---:|
| brass | 27.9 | 135.7, 96.6, 20.5 | 99.6 | 250 | 1.45% of the ball |
| polished silver | 89.6 | 46.7, 45.2, 44.7 | 45.5 | 254 | **0.09%** |
| black plastic | 32.0 | 3.1, 3.0, 2.8 | **3.0** | **125** | 0.10% |

Three results worth reading twice:

1. **The closest pair of mean colours is 73.8 apart** out of a possible 441. Not a
   subtle difference - obviously different materials.
2. **Silver's highlight is 16 times tighter than brass's**, 0.09% against 1.45% of
   the ball. Silver's `n_s` is 89.6 and brass's is 27.9. The exponent does exactly
   what it claims.
3. **Black plastic has a mean luminance of 3 and a maximum of 125** - a factor of
   42. It is genuinely black and genuinely shiny at the same time.

That third row is the whole argument for materials existing, and the next section
explains why.

## What changed

| File | Change |
|---|---|
| `src/Material.h` | **New.** `struct Material` and six named materials |
| `shaders/basic.frag` | `uKa`/`uKd`/`uKs` are now the appearance; `vColor` and `uTint` are gone from the lit path |
| `shaders/basic.vert` | A comment saying `vColor` is now unread |
| `src/main.cpp` | `drawMesh()` takes a `const Material&`; every draw names one; `SurfaceConfig` deleted; `MaterialDemoConfig` and three draws added |

## The one idea

**A surface needs four numbers, not one colour.**

```cpp
struct Material {
    glm::vec3 ka;   // how much ambient light it reflects
    glm::vec3 kd;   // how much diffuse light it reflects  - its apparent colour
    glm::vec3 ks;   // how much specular light it reflects  - its shine
    float ns;       // the shininess exponent               - how TIGHT that shine is
};
```

Look at black plastic:

```cpp
const Material BLACK_PLASTIC = {
    { 0.000f, 0.000f, 0.000f },   // ka - reflects NO ambient light at all
    { 0.010f, 0.010f, 0.010f },   // kd - essentially no colour: it is black
    { 0.500f, 0.500f, 0.500f },   // ks - and yet half the specular light
    32.0f,
    "black plastic"
};
```

**Colour and shine are independent properties.** A single "colour" value cannot
express "black but polished" - if you made it dark it would be matt, and if you
made it bright it would not be black. Four numbers can, and the measurement above
proves it: mean luminance 3, peak luminance 125.

That is why L8 slide 60 is a table of four columns and not a list of colours.

## The numbers are verbatim, and that is the point

```
                     k_a                       k_d                      k_s                 n_s
Brass            (0.329,0.224,0.027)   (0.780,0.569,0.114)   (0.992,0.941,0.808)   27.9
Polished silver  (0.231,0.231,0.231)   (0.278,0.278,0.278)   (0.774,0.774,0.774)   89.6
Black plastic    (0,0,0)               (0.010,0.010,0.010)   (0.500,0.500,0.500)   32
```

These three are **copied from the lecture slide and must not be "improved"**. Their
value as report evidence comes precisely from not being tuned until they looked
nice. If a grader asks where brass's `27.9` came from, the answer is "slide 60",
not "it looked about right".

Three more are tuned for this project's own surfaces, and are labelled as such in
[src/Material.h](../src/Material.h): `OCEAN`, `HULL_WOOD`, `SAILCLOTH`.

### The 40x shininess spread is deliberate

```text
sailcloth   n_s =   4     cloth scatters light everywhere - matt
hull wood   n_s =   8     a broad weak sheen
brass       n_s =  27.9
black plastic n_s = 32
polished silver n_s = 89.6
ocean       n_s = 160     a sun streak glitters on water
```

**Four to 160 is a forty-fold range, visible in a single frame.** That spread
reproduces L8 slide 46 with real objects instead of test spheres, and narrowing it
to make the scene look nicer would throw the demonstration away. It is the reason
`SAILCLOTH` and `OCEAN` are in the table even though the sails and the sea do not
exist yet.

## Reading the metals against each other

Brass and polished silver are both metals, and the table says something specific
about how they differ.

| | brass | polished silver |
|---|---|---|
| `k_d` | `(0.780, 0.569, 0.114)` - strongly coloured | `(0.278, 0.278, 0.278)` - grey |
| `k_s` | `(0.992, 0.941, 0.808)` - nearly white | `(0.774, 0.774, 0.774)` - grey |
| `n_s` | 27.9 | 89.6 |

Measured: brass's mean red minus mean blue is **+115.2**; silver's is **+2.0**.

So brass tints the light it *absorbs* heavily but barely tints the light it
*bounces straight off* - which is what a coloured metal does. Silver has almost no
colour of its own in any column; it simply reflects, and tightly.

## What the per-vertex colour was, and why it is gone

From Phase 2 until Phase 28, an object's appearance came from the colour stored at
each vertex. That was always a stand-in - something to make the cube's six faces
distinguishable before materials existed. From this phase the lit path does not
read it at all:

```glsl
vec3 ambient = uGlobalAmbient * uKa;
vec3 diffuse = uSunColor * uKd * lambert;
vec3 spec    = uSunColor * uKs * specular;
```

No `vColor`, no `uTint`. The material *is* the appearance, which is what L8's model
actually says.

**The `color` field in `Vertex` was deliberately left in place**, though nothing
lit reads it. The phase plan expected it to disappear here, and it has not, for a
concrete reason: removing it means changing `Vertex`, every one of the five
generators, and every vertex count in the project - which would invalidate the
byte-for-byte frame evidence recorded in Phases 14 and 17. The cost of removing it
is high and the benefit is tidiness. It is unread, documented as unread, and the
counts stay comparable with every earlier phase.

`uTint` is gone entirely, and the two test triangles that used it to tell
themselves apart - Phase 8's depth-test evidence - now use **different materials**
instead: brass for the near copy, sailcloth for the far one. Still obviously
different, and now for a reason that is part of the lighting model rather than a
filter bolted on top.

## A material belongs to an object

```cpp
static void drawMesh(ShaderProgram& shader, RenderStats& stats, const Mesh& mesh,
                     const glm::mat4& model, const Material& material)
{
    shader.setVec3("uKa", material.ka);
    shader.setVec3("uKd", material.kd);
    shader.setVec3("uKs", material.ks);
    shader.setFloat("uShininess", material.ns);
    ...
```

`drawMesh()` is the one place that knows which object is being drawn, so it is the
only place that can know its material. Phase 27 and 28 set these four once per
frame because there was one surface for the whole scene; now they move inside, next
to the model matrix, because a material belongs to an object in exactly the same
way.

What stays per frame is only what belongs to the whole frame: the sun, the global
ambient and the camera position. `SurfaceConfig` was deleted rather than left
sitting unused.

## What each object became

Several assignments are previews of the finished project, which is deliberate:

| Object | Material | Why |
|---|---|---|
| the grid floor | `OCEAN` | it becomes the sea in Phase 40 |
| the ball in the left column | `BLACK_PLASTIC` | it becomes the cannonball |
| the tube | `BRASS` | it becomes the cannon barrel |
| the three cubes | `HULL_WOOD` | they become the hull in Stage D |
| the quad | `SAILCLOTH` | a flat sheet of cloth |
| both smooth cubes | `POLISHED_SILVER` | the cannon's fittings |
| near test triangle | `BRASS` | replaces Phase 3's tint |
| far test triangle | `SAILCLOTH` | replaces Phase 8's second tint |

## The three spheres cost no extra memory

```cpp
for (int i = 0; i < MaterialDemoConfig::COUNT; ++i) {
    drawMesh(shader, stats, sphereMesh, frame * demoScale, *demoMaterials[i]);
}
```

One mesh, three draws, three materials. The counters went from 11 draws to 14 and
the vertices on the graphics card did not change at all. This is the Phase 16
mesh-reuse rule paying for itself somewhere it is easy to see - and it means the
comparison is genuinely fair, because the three balls are *identical geometry*.
Measured: the two outer silhouettes are **pixel-identical** in area.

## Likely teacher questions

### Why does a surface need four numbers instead of a colour?

Because colour and shine are independent. Black plastic has `k_d` of 0.01 - it is
genuinely black - and `k_s` of 0.50 - it is genuinely shiny. Measured mean
luminance 3, peak 125. No single colour value can say that.

### Where did these numbers come from?

The first three are copied verbatim from L8 slide 60. That is what makes them
evidence rather than something tuned until it looked nice. The other three -
ocean, hull wood, sailcloth - are tuned for this project and are labelled as such.

### Why is the shininess range so wide?

On purpose. Sailcloth is 4 and the ocean is 160, a forty-fold spread visible in one
frame, which reproduces L8 slide 46 with real objects. Narrowing it to make the
scene prettier would throw away the demonstration.

### Show me that the exponent does something.

Silver's `n_s` is 89.6 and brass's is 27.9. Measured, silver's highlight covers
0.09% of its ball and brass's covers 1.45% - **sixteen times tighter**, on
identical geometry under identical light.

### How are brass and silver different, in the table?

Brass's `k_d` is strongly coloured and its `k_s` is nearly white: it tints what it
absorbs and barely tints what it bounces off. Silver is grey in every column.
Measured, brass's red-minus-blue is +115; silver's is +2.

### What happened to the per-vertex colours?

Nothing reads them any more. They were the stand-in for a material from Phase 2 to
Phase 28. The field is still in the `Vertex` struct because removing it would mean
rebuilding every generator and would invalidate the byte-identical frame evidence
from Phases 14 and 17 - but the lit path ignores it.

### Then how do the two test triangles stay distinguishable for the depth-test demo?

They have different materials now - brass and sailcloth - instead of different
tints. Still obviously different, and for a reason that is part of the lighting
model rather than a filter on top of it.

### Why is the material set in `drawMesh` rather than once per frame?

Because a material belongs to an object, exactly like its model matrix, and
`drawMesh` is the only place that knows which object is being drawn. The sun and
the global ambient stay per frame, because they belong to the whole frame.

### Three spheres - did that cost three times the memory?

No. It is one sphere mesh drawn three times. The draw count went from 11 to 14 and
the graphics card holds exactly the same vertices as before.

## Simple viva modifications

- **The named exercise: swap one object's material.** Change `OCEAN` to `SAILCLOTH`
  on the grid in [src/main.cpp](../src/main.cpp) and the floor goes from dark and
  glittering to pale and matt.
- **Reorder the demo spheres:** edit `demoMaterials` to put `OCEAN`, `SAILCLOTH`
  and `HULL_WOOD` side by side instead - that shows the 4-to-160 shininess spread
  directly.
- **Prove `n_s` is independent of brightness:** copy `BRASS` and change only `ns` to
  160. The colour is identical; the highlight collapses to a point.
- **Prove the black-and-shiny point:** set `BLACK_PLASTIC`'s `ks` to `(0,0,0)`. The
  ball becomes an invisible black disc, which is what a single dark colour would
  have given you.
- **Break the verbatim rule on purpose** and then put it back, to see why a tuned
  number is worth less than a cited one.

## Checkpoint

Phase 29 passes when:

- Debug and Release build with no compiler warnings;
- both shaders compile and link with no missing-uniform warning;
- `src/Material.h` holds the L8 slide 60 table, verbatim for the three named
  materials, with the tuned ones labelled;
- **three spheres sit side by side and are obviously different materials** -
  measured, the closest pair of mean colours is 73.8 apart out of 441;
- a higher `n_s` gives a tighter highlight - measured, silver at 89.6 is 16x
  tighter than brass at 27.9;
- black plastic is both nearly black and clearly shiny - measured, mean 3 and peak
  125;
- the three spheres are the same mesh, so the graphics card holds no extra
  geometry, and the two outer silhouettes are pixel-identical;
- the material is set per object in `drawMesh()`, and the lights stay per frame;
- every earlier object and key still behaves as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 29

No second light - the point light with attenuation and the `L` key are Phase 30. No
shading-mode comparison: Phase 31. No `K` term mask or Blinn-Phong toggle:
Phase 32. The sails and the sea do not exist yet, so `SAILCLOTH` and `OCEAN` are in
the table ahead of the objects that will use them, standing in on the quad and the
grid in the meantime.

Phase 30 adds the **second light**: a point light with attenuation
`1/(a0 + a1·d + a2·d²)`, and the `L` key to isolate each light's contribution.

---

## Note: verification

### 17 image checks, no failures

Each sphere was rendered **alone**, by driving the real draw loop with an index
filter, and measured for area, mean colour, peak luminance and highlight size.

### One real placement error, found and fixed

The demo spheres were first placed high up, at `y = 1.35`. At the default camera
that shows their **undersides**, which face away from both the sun and the mirror
direction - and the black plastic ball rendered **pure black, maximum luminance 0**:

```text
camera pitch | black plastic ball
          0  | area  3090   max   0
          5  | area  7293   max   1
         15  | area 23036   max   4
         20  | area 34485   max 125
```

Only from 20 degrees above did the highlight appear at all. Since specular is the
*only* light black plastic reflects, a view with no highlight renders it as an
invisible disc - a material demonstration showing nothing. The silhouette was also
being clipped by the top of the screen, which is why the area swings by a factor of
eleven in that table.

Moved to `y = 0.00, z = 1.90`, where the sun-plus-viewer half vector lands on the
face you are looking at. The reason is recorded in `MaterialDemoConfig` in the
source so it cannot be quietly undone.

This is worth noting as a general point: the first three phases of Stage C could all
be checked from any angle, because ambient and diffuse do not care where you stand.
From Phase 28 onward **where the camera is becomes part of whether a demonstration
works**, and placing a demo object is no longer a cosmetic decision.

### One threshold of mine that was too tight

I asserted the three silhouettes would be within 5% of each other. They are within
9%: the two outer balls are pixel-identical to each other, as mirror images must be,
and the middle one is smaller. That is **perspective, not a difference in the
objects** - an off-axis sphere projects to an ellipse stretched radially, so it
covers more pixels than an on-axis one of the same size. Threshold corrected to 15%,
with a pixel-identical check on the mirror pair added as the stricter evidence.

### Earlier phases still correct

14 draws, 1840 triangles, 1045 vertices - exactly 11 + 3 draws, 652 + 3x396
triangles and 445 + 3x200 vertices. The `N` view, the `M` toggle, the `+`/`-` detail
levels and every Stage B key behave as before.
