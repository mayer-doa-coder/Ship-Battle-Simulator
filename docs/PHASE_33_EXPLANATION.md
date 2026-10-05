# Phase 33 - The Two Demonstrations, Measured

## Status

Phase 33 is complete and verified. It is a **graded milestone** (`*` in the plan) and
**Stage C is finished**.

```text
Ship Battle Simulator - Phase 33 | draws 14 | tris 1840 | verts 1045
```

**This phase adds no feature.** Not one line of rendering code changed. Its job is to
set up, measure and record the two demonstrations the L9 part of the report is built
on - and to confirm that both are reachable with the keys the program already ships
with.

10 measurement checks passed with zero failures, and the final suite - Debug, Release
and strict `/W4` - passed 32 of 32.

## Why a phase that adds nothing

Because a demonstration that only works by accident is worthless in a viva.

Both demos depend on a specific combination of material, tessellation, camera angle
and shading mode. Stage C has already shown, three separate times, that getting one of
those wrong silently produces a demonstration that proves nothing:

- Phase 26 first demonstrated the normal matrix on an object that **physically cannot
  show the bug**;
- Phase 29 first placed the material spheres where the highlight is **invisible**, so
  the black plastic ball rendered pure black;
- Phase 31's own test assumed the brass tube would show Gouraud failing, and it
  **does not**.

So this phase pins both demos down with numbers, and records exactly which keys reach
them.

---

## DEMO A - Gouraud misses the highlight (L9 slide 28)

**The object:** the sea - the grid mesh, material `OCEAN`, `k_s` = (0.90, 0.94, 0.98)
at `n_s` = **160**, the tightest highlight in the material table. The sun makes a
narrow streak across it.

**The keys:** `2` then `3`. Press `+` or `-` to change the tessellation.

### The measurement

```text
tessellation | Gouraud peak | Phong peak
CELLS      8 |          186 |        255
CELLS     16 |          153 |        255
CELLS     32 |          255 |        255
```

At 8 and 16 cells Gouraud's brightest pixel is **186 and 153** against Phong's 255.
The highlight is there in Phong and missing in Gouraud, on identical geometry. By 32
cells Gouraud finally catches it - **so the failure was coarseness, not Gouraud**.

### The part that makes this demo better than expected

The grid is **flat**. Every one of its normals is `(0,1,0)` no matter how finely it is
divided. So:

- **Phong interpolates normals.** Interpolating between identical normals gives that
  same normal back. Phong's answer therefore cannot depend on the tessellation at all.
- **Gouraud interpolates colours.** Where the vertices sit decides where the lighting
  is sampled, so its answer must depend on the tessellation.

Measured, comparing each mode's own image across the three tessellations:

```text
Phong   : CELLS  8 vs 16 ->      0 pixels differ
          CELLS  8 vs 32 ->      2 pixels differ
Gouraud : CELLS  8 vs 16 -> 17,404 pixels differ
          CELLS  8 vs 32 -> 19,410 pixels differ
```

**Phong's picture is byte-identical across a four-fold change in tessellation.
Gouraud's changes by seventeen thousand pixels.**

That is the cleanest statement of L9 slide 28 the project can make. The error is not
in the geometry - the geometry is provably identical, and Phong proves it by returning
the same image. The error is in **interpolating the wrong quantity**.

### What to say while demonstrating it

> "The sea is one flat surface, so all its normals are the same whatever the
> tessellation. Phong interpolates normals, so its picture does not change at all -
> zero pixels different between 8 cells and 16. Gouraud interpolates colours, so its
> picture changes by seventeen thousand pixels, and its highlight peaks at 186 instead
> of 255. The geometry never changed. Only where the lighting was sampled."

---

## DEMO B - highlights distort on coarse polygons (L9 slide 27)

**The object:** the brass tube - `BRASS` at `n_s` = 27.9, cut into only **8 flat
strips**.

**The keys:** `-` once to reach 8 segments, then `1`, `2`, `3`.

### The measurement

```text
mode    | lit px | mean  | max | distinct brightness levels
Flat    |  24982 | 179.0 | 231 |                        42
Gouraud |  24982 | 172.4 | 231 |                       209
Phong   |  24982 | 175.1 | 238 |                       216
```

The same 24,982 pixels in all three modes - identical geometry.

**Flat uses 42 distinct brightness levels where Phong uses 216.** That is the faceting
and the Mach banding: each strip is one flat band of colour, and the eye exaggerates
the step where two bands meet.

And at the highest tessellation the three converge:

