# Phase 26 - The Normal Matrix

## Status

Phase 26 is complete and verified. It is the **first phase of Stage C**, the
lighting stage, and it adds no lighting at all - it fixes something that must be
right *before* any light is switched on.

Debug and Release both build with no warnings. 78 checks passed with zero failures.

```text
Ship Battle Simulator - Phase 26 | draws 11 | tris 652 | verts 445
```

### THE CHECKPOINT: the debug view stays correct on a stretched object

Press `N` for the normals view, then `M` to switch the correction off.

```text
the stretched cube ALONE, M on vs M off
  covers 30,629 pixels
  pixels that change : 30,629  (100.0%)   worst colour change: 129 of 255

everything ELSE in the scene, M on vs M off
  covers 406,690 pixels
  pixels that change : 0       (0.00%)
```

**All of the demo object changes. None of the rest of the scene changes.** That is
the phase in two numbers.

## What changed

| File | Change |
|---|---|
| `src/Shader.h` | New `setMat3()` |
| `shaders/basic.vert` | New `uniform mat3 uNormalMatrix` and `uniform int uUseNormalMatrix`; `vNormal` is now transformed and renormalised |
| `src/main.cpp` | `drawMesh()` computes and sends the normal matrix |
| `src/main.cpp` | New `StretchedCubeConfig`, a frame for it, and draw 11 |
| `src/main.cpp` | The `M` key, and a per-frame `uUseNormalMatrix` |

## The one idea

**A position and a direction are different kinds of thing, and an uneven scale
treats them as opposites.**

Take a cube squashed flat with `scale(1.0, 0.25, 1.0)`. Its sloping faces tilt, so
their normals must tilt too - but they must tilt **the other way** from the
surface. Squashing a surface downward makes its normals point *more* upward.
Multiplying the normal by the same squash would flatten it, which is exactly
backwards.

The matrix that performs the opposite operation is the inverse, transposed:

```cpp
glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
```

### Why that formula, in one line of algebra

A normal is defined by being perpendicular to the surface, so it is perpendicular
to every tangent `t`:  `n · t = 0`.

After a transform `M`, a tangent becomes `M t` - tangents are ordinary directions
along the surface and nobody disputes that. So the question is: which vector is
still perpendicular to `M t`?

```text
((M^-1)^T n) · (M t)  =  n^T M^-1 M t  =  n^T t  =  0
```

The `M^-1` and the `M` cancel. It works for **every** `M` and **every** surface,
which is why this is not a trick that happens to look right - it is the only
answer.

The verification for this phase is built on exactly that definition. It never
assumes the formula: it transforms two tangents at every vertex, then measures how
far each candidate normal is from perpendicular to them.

```text
                               worst degrees away from perpendicular
                               naive mat3(M)        (M^-1)^T
stretched shared-corner cube        62.614           0.0000
squashed ball (1.3, 0.35, 1.3)      59.863           0.0000
343 scale combinations, worst       78.510           0.0000
```

`(M⁻¹)ᵀ` is exact everywhere. The naive version is wrong by up to 78 degrees.

## A trap that nearly broke this phase

The phase plan says the checkpoint is "the debug view stays correct on a
non-uniformly scaled **cube**". The first version of this phase did exactly that -
a stretched copy of the flat 24-vertex cube - and **it demonstrated nothing**. The
test caught it:

```text
FLAT cube, stretched (1.3, 0.22, 0.55), spun 0 deg
    naive 0.000 deg        (M^-1)^T 0.000 deg
```

Both matrices are perfectly correct. Here is why:

The flat cube's normals are axis-aligned - `(1,0,0)`, `(0,1,0)` and so on. Under an
axis-aligned scale:

```text
diag(sx, sy, sz) * (1, 0, 0)  =  (sx, 0, 0)   ->  normalize  ->  (1, 0, 0)
```

Only one component is non-zero, so scaling changes the vector's **length** and not
its **direction** - and `normalize()` throws the length away. A flat axis-aligned
cube physically cannot show this bug.

So the demo object is the **Phase 23 shared-corner cube** instead. Its normals are
the corner diagonals, roughly `(1,1,1)/√3`, with all three components non-zero:

```text
corner normal (1,1,1)/sqrt(3) under scale (1.30, 0.22, 0.55)
  naive    -> (+0.910, +0.154, +0.385)   mostly +X
  (M^-1)^T -> (+0.155, +0.917, +0.367)   mostly +Y
  64.9 degrees apart
```

The same is true of a squashed ball, or any surface whose normals point in
directions the scale axes do not.

### The general rule this gives you

> The normal matrix matters when a normal is **not aligned with the axes being
> scaled unevenly**.

"Non-uniform scale" is not the condition - it is only a hint. The project's own
cylinder proves the distinction. It is scaled `(DIAMETER, HEIGHT, DIAMETER)` =
`(0.5, 0.9, 0.5)`, which is non-uniform, and yet:

