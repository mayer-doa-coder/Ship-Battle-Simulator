# Phase 6 - Scale, and the T * R * S Order

## Status

Phase 6 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed the
triangle sliding, spinning, and pulsing in size together. Nothing was printed
to the error output, the loop stayed near `120 FPS`, and the program exited
with code `0`.

The behaviour was measured from screen pixels, not judged by eye, in both
transform orders:

| Build | Fits the correct `T * R * S` model | Fits the reversed `S * R * T` model |
|---|---:|---:|
| Default (`O` not pressed) | **5.2 px** error | 192.8 px error |
| After pressing `O` | 191.9 px error | **5.4 px** error |

Each build matches only its own order, to a few pixels, and misses the other by
close to 200. The centroid of the triangle also swings far wider once `O` is
pressed (`+-257` px sideways instead of `+-16`), which is the "smear" the code
predicts.

The key-repeat guard was tested directly. Holding `O` down for about one second,
roughly `120` frames at this frame rate, produced exactly **one** `[order]`
line in the console, not one hundred and twenty.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | Named values `TriangleScale::PULSE_MIN`, `PULSE_MAX`, `PULSE_SPEED` |
| `src/main.cpp` | `SceneState` gained `reverseOrder` and `orderKeyWasDown` |
| `src/main.cpp` | `processInput()` now edge-detects the `O` key and flips `reverseOrder` |
| `src/main.cpp` | `updateScene()` builds a `scaleMat` and chooses `slide * spin * scaleMat` or `scaleMat * spin * slide` |
| `shaders/basic.vert` | Unchanged. Still one `uModel`, still one multiplication |

## The one idea

A **scale matrix** resizes every point, measured from the origin, the same way
translation moves and rotation turns. Phase 6 now has three kinds of matrix, and
the real lesson is not scaling itself. It is that **the order they are
multiplied in changes the result**, even though every matrix involved is
exactly the same.

```text
slide  x  spin  x  scaleMat   =   correct: resize, then turn, then move
scaleMat  x  spin  x  slide   =   backwards: move, then turn the move, then resize the move
```

## What a scale matrix looks like

Scaling by a factor `k` on x and y uses this matrix:

```text
| k  0  0  0 |
| 0  k  0  0 |
| 0  0  1  0 |
| 0  0  0  1 |
```

Multiplying it by a position `(x, y)` gives `(kx, ky)`. Every point moves toward
or away from the origin in proportion to its distance from it. A point already
at the origin does not move at all.

This matters together with the rotation lesson from Phase 5: **scale, like
rotation, acts around the origin, not around the object's own middle.** An
object built with its shape not centred at the origin will scale unevenly, in
the same way it wobbled when it turned.

## Why the order matters: read right to left

```cpp
scene.triangleModel = slide * spin * scaleMat;
```

The vertex meets the rightmost matrix first, so this is applied as:

1. **`scaleMat`** resizes the triangle around the origin, where its corners sit.
2. **`spin`** turns the already-resized triangle around the origin.
3. **`slide`** carries the turned, resized triangle to its place on screen.

Each step only ever acts on the result of the step before it, and that result
is still centred where the triangle's own shape is centred. The triangle grows
and shrinks on the spot, spins on the spot, and both of those together travel as
one rigid trip. This is the standard order for a reason: **Translate x Rotate x
Scale**, always read right to left, always scale first.

Now look at the reversed build:

```cpp
scene.triangleModel = scaleMat * spin * slide;
```

1. **`slide`** carries the triangle away from the origin first.
2. **`spin`** turns the whole trip, slide included, around the origin.
3. **`scaleMat`** resizes the whole trip, slide and turn included, around the
   origin.

A scale meant to resize the *shape* now also stretches how *far* the triangle
travels, because the travel is measured from the origin too. A spin meant to
turn the *shape* now swings the *entire slid-out path* around the origin,
because the slide already moved the triangle away from the turning centre. The
measured result was a centroid swinging `+-257` pixels sideways, compared with
`+-16` in the correct order: the triangle smears through a wide, pulsing loop
instead of pulsing and spinning where it stands.

Nothing is broken in the reversed build. Every matrix is correct on its own.
Only the sequence of operations, read right to left, is different, and that
alone is enough to produce a completely different picture from the same three
matrices.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `processInput()` | Edge-detects the `O` key and flips `scene.reverseOrder` |
| `updateScene()` | Builds `slide`, `spin`, and `scaleMat`, then multiplies them in the chosen order |
| `renderScene()` | Unchanged: uploads `triangleModel` with `setMat4` |
| `basic.vert` | Unchanged: multiplies each vertex by whatever matrix it is given |

## Why the key needs "was it already down"

```cpp
const bool orderKeyIsDown = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
if (orderKeyIsDown && !scene.orderKeyWasDown) {
    scene.reverseOrder = !scene.reverseOrder;
    ...
}
scene.orderKeyWasDown = orderKeyIsDown;
```

`glfwGetKey` reports a key as `GLFW_PRESS` for **every frame** it is held down,
not once per press. At `120 FPS`, holding the key for a single second would call
this code about `120` times. Without remembering the previous frame's state, the
toggle would flip `120` times in that second, alternating so fast it looks like
nothing happened.

`orderKeyWasDown` remembers whether the key was already down last frame. The
toggle only fires on the frame the key transitions from up to down: the leading
edge of the press. This was measured directly: holding `O` for about a second
produced exactly one console line, not dozens.

