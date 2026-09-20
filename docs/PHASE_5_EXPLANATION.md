# Phase 5 - Rotation: Making the Triangle Spin

## Status

Phase 5 is complete and verified. Debug and Release builds succeeded with no
compiler warnings and both shaders linked. A live Release run showed the
triangle spinning while it slides. The Debug build was run too: it started,
linked the shaders, and closed normally. Neither printed anything to the error
output, the loop stayed near `120 FPS` with V-sync, and both exited with code
`0`.

The behaviour was measured from screen pixels, not judged by eye:

| Check | Predicted | Measured |
|---|---:|---:|
| Spin rate (`SPIN_SPEED`) | `2.00` rad/s | `2.00` rad/s, fit quality R^2 = `1.000` |
| Wobble of the triangle's middle, sideways | `+-85` px | `+-85` px |
| Wobble of the triangle's middle, up and down | `+-48` px | `+-48` px |
| Triangle visible while spinning about Z | every frame | `80` of `80` frames |

The triangle is still the temporary test object. This phase changes how it is
turned, not what it is.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | Named values `TriangleSpin::SPIN_SPEED` and `SPIN_AXIS` |
| `src/main.cpp` | `updateScene()` builds a `spin` matrix with `glm::rotate` and multiplies it with the `slide` matrix |
| `shaders/basic.vert` | **Comment only.** The shader logic did not change |

The vertex data did not change, the upload code did not change, and the shader
did not change. That is the point of this phase: `uModel` is a plain matrix, and
a matrix can hold a turn just as easily as a move.

## The one idea

A **rotation matrix** is a matrix that turns every point around a centre. Phase 4
made a matrix that moves things. Phase 5 makes a second one that turns things,
then joins the two by multiplying them.

```text
slide matrix  x  spin matrix   =   one matrix that does both
```

Multiplying two matrices gives a single matrix whose effect is "do one, then the
other". The vertex shader still receives one `uModel` and does not know or care
that it was built from two parts.

## What a rotation matrix looks like

Turning around the Z axis (the line pointing straight out of the screen) by an
angle `a` uses this matrix:

```text
| cos(a)  -sin(a)  0  0 |
| sin(a)   cos(a)  0  0 |
|   0        0     1  0 |
|   0        0     0  1 |
```

Multiplying it by a position `(x, y)` gives:

```text
x' = x * cos(a) - y * sin(a)
y' = x * sin(a) + y * cos(a)
```

A quick check with `a = 90 degrees`, where `cos = 0` and `sin = 1`: the point
`(1, 0)` becomes `(0, 1)`. A point on the right moves to the top, which is a
counter-clockwise quarter turn. A positive angle turns counter-clockwise on the
screen.

Notice the fourth column is all zeros except the last. A rotation has no move in
it, so it never touches the position part that Phase 4 used.

## Radians, not degrees

`glm::rotate` measures angles in **radians**.

| Turn | Degrees | Radians |
|---|---:|---:|
| Quarter turn | 90 | `1.57` (pi / 2) |
| Half turn | 180 | `3.14` (pi) |
| Full turn | 360 | `6.28` (2 * pi) |

`SPIN_SPEED = 2.0` therefore means `2.0` radians every second. One full turn
takes `6.28 / 2.0 = 3.14` seconds.

Passing degrees by mistake is a common bug: `glm::rotate(m, 90.0f, axis)` does
not turn a quarter, it turns `90` radians, which is about `14` full turns. The
result looks like random flickering.

## The turn happens around the origin

This is the most important idea in the phase, so read it twice.

**A rotation always turns things around the point `(0, 0)` of the space it is
applied in.** It does not turn around the middle of the object. It turns around
the origin, and the object goes wherever that carries it.

Look at the vertices in `TriangleConfig::VERTICES`. Their positions are measured
from the origin `(0, 0)`. If the triangle's corners are spread evenly around the
origin, it appears to spin on the spot. If they are not, it swings around the
origin like a stone on a string.

This triangle is slightly off. The average of its three corners, which is its
visual middle, is at:

```text
y = (0.5 + 0.5 - 0.6) / 3 = +0.133
```

