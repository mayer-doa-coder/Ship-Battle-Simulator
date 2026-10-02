# Phase 15 - Normals as Colour

## Status

Phase 15 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed the normals debug view switching on and off with the
`N` key. Nothing was printed to the error output, and the program closed
cleanly.

The two things this phase promises were both measured, not judged by eye.

### 1. With the debug view OFF, nothing changed at all

The frame was read straight out of the framebuffer with `glReadPixels`, with
the clock pinned to exactly `t = 2.0 s`, and compared against the Phase 14
frame captured the same way:

```text
sha256, Phase 14 frame        : 9879EC77E7D7DEDD...B7FDF1097
sha256, Phase 15 frame, N off : 9879EC77E7D7DEDD...B7FDF1097
```

**Byte-for-byte identical.** The debug view is a pure addition: it cannot
change the normal picture, because when `uDebugNormals` is `0` the shader takes
exactly the path it took before.

### 2. With the debug view ON, every pixel is one of the predicted colours

The same frame, with the debug view forced on, contains **exactly 4 distinct
colours in the whole 1280 x 720 image**:

| Colour found | Pixels | What it is |
|---|---:|---|
| `(209, 168, 10)` | 799 536 | the background |
| `(0, 128, 128)` | 55 093 | the cube's **-X** face |
| `(128, 128, 255)` | 44 585 | everything facing **+Z**: both triangles, the quad, and the cube's +Z face |
| `(128, 255, 128)` | 22 386 | the cube's **+Y** face |

Every one of those was written down **before** the frame was captured, from the
normals stored in the mesh. Four flat colours and nothing else - no gradient,
no in-between shade anywhere - which is itself worth noticing, and is explained
under "Why there is no `normalize()` yet".

### 3. All six faces were checked, across the whole spin

One frame only shows the two or three faces that happen to be turned toward the
camera, so the capture was repeated at **14 different pinned times** between
`t = 0` and `t = 11 s`, letting the tilted spin axis bring every face around.

```text
t = 0     3 distinct colours        t = 4.8    5
t = 0.8   4                         t = 5.6    4
t = 1.6   4                         t = 6.4    5
t = 2.4   5                         t = 7.2    4
t = 3.2   5                         t = 8      4
t = 4     5                         t = 9      4
                                    t = 10     4
                                    t = 11     4
```

Across all 14 frames the **complete** set of colours produced was:

```text
(255, 128, 128)   +X   pale pink
(  0, 128, 128)   -X   dark teal
(128, 255, 128)   +Y   light green
(128,   0, 128)   -Y   purple
(128, 128, 255)   +Z   light blue
(128, 128,   0)   -Z   olive
(209, 168,  10)        background

total distinct colours observed : 7
colours outside the predicted set: 0
```

All six face colours appeared, every one matched its prediction exactly, and
**no other colour was ever produced**. Every pixel the debug view draws is one
of six values that can be worked out on paper from the mesh data.

