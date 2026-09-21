# Phase 9 - Indexed Drawing: Building a Quad Without Repeating Corners

## Status

Phase 9 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed a solid,
correctly-coloured quad next to the two triangles from Phase 8, all three
still behaving correctly. Nothing was printed to the error output, the loop
stayed near `120 FPS`, and the program exited with code `0`.

The quad was seen and tested, not just assumed to work:

| Test | Result |
|---|---|
| Normal build | A complete square, no seam, no gap, four smoothly blended corner colours |
| One index deliberately broken (`1` used instead of `0`) | The square tore: half of it vanished completely |
| `D` key (Phase 8's depth-test toggle) pressed with the quad on screen | The two triangles still broke and re-sorted correctly; the quad, not overlapping anything, was unaffected |

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | New `QuadConfig` namespace: 4 vertices, 6 indices, a fixed position |
| `src/main.cpp` | New `QuadGpu` struct, adding an EBO to the VAO/VBO pair `TriangleGpu` already had |
| `src/main.cpp` | `createQuad()` / `destroyQuad()`, alongside the existing triangle versions |
| `src/main.cpp` | `SceneState` gained `quadModel` |
| `src/main.cpp` | `renderScene()` draws a third object, with `glDrawElements` instead of `glDrawArrays` |

No shader changed. The quad uses the exact same `basic.vert` / `basic.frag`
as the triangles.

## The problem this phase solves

A quad is two triangles glued together along a shared edge. Drawn the way
every earlier phase drew things - one triangle, its three corners listed one
after another - a quad needs **six** corners written out:

```text
triangle 1: top-left,     top-right,    bottom-right
triangle 2: bottom-right, bottom-left,  top-left
```

Look closely: `bottom-right` and `top-left` are each typed out **twice**. They
are the same point in space, sitting exactly on the shared edge, but the old
way of drawing has no way to say "reuse that one" - it can only list numbers
one after another and trust that two identical-looking rows really do mean
the same corner.

This phase's whole idea is to fix that.

## The one idea

Instead of listing corners over and over, list each **unique** corner once,
and separately say, in a short list of whole numbers, which corner to use for
which triangle. That short list is called an **index buffer** or **EBO**
(Element Buffer Object).

```text
4 unique corners (24 numbers: 4 x position+colour)
        +
6 indices (6 small whole numbers, saying "corner 0", "corner 3", and so on)
        =
one quad, two triangles, zero duplicated corners
```

## The two buffers, side by side

| | VBO (used since Phase 2) | EBO (new this phase) |
|---|---|---|
| GPU buffer type | `GL_ARRAY_BUFFER` | `GL_ELEMENT_ARRAY_BUFFER` |
| Holds | Vertex data: position and colour | Index data: whole numbers |
| One entry means | "Here is one corner" | "Use the corner already uploaded at this position" |
| Drawn with | `glDrawArrays` | `glDrawElements` |

They are genuinely different **kinds** of buffer to OpenGL, which is why they
need a different constant (`GL_ELEMENT_ARRAY_BUFFER` instead of
`GL_ARRAY_BUFFER`) even though the C++ code that uploads them,
`glBufferData`, looks almost identical for both.

## The quad's data

```cpp
constexpr float VERTICES[] = {
    -0.4f,  0.4f, 0.0f,   1.0f, 0.0f, 0.0f,   // 0: top-left,     red
     0.4f,  0.4f, 0.0f,   0.0f, 1.0f, 0.0f,   // 1: top-right,    green
     0.4f, -0.4f, 0.0f,   0.0f, 0.0f, 1.0f,   // 2: bottom-right, blue
    -0.4f, -0.4f, 0.0f,   1.0f, 1.0f, 0.0f,   // 3: bottom-left,  yellow
};

constexpr unsigned int INDICES[] = {
    0, 3, 2,   // top-left, bottom-left, bottom-right
    2, 1, 0,   // bottom-right, top-right, top-left
};
```

Four corners, each given a different colour, so the finished square is
obviously **one continuous shape** - the colours blend smoothly straight
across the shared diagonal, with no seam and no jump. Corners `0` and `2` each
appear in **both** triangles, and each is only ever typed out **once** in
`VERTICES`.

Six small integers replace two whole extra rows of position-and-colour data.
That is the saving indexing buys, and it grows enormously on more complex
shapes: a sphere or a grid shares far more corners between triangles than a
single quad does.

## Why the winding order had to be fixed

The first version of this quad's indices was `{0, 1, 2, 2, 3, 0}` - reading
the corners in the order they were listed. **It did not appear on screen at
all.**

The reason is `glEnable(GL_CULL_FACE)`, switched on since Phase 1. OpenGL
decides which side of a triangle is its "front" from the order its three
corners are listed in, seen from the camera: counter-clockwise is the front by
default. `{0, 1, 2}` in this quad's layout reads top-left, top-right,
bottom-right - going clockwise, not counter-clockwise. Both triangles were
being drawn back-to-front and then silently discarded as back faces.

The fix was to read the corners in the other rotational direction:
`{0, 3, 2, 2, 1, 0}` - top-left, bottom-left, bottom-right, and
bottom-right, top-right, top-left. Same four corners, same diagonal shared
between them, just visited in the order that makes both triangles
counter-clockwise as the camera sees them.

**This was tested, not just reasoned about.** Screenshots were taken of both
versions. The clockwise version showed nothing at all where the quad should
have been - not a distorted quad, nothing. The corrected version showed a
complete, correctly coloured square.

## A second thing that was tested on purpose: a broken index

`INDICES` was changed to `{0, 3, 2, 2, 1, 1}` - the second triangle's last
corner, which should be `0` (top-left), was changed to `1` (top-right) by
mistake.

The result: a triangle whose three corners are `2, 1, 1` has two corners in
**exactly the same place**. A triangle with two identical corners has zero
area - it cannot be seen at all. The screenshot showed the first triangle
(`top-left, bottom-left, bottom-right`) rendering normally, and the whole
second half of the square simply missing, as if the quad had been cut along
its diagonal and one half thrown away.

This is a very realistic bug. A single wrong digit in an index list does not
usually produce an error message - the GPU is perfectly happy to draw a
zero-area triangle, or the wrong corner of a real one. It just does not draw
the shape the programmer meant. This is exactly why the viva modification for
this phase is "change one index and see the tear": the tear is the whole
lesson.

## Why the EBO must not be unbound before the VAO

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);   // unbinding the VBO here is safe
glBindVertexArray(0);               // unbind the VAO LAST
```

A VAO remembers which `GL_ELEMENT_ARRAY_BUFFER` was bound while it was itself
bound - that memory is part of what makes a VAO useful. Calling
`glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)` while this VAO is still bound would
overwrite that memory with "no index buffer at all", and every later
`glDrawElements` call using this VAO would fail or draw nothing. The fix is
simply never to unbind the EBO on its own: unbinding the VAO first protects
whatever EBO it is holding onto.

## Why the quad is completely still

Unlike the triangles, the quad has no slide, spin, or scale. Its model matrix
is just `QuadConfig::POSITION` turned into a translation, rebuilt every frame
out of habit but never actually changing. This is deliberate: the only new
idea in this phase is indexed drawing, and giving the quad motion too would
make it harder to tell which part of the picture is proving which idea.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `createQuad()` | Uploads the 4 vertices and 6 indices once, at startup |
| `updateScene()` | Rebuilds `quadModel` (a fixed translation) each frame |
| `renderScene()` | Binds the quad's own VAO and calls `glDrawElements` |
| `destroyQuad()` | Frees the VAO, VBO, and EBO when the program closes |

## Likely teacher questions

### What is an EBO?

Element Buffer Object: a GPU buffer holding whole numbers that say which
already-uploaded vertex to use next, instead of holding vertex data itself.

### Why not just list six vertices for a quad?

It works, but two of those six rows would hold exactly the same position and
colour as two other rows, just to describe the shared corner twice. Indexing
removes that duplication.

### What does `glDrawElements` do differently from `glDrawArrays`?

`glDrawArrays` walks straight through the VBO in the order the vertices were
uploaded. `glDrawElements` instead walks through the EBO, and for each index
it finds there, fetches that vertex from the VBO. The VBO's order barely
matters any more - the EBO decides the drawing order.

### Why does the last argument to `glDrawElements` say `nullptr`?

Because a real GPU buffer is already bound as the index source. `nullptr`
means "start reading indices from the very beginning of that buffer", not "no
indices exist". If the indices were still living in ordinary CPU memory
instead of an EBO, that argument would be a real pointer to them - but that
is an older, slower way of working this project does not use.

### Why did the quad disappear when its indices went in the "wrong" order?

Back-face culling. OpenGL decides which side of a triangle faces the camera
from the order its corners are listed - counter-clockwise is the front by
default. Reading this quad's corners clockwise made both of its triangles
count as back faces, and `GL_CULL_FACE` discards those before they are ever
coloured in.

### What happens if one index is wrong by a small amount?

Usually nothing crashes. The GPU draws exactly what the index list describes,
even if that is a triangle with two identical corners (invisible, since it has
no area) or a triangle built from the wrong three corners entirely (a visibly
wrong shape). Finding this kind of bug means checking the index numbers by
hand, since there is no error message for it.

### Why must the EBO stay bound when the VAO is unbound?

The VAO records which EBO was bound to it. Unbinding the EBO first would
erase that record, leaving the VAO believing it has no index buffer, and every
later draw using it would fail silently or draw nothing.

### Why is the quad's shape and position fixed, instead of moving like the triangles?

So the phase's one new idea - indexing - is not tangled up with the moving-and-
turning ideas from earlier phases. A still, simple shape makes it obvious
exactly what indexing does and does not do.

## Simple viva modifications

- Break one index on purpose: change the final `0` in `INDICES` to `1`, rebuild,
  and watch half the quad vanish. Then put it back.
- Move the quad: change `QuadConfig::POSITION`.
- Resize the quad: edit the four `-0.4f`/`0.4f` values in `VERTICES`.
- Recolour one corner: change one corner's red/green/blue triple and watch
  the blended gradient shift.
- Show the winding rule directly: swap any two indices within one triangle
  (for example change `{0, 3, 2}` to `{3, 0, 2}`) and watch that half of the
  quad disappear, the same way the whole quad did before the winding fix.

## Checkpoint

Phase 9 passes when:

- Debug and Release builds succeed with no compiler warnings;
- all shaders compile and link, with no missing-uniform warning;
- a complete, correctly coloured square is visible, with no seam or gap at
  its diagonal;
- changing one index produces a visible tear, not a crash or a silent no-op;
- the two Phase 8 triangles and their `D` key still behave exactly as before;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 9

No `Mesh` class and no reusable generator functions yet - the quad's data is
still written out by hand, the same way the triangle's always has been. No
motion on the quad. No cube.

Phase 10 uses this same idea, indexing, to build the project's first real 3D
object: a cube, with 24 vertices and 36 indices.