```text
cylinder (0.5, 0.9, 0.5)   naive 0.000 deg     -> no error at all
cylinder (0.5, 0.9, 0.9)   naive 31.891 deg    -> badly wrong
```

Its wall normals are `(x, 0, z)`, and in the first case the x and z factors
**match**, so those normals only change length. Make x and z disagree and it breaks
immediately.

This is worth knowing because it is why the bug survives in real code for a long
time: it is invisible until something is both unevenly scaled *and* has normals
pointing off-axis.

## Why `normalize()` afterwards is not optional

```glsl
vNormal = normalize(normalTransform * aNormal);
```

The normal matrix fixes the **direction** and does nothing helpful to the
**length**. Measured on the demo cube:

```text
length of (M^-1)^T * n ranges 2.3971 to 3.1326
```

Two to three times too long, and by different amounts at different corners. Every
lighting term from Phase 27 onward assumes a unit normal, so a non-unit one would
show up as the wrong *brightness* rather than the wrong direction - a much harder
bug to spot than the one this phase fixes.

## Why it is computed on the CPU, and why `mat3`

**On the CPU, once per object per frame.** Inverting a matrix is not cheap, and
every vertex of an object shares the same model matrix - so doing it in the vertex
shader would repeat identical work thousands of times for one object. GLSL's
`inverse()` is also not guaranteed in every OpenGL 3.3 profile.

**A `mat3`, not a `mat4`.** A normal is a direction, so by the Phase 4 rule it has
`w = 0` - which means the translation column of a 4x4 could never affect it anyway.
Dropping it is both cheaper and clearer about intent.

Note the order inside the expression: the inverse and the transpose happen on the
full 4x4, and `glm::mat3(...)` takes the top-left corner **afterwards**. The
translation has to be inverted along with everything else before it is discarded.

## Why the comparison lives in the shader

`drawMesh()` always sends the correct matrix. The *choice* is one uniform set once
per frame, and the vertex shader picks:

```glsl
mat3 normalTransform = (uUseNormalMatrix != 0) ? uNormalMatrix : mat3(uModel);
```

This keeps `drawMesh()` simple - it has one job and does it unconditionally - and
it keeps the toggle to a single per-frame uniform rather than an extra argument at
eleven call sites. It follows the same pattern `uDebugNormals` has used since
Phase 15.

## Why a toggle key at all

Because the correction is **invisible** on a rotated or uniformly scaled object.
Without a before/after on a deliberately stretched one, there would be nothing to
show and a grader would have to take the algebra on trust.

This is the same reasoning behind three keys already in the project: `D` switches
depth testing off, `O` builds the transform matrices backwards, and `W` turns face
culling off. In each case the project demonstrates the **wrong** way deliberately,
because that is what makes the right way visible.

## Where each job happens

| Place | Job |
|---|---|
| `drawMesh()` | Computes `(M^-1)^T` from the final model matrix and sends it |
| `renderScene()` | Sets `uUseNormalMatrix` once per frame |
| `basic.vert` | Chooses a matrix, applies it, renormalises |
| `processInput()` | The `M` key |
| `StretchedCubeConfig` | The uneven scale, and the reasoning for it |

## Likely teacher questions

### What is the normal matrix for?

Non-uniform scaling. A normal is a direction, not a position, and an uneven scale
tilts a surface one way while tilting its normal the other way. `(M⁻¹)ᵀ` is the
matrix that performs that opposite operation.

### Why is it the inverse transposed, specifically?

Because a normal is perpendicular to every tangent, and a tangent transforms by `M`.
Then `((M⁻¹)ᵀn)·(Mt) = nᵀM⁻¹Mt = nᵀt = 0`: the inverse and the matrix cancel, so
perpendicularity survives. It works for every `M`, which is why it is the answer
rather than a trick.

### Show me where it matters.

Press `N`, then `M`. The stretched cube at the front changes completely - every one
of its 30,629 pixels - and nothing else in the scene changes by a single pixel.

### Why doesn't the flat cube show it?

Its normals are axis-aligned, like `(1,0,0)`. An axis-aligned scale turns that into
`(sx,0,0)`, which normalizes straight back to `(1,0,0)`. The length changes and the
direction does not. That is why the demo uses the shared-corner cube, whose normals
are corner diagonals.

### Your cylinder is scaled non-uniformly. Is its normal wrong?

No, and for an interesting reason. It is scaled `(0.5, 0.9, 0.5)` and its wall
normals are `(x, 0, z)` - the x and z factors are equal, so again only the length
changes. Make them disagree, say `(0.5, 0.9, 0.9)`, and the naive version is
immediately wrong by 32 degrees. The condition is not "non-uniform", it is "uneven
across the axes the normal actually points along".

### Why on the CPU rather than in the shader?

Every vertex of one object shares one model matrix, so inverting it per vertex
would repeat the same work thousands of times. GLSL's `inverse()` is also not
available in every 3.3 profile.

