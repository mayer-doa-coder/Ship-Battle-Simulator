# Phase 31 - Flat, Gouraud and Phong in One Program

## Status

Phase 31 is complete and verified. It is a **graded milestone** (`*` in the plan)
and the phase the L9 part of the report is built on. Debug and Release build with no
warnings, and 12 checks passed with zero failures.

```text
Ship Battle Simulator - Phase 31 | draws 14 | tris 1840 | verts 1045
```

Counts unchanged. Three shading models, no extra geometry and **no extra shader
program**.

### THE CHECKPOINT: keys 1/2/3 give three different pictures of identical geometry

```text
mode    | lit pixels | mean lum | max lum | distinct colours
Flat    |     451419 |    84.73 |  248.62 |             2025
Gouraud |     451419 |    85.08 |  252.49 |            13486
Phong   |     451419 |    84.27 |  255.00 |             9510
```

**The same 451,419 pixels in all three modes** - identical geometry, as it must be.
And they differ substantially:

```text
Flat vs Gouraud  : 167,390 pixels differ  (37.1% of the scene)
Flat vs Phong    : 130,493 pixels differ  (28.9%)
Gouraud vs Phong : 109,014 pixels differ  (24.1%)
```

## What changed

| File | Change |
|---|---|
| `src/Lighting.h` | **New.** The whole illumination model as GLSL in one C++ string, plus the `ShadingMode` constants |
| `src/Shader.h` | `loadFromFiles()` takes a `commonSource` spliced into both stages; new `injectAfterVersion()` |
| `shaders/basic.vert` | Declares `out vec3 vLitColor` and computes the lighting there in Gouraud mode |
| `shaders/basic.frag` | Recovers the facet normal with `dFdx`/`dFdy`; one branch choosing the model |
| `src/main.cpp` | `SceneState::shadingMode`; keys `1`/`2`/`3`; `uShadingMode`; passes the shared source |

## The one idea

**All three models run the same lighting code. They differ only in *what normal*
they use and *where* the code runs.**

| | What is interpolated | Where lighting runs | Result |
|---|---|---|---|
| **Flat** | nothing relevant - the facet normal is constant | per fragment | visible faceting, Mach bands (L9 s27) |
| **Gouraud** | the **colour** | per **vertex** | cheap; misses highlights between vertices (L9 s28) |
| **Phong** | the **normal** | per **fragment** | correct; most expensive (L9 s36) |

The thing to hold onto is the middle column. Gouraud interpolates a *colour*;
Phong interpolates a *normal*. Everything else follows from that one difference.

## Why the lighting lives in a C++ string

This is the structural decision of the phase, and it exists to make the comparison
**honest**.

```cpp
// src/Lighting.h
inline const std::string& sharedLightingSource()
{
    static const std::string source = R"GLSL(
        uniform int uShadingMode;
        ...
        vec3 computeLighting(vec3 N, vec3 worldPos) { ... }
    )GLSL";
    return source;
}
```

Gouraud evaluates the lighting per vertex; Phong per fragment. **The entire value
of comparing them is that this is the only difference.** If the vertex shader and
the fragment shader each held their own copy of the formula, that claim would be
unverifiable - a typo in one, or an improvement made to one and forgotten in the
other, would quietly make the comparison a lie. And this comparison is the single
biggest piece of evidence in the report.

So the text exists once and is spliced into both stages:

```cpp
shader.loadFromFiles(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH, sharedLightingSource());
```

`R"GLSL( ... )GLSL"` is a C++11 raw string, so the GLSL keeps its own line breaks
and needs no `\n` escapes - without it this would be an unreadable wall of
quotation marks.

### Why it is spliced *after* `#version`

```cpp
const std::string::size_type versionAt = source.find("#version");
const std::string::size_type lineEnd = source.find('\n', versionAt);
source.insert(lineEnd + 1, common);
```

