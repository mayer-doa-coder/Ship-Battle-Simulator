# Phase 42 - Anti-Aliasing: 4x MSAA, with a Clean Fallback

## Status

Phase 42 is complete and verified.

```text
Showcase : draws 15 | tris 496 | verts 507      (unchanged)
Console  : [msaa] requested 4 samples per pixel, the window has 4
```

No new mesh, shader or draw call. 19 image checks plus the build gate passed with zero failures.

## The one idea

**Anti-aliasing is a property of the window's pixel format, so it is requested when the window is
created - not applied to the picture afterwards.**

```cpp
glfwWindowHint(GLFW_SAMPLES, AppConfig::MSAA_SAMPLES);     // 4
```

With 4 samples each pixel records 4 coverage samples instead of 1. A pixel half-covered by a
triangle comes out half its colour instead of all or nothing, so edges have five levels of
coverage (0, 1/4, 1/2, 3/4, full) where they had two. Each pixel is still *shaded* once - the
fragment shader runs once per pixel, not four times - so the cost is framebuffer memory and
bandwidth, not lighting. That is why this is **not a post-process**: nothing reads the finished
image and blurs it (Phase 42 is the brief's "anti-aliasing where available", and the project's
"no post-processing" rule stands).

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | `AppConfig::MSAA_SAMPLES`; the `GLFW_SAMPLES` hint; a retry without samples if the window is refused; a `[msaa]` report of what the window really has; `glEnable`/`glDisable(GL_MULTISAMPLE)` per scene in `renderScene()` |

## Three behaviours

### 1. It is a request, and the program reads back what it got

GLFW treats `GLFW_SAMPLES` as a preference and picks the closest format the driver offers. On this
machine:

```text
asked for 0  -> window has none
asked for 4  -> window has 4
asked for 8  -> window has 8
asked for 16 -> window has 16
asked for 64 -> window has 16     (the closest the driver has)
```

So the hint almost never fails; what matters is what came back. The program asks
`glGetIntegerv(GL_SAMPLES)` once there is a context and prints it - `has 4`, or `has none -
rendering without anti-aliasing` - so a screen recording's console shows which one you are
looking at.

### 2. The fallback

If a driver *refuses* the multisampled format outright, `glfwCreateWindow` returns null. The
program then says so, asks for `GLFW_SAMPLES = 0` and tries again; only if that also fails does it
give up (exit code 1, "Failed to create the OpenGL window"). Anti-aliasing improves the picture;
it is never a requirement for having one.

**This machine's driver never refuses**, so the retry could not be reached by ordinary means. The
test harness therefore has a `-failcreate N` switch that makes the first `N` window creations
return null:

```text
-failcreate 1 : "[msaa] the driver refused a 4-sample window - trying again without"
                "[msaa] ... the window has none - rendering without anti-aliasing"
                the picture is byte-identical to a window created without samples,
                and byte-identical to the Phase 41 showcase
-failcreate 2 : exit code 1 and "Failed to create the OpenGL window."
```

That is a simulated refusal, tested through the real code path - not a real driver refusing.

### 3. The showcase is smoothed; the gallery is not

`renderScene()` calls `glEnable(GL_MULTISAMPLE)` in the showcase and `glDisable(GL_MULTISAMPLE)`
in the gallery. Disabling it makes a multisampled window rasterise as if it had one sample.

Why the gallery is exempt: Demo A and Demo B were measured pixel by pixel (Gouraud's highlight
peaks at 186 and Phong's at 255; Flat uses 42 brightness levels against Phong's 216) on images
with hard, single-sample edges, and every phase since proves it has not disturbed them by
comparing renders byte for byte. Smoothing every edge in the gallery would change those bytes
without changing any lesson.

## Evidence

### The showcase is measurably smoother

The "partly-covered edge pixel" metric (defined in the test tool: across a real edge, a pixel
that lies *between* its neighbours and differs from both) counts pixels an aliased edge cannot
produce:

```text
ship alone       without MSAA: 524      with 4x MSAA: 3040
whole showcase   without MSAA: 570      with 4x MSAA: 3812
```

Rising from 524 to 3040 is a factor of 5.8. Only edges change: **5148 of 921,600 pixels (0.56%)**
differ between the two renders, by up to 159 in a colour channel - an interior pixel never does.

### Demo A and Demo B are unchanged

With the ship hidden, in the production configuration (a 4-sample window, gallery on), all ten
regression views are **byte-identical to the real Phase 33**. Demo A's sea and Demo B's tube are
among them, in Flat, Gouraud, Phong, Blinn-Phong and the normals view. In a window created with
no samples (the harness setting the plan prescribes) they are identical too.

