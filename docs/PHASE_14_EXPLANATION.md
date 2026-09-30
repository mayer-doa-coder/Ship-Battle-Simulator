# Phase 14 - One Reusable Mesh, Instead of Three Copies of the Same Idea

## Status

Phase 14 is complete and verified. Debug and Release builds succeeded with no
compiler warnings (only the pre-existing, accepted `LNK4098` from the vendored
GLFW library). A live Release run was driven with real simulated keyboard and
mouse input, not just launched and assumed correct:

| Test | Result |
|---|---|
| Default view, no input | Identical to Phase 13: the same triangle, quad, and cube, in the same places, same colours |
| `W` (wireframe) | The cube's edges show on all six faces, including the ones culling normally hides - unchanged from Phase 11/13 |
| `D`, `O` | Both still run without error; `O` still visibly sends the quad's transform order wrong, exactly as before |
| Left-drag across the window | Smooth orbit; the cube is seen from a new angle (its green top and red `+X` face, previously hidden) |
| Scroll wheel in, then out | Zooms to the `1.5` minimum (past the near-clip distance, as expected) and back out to `15.0`'s worth of view, with the cube reappearing correctly at each distance |
| Console output | Startup banner, shader link message, "Phase 14 ready...", and steady ~120 FPS frame reports. No `stderr` output |
| Shutdown | Clean exit, no leaked GPU objects, no stray process left running |

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` (new) | `struct Vertex { position, color }`; `class Mesh` owning a VAO/VBO/EBO with `draw()`, `valid()`, `destroy()`; `makeCube()` |
| `src/main.cpp` | `QuadConfig`/`CubeConfig`'s raw vertex/index arrays replaced by `Mesh` objects; `QuadGpu`, `CubeGpu`, `createQuad()`, `destroyQuad()`, `createCube()`, `destroyCube()` all removed; `renderScene()` now calls `quad.draw()` / `cube.draw()` |

The triangle (`TriangleGpu`, `createTriangle()`, `destroyTriangle()`) is
**untouched** - it still uses its own hand-written setup, on purpose. It is a
different shape from the quad and cube (not indexed, still `glDrawArrays`),
and this phase only unifies the two structs that were already identical.

## Why this phase exists: two structs that were the same struct

```cpp
struct QuadGpu { GLuint vao, vbo, ebo; };
struct CubeGpu { GLuint vao, vbo, ebo; };
```

Since Phase 10, `QuadGpu` and `CubeGpu` have owned exactly the same three
handles, and `createQuad()`/`createCube()` and `destroyQuad()`/`destroyCube()`
have done exactly the same steps - generate, bind, upload, set attribute
pointers, unbind; delete, delete, delete - differing only in which vertex data
went in. That repetition is that this phase removes: one `Mesh` class replaces
both structs and all four functions.

## The design decision: `Vertex{position, color}`, not `Vertex{position, normal}`

The reference implementation guide gives `Mesh` a normal on every vertex, for
lighting. This project has no lighting yet - that is Stage C, starting at
Phase 26 - and every object drawn so far, including the triangle that Phase 14
leaves alone, is told apart from every other object **only by its per-vertex
colour**. Building `Vertex` with a `normal` field today would mean one of two
things: the fragment shader loses the colour it currently draws with (breaking
"the picture is identical to Phase 13", this phase's own checkpoint), or
`normal` sits on the struct completely unused, which is exactly the "do not
build ahead" mistake this project's rules warn against.

`normal` arrives in Phase 15, together with the debug view that is the actual
reason to compute it (`docs/PHASE_PLAN.md` has been updated to reflect this
split, with a note explaining why). `docs/PHASE_PLAN.md`'s original Phase 14
wording is the one place this project's own plan asked for two ideas in one
phase; splitting it further is the same kind of division already applied
across all of Stage A and Stage B.

## `Mesh`: one constructor instead of four functions

```cpp
Mesh quad(QuadConfig::VERTICES, QuadConfig::INDICES);
Mesh cube = makeCube();
```

The constructor takes the raw vertex and index data and immediately uploads
it - what `createQuad()` and `createCube()` used to do by hand, once each, now
happens once, in one place, for any mesh's data:

```cpp
Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
    create(vertices, indices);
}
```

A constructor cannot report success or failure the way `createQuad()` used to
return a `bool`, so `main()` checks `.valid()` immediately afterward instead -
the same idea, moved to a separate call because a constructor's job is only to
build the object.

## Move-only, and why copying a `Mesh` must be illegal

```cpp
Mesh(const Mesh&) = delete;
Mesh& operator=(const Mesh&) = delete;

Mesh(Mesh&& other) noexcept { moveFrom(other); }
```

A `Mesh` holding `GLuint vao = 7` does not mean "the number 7" - it means "I
own VAO handle 7, and I am responsible for deleting it." If a `Mesh` could be
copied, two C++ objects would both believe they own VAO 7, and whichever one
is destroyed second would call `glDeleteVertexArrays` on a handle that is
already gone. Deleting the copy constructor makes that mistake impossible to
write by accident - the compiler refuses it - rather than merely being
something to remember not to do. `ShaderProgram` (Phase 2) already disables
copying for the identical reason.

Moving is different: it transfers ownership rather than duplicating it. This
is exactly what makes `Mesh cube = makeCube();` work - `makeCube()` builds a
`Mesh` as a local variable and returns it by value, and the move constructor
lets that returned object's VAO/VBO/EBO become the caller's `cube`, without
ever creating a second owner of the same handles along the way.

## Why `main()` still calls `.destroy()` explicitly

The obvious-looking design would be "the destructor frees the GPU objects, so
`main()` never needs to call anything." That is almost right, and the almost
matters:

```cpp
cube.destroy();
quad.destroy();
destroyTriangle(triangle);
shader.destroy();