```text
at 128 segments
Flat    | 137 distinct levels
Gouraud | 230
Phong   | 230

Gouraud-vs-Phong mean difference: 2.65 at 8 segments, 0.42 at 128
```

Flat climbs from 42 levels to 137 as the tube gains facets - more facets, more bands,
until the bands are too small to see. Gouraud and Phong close from 2.65 to 0.42.

### An honest note about Demo B

Phase 31 discovered that Gouraud does **not** dramatically fail on this tube, and that
is recorded rather than hidden. Brass's `n_s` is 27.9, so its highlight is broad enough
to cover a vertex - and a highlight Gouraud can see at a vertex is one it can
interpolate.

So Demo B is primarily a **Flat vs smooth** demonstration: it is about faceting and
Mach banding, which is exactly what L9 slide 27 is about. Demo A is the
Gouraud-vs-Phong demonstration, and it needs `n_s` = 160 to work.

That division of labour is not a workaround - it is the correct reading of the two
slides, and the project now has a measurement for each.

---

## The complete Stage C demonstration script

Everything below uses only keys the program already has. No code change, no special
build, no hidden flag.

| Time | Press | Say |
|---|---|---|
| 0:00 | - | "Two lights, six materials, three shading models, one shader program." |
| 0:10 | `K` x4 | "This is L8 slide 54, live. Ambient alone - flat, no direction. Plus diffuse - the shape appears. All three. Now specular alone: a mean brightness of 2 and a peak of 253. That is why it has to be a key." |
| 0:45 | `L` x3 | "Two lights, summed per fragment. The sun has no position so it never falls off - on the flat floor its brightest pixel and its average are the same number. The point light makes a pool." |
| 1:10 | `B` | "Blinn-Phong. Same highlight, same place - 0.8 pixels apart - but four times broader for the same exponent, because the angle it measures is halved." |
| 1:25 | look at the three spheres | "Brass, polished silver, black plastic, from L8 slide 60 verbatim. One mesh, three materials. The black one is the argument: `k_d` of 0.01 so it is genuinely black, `k_s` of 0.50 so it is clearly shiny." |
| 1:45 | `2`, `3` on the sea | "**Demo A.** Gouraud's streak peaks at 186, Phong's at 255. Identical geometry." |
| 2:00 | `+` `+`, then `2`, `3` | "Refine the sea and Gouraud catches up. The failure was coarseness. And Phong's picture did not change by a single pixel, because the sea is flat and Phong interpolates normals." |
| 2:25 | `-` `-` `-`, then `1`, `2`, `3` on the tube | "**Demo B.** Flat uses 42 brightness levels where Phong uses 216 - that is the faceting, and the banding where facets meet." |
| 2:45 | `N`, then `1` | "And the normals themselves: in Flat the sphere stops being a smooth sweep and becomes a polyhedron - 99 distinct normals against 19,312." |
| 2:55 | `M` | "Last one: the normal matrix. Watch the stretched cube at the front, and nothing else in the scene." |

---

## Likely teacher questions

### What is Demo A?

The sea at `n_s` = 160 with a low tessellation. Press `2` then `3`: Gouraud's sun
streak peaks at 186 and Phong's at 255, on identical geometry. Press `+` twice and
Gouraud catches up, which shows the failure was coarseness.

### Why does Gouraud miss it?

It computes the lighting at the vertices and interpolates the resulting **colour**. A
highlight falling between vertices is never computed at all, and no amount of blending
can invent a bright spot that was never calculated.

### How do you know the geometry really is identical?

Two ways. The lit pixel count is the same in every mode - 241,846 for the sea. And the
sea is flat, so Phong's picture is byte-identical across tessellations: **0 pixels
different** between 8 cells and 16. If the geometry were changing, Phong's picture
would change too.

### What is Demo B?

The brass tube at 8 segments, cycled through `1`, `2`, `3`. Flat uses 42 distinct
brightness levels where Phong uses 216 - visible faceting and Mach banding.

### Does Gouraud fail on the tube too?

Not dramatically, and that is worth knowing. Brass's `n_s` is 27.9, so its highlight is
broad enough to cover a vertex - and a highlight Gouraud can see at a vertex it can
interpolate. Demo B is the Flat-versus-smooth demonstration; Demo A is the
Gouraud-versus-Phong one, and it needs `n_s` = 160.

### Why does Flat show banding?

Each triangle gets one normal, so each triangle is one flat band of brightness. The eye
exaggerates the step between two adjacent bands - that is Mach banding. Measured, 42
bands at 8 segments rising to 137 at 128 segments, as the bands get too small to see.