So the triangle's middle is `0.133` above the origin, and as it turns, its middle
travels around a small circle of that radius. It **wobbles** a little instead of
spinning perfectly on its own middle. On screen the wobble measured `+-85`
pixels sideways and `+-48` pixels up and down, exactly as predicted.

To spin exactly on its middle, move the corners so their `y` values add up to
zero. For example, subtract about `0.133` from each: `0.37, 0.37, -0.73`.

This idea is why every object in the final project is built centred on its own
origin: a ship, a barrel, and a crew member all need to turn around a sensible
point.

## Why the order of multiplication matters

```cpp
scene.triangleModel = slide * spin;
```

Read a matrix product from **right to left**. The vertex meets the rightmost
matrix first:

1. `spin` turns the triangle around the origin, where its corners sit.
2. `slide` then carries the already-turned triangle to its place.

The result is a triangle that turns on the spot while it travels.

Now swap them:

```cpp
scene.triangleModel = spin * slide;
```

1. `slide` carries the triangle away from the origin first.
2. `spin` then turns **the whole trip** around the origin.

The triangle now circles the origin instead of spinning. This was tested on
purpose. The path of the triangle's middle was compared against both orders:

| Build | Fits `slide * spin` | Fits `spin * slide` |
|---|---:|---:|
| `slide * spin` (shipped) | **3.5 px** error | 181.5 px error |
| `spin * slide` (swapped on purpose) | 185.0 px error | **2.6 px** error |

Each build matches its own order to a few pixels and misses the other order by
over `180`. The swapped build moved the triangle's middle `+-325` pixels sideways,
compared with `+-85` for the shipped order. That is the difference between
circling and spinning.

The order is the main lesson of Phase 6, which adds scaling and a key that
builds the product backwards on purpose. Phase 5 only needs you to know that the
order matters and how to read it.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `updateScene()` | Builds `slide`, builds `spin`, multiplies them into `triangleModel` |
| `renderScene()` | Unchanged: uploads the matrix with `setMat4` |
| `basic.vert` | Unchanged: multiplies each vertex by the matrix |

The spin angle comes from the clock, like the slide:

```text
spinAngle = SPIN_SPEED * now
```

Nothing is stored between frames. The angle is worked out fresh each frame from
`glfwGetTime()`, so there is no table of angles and no pre-computed animation.

## Important changeable values

`TriangleSpin` in `src/main.cpp`:

| Value | Current | What happens if it changes |
|---|---:|---|
| `SPIN_SPEED` | `2.0` | Radians per second. Larger is faster. Negative spins the other way. `0` stops the spin |
| `SPIN_AXIS` | `(0, 0, 1)` | The line the triangle turns around. `(0, 0, 1)` is Z, straight out of the screen |

`TriangleMotion` from Phase 4 still controls the slide, which in the current
code moves the triangle up and down. `TINT`, `CLEAR_COLOR`, and
`TriangleConfig::VERTICES` still work as before.

## Two things that look wrong but are expected

### The triangle stretches as it turns

Watch the triangle in a wide window and its shape seems to change as it turns.
It is not turning rigidly. It stretches sideways and squashes upward.

This is real and expected. There is no projection matrix yet, so the space the
triangle lives in maps `-1` to `+1` onto the window's width and height
**separately**. In a `1280 x 720` window:

```text
1 unit sideways = 640 pixels
1 unit upward   = 360 pixels
```

Horizontal distances are drawn nearly twice as big as vertical ones. A shape
that is `0.8` wide and `1.1` tall is `512 x 396` pixels the right way up. Turned a
quarter, it is `1.1` wide and `0.8` tall, which is `704 x 288` pixels. The
measured widths ranged from `476` to `734` pixels, matching the prediction
of `481` to `749`.

The fix is Phase 7. A perspective projection includes the window's aspect ratio,
so a turned triangle keeps its shape.

### Turning around a different axis makes the triangle vanish

Change `SPIN_AXIS` to `(0, 1, 0)`, the vertical Y axis. The triangle narrows to a
thin sliver, then disappears, then comes back.

This was measured. With the Y axis the triangle was visible in only `43` of `80`
frames (`54` percent), and when visible it was as thin as `17` pixels. With back-
face culling switched off (`glEnable(GL_CULL_FACE)` in `main()`), it was visible
in `60` of `60` frames.