glfwDestroyWindow(window);
glfwTerminate();
```

`quad` and `cube` are declared near the top of `main()`, so as plain local
variables their destructors would not run until the very end of `main()`'s own
scope - **after** `glfwDestroyWindow()` and `glfwTerminate()` have already run
on the lines above. Deleting a GPU object with no OpenGL context left to
delete it in is undefined behaviour. `ShaderProgram` has faced this exact
question since Phase 2, and its answer is the one `Mesh` copies: call
`.destroy()` explicitly, at the well-defined point where the context still
exists, and keep the destructor only as a safety net for cases where that call
never happens (an early return, for instance). `destroy()` zeroes its handles
once it runs, so the later, automatic destructor call finds nothing left to
delete and does nothing.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `Mesh::Mesh(vertices, indices)` | Uploads the data once; replaces `createQuad()`/`createCube()` |
| `Mesh::draw()` | Binds the VAO and calls `glDrawElements`; replaces the manual bind/draw lines in `renderScene()` |
| `Mesh::destroy()` | Frees the VAO/VBO/EBO; replaces `destroyQuad()`/`destroyCube()`, called explicitly at shutdown |
| `Mesh::~Mesh()` | Calls `destroy()` again as a safety net, in case the explicit call never happened |
| `makeCube()` | Builds the same 24-vertex, 36-index, one-colour-per-face cube Phase 10 hand-wrote, and returns it as a `Mesh` |

## Likely teacher questions

### Why does `Vertex` store colour instead of a normal, when the reference guide says normal?

Because nothing in this project reads a normal yet - lighting starts at Phase
26 - and every object currently on screen relies on per-vertex colour to look
correct. Adding `normal` now would either remove that colour (breaking this
phase's own "identical picture" checkpoint) or add a field nothing uses,
which this project's rules treat as building ahead. `normal` is added in
Phase 15, arriving together with the debug view that actually needs it.

### Why can a `Mesh` not be copied?

Because a `Mesh` represents ownership of specific GPU handles, not just their
numeric values. Two copies would both try to delete the same VAO/VBO/EBO,
and whichever runs second would delete objects that no longer exist.

### How does `Mesh cube = makeCube();` avoid that same problem?

`makeCube()` returns its local `Mesh` **by move**, not by copy. The move
constructor empties out the source object's handles as it transfers them, so
only one `Mesh` ever owns a given VAO at a time - the temporary inside
`makeCube()` is left holding zeroes by the time it disappears.

### Why does `main()` still call `quad.destroy()` if `Mesh` has a destructor?

Because the destructor would not run until after `glfwDestroyWindow()` and
`glfwTerminate()` have already torn down the OpenGL context, for any `Mesh`
declared as a plain local variable in `main()`. `ShaderProgram` has called its
own `destroy()` explicitly since Phase 2 for exactly this reason; `Mesh`
follows the same pattern instead of introducing a new one.

### What would happen if `destroy()` were called twice?

Nothing unwanted. Each `glDelete*` call is guarded by `if (handle != 0)`, and
`destroy()` sets every handle back to `0` once it runs. A second call - for
example, from the destructor, after the explicit shutdown call already ran -
finds every handle already zero and deletes nothing.

### Did the picture change at all in this phase?

No. Same triangle, same quad, same cube, same colours, same winding, same
camera behaviour. This phase changes what owns the GPU objects and how many
times the upload/draw/destroy code was written out, not what appears on
screen.

## Simple viva modifications

- Show the duplication this phase removed: point to how `createQuad()` and
  `createCube()` used to differ only in which array they uploaded.
- Add a third `Mesh` built from new data (for example, a second quad at a
  different position) and show it needs no new struct and no new
  create/destroy pair - just another `Mesh` variable.
- Demonstrate the move-only rule: try writing `Mesh copy = cube;` and show the
  compiler refuses it, because the copy constructor is `= delete`d.
- Explain the `.destroy()` timing question directly: why it cannot simply be
  removed in favour of relying only on the destructor.

## Checkpoint

Phase 14 passes when:

- Debug and Release builds succeed with no new compiler warnings;
- the default view, with no input, looks identical to Phase 13's;
- `W`, `D`, `O`, left-drag orbit, and scroll-wheel zoom all still work exactly
  as before, now driving `Mesh`-based objects;
- shutdown frees every GPU object while the context still exists, with no
  leaks and no errors;
- `main.cpp` no longer contains `QuadGpu`, `CubeGpu`, or their four
  create/destroy functions.

## What is not part of Phase 14

No `normal` field yet - that is Phase 15, together with the normals-as-colour
debug view. No lighting. No additional mesh shapes (quad generator, grid,
cylinder, sphere) - those start at Phase 17. The triangle still uses its own
Phase 2 setup, unconverted, since it is a genuinely different (non-indexed)
shape and converting it is not part of what this phase set out to do.