### Why `mat3` and not `mat4`?

A normal is a direction, so `w = 0` - the translation column could not affect it
even if it were there.

### Why normalize after multiplying?

Because the normal matrix corrects the direction but changes the length - by a
factor of 2.4 to 3.1 on this cube, and by different amounts at different corners.
Lighting needs a unit normal, so a non-unit one would come out as wrong brightness.

### Does this change the lighting?

There is no lighting yet - that starts in Phase 27. This phase makes sure the
normals are right **first**, because a wrong normal inside a lighting equation just
looks like slightly odd shading, which is far harder to diagnose.

## Simple viva modifications

- **The named exercise: stretch a cube and show the correction.** Change
  `StretchedCubeConfig::SCALE` in [src/main.cpp](../src/main.cpp) - try
  `(2.0, 0.15, 0.4)` for an extreme case - then press `N` and `M`.
- **Prove the trap:** change draw 11 from `smoothCubeMesh` to `cubeMesh`. Pressing
  `M` now does nothing at all, and you can explain why in one sentence.
- **Prove it on a ball:** scale the sphere `(1.3, 0.35, 1.3)` instead of uniformly.
  A squashed ball is the textbook example.
- **Make a uniform scale and show it does not matter:** set `SCALE` to
  `(0.5, 0.5, 0.5)`. `M` does nothing, because `(M⁻¹)ᵀ` and `M` point the same way.
- **Show the length problem:** delete the `normalize()` in the vertex shader. The
  debug view's colours go pale or saturated instead of wrong-angled.
- **Change the cylinder's scale** to `(0.5, 0.9, 0.9)` and watch a mesh that was
  previously immune start to care.

## Checkpoint

Phase 26 passes when:

- Debug and Release build with no compiler warnings;
- both shaders compile and link with no missing-uniform warning;
- the normal matrix is computed on the **CPU**, once per object per frame, as
  `glm::mat3(glm::transpose(glm::inverse(model)))`;
- the normal is transformed by it **and renormalised** in the vertex shader;
- the `N` debug view stays correct on a deliberately stretched object, and `M`
  proves it does **not** without the correction;
- the demo object's normals genuinely move - measured 100% of its pixels, with a
  worst colour change of 129 of 255 - while the rest of the scene changes by zero
  pixels;
- every earlier object and key still behaves as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 26

No lighting. No ambient, diffuse or specular term, no light position, no material -
Phases 27 to 29 add those one at a time. No second light, no shading-mode
comparison, no `K` mask.

Phase 27 adds **ambient plus diffuse from one directional sun**, with
`uGlobalAmbient`, `uKa` and `uKd`, and its checkpoint is a sphere with an obvious
lit side and dark side.

---

## Note: verification

### 78 checks, no failures

| Suite | What it does | Checks |
|---|---|---:|
| Normal maths | Builds four real meshes, transforms two tangents at every vertex, and measures how far each candidate normal is from perpendicular. Covers the flat cube, the shared-corner cube, the sphere and the cylinder, across 343 scale combinations | 71 |
| Pixels | Renders the `N` view with `M` on and off, isolating the stretched cube from the other ten draws, and compares the images | 7 |
| Build and run | Debug and Release, clean, run, and read the output | - |

The maths suite is built on the **definition** of a normal rather than on the
formula, so it could in principle have shown `(M⁻¹)ᵀ` to be wrong. It did not: the
error is `0.0000` degrees in every single case.

### One real design error, found and fixed inside the phase

The first version demonstrated on a stretched **flat** cube, where both matrices
are correct and nothing can be seen. This was not a small slip - it would have
produced a phase that passed a visual inspection while proving nothing, and it is
also a correction to the phase plan's own wording. Fixed by using the shared-corner
cube, and the reasoning is recorded in `StretchedCubeConfig` in the source so it
cannot be quietly undone.

### Two faults in my own test code, both corrected

- The first test compared each candidate against the **flat facet normal** of each
  triangle. That is wrong for a curved mesh: a sphere's stored normal is the
  *smooth* normal and deliberately differs from the facet it sits on, which showed
  up as 8 degrees of fake "error" on the sphere and would have hidden a real one.
  Replaced with the tangent-perpendicularity test, which is exact for any mesh.
- The test called `buildSharedCubeGeometry()` and forgot that it leaves the normals
  at zero on purpose - `computeSmoothNormals()` fills them in afterwards, which is
  the whole point of Phase 23. Every shared-cube result was silently zero until
  that was fixed.

### Earlier phases still correct

The frame went from 10 draws / 640 triangles / 437 vertices to **11 / 652 / 445**,
which is exactly one more `smoothcube` draw (+12 triangles, +8 vertices). Every
Stage B key still behaves as before, and the `+`/`-` detail levels, the `W`
wireframe prediction and the Phase 23 cube comparison are all unaffected.