### Why is Phong the most expensive?

It re-evaluates the illumination model for every pixel rather than every vertex
(L9 s36). On the sea that is 241,846 evaluations against 81 vertices at CELLS = 8.

### What did this phase add?

Nothing. It measured and recorded the two demonstrations and confirmed both are
reachable with the shipped keys.

---

## Checkpoint

Phase 33 passes when:

- Debug, Release and strict `/W4` all build with no warnings of any kind;
- **Demo A** works: at `CELLS` 8 and 16 Gouraud's peak is far below Phong's, and by
  `CELLS` 32 it catches up;
- Phong's image is effectively unchanged by the sea's tessellation - measured 0 and 2
  pixels - while Gouraud's changes by over 17,000;
- **Demo B** works: Flat uses markedly fewer brightness levels than Phong on the
  8-segment tube, and gains levels as the tube is refined;
- all three modes cover identical pixels in both demos;
- both demos are reachable using only `+`, `-`, `1`, `2` and `3`;
- every earlier key still behaves as before;
- the scene is 14 draws / 1840 triangles / 1045 vertices, matching a hand calculation.

### The hand calculation

```text
mesh        verts  tris  drawn   verts sent  tris sent
triangle        3     1      2            6          2
quad            4     2      1            4          2
grid           81   128      1           81        128
cylinder       66    64      1           66         64
sphere        200   396      4          800      1,584
cube           24    12      3           72         36
smoothcube      8    12      2           16         24
                            --        -----      -----
                            14        1,045      1,840
```

The sphere is drawn **four** times - once in the left-hand column and three times for
the material comparison - and the smooth cube **twice**, once for Phase 23's normal
comparison and once stretched for Phase 26's.

---

## What is not part of Phase 33

Nothing is added, and nothing after this may break either demonstration. **If a later
change stops Demo A or Demo B working, the change is wrong.**

No FPS comparison between the shading modes - that measurement belongs to Phase 62,
the optimization pass, along with the argument that one program means no state-change
cost when the mode switches.

**Stage D begins at Phase 34**: `src/Ship.h`, and the root, hull and deck **frames** of
the ship hierarchy. Its rule is one this project has enforced since Phase 16 - a frame
stores translation and rotation only, and `glm::scale` appears solely inside the
`drawMesh` call - and its checkpoint is a hull and a deck at the right size, with the
deck **not** inheriting the hull's stretch.

---

## Note: verification

### 10 measurement checks and 32 build checks, no failures

Twelve images were rendered for the two demos - the sea at three tessellations in two
modes, and the tube at two tessellations in three modes - and measured for lit area,
mean and peak brightness, hot-pixel count, distinct brightness levels and connected
highlight regions.

### One wrong expectation of mine, and it improved the demo

My first Demo A check asserted that Phong's streak would cover **more** pixels than
Gouraud's. It covers fewer - 224 against 1,353 at `CELLS` 8. Gouraud does not simply
lose the highlight; it **smears** a dimmer version of it over a much larger area,
because it interpolates a moderately bright vertex colour across a big triangle.

Gouraud's hot-pixel count also turned out to be erratic across tessellations - 1,353,
then 24, then 686 - because where the vertices happen to land relative to the streak is
essentially arbitrary. That is a true observation about Gouraud and a useless
measurement.

Replacing it with **peak brightness** (186 / 153 / 255 against a constant 255) and with
**how much each mode's own image changes across tessellations** (0 against 17,404) gave
a far stronger demonstration than the one I set out to measure - and it is the version
now recorded in the report script.

### A note on Stage C's testing overall

Six of the eight Stage C phases had a fault in the *verification* rather than in the
code, and every one of them produced a confident-looking wrong number first. The
pattern is consistent enough to be worth stating:

> A test that compares one image to another is hard to get wrong. A test that compares
> an image to a prediction is easy to get wrong, and has to predict the **whole**
> model, not one term of it.

Phases 30 and 32 are the two that ended up strongest, and both are of the first kind:
Phase 30 predicted the entire illumination model and matched it to 0.25 of 255 levels;
Phase 32 summed three images and matched the fourth to 0.192.

### Earlier phases still correct

14 draws, 1840 triangles, 1045 vertices, matching the hand calculation above. Every
key from Stage A, Stage B and Stage C behaves as before: `N`, `M`, `W`, `D`, `O`, `L`,
`K`, `B`, `1`/`2`/`3`, `+`/`-`, mouse orbit and scroll.