GLSL requires `#version` to be the very first thing in a shader - before even a
comment. So the shared block cannot simply be stuck on the front. Finding the end
of that first line and splicing after it keeps each file valid on its own, and keeps
compiler error line numbers pointing somewhere sensible.

## Why one program and not three

The plan is emphatic about this, and there are three separate reasons:

1. **One copy of the lighting code.** Three programs would mean three files to keep
   in step. See above - that is the whole point.
2. **Switching mode costs one integer.** With a single program, changing mode is
   `shader.setInt("uShadingMode", mode)`. With three programs it is a
   `glUseProgram` every time, which is a pipeline state change - and the project
   would then have to give up the "no state-change cost on mode switch" argument in
   the optimization section at Phase 103.
3. **It is a smaller thing to explain.** One uniform, one branch.

The cost is a branch in the shader. Every fragment evaluates `if (uShadingMode == 0)`
even though the answer is the same for the whole frame. On any modern GPU that is a
*uniform* branch - every thread in a warp takes the same path - so it costs
essentially nothing. Divergent branches are the expensive kind, and this is not one.

## How Flat gets its normal

The mesh does not store facet normals anywhere. A flat-shaded triangle needs one,
so it is recovered in the fragment shader:

```glsl
vec3 faceNormal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
```

`dFdx` and `dFdy` are screen-space derivatives: they report how a value changes
between this pixel and the one beside it, horizontally and vertically. The GPU can
answer that because it shades pixels in 2x2 blocks and can compare neighbours.

Applied to the world position they give two vectors lying **in** the surface, and
the cross product of two vectors in a plane is perpendicular to that plane. So this
is the triangle's true geometric normal, constant across the triangle because the
surface is planar between its three corners.

```glsl
if (dot(faceNormal, vNormal) < 0.0)
    faceNormal = -faceNormal;
```

The cross product's **sign** depends on screen-space handedness, so it can come out
pointing into the surface rather than out of it. Comparing against the interpolated
normal - which is known to point outward - and flipping when they disagree fixes
that without needing to care about winding order or which way the derivatives
happened to run.

### A nuance worth being honest about

In the strict lecture sense, flat shading evaluates the lighting **once per
polygon**. This implementation keeps the *normal* constant per facet but still works
out the view direction and the point light's distance per fragment. So a facet
shades slightly smoothly even though its normal does not change - measured, the
low-detail ball shows 214 distinct colours in Flat rather than one per triangle.

The faceting is in the **normals**, and that is where it was measured:

```text
the ball in the N debug view
  Flat    :    99 distinct normal colours   <- a patchwork of facets
  Gouraud : 19312
  Phong   : 19312
```

Pressing `N` in Flat mode is a good demonstration in its own right: the sphere stops
being a smooth colour sweep and becomes visibly a polyhedron.

## Why Gouraud misses a highlight

This is L9 slide 28, and the project now demonstrates it with a number.

The black plastic ball at its lowest detail - 3 stacks by 4 slices, 12 triangles -
is the sharpest case in the scene, because black plastic reflects essentially **no**
diffuse light (`k_d` = 0.01), so almost all its brightness is specular:

```text
the ball at 3 x 4
  Flat    : max luminance  92.26
  Gouraud : max luminance  25.96   <- the highlight is simply absent
  Phong   : max luminance 243.49   (9.4x Gouraud)
```

**Gouraud leaves it nearly black.** The reason is exactly the one the slide gives:
Gouraud computes the lighting at each vertex and then interpolates the **colour**
between them. If the highlight falls in the middle of a triangle, there is no vertex
there to compute it at - and no amount of blending can invent a bright spot that was
never calculated.

### But Gouraud is not simply "worse", and that is a sharper point

The brass tube at 4 segments tells the other half of the story:

```text
the brass tube at 4 segments
  Flat    : max luminance 231.06
  Gouraud : max luminance 230.95
  Phong   : max luminance 233.23
```

