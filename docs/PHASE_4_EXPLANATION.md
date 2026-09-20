# Phase 4 - The Model Matrix: Moving the Triangle

## Status

Phase 4 is complete and verified. Debug and Release builds succeeded, both
shaders linked, and a live Release run showed the triangle sliding left and
right with nothing printed to the error output. The loop stayed near `120 FPS`
with V-sync and closed normally with exit code `0`.

The motion was measured from screen pixels, not judged by eye. Over 9 seconds of
samples the triangle's centre swung from `-321` to `+319` pixels against a
prediction of `+-320`, and the measured period was `4.21` seconds against a
predicted `4.19`.

The triangle is still the temporary test object. This phase changes where it
is, not what it is.

## What changed

| File | Change |
|---|---|
| `shaders/basic.vert` | A new `uniform mat4 uModel`, and `gl_Position = uModel * vec4(aPosition, 1.0)` |
| `src/main.cpp` | `SceneState` holds the matrix, `updateScene()` builds it from the clock, `renderScene()` uploads it with `setMat4` |
| `src/main.cpp` | Named values `TriangleMotion::SLIDE_DISTANCE` and `SLIDE_SPEED` |

The vertex data did not change. The VBO was never touched after Phase 2. The
triangle moves because the matrix changes, not because any vertex was edited.

## The one idea

A **matrix** is a block of 16 numbers that describes a change of position. The
vertex shader multiplies every vertex by it, so one small matrix moves the
whole object.

```text
vertex position from the VBO      (never changes)
        |
        x  uModel                 (rebuilt every frame from the clock)
        |
gl_Position                       (where the vertex ends up)
```

The vertices describe the triangle's **shape**, measured from its own centre.
The model matrix says **where that shape is placed**. Keeping the two separate
is the most important habit in the whole project: later, every ship, cannon,
crew member, and raindrop is one shape drawn many times with a different matrix.

## What a translation matrix looks like

`glm::translate` produces this, where `tx, ty, tz` is the move:

```text
| 1  0  0  tx |
| 0  1  0  ty |
| 0  0  1  tz |
| 0  0  0  1  |
```

Multiplying it by a position `(x, y, z, 1)` gives:

```text
(x + tx,  y + ty,  z + tz,  1)
```

Every vertex is shifted by the same amount, so the shape does not change. It
just moves.

The start of the matrix, `glm::mat4(1.0f)`, is the **identity matrix**: ones down
the diagonal, zeros elsewhere. It moves nothing, exactly like multiplying a
number by `1`. `glm::translate` takes a matrix and returns a new one with a move
added, so starting from the identity gives a pure translation.

## Why `w = 1` finally matters

Phase 2 wrote `vec4(aPosition, 1.0)` and noted that `w = 1` marks a position.
Now you can see why. The translation values sit in the **fourth column** of the
matrix, and that column is multiplied by `w`:

| Vector | `w` | Result of the translation |
|---|---:|---|
| A position `(x, y, z, 1)` | 1 | Moved by `(tx, ty, tz)` |
| A direction `(x, y, z, 0)` | 0 | **Not moved at all** |

That is correct behaviour. Moving a house changes where it is. It does not
change which way north points. Directions such as the barrel's forward axis
(Phase 45) rely on this rule.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `updateScene()` | Works out the offset from the clock and builds `triangleModel` |
| `renderScene()` | Uploads the matrix with `setMat4`, then draws |
| `basic.vert` | Multiplies each vertex by the matrix |

`SceneState` is the small struct connecting the first two. `updateScene()` writes
it and `renderScene()` reads it, so neither needs to know how the other works.
This is the same split Phase 1 set up: update calculates, render displays.

## The motion formula

```text
offsetX = SLIDE_DISTANCE * sin(SLIDE_SPEED * now)
```

`sin` swings smoothly between `-1` and `+1`, so the offset swings between
`-SLIDE_DISTANCE` and `+SLIDE_DISTANCE`. One full swing takes
`2 * pi / SLIDE_SPEED` seconds, which is about `4.19` seconds for `1.5`.

`now` is `glfwGetTime()`, so the position is worked out fresh every frame from
the clock. **Nothing is stored between frames, and no list of positions exists.**
That is exactly what the project rule "no pre-computed animation" requires:
autonomous motion must be a formula of the current time. Wave motion in Phase 40
and the sail flutter in Phase 43 work the same way.

## Important changeable values

`TriangleMotion` in `src/main.cpp`:

| Value | Current | What happens if it changes |
|---|---:|---|
| `SLIDE_DISTANCE` | `0.5` | How far the triangle swings each way. Above about `0.6` part of it leaves the screen |
| `SLIDE_SPEED` | `1.5` | Larger is faster. One full swing takes `2 * pi / SLIDE_SPEED` seconds |

The distance is in clip-space units, where the visible x range is `-1` to `+1`.
The triangle extends `0.4` to each side of its centre, so `0.5 + 0.4 = 0.9`
stays inside the screen.

