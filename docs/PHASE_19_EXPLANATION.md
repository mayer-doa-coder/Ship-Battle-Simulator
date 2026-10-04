# Phase 19 - The Wireframe Comparison (and a Bug Fixed)

## Status

Phase 19 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run behaved correctly. This phase is marked `(o)` in the plan - the
smallest kind - and it is unusual in two ways:

- **It needs almost no new code.** The `W` key and
  `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` have existed since Phase 11. What
  this phase adds is the *comparison*, and one printed line to make it
  self-checking.
- **It also fixes the oldest open defect in the project**, carried since Phase 12
  and recorded as finding F1 in [STAGE_A_REVIEW.md](STAGE_A_REVIEW.md).

---

# Part 1: the bug fix

## What was wrong

[src/main.cpp](../src/main.cpp), in `updateScene()`:

```cpp
const float scalePulseMid =
    (TriangleScale::PULSE_MIN + TriangleScale::PULSE_MAX) * 0.1f;   // should be 0.5f
```

That `0.5f` is a **midpoint**: halfway between the two ends of the pulse. Written
as `0.1f` it produced a midpoint of `0.18` instead of `0.9`, so the triangle's
scale swung from `-0.22` to `+0.58` instead of the intended `0.5` to `1.3`.

It arrived in the Phase 12/13 commit, unrelated to anything that commit was
about, and survived five phases. It was held back deliberately through Phases 14
to 18 so that those phases' frame-by-frame pixel comparisons stayed exact - a
refactor that is supposed to change nothing can only be checked against an
unchanged baseline. All of those comparisons are done, so the fix lands here.

## The fix, and the proof

One character. To measure the difference rather than assume it, two builds were
made from the same source - one with the bug restored - and both printed the
scale factor they actually used while rendering the same 20 instants across one
full pulse period (`2 pi / 1.2 = 5.236 s`):

| `t` | fixed | buggy |
|---:|---:|---:|
| 0.0000 | 0.9000 | 0.1800 |
| 1.3090 | **1.3000** | **0.5800** |
| 2.6180 | 0.9000 | 0.1800 |
| 3.1416 | 0.6649 | **-0.0551** |
| 3.9270 | **0.5000** | **-0.2200** |
| 4.7124 | 0.6649 | -0.0551 |

The fixed build's range is **exactly** `PULSE_MIN` to `PULSE_MAX` - `0.5000` to
`1.3000` - which is what the code's own comment promises: *"keeps the factor
inside [PULSE_MIN, PULSE_MAX] without a clamp"*. That comment was false for five
phases and is true again.

The buggy build's range is `-0.2200` to `+0.5800`, matching the Stage A review's
prediction to four decimal places.

### What that looked like on screen

The two triangles were rendered alone at each of those 20 instants and their
pixels counted:

```text
FIXED : smallest  5 943 px, largest 85 698 px
BUGGY : smallest     66 px, largest 19 914 px
```

- The buggy triangle **collapses to 66 pixels** - a dot - because its scale
  passes through zero twice per period. The fixed one never falls below 5 943.
- At its largest the fixed triangle is **4.3x** the buggy one's largest.
- At its smallest it is **90x** bigger.

A negative scale factor also point-reflects the shape, so the buggy triangle came
back **inside out** after each collapse. It was never culled, because
`scale(s, s, 1)` has determinant `s * s`, which stays positive however negative
`s` is - so the winding survived and nothing looked broken enough to notice.

## Why this one was hard to see

It is worth knowing why a bug this large hid for five phases.

- The triangle is the project's **temporary test object** (Phase 2). Nobody is
  looking at it closely; attention has been on the cube, the quad and the grid.
- It is *supposed* to pulse. A pulsing thing that pulses wrongly still pulses.
- The collapse is brief and the recovery is symmetric, so it reads as "the
  animation dips" rather than "the arithmetic is wrong".
- Every automated check in Phases 14 to 18 compared *against the buggy build*,
  on purpose. A refactor test can only tell you nothing changed - it cannot tell
  you the thing it preserved was right.

That last point is the real lesson: **"identical to last phase" and "correct" are
different claims.** Phases 14 to 18 proved the first, rigorously, while this
defect sat untouched inside the thing being preserved. Catching it needed someone
to compare the code against its own stated intent, which is what the Stage A
review did.

[STAGE_A_REVIEW.md](STAGE_A_REVIEW.md) has been updated to mark F1 fixed.

---

# Part 2: Phase 19, the wireframe comparison

## The one idea

Phase 18 introduced a mesh whose geometry depends on a number. This phase answers
the obvious follow-up: **how do you actually see that the number did something?**

In solid shading, mostly you cannot. Phase 18 measured the grid at `N = 4` and
`N = 32` covering **exactly the same pixels**, with colours differing by one level
out of 255. A flat surface looks the same however finely it is cut.