This same technique returns for `TAB` (Phase 46) and every other mode-switch key
in the project.

## Keeping the factor inside a range without a clamp

```cpp
const float scalePulseMid = (PULSE_MIN + PULSE_MAX) * 0.5f;
const float scalePulseAmp = (PULSE_MAX - PULSE_MIN) * 0.5f;
const float scaleFactor = scalePulseMid + scalePulseAmp * std::sin(PULSE_SPEED * now);
```

`sin` swings between `-1` and `+1`. Multiplying by the half-difference and
adding the midpoint stretches that swing to sit exactly between `PULSE_MIN` and
`PULSE_MAX`, with no `if` and no `glm::clamp` needed. For `PULSE_MIN = 0.5` and
`PULSE_MAX = 1.3`: mid is `0.9`, amplitude is `0.4`, and the factor swings
between `0.9 - 0.4 = 0.5` and `0.9 + 0.4 = 1.3`, exactly the bounds asked for.

## Important changeable values

`TriangleScale` in `src/main.cpp`:

| Value | Current | What happens if it changes |
|---|---:|---|
| `PULSE_MIN` | `0.5` | Smallest size, as a fraction of the original |
| `PULSE_MAX` | `1.3` | Largest size, as a fraction of the original |
| `PULSE_SPEED` | `1.2` | Radians per second. Larger pulses faster |

`TriangleMotion::SLIDE_DISTANCE`/`SLIDE_SPEED` and `TriangleSpin::SPIN_SPEED`/
`SPIN_AXIS` from Phases 4 and 5 still control the slide and the spin.

## Likely teacher questions

### What is a scale matrix?

A matrix that resizes every point by a factor, measured from the origin. A
factor above `1` enlarges, below `1` shrinks, and `1` leaves the size unchanged.

### Why does scale, like rotation, act on the origin and not the object's middle?

Because that is what the matrix multiplication does: every coordinate is
multiplied by the factor, including coordinates that are not zero. A point far
from the origin moves further than a point close to it. If the object's own
middle is not at the origin, scaling changes its position as well as its size.

### What is the correct order for combining translate, rotate, and scale?

`T * R * S`, read right to left: scale first, then rotate, then translate. This
keeps each step acting on an origin-centred result, so the object resizes and
turns on the spot and then travels as one rigid piece.

### What happens if the order is reversed?

The operations that were meant to affect the *shape* end up affecting the
*already-moved position* instead. A resize stretches how far the object
travelled: a turn swings the whole trip around the origin. The object smears
through a wide path instead of moving as a rigid shape. This was measured:
reversing the order changed the centroid's sideways swing from `+-16` pixels to
`+-257`.

### Why does `glfwGetKey` need an edge-detection check?

Because it reports `GLFW_PRESS` for every frame the key stays down, not once per
physical press. Without checking the previous frame's state, a single one-second
key hold would trigger the action roughly `120` times at this frame rate.

### How does the pulsing factor stay inside its range without an `if`?

`sin` already swings between `-1` and `+1`. Scaling that swing by the half-range
and shifting it by the midpoint produces exactly the wanted range, with no
comparison needed.

### Is the pulse pre-computed?

No. Like the slide and the spin, the factor is `mid + amp * sin(PULSE_SPEED *
now)`, calculated fresh every frame from the clock. Nothing is stored.

### Why did the shader not need to change again?

`uModel` is still one `mat4`, and the shader still just multiplies every vertex
by it. Whether that matrix holds a move, a turn, a resize, or all three in some
order is entirely decided in `updateScene()`. The shader has no way to know and
does not need to.

### What does `glm::vec3(scaleFactor, scaleFactor, 1.0f)` mean?

It scales x and y equally, keeping the triangle's proportions correct as it
grows and shrinks, and leaves z alone since this triangle has no depth yet.

## Simple viva modifications

- Pulse bigger or smaller: change `PULSE_MIN` or `PULSE_MAX`.
- Pulse faster: increase `PULSE_SPEED`.
- Stop the pulse: set `PULSE_MIN` and `PULSE_MAX` to the same value.
- Stretch instead of scale evenly: change `glm::vec3(scaleFactor, scaleFactor,
  1.0f)` to `glm::vec3(scaleFactor, 1.0f, 1.0f)` and watch only the width pulse.
- Demonstrate the order lesson live: press `O`, watch the smear appear, read the
  console line, and press `O` again to return to normal.
- Demonstrate the key-repeat guard: hold `O` down for a few seconds and count
  the console lines. There should be exactly one per physical press, not one
  per frame.

## Checkpoint

Phase 6 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the triangle slides, spins, and pulses in size together, all correctly, by
  default;
- pressing `O` visibly changes the motion to a wide, smeared loop and prints
  `[order] S * R * T (reversed on purpose - watch it smear)`;
- pressing `O` again returns the correct motion and prints `[order] T * R * S
  (correct)`;
- holding `O` down produces exactly one toggle per press, not one per frame;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 6

No view matrix, no projection matrix, and no camera. The triangle is still
drawn straight into the window's flat `-1..+1` space, so it still stretches
sideways in a wide window exactly as it did in Phase 5.

Phase 7 adds the view and projection matrices together, which is needed before
any of this can look correct in true 3D.