Everything from earlier phases, including `TINT`, `CLEAR_COLOR`, and
`TriangleConfig::VERTICES`, still works as before.

## Two silent failures to know

Phase 3 had one silent failure. Phase 4 adds a different one.

| Mistake | What you see | Console message |
|---|---|---|
| Misspell a uniform name (Phase 3) | Nothing changes | One warning line |
| **Never upload the matrix** (Phase 4) | **The triangle vanishes** | **Nothing** |

The second one matters because it looks like a rendering bug. If `setMat4` is
commented out, the uniform still exists in the shader, so Phase 3's name check
finds it and prints nothing. But nothing was ever written into it, so it keeps
its default value: a matrix of all zeros. Multiplying any vertex by zeros gives
`(0, 0, 0, 0)`, all three corners land on the same point, and OpenGL draws a
triangle with no area.

This was tested on purpose. With the upload line commented out, the triangle was
found in `0` of `8` screen samples and the console printed no warning.

**Rule of thumb:** if an object disappears after adding a matrix, check first
that the matrix was uploaded, and that it is not all zeros.

## Likely teacher questions

### What is a model matrix?

A matrix that places one object in the world. The vertices describe the object's
shape around its own centre, and the model matrix says where that shape sits.

### Why use a matrix instead of adding an offset to each vertex?

One matrix moves every vertex at once, and the vertex data on the GPU never has
to be re-uploaded. It also scales up: in later phases rotation and scaling are
combined with translation by multiplying matrices together, which adding offsets
could not do.

### What is the identity matrix?

The matrix that changes nothing: ones on the diagonal, zeros elsewhere. It is the
starting point that `glm::translate` builds on.

### Why is the matrix on the left, `uModel * vec4(...)`?

That is how matrix and vector multiplication is written in OpenGL and GLSL. The
matrix acts on the vector to its right. Reversing them gives a different and
wrong result.

### Why does the vertex shader need `w = 1`?

Translation lives in the fourth column of the matrix, and that column is
multiplied by `w`. With `w = 1` the move is applied. With `w = 0` it is not,
which is how directions are kept from being moved.

### Where is the translation stored in the matrix?

In the fourth column: `(tx, ty, tz, 1)`. GLM stores matrices column by column,
which is the order OpenGL expects, so `setMat4` passes `GL_FALSE` for the
transpose flag.

### Why is the matrix built in `updateScene` and not in `renderScene`?

Because building it is calculation and drawing it is display. Keeping them apart
means the render function only shows the data it is given, which makes each
easier to read and change.

### Why is `sin` used?

It gives smooth back-and-forth motion from a single formula. The triangle
slows near each end and moves fastest through the middle, which looks natural.

### Is this motion pre-computed?

No. The offset is calculated from `glfwGetTime()` every frame and nothing is
saved. There is no array of positions and no keyframes.

### Why is `deltaTime` still unused?

`deltaTime` is for motion that responds to the player, such as steering, where
you add a little each frame. This motion is a formula of absolute time instead,
so it uses `now`. `deltaTime` is used first in Phase 38.

### Why does the triangle not stretch or turn?

Only translation is in the matrix. Rotation arrives in Phase 5 and scaling in
Phase 6.

### The triangle vanished. What should I check?

That `setMat4("uModel", ...)` is actually being called, and that the matrix is
not all zeros. There is no console warning for this one.

## Simple viva modifications

- Slide faster or slower: change `SLIDE_SPEED`.
- Slide further: change `SLIDE_DISTANCE`, and find the value where the triangle
  starts leaving the screen.
- Slide up and down instead: in `updateScene()`, change
  `glm::vec3(offsetX, 0.0f, 0.0f)` to `glm::vec3(0.0f, offsetX, 0.0f)`.
- Slide diagonally: use `glm::vec3(offsetX, offsetX, 0.0f)`.
- Stop the motion: change `SLIDE_DISTANCE` to `0.0f` and the matrix becomes the
  identity.
- Shift it permanently: change `sin(...)` to a constant such as `0.3f`, so it
  sits to one side and stays there.
- Demonstrate the vanishing triangle: comment out the `setMat4` line, rebuild,
  and see the triangle disappear with no message. Then restore it.

## Checkpoint

Phase 4 passes when:

- Debug and Release builds succeed;
- both shaders compile and link with no `uModel not found` warning;
- the triangle slides smoothly left and right, keeping its shape and colours;
- changing `SLIDE_DISTANCE` or `SLIDE_SPEED` changes the motion with no edit to
  the shader;
- the triangle does not flicker, stretch, or disappear during a normal run;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 4

No rotation, no scaling, no view or projection matrix, and no camera. The model
matrix output still goes straight to the screen in clip space, exactly as the
raw positions did in Phase 2 and 3. Combining several transformations and the
order they must be multiplied in is Phase 6.

Phase 5 adds `glm::rotate`, so the triangle can spin as well as slide.