All three nearly agree. Brass's `n_s` is 27.9, so its highlight is **broad** - broad
enough to cover a vertex. And a highlight Gouraud can see at a vertex is a highlight
it can interpolate perfectly well.

So the real claim is not "Gouraud is worse". It is:

> **Gouraud fails on TIGHT highlights on COARSE geometry.**

Both conditions are needed. That is why Phase 33's Demo A uses the ocean at
`n_s = 160` on a low-tessellation grid - a tight highlight on big triangles - and
why it would not work on sailcloth at `n_s = 4`.

The same effect in reverse: at the scene's shipped detail, Gouraud and Phong differ
over only 24% of the scene. Refine the geometry and Gouraud converges on the right
answer.

## Why Phong is the slowest

L9 slide 36. Phong re-evaluates the whole illumination model for **every pixel**,
where Gouraud evaluates it once per **vertex**. On the ball at the shipped detail
that is 200 vertices against tens of thousands of pixels.

In this project the saving is visible in the shader itself: the vertex shader's
Gouraud branch means the other two modes do not pay for work they will not use, and
the fragment shader's Gouraud branch is a single assignment rather than a call to
`computeLighting`.

## Where each job happens

| Place | Job |
|---|---|
| `src/Lighting.h` | The only copy of the illumination model, as GLSL text |
| `ShaderProgram::injectAfterVersion()` | Splices it into both stages after `#version` |
| `basic.vert` | Gouraud: calls `computeLighting` per vertex into `vLitColor` |
| `basic.frag` | Recovers the facet normal; branches on the mode |
| `processInput()` | Keys `1`, `2`, `3` |
| `renderScene()` | Sends `uShadingMode` once per frame |

## Likely teacher questions

### Why not three shader programs?

Three reasons. One copy of the lighting code, so the comparison between Gouraud and
Phong cannot be a lie. Switching mode costs one integer upload instead of a
`glUseProgram` pipeline state change - which is what lets the optimization section
claim no state-change cost on mode switch. And one uniform with one branch is a
smaller thing to explain than three programs.

### Doesn't the branch cost you?

Barely. `uShadingMode` is a uniform, so every thread in a warp takes the same path -
it is a uniform branch, not a divergent one, and uniform branches are nearly free.
Divergent branching is the expensive kind.

### How do you know Gouraud and Phong really use the same lighting?

Because they share the same text. It lives once in `src/Lighting.h` and is spliced
into both shaders at load time. There is no second copy to drift.

### Why splice after `#version` rather than at the top?

GLSL requires `#version` to be the first thing in the file, before even a comment.

### What is the difference between the three modes, in one sentence each?

Flat uses one normal for the whole triangle, so you see the facets. Gouraud computes
the lighting per vertex and interpolates the **colour**. Phong interpolates the
**normal** and computes the lighting per pixel.

### Where does Flat get its normal from?

`cross(dFdx(vWorldPos), dFdy(vWorldPos))` - screen-space derivatives of the world
position give two vectors lying in the surface, and their cross product is
perpendicular to it. The sign is flipped if it disagrees with the interpolated
normal, because the cross product's handedness depends on screen space.

### Show me Gouraud missing a highlight.

Press `-` three times to drop to the lowest detail, then look at the black plastic
ball and press `2` then `3`. Measured: Gouraud's peak is 25.96 and Phong's is 243.49
on the same geometry - **9.4 times** brighter.

### So Gouraud is just worse?

No, and this is the more interesting answer. On the brass tube at 4 segments all
three modes agree to within 2 luminance levels, because brass's `n_s` is 27.9 and its
highlight is broad enough to cover a vertex. Gouraud fails on **tight** highlights on
**coarse** geometry - it needs both.

### Why is Phong slower?

It re-evaluates the illumination model per fragment instead of per vertex (L9 s36).
On the ball that is 200 evaluations against tens of thousands.