Wireframe is the answer, and the reason it works is that it draws **edges**, which
is the one thing subdivision really changes.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | **Bug fix:** the pulse midpoint `0.1f` to `0.5f` |
| `src/main.cpp` | The `W` key now also prints the grid's expected counts and the arithmetic behind them |
| everything else | Unchanged. `glPolygonMode` and the `W` key are Phase 11's |

Pressing `W` now prints:

```text
[wireframe] ON, culling OFF (every triangle's outline is visible, front and back)
            grid: CELLS = 8, so expect 8 x 8 = 64 quads, 2 x 8 x 8 = 128 triangles, and (8 + 1)^2 = 81 vertices
```

Those numbers come from `GridConfig`, so they are the **prediction**. The counts
the mesh itself reported at startup are what they must match:

```text
[mesh] grid     vertices=81 indices=384 triangles=128
```

That is the phase's viva exercise - *count the triangles and check against
`2 * N * N`* - with the arithmetic put next to the picture so it can be checked
in one glance. Verified at `N = 4`, `8` and `32`; the printed prediction matches
the mesh's own report in every case.

(The *live, per-frame* counters in the window title are a different thing and
belong to Phase 24. This is one line, printed once, on a key press.)

## The comparison, measured

Grid alone, camera lifted to look down on it, same scene otherwise:

| `N` | solid footprint | wireframe edge px | ink density | unique edges | triangles |
|---:|---:|---:|---:|---:|---:|
| 1 | 334 340 px | 2 108 | 0.6% | 5 | 2 |
| 2 | 334 338 px | 3 852 | 1.2% | 16 | 8 |
| 4 | 334 338 px | 7 312 | 2.2% | 56 | 32 |
| 8 | 334 338 px | 13 418 | 4.0% | 208 | 128 |
| 16 | 334 338 px | 26 179 | 7.8% | 800 | 512 |
| 32 | 334 338 px | 50 203 | **15.0%** | 3 136 | 2 048 |

Two things to read off it:

- **The solid footprint never changes.** 334 338 pixels at every `N` from 2 to
  32. (`N = 1` differs by 2 pixels out of 334 340 - sub-pixel rasterisation along
  its single diagonal, not a real difference.) Subdivision changes nothing about
  where the grid is or how much ground it covers.
- **The wireframe changes enormously.** From 0.6% of the footprint inked to 15.0%
  - about 24 times as many edge pixels on exactly the same patch of floor. At
  `N = 4` you can count the cells; at `N = 32` it reads as a dense mesh.

That contrast is the checkpoint: `N = 4` and `N = 32` are *obviously* different
in wireframe and *almost indistinguishable* without it.

## Counting by eye at `N = 4`

From above, a grid at `N = 4` is a 4-by-4 arrangement of cells with one diagonal
across each:

```text
    +---+---+---+---+        16 cells, each cut in two
    | / | / | / | / |
    +---+---+---+---+        16 x 2 = 32 triangles
    | / | / | / | / |
    +---+---+---+---+        and 2 x 4 x 4 = 32 from the formula
    | / | / | / | / |
    +---+---+---+---+        5 x 5 = 25 vertices
    | / | / | / | / |
    +---+---+---+---+
```

Count the cells (16), double it (32), and check against `2 * N * N` = `2 * 4 * 4`
= 32. The `[mesh] grid` line at startup says `triangles=32`, and the `W` key
prints the same prediction. Three independent routes to one number.

At `N = 32` there are 2 048 triangles, which is exactly why you check the formula
at `N = 4` and then trust it.

## A surprise worth understanding: the ink grows with `N`, not `N squared`

The triangle count quadruples when `N` doubles - `2N^2`. The measured edge pixels
only **double**:

```text
N:        1      2      4      8     16     32
edge px:  2108   3852   7312   13418  26179  50203
ratio:         1.83   1.90   1.83   1.95   1.92
```

That looks wrong at first and is not. **Edge *count* grows like `N^2`, but total
edge *length* grows like `N`**, and pixels follow length.

Work it out for a grid of width `L`:

```text
(N+1) lines across  x, each the full width      ->  (N+1) L
(N+1) lines across  z, each the full width      ->  (N+1) L
N^2   diagonals, each only L/N long, times √2   ->  N^2 x (L/N) x √2  =  √2 N L
                                                    ------------------------------
total length                                    =  L [ 2(N+1) + √2 N ]
```

The `N^2` cancels, because each new diagonal is shorter in exact proportion to
there being more of them. Dividing the measured pixels by that predicted length
should give a constant - the width of a drawn line - and it does:

| `N` | predicted length (in widths of `L`) | measured px | px per unit length |
|---:|---:|---:|---:|
| 2 | 8.83 | 3 852 | 436 |
| 4 | 15.66 | 7 312 | 467 |
| 8 | 29.31 | 13 418 | 458 |
| 16 | 56.63 | 26 179 | 462 |
| 32 | 111.26 | 50 203 | 451 |

Constant to within about 4% across a 16-fold change in `N`. The model is right.