So the triangle is not being lost. Turning it around Y flips it so you are
looking at its back, and OpenGL is told to skip back faces. Which side of a
triangle is the front is decided by the order its corners are listed in. That is
called **winding order**, and Phase 11 explains it. It is the same effect you met
when flipping the triangle upside down earlier.

Turning around the Z axis never shows the back of the triangle, so it stays
visible all the time.

## Likely teacher questions

### What is a rotation matrix?

A matrix that turns every point around a fixed centre. For a turn around the Z
axis it holds the cosine and sine of the angle, arranged so that `(x, y)` becomes
`(x cos a - y sin a, x sin a + y cos a)`.

### Why does `glm::rotate` take an axis?

In 3D a turn needs a line to turn around. `(0, 0, 1)` is the Z axis, straight out
of the screen, which is the right choice for spinning something flat on the
screen. The X and Y axes tip the object forwards or sideways.

### Why radians?

Radians are the unit the maths functions and GLM use. A full turn is `2 * pi`
radians. Passing degrees by mistake gives a huge wrong angle. `glm::radians()`
converts degrees to radians when you prefer to think in degrees.

### Why does the triangle not spin exactly on its middle?

Rotation turns around the origin `(0, 0)`, and this triangle's corners average to
`y = 0.133`, not `0`. Its middle circles the origin at that radius. Move the
corners so their `y` values sum to zero and the wobble goes away.

### What does the order `slide * spin` mean?

Read right to left. The triangle is turned around the origin first, then carried
to its place. Matrices are applied to a vertex starting from the one nearest to
it.

### What happens if the order is swapped?

The triangle is carried first and the whole trip is then turned around the origin,
so it circles the origin instead of spinning. Measured, its middle moved `+-325`
pixels sideways instead of `+-85`.

### Why did the vertex shader not change?

The shader multiplies every vertex by whatever matrix it is given. It does not
know whether the matrix moves, turns, or both. All the new work is in `updateScene()`.

### Why does the triangle stretch while turning?

There is no projection matrix yet, so the window's width and height are mapped
onto `-1..+1` separately. Horizontal units are `640` pixels and vertical units
are `360` in a `1280 x 720` window. Phase 7 adds a projection that includes the
aspect ratio.

### Why does the triangle disappear when turned around the Y axis?

It flips to show its back face, and back-face culling skips those. Culling is
switched on in `main()`. Turning it off makes the triangle visible all the time.

### Is the spin pre-computed?

No. The angle is `SPIN_SPEED * now`, calculated fresh every frame from the clock.
No angles are stored.

### Which way does a positive angle turn?

Counter-clockwise on the screen, when turning around the Z axis.

### Why is `deltaTime` still unused?

The spin is a formula of absolute time, like the slide, so it uses `now`.
`deltaTime` is for motion that responds to the player, first used in Phase 38.

## Simple viva modifications

- Spin faster or slower: change `SPIN_SPEED`.
- Spin the other way: make `SPIN_SPEED` negative.
- Stop the spin: set `SPIN_SPEED` to `0.0f`.
- Turn it into a slow clock hand: set `SPIN_SPEED` to `0.5f`.
- Spin exactly on its middle: change the three `y` values in `VERTICES` to
  `0.37, 0.37, -0.73` and watch the wobble nearly disappear.
- Make it circle instead of spin: in `updateScene()`, change
  `scene.triangleModel = slide * spin;` to `spin * slide;` and explain why.
- Turn it around a different axis: change `SPIN_AXIS` to `(0, 1, 0)` and explain
  why it disappears half the time.
- Stop the slide and keep the spin: set `SLIDE_DISTANCE` to `0.0f`.

## Checkpoint

Phase 5 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the triangle spins smoothly while it slides, and never disappears during a
  normal run;
- changing `SPIN_SPEED` changes the spin with no edit to the shader;
- `SPIN_SPEED = 2.0` completes one turn in about `3.14` seconds;
- swapping the order to `spin * slide` makes the triangle circle instead of spin;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 5

No scaling, no view matrix, no projection matrix, and no camera. The triangle is
still drawn straight into the window's flat `-1..+1` space, which is why it
stretches as it turns.

Phase 6 adds `glm::scale`, the full `Translate x Rotate x Scale` order, and a key
that builds the product backwards on purpose so the effect is visible live.