### What is *not* identical - and the measured size of it

Here a result went against what I expected, and it is recorded rather than hidden.

I expected `glDisable(GL_MULTISAMPLE)` to make the gallery *exactly* what it was, ship included.
It is not. With the **ship shown**, in a 4-sample window with multisampling disabled:

```text
10 views vs Phase 37: 2375 differing pixels in total, 330 in the worst view
every one of them lies inside the ship's own bounds; 0 outside it
```

In a window created *without* samples, the same ten views are byte-identical to Phase 37.

The pixels are a diagonal line (for example `(541,467)`, `(540,468)`, `(539,469)` ...). The gallery
ship is rolled exactly 45 degrees (`PROOF_ROLL`), so some of its edges run at exactly 45 degrees
through the centres of pixels. For a pixel centre that lies exactly *on* an edge, which triangle
owns it is a tie-break, and this driver breaks that tie differently in a multisampled window with
multisampling disabled than in a window with no samples. No gallery object has an edge that falls
exactly on pixel centres in these ten views, which is why they match exactly. This is my
explanation, consistent with the evidence (a one-pixel-wide diagonal, only on the 45-degree ship);
I did not prove it at the rasteriser level.

What this means for the evidence:

- The **regression against Phase 33 is exact** in both configurations - the thing the plan asks
  for.
- The gallery's *ship* (the Phase 37 hierarchy proof) can differ by up to 330 tie-break pixels from
  the old picture in a 4-sample window. The proof itself - frames, rigidity, `H` - does not depend
  on pixels and is untouched.
- That `glDisable(GL_MULTISAMPLE)` gives exact single-sample output for ordinary geometry is
  **a measured property of this driver**, not something the OpenGL specification guarantees. On
  another GPU the gallery's edges could differ. If that mattered, the fix is to create the window
  without samples (set `MSAA_SAMPLES` to 0) when running the regression, which is what the harness
  does.

## Likely teacher questions

### Is this a post-process?
No. Nothing reads the finished image. The window is created with 4 coverage samples per pixel and
the hardware resolves them to one colour when the frame is shown.

### Does it make the shading four times slower?
No. The fragment shader still runs once per pixel; only coverage and depth are stored four times.
Measured here: the picture changes on 0.56% of pixels, all of them on edges.

### What if the computer does not support it?
The program asks for it, reads back what it got, and prints which. If the window is refused it
retries without samples. With none, it renders exactly as in Phase 41 (byte-identical).

### Why is the gallery not smoothed?
Its edges are part of Stage C's evidence, measured and compared byte for byte. The showcase is the
presentation scene, and it gets the smoothing.

### Why can the gallery's ship differ by a few hundred pixels?
Its 45-degree roll puts edges exactly on pixel centres, where the rasteriser's tie-break differs
between the two modes. It is bounded (330 pixels at most) and confined to the ship.

## Simple viva modifications

- **The named exercise: turn MSAA off.** `AppConfig::MSAA_SAMPLES = 0` in
  [src/main.cpp](../src/main.cpp). The console reports "the window has none" and the showcase's
  edges go back to Phase 41's.
- **Ask for 8:** set it to 8. On this machine the window has 8; on another it may have fewer, and
  the `[msaa]` line says so.
- **See the difference:** the edge of a sail against the sea, zoomed in with `E`, is a staircase
  at 0 and a soft ramp at 4.
- **Show the fallback:** run the harness with `-failcreate 1` (a test-only switch).

## Checkpoint

Phase 42 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the window reports 4 samples, and reports honestly when it has none;
- partly-covered edge pixels rise measurably (524 to 3040) and only edges change;
- with the ship hidden the gallery is byte-identical to Phase 33 - in a 4-sample window and in a
  window with none - so Demo A and Demo B are unchanged;
- with the ship shown it is byte-identical to Phase 37 in a window with no samples, and in a
  4-sample window every difference lies inside the ship and totals at most 330 pixels a view;
- a refused window falls back to none and gives the Phase 41 picture; a second refusal exits with
  code 1;
- `G` switches the profile at run time, byte for byte in both directions;
- the showcase is still 15 draws / 496 triangles / 507 vertices.

## What is not part of Phase 42

No MSAA toggle key: the sample count is fixed when the window is created, so only the per-scene
switch is live. No anti-aliasing of the gallery. No measurement of the frame-time cost (Phase 103).

**Phase 43 is next**: the golden-hour lighting profile - a warm low sun and a cool sky-coloured
ambient for the showcase, with the Stage C lighting returning with `G`.