## Simple viva modifications

- **The named exercise: explain why three programs were not used.** Covered above.
- **See the difference at its biggest:** press `-` three times, then cycle `1`, `2`,
  `3` on the black plastic ball.
- **See it disappear:** press `+` four times and cycle again. The three modes
  converge as the triangles get smaller.
- **See Flat's normals:** press `1` then `N`. The sphere becomes a polyhedron.
- **Prove the sharing:** change `uKd * lambert` to `uKd * lambert * 0.5` in
  `src/Lighting.h` and notice that **both** Gouraud and Phong dim. One edit, two
  shaders.
- **Break the facet-normal sign flip:** delete the `if (dot(...) < 0.0)` line and
  watch half the facets go black in Flat mode.

## Checkpoint

Phase 31 passes when:

- Debug and Release build with no compiler warnings;
- there is exactly **one** shader program, and the console reports one linked
  program;
- `uniform int uShadingMode` with **one branch** selects Flat, Gouraud or Phong;
- keys `1`, `2` and `3` give three visibly different images of identical geometry -
  measured, the same 451,419 pixels covered in all three modes, differing over 24%
  to 37% of the scene;
- the illumination model lives in **one** C++ string prepended to both stages;
- Flat is visibly faceted - measured, 99 distinct normal colours against 19,312;
- Gouraud misses a tight highlight on coarse geometry - measured, peak 25.96 against
  Phong's 243.49 on the same ball;
- the `N` debug view, the `M` toggle, the `L` cycle, the `+`/`-` detail levels and
  every earlier key still behave as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 31

No `K` term mask and no Blinn-Phong toggle - those are Phase 32. The two formal
demonstrations are **not** set up yet: Phase 33 is the verification phase that places
and photographs Demo A (the grid at `n_s` 160, low tessellation) and Demo B (the
barrel at 6-8 segments).

No FPS comparison between the modes. That measurement belongs to Phase 103, which is
where the optimization argument is made with numbers.

Phase 32 adds the **`K` term mask** - ambient / +diffuse / +specular / full, L8
slide 54 walked through live - and the **`B` key** for Blinn-Phong `(N·H)^n`.

---

## Note: verification

### 12 checks, no failures

Twelve images were rendered - three modes x four subjects - and compared: the whole
scene, the ball alone at the lowest detail, the tube alone at its lowest segment
count, and the ball in the normals view.

### Four wrong expectations of mine, all corrected

Every one was in the *test*, and each had produced a confident-looking failure
against working code.

1. **"Flat shows one colour per triangle."** It shows 214 on the low-detail ball.
   That is correct behaviour, not a bug: only the *normal* is constant per facet
   here, while the view direction and the point light's distance are still per
   fragment. The faceting had to be measured in the normals view instead, where it
   is stark - 99 against 19,312.

2. **"Gouraud shows more distinct colours than Flat."** It shows *fewer* - 145
   against 214 - on the low-detail ball, because it misses the highlight entirely
   and the whole ball stays dark. The failing assertion was pointing at the very
   effect the phase is about.

3. **"The brass tube at 4 segments will show Gouraud failing."** It does not; all
   three modes agree to within 2 levels. Brass's `n_s` of 27.9 gives a highlight
   broad enough to cover a vertex. This turned out to be the most useful thing the
   test found, because it sharpens the claim from "Gouraud is worse" to "Gouraud
   fails on tight highlights on coarse geometry" - and that is the reason Phase 33's
   Demo A specifies `n_s = 160` rather than any shiny material.

4. **A leftover patch line** in the harness script referenced a guard that no longer
   existed, so the script aborted before rendering anything.

### Earlier phases still correct

14 draws, 1840 triangles, 1045 vertices - unchanged from Phase 30. The `N` view, `M`,
`L`, `+`/`-`, `W`, `D` and `O` all behave as before, and the three material spheres
still read as brass, silver and black plastic.