This matters beyond being a neat fact. **Wireframe density is not a measure of
triangle count** - it understates it badly. A wireframe that looks "twice as
busy" has four times the triangles. For judging cost, read the number the mesh
reports; use the wireframe to see *structure*.

## Why wireframe needs culling off as well

Unchanged from Phase 11, but it is the mechanism this phase depends on, so it is
worth restating: the `W` key changes **two** things together.

```cpp
if (scene.wireframeEnabled) {
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
} else {
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
```

Culling happens **before** rasterisation, so a triangle discarded for facing away
is gone before the polygon mode gets a say. Line mode alone would not reveal it.
For the grid specifically this is what lets you inspect the lattice from
underneath as well as from above.

## Likely teacher questions

### Why does the grid look the same at `N = 4` and `N = 32` in solid shading?

Because it is flat and its colour barely curves, so more triangles approximate
the same surface. Measured: the footprint is identical to the pixel, and the
colours differ by one level out of 255.

### How do you show that `N` changed anything, then?

Press `W`. Measured on the same patch of floor, `N = 4` inks 0.6% of the
footprint and `N = 32` inks 15.0% - about 24 times the edge pixels.

### How do you check the triangle count by eye?

At `N = 4`, count the cells from above: 4 by 4 is 16, each cut by one diagonal, so
32 triangles. Check against `2 * N * N = 32`. The startup line and the `W` key's
printed prediction both say 32 as well.

### Why does the wireframe only get twice as dense when the triangle count quadruples?

Because pixels follow total edge *length*, not edge count, and each new diagonal
is shorter in proportion to there being more of them. The `N^2` cancels and total
length grows like `N`. Verified: dividing measured pixels by predicted length
gives a constant line width (451-467) across a 16-fold change in `N`.

### So is wireframe a good way to judge polygon count?

No, and that is worth saying. It understates it: twice as busy means four times
the triangles. Use it to see structure; read the mesh's reported count for cost.

### Why does wireframe switch culling off too?

Because culling discards a triangle before rasterisation, so line mode never gets
to draw it. Switching both together is what lets back faces be inspected
(Phase 11).

### What was the bug that got fixed here?

A midpoint written as `0.1f` instead of `0.5f`, so the test triangle's pulse ran
from `-0.22` to `+0.58` instead of `0.5` to `1.3`. It was under half its intended
size, collapsed to a 66-pixel dot twice per period, and came back inside out.

### Why wasn't it caught by the checks in Phases 14 to 18?

Because those checks compared each phase against the *previous* one, to prove a
refactor changed nothing - and the defect was inside the thing being preserved.
"Identical to last phase" and "correct" are different claims. It took reading the
code against its own stated intent to find it.

## Simple viva modifications

- **The named exercise:** set `GridConfig::CELLS` to `4`, rebuild, press `W`,
  drag the mouse up to look down, and count the cells. Then check your count
  against the `2 * N * N` line the `W` key prints.
- **Then set it to `32`** and press `W` again. Same ground, 64 times the
  triangles, and no longer countable - which is the point.
- **Show solid mode cannot tell:** switch between `4` and `32` with `W` **off**.
  The picture is effectively identical.
- **Prove culling is the other half:** comment out the `glDisable(GL_CULL_FACE)`
  line, rebuild, and look at the grid from underneath in wireframe. The outlines
  are gone, which shows polygon mode was never the thing hiding them.
- **Check the ink argument:** compare `N = 8` and `N = 16` in wireframe. It looks
  about twice as dense, not four times, even though there are four times the
  triangles.
- **Demonstrate the fixed pulse:** watch the triangle for one full pulse (about
  5.2 seconds). It should swell and shrink smoothly between half and 1.3 times
  its size, and never vanish or flip. Setting the midpoint back to `0.1f` shows
  what it used to do.

## Checkpoint

Phase 19 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the test triangle pulses smoothly between `0.5` and `1.3` times its size,
  **never collapsing to a point and never turning inside out**;
- `N = 4` and `N = 32` are obviously different in wireframe, and nearly
  indistinguishable without it;
- pressing `W` prints the grid's expected quad, triangle and vertex counts, and
  they match the `[mesh] grid` line printed at startup;
- the triangle count at `N = 4` can be counted by eye and checked against
  `2 * N * N`;
- every earlier object and key still behaves as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 19

No runtime change of `CELLS` - the `+` and `-` keys that rebuild meshes live are
Phase 25, so comparing `4` with `32` still means editing the value and
rebuilding. No live counters in the window title; that is Phase 24, and this
phase's one printed line is a one-off on a key press, not a per-frame HUD. No new
geometry: the cylinder and sphere are Phases 20 to 22. No lighting.

Phase 20 adds `makeCylinder(segments)`: the side wall only, a ring of vertices
from `sinf`/`cosf` with the analytic normal `normalize(vec3(x, 0, z))` - the
project's first **curved** surface, and the first mesh whose normals are not all
the same.