(This was measured twice, by two independently written analyses, because the
first attempt had a bug of its own: PowerShell's `-shl` keeps the *type* of its
left operand, so shifting a `byte` left by 16 truncates to zero and silently
collapsed different colours together. Worth remembering as a general lesson -
a measurement tool can be wrong while the program it is measuring is perfectly
correct, which Phase 7's explanation also ran into with a screen-capture tool.)

## What changed

| File | Change |
|---|---|
| `shaders/basic.vert` | `out vec3 vNormal;` and `vNormal = aNormal;` - the first time the normal is read at all |
| `shaders/basic.frag` | `in vec3 vNormal;`, `uniform int uDebugNormals;`, and a branch writing `vNormal * 0.5 + 0.5` |
| `src/main.cpp` | `SceneState` gained `debugNormalsEnabled` and `debugNormalsKeyWasDown` |
| `src/main.cpp` | `processInput()` edge-detects `N` and flips the flag |
| `src/main.cpp` | `renderScene()` uploads `uDebugNormals` once per frame - the **first use of `setInt`** |
| `src/Mesh.h` | Unchanged. The normals were already there and already correct |

No geometry changed, no mesh changed, and no new object was added. This phase
adds a way of **looking** at what Phase 14 already built.

## The one idea

A normal is just three numbers, and so is a colour. If you show one as the
other, you can **see** a normal instead of having to trust it.

```text
normal  (x, y, z)   each component from -1 to +1
colour  (r, g, b)   each component from  0 to  1
```

That is the whole trick, and it is one line of GLSL:

```glsl
FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
```

## Why `-1..+1` has to become `0..1`

The two ranges do not match. A normal pointing along `-X` has `x = -1`, and
there is no such thing as a colour channel of `-1`; it would simply clamp to
black, and `-1` and `0` would look the same. So half the possible directions
would be invisible.

Halving and shifting fixes it exactly:

| Normal component | `* 0.5` | `+ 0.5` | Meaning |
|---:|---:|---:|---|
| `-1` | `-0.5` | **`0.0`** | none of that channel |
| `0` | `0.0` | **`0.5`** | half |
| `+1` | `0.5` | **`1.0`** | full |

Every direction now lands somewhere different and visible. A useful thing to
remember: **`0.5` grey means "this axis contributes nothing"**, so a mid-grey
channel is the normal saying "I do not point along this axis at all".

In 8-bit colour, `0.5` comes out as **128**, which is why so many of the
numbers below are 128.

## The colour of every face, worked out in advance

This is the viva change the plan asks for - *predict the colour of the `+X`
face before pressing the key* - so it is worth being able to do all six.

| Face | Normal | `N * 0.5 + 0.5` | 8-bit | Looks like |
|---|---|---|---|---|
| **+X** right | `( 1,  0,  0)` | `(1.0, 0.5, 0.5)` | `(255, 128, 128)` | pale pink |
| **-X** left | `(-1,  0,  0)` | `(0.0, 0.5, 0.5)` | `(0, 128, 128)` | dark teal |
| **+Y** top | `( 0,  1,  0)` | `(0.5, 1.0, 0.5)` | `(128, 255, 128)` | light green |
| **-Y** bottom | `( 0, -1,  0)` | `(0.5, 0.0, 0.5)` | `(128, 0, 128)` | purple |
| **+Z** front | `( 0,  0,  1)` | `(0.5, 0.5, 1.0)` | `(128, 128, 255)` | light blue |
| **-Z** back | `( 0,  0, -1)` | `(0.5, 0.5, 0.0)` | `(128, 128, 0)` | olive |

The way to work one out in your head: **the axis it faces gets pushed to full
or to zero, and the other two stay at half.** `+X` means "red all the way up,
green and blue left at half" - pale pink. `-X` means "red all the way down,
green and blue at half" - dark teal.

Notice that a face and its opposite are **not** the same colour, and are not
even similar. That matters: it means a face wound or normalled backwards shows
up as a completely different colour, not as a subtle shade, which is exactly
what a debug view should do.

The triangle and the quad are both flat in the XY plane with normal
`(0, 0, 1)`, so both come out the same light blue as the cube's front face.

## Why the tint is switched off in debug mode

`AppConfig::TINT` is `(0.6, 2.4, 3.0)`. If the tint were still applied, the
`+X` face's `(1.0, 0.5, 0.5)` would become `(0.6, 1.2, 1.5)`, clamp to
`(0.6, 1.0, 1.0)`, and arrive on screen as pale cyan - a colour that has
nothing to do with the normal and cannot be predicted or checked.

So the debug branch returns immediately and never touches `uTint`:

```glsl
if (uDebugNormals == 1) {
    FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
    return;
}
FragColor = vec4(vColor * uTint, 1.0);
```

A view whose only job is to be **read accurately** must not have a filter in
front of it. This is the same reason the quad and cube already pass
`uTint = (1, 1, 1)`: a value you want to inspect should not be multiplied by
something else on the way out.

## Why the normal is NOT transformed yet

`vNormal = aNormal;` - no matrix. The normal is shown in the mesh's **own**
space, exactly as the generator stored it.

That is deliberate, and it is what makes the checkpoint possible. The cube
spins. If the normal were transformed into world space, every face's colour
would change continuously as the cube turned, and "each cube face shows one
flat, predictable colour" would be impossible - there would be nothing steady
to predict.

What needs verifying **now** is the number the generator wrote down. Whether
that surface has since been moved, turned, or stretched is a separate question,
and Phase 26 is the phase that asks it: it adds the normal matrix
`(M^-1)^T`, and its checkpoint is that this same debug view *stays correct on a
non-uniformly scaled cube, which it does not without the normal matrix*. So the
debug view gets used twice - first to check the data, later to check the
transform.

One rule is already worth noting for when that happens. A normal is a
**direction**, not a position, so when it is finally multiplied by a matrix it
takes `w = 0`, never `w = 1` (Phase 4). Getting that backwards is one of the
project's classic bugs.

## Why there is no `normalize()` yet

The shader uses `vNormal` exactly as it arrives. That is safe right now, and
the captured frame proves it: the debug image contains **4 flat colours and no
in-between shade at all**.

The reason is that every surface in the project so far is flat, and all four
corners of a cube face carry the *same* normal. Interpolating between identical
values gives that value back, unchanged, at every pixel in between - so the
interpolated normal is still exactly unit length.

Curved surfaces are different. When Phases 20 to 22 add the cylinder and the
sphere, neighbouring corners will have **different** normals, and the
interpolated value halfway between two of them is shorter than either - it cuts
across the curve like a chord instead of following the arc. From that point on
`normalize()` belongs in this shader, and there will be a visible reason for it:
without it, a sphere's debug colours come out slightly dull toward the middle of
each quad. Adding it now, with nothing to fix, would hide that lesson.

There is one trade-off to be honest about. Because there is no `normalize()`,
this view shows a normal's **length** as well as its direction - so a generator
producing a non-unit normal would show up as a washed-out colour. That is
arguably useful. But length errors are much better caught by measuring than by
eye, which is what the Phase 14 check did: all 24 of the cube's normals were
confirmed to be exactly unit length.

## Why this comes eleven phases before the first light

This is the question most worth being able to answer, because the plan moves
this phase deliberately early.

A normal is not visible. Nothing on screen goes wrong in an obvious way when
one is wrong - and from Phase 27 onward, every normal is buried inside a
lighting equation, where a mistake shows up as "that surface looks a bit dark"
or "the highlight is in a slightly odd place". Those are extremely hard symptoms
to trace, because the lighting maths, the light direction, the material values,
the camera position, and the normal are all suspects at once.

With this key, a normal stops being an invisible number and becomes a colour
you can check against a prediction in one second. Every mesh generator written
from Phase 17 onward - quad, grid, cylinder, sphere - can be verified the moment
it is written, before any light exists to hide its mistakes.

That is also why Phase 23's `computeSmoothNormals()` demonstration works at
all: hard face colours against blended corners is a comparison you can only
*see* because of this key.

## The first use of `setInt`

`Shader.h` has had four uniform setters since Phase 3. Three of them went to
work immediately or in Phase 4. `setInt` has sat unused ever since, with a
comment saying "Later, for mode switches such as `uShadingMode`".

This is that later. `uDebugNormals` is an `int` uniform holding `0` or `1`:

```cpp
shader.setInt("uDebugNormals", scene.debugNormalsEnabled ? 1 : 0);
```

It is an `int` rather than a `bool` because that is what OpenGL's
`glUniform1i` provides, and because the same pattern grows naturally into
Phase 31's `uniform int uShadingMode`, which holds `0`, `1`, or `2` for Flat,
Gouraud, and Phong. A mode switch in one program with one branch, rather than
three separate shader programs, is a scope rule for this project - and this
phase is the small first version of it.

## Uploaded once per frame, not once per object

```cpp
shader.setMat4("uView", scene.view);
shader.setMat4("uProjection", scene.projection);
shader.setInt("uDebugNormals", scene.debugNormalsEnabled ? 1 : 0);
```

`uDebugNormals` sits with the camera matrices, above the draws, because like
them it is the same for every object in the frame. Only `uTint` and `uModel`
differ per object, so only those two are set inside the drawing section.

Sorting uniforms into "same for the whole frame" and "different per object" is a
habit worth forming now: Phase 62's optimization pass is explicitly about
hoisting the first kind out of the per-object loop, and doing it as each uniform
is added means there is nothing to clean up later.

## Where each job happens

| Function / file | Job in this phase |
|---|---|
| `processInput()` | Edge-detects `N` and flips `scene.debugNormalsEnabled` |
| `renderScene()` | Uploads `uDebugNormals` once, before the draws |
| `basic.vert` | Passes `aNormal` through to the fragment shader, untransformed |
| `basic.frag` | Chooses between the normal colour and the vertex colour |
| `Mesh.h` | Nothing. The normals were already correct |

## Likely teacher questions

### What does the debug view actually show?

Each pixel's normal, turned into a colour by `N * 0.5 + 0.5`.

### Why `* 0.5 + 0.5`?

A normal's components run from `-1` to `+1` and a colour channel runs from `0`
to `1`. Halving and shifting maps one range onto the other, so `-1` becomes
black, `0` becomes half-grey, and `+1` becomes full. Without it, every negative
component would clamp to `0` and half the possible directions would look
identical.

### What colour is the `+X` face, and why?

Pale pink, `(1.0, 0.5, 0.5)`. Its normal is `(1, 0, 0)`, so red goes to full
and green and blue stay at half.

### What colour is the `-X` face?

Dark teal, `(0.0, 0.5, 0.5)`. Same axis, opposite direction, so red goes to
zero instead of full. A face and its opposite look completely different, which
is what makes a reversed normal obvious.

### What does a mid-grey channel mean?

That the normal does not point along that axis at all - the component is `0`.

### Why is the tint not applied in debug mode?

Because the tint is `(0.6, 2.4, 3.0)` and multiplying it into the normal colour
would produce something unpredictable and unreadable. A view meant to be read
accurately must not have a filter in front of it.

### Why is the normal not multiplied by any matrix?

Because this phase checks the number the generator stored, and the cube spins -
a transformed normal would change colour continuously and there would be nothing
steady to predict. Phase 26 adds the normal matrix, and uses this same view to
prove why it is needed.

### When the normal IS transformed, what `w` does it use?

`w = 0`, because it is a direction, not a position. `w = 1` would let the
matrix's translation move it, which is meaningless for a direction.

### Why is there no `normalize()`?

Every surface so far is flat, and all the corners of one face share a single
normal, so interpolating between them returns that same normal exactly - the
captured frame contains only flat colours with no in-between shades at all.
Curved surfaces in Phases 20 to 22 have different normals at neighbouring
corners, and interpolating between those shortens the result. `normalize()`
belongs here from that point on, when there is a visible reason for it.

### Why add this eleven phases before the first light?

Because a wrong normal is nearly invisible once it is inside a lighting
equation - it shows up as a slightly dark surface or a slightly misplaced
highlight, with several other suspects. Checking normals directly, before any
light exists, means every generator can be verified the moment it is written.

### Why an `int` uniform instead of a `bool`?

Because `glUniform1i` is what OpenGL provides, and because the same pattern
grows into Phase 31's `uniform int uShadingMode` with three values. This is the
first real use of `setInt`, which has existed unused since Phase 3.

### Does the debug view change the normal picture at all?

No, and that was checked: with `N` off, the rendered frame is byte-for-byte
identical to Phase 14's, with the same SHA256.

### Why does `N` need an edge-detection check?

For the same reason `D`, `W`, and `O` do: `glfwGetKey` reports a key as pressed
on every frame it is held, so without remembering the previous frame's state a
one-second hold would flip the toggle over a hundred times.

## Simple viva modifications

- **Predict before you press** - the modification the plan names. Say what
  colour the `+X` face will be, then press `N` and orbit until that face is in
  view.
- **Prove a face is what you think** - orbit to put one face flat toward the
  camera, press `N`, and name it from its colour alone.
- **Break a normal on purpose:** in `buildCubeGeometry()`, negate one face's
  `normal`. With `N` on, that face jumps to its opposite colour - teal instead
  of pink - while the ordinary picture looks completely unchanged. This is the
  whole argument for the phase in one edit. Put it back afterwards.
- **Show the tint would ruin it:** temporarily change the debug line to
  `vec4((vNormal * 0.5 + 0.5) * uTint, 1.0)` and watch the colours stop
  matching the table.
- **Show what happens with no remapping:** temporarily write
  `vec4(vNormal, 1.0)`, and watch every face that points down a negative axis
  turn black and become indistinguishable from its neighbours.
- **Combine it with wireframe:** press `N` then `W`, and see the normals
  view on the back faces as well.
- **Change the key:** swap `GLFW_KEY_N` for another key and say why `N` was
  chosen (it is free, and later phases claim `1`/`2`/`3`, `B`, `H`, `K`, `L`,
  `T`, `P`, `TAB`, `SPACE`, `Q`/`E`, and `W`/`A`/`S`/`D`).

## Checkpoint

Phase 15 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning for
  `uDebugNormals`;
- with `N` not pressed, the picture is exactly Phase 14's;
- pressing `N` switches the whole scene to normal colours and prints the `ON`
  message; pressing it again returns and prints `OFF`;
- holding `N` produces exactly one toggle per press, not one per frame;
- each cube face shows **one flat colour**, steady as the cube turns, matching
  the table above;
- the `+X` face's colour can be predicted before the key is pressed;
- the `D`, `W`, and `O` keys and the mouse orbit and zoom all still work;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 15

No lighting - nothing is illuminated, and the normal is only being displayed,
never used in a calculation. No normal matrix: the normal is untransformed, so
a non-uniformly scaled object would show misleading colours, which is exactly
the problem Phase 26 exists to fix. No `normalize()`, for the reason given
above. No new geometry: `makeQuad()`, `makeGrid()`, `makeCylinder()`, and
`makeSphere()` arrive in Phases 17 to 22, and the unit-mesh rule with
`drawMesh()` is Phase 16.

**Still deliberately unfixed.** The Stage A review's finding on the Phase 6
pulse maths (`* 0.1f` where `* 0.5f` was meant, in `src/main.cpp`) is untouched
again, so that "the picture with `N` off is identical to Phase 14's" could be
checked exactly. It is worth fixing as its own small change now that both
milestones are proven.

Phase 16 introduces the unit-mesh rule and `drawMesh(mesh, model)`: scale
applied in the model matrix and never baked into vertex data, demonstrated by
drawing three differently sized cubes from **one** mesh and one VAO.
