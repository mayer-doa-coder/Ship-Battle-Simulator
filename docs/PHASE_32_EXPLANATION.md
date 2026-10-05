# Phase 32 - The `K` Term Mask and Blinn-Phong

## Status

Phase 32 is complete and verified. Debug and Release build with no warnings, and 14
checks passed with zero failures - **first time**, which is a first for Stage C.

```text
Ship Battle Simulator - Phase 32 | draws 14 | tris 1840 | verts 1045
```

### THE CHECKPOINT: `K` walks L8 slide 54 live

```text
mask | terms                | mean lum |  max lum | near-black pixels
   1 | ambient only         |     6.12 |     9.28 |          136,992
   2 | diffuse only         |    76.73 |   246.68 |           47,445
   4 | specular only        |     2.37 |   252.72 |          423,292
   3 | ambient + diffuse    |    82.42 |   247.59 |           43,338
   7 | all three            |    84.27 |   255.00 |           37,551
```

Read the `specular only` row twice. Its mean brightness is **2.37** and its peak is
**252.72** - and 423,292 of the scene's 451,419 lit pixels are essentially black. A
highlight is a tiny, blinding thing, and that row is the only way to see it on its
own.

And the claim the slide actually makes was checked pixel by pixel:

```text
ambient + diffuse + specular   vs   all three terms
compared 404,820 unclipped pixels
mean error 0.192 levels, worst 1.0
```

**The three terms add up.** Not approximately - to within rounding, over four
hundred thousand pixels.

## What changed

| File | Change |
|---|---|
| `src/Lighting.h` | New `uTermMask` and `uUseBlinn`; the specular base chooses between `(R·V)` and `(N·H)`; each term is gated by its bit |
| `src/main.cpp` | New `TermMask` namespace with the cycle; `K` and `B` keys; two more uniforms |

No new geometry, no new program, no change to any mesh.

## The one idea

**Three numbers summed into one pixel cannot be taken apart again by looking.**

That is the whole reason this is a key rather than a diagram. Ambient, diffuse and
specular each contribute to every lit pixel, and once they are added the result is a
single colour. The only way to see what each one did is to switch the others off.

```glsl
vec3 result = vec3(0.0);
if ((uTermMask & 2) != 0) result += uKd * lambert;
if ((uTermMask & 4) != 0) result += uKs * specular;
return attenuation * lightColor * result;
```

with the ambient term, which belongs to the scene rather than to a light, gated
separately:

```glsl
vec3 total = ((uTermMask & 1) != 0) ? (uGlobalAmbient * uKa) : vec3(0.0);
```

## The `K` cycle, and why it ends on specular

```cpp
const int CYCLE[4] = { AMBIENT, AMBIENT | DIFFUSE, ALL, SPECULAR };
```

```text
K -> ambient only        what a surface looks like with no directional light at all
K -> ambient + diffuse   the shape appears
K -> all three           the finished picture
K -> specular ONLY       the highlight on its own, against black
K -> back to ambient
```

The order is deliberately not just counting upward. The first three build the
picture up, which is what the slide shows. **Ending on specular-alone is the useful
part**: it is the one term you genuinely cannot pick out by eye once it has been
added to the others, so seeing it isolated is what makes the sum believable.

What each stop tells you, from the measurements:

- **Ambient alone**: mean 6.12, peak 9.28 - a range of about three levels across the
  entire scene. Essentially flat, as something with no direction must be. This is
  also what every surface facing away from both lights is reduced to.
- **Diffuse alone**: mean 76.73. This is where the *shape* lives. Switch it off and
  objects stop looking three-dimensional.
- **Specular alone**: mean 2.37, peak 252.72. Almost nothing, almost everywhere, and
  blinding in a few places.

## Blinn-Phong: the same question asked differently

```glsl
if (uUseBlinn != 0) {
    vec3 H = normalize(L + V);
    base = max(dot(N, H), 0.0);
} else {
    base = max(dot(reflect(-L, N), V), 0.0);
}
```

`H` is the **halfway vector** between the light and the viewer. It is the direction a
surface would have to face for you to be looking straight down the mirror bounce. So
the two formulas ask:

| | Question | Needs |
|---|---|---|
| Phong `(R·V)^n` | how far am I from where the light actually went? | a `reflect()` |
| Blinn `(N·H)^n` | how far is this surface from the orientation that would send the light at me? | one add and one normalize |

Blinn is **cheaper**, and it avoids an artefact Phong can produce: when the light and
the viewer are on nearly opposite sides, `R·V` can go negative and cut the highlight
off abruptly at a glancing angle, where `N·H` stays smooth.

### Measured: same place, 4x broader

```text
the black plastic ball, n_s = 32 in both cases
  Phong       (R.V)^n : peak 243.49   brightest region   28 px
  Blinn-Phong (N.H)^n : peak 244.28   brightest region  116 px

  Blinn's bright region is 4.14x Phong's
  Phong's highlight centred at (302.6, 173.4)
  Blinn's highlight centred at (301.9, 173.6)   - 0.8 px apart
```

Two things in that table matter.

**The highlights are 0.8 pixels apart.** The two formulas agree about *where* the
highlight belongs - which is the check that neither implementation is simply wrong.

**Blinn's is four times broader at the same exponent.** That is not a defect in
either one. The angle between `N` and `H` is roughly **half** the angle between `R`
and `V`, so for the same `n` Blinn's falloff is gentler. Matching them needs Blinn's
exponent to be about 2 to 4 times Phong's.

This is exactly why `B` is a **comparison** key and not a drop-in replacement. If the
project simply switched to Blinn, every `n_s` in the L8 slide 60 table would silently
mean something different - and those numbers are cited evidence.

## Likely teacher questions

### What does the `K` key show?

L8 slide 54, in this scene instead of on the slide's test spheres. Four stops:
ambient alone, ambient plus diffuse, all three, then specular alone against black.

### Why does it need to be a key?

Because three numbers summed into one pixel cannot be separated by looking. Measured,
specular alone has a mean brightness of 2.37 and a peak of 252.72 - that is invisible
as a contribution to the full picture, and unmistakable on its own.

### Why does the cycle end on specular rather than building up and stopping?

Because specular is the term you cannot pick out by eye once it is added. Seeing it
isolated is what makes the sum believable.

### How do you know the terms really add?

Each one was rendered alone and the three images were summed and compared against the
all-three render, pixel by pixel. Over 404,820 unclipped pixels the mean error is
**0.192 levels out of 255** and the worst is 1.0 - rounding, nothing more.

### What does ambient alone look like, and why?

Almost perfectly flat: mean 6.12, peak 9.28, a range of three levels across the whole
scene. It has no direction, because it is standing in for light that has bounced off
other surfaces - which a local illumination model cannot compute (L8 s10-11).

### What is Blinn-Phong?

The same specular idea with a different measurement. `H = normalize(L + V)` is the
halfway vector between the light and the viewer, and `(N·H)^n` asks how far the
surface is from the orientation that would reflect the light at you, where `(R·V)^n`
asks how far you are from where the light went.

### Why would you use it?

It is cheaper - one add and one normalize instead of a `reflect()` - and it does not
suffer Phong's abrupt cut-off when the light and viewer are on nearly opposite sides.

### Is it the same result?

No, and that is the point of the comparison. Measured on the same ball with the same
`n_s = 32`, Blinn's highlight is **4.14 times broader**, because the angle it measures
is about half the angle Phong measures. Its centre is only 0.8 pixels away, so they
agree about *where* - just not about *how wide*.

### So why not just switch to Blinn permanently?

Because every `n_s` in the L8 slide 60 table would then mean something different, and
those numbers are cited evidence in the report. Matching Blinn to Phong needs the
exponent multiplied by roughly 2 to 4.

## Simple viva modifications

- **The named exercise: change the `K` cycle order.** `TermMask::CYCLE` in
  [src/main.cpp](../src/main.cpp). Try `{ SPECULAR, DIFFUSE, AMBIENT, ALL }` to walk
  the terms individually before summing them.
- **Add a stop:** the mask is three independent bits, so `DIFFUSE | SPECULAR` -
  mask 6 - shows the scene with no ambient at all. Every surface facing away from
  both lights goes pure black, which is the Phase 27 argument in one image.
- **Match Blinn to Phong:** press `B`, then raise `SHININESS`-equivalent values in
  [src/Material.h](../src/Material.h) by about 3x and watch the highlights line up
  again.
- **Watch the artefact Blinn avoids:** orbit until the light is almost behind the
  object and switch between `B` states at a glancing angle.
- **Combine with `L`:** specular-only plus point-light-only isolates one highlight
  from one light - four keys away from the finished picture.
- **Combine with `1`/`2`/`3`:** specular-only in Gouraud mode on coarse geometry is
  the starkest possible version of Phase 31's point: there is simply nothing there.

## Checkpoint

Phase 32 passes when:

- Debug and Release build with no compiler warnings;
- `K` walks ambient / +diffuse / all / specular-alone, and prints which;
- the three terms **sum to the full picture** - measured, mean error 0.192 levels
  over 404,820 pixels;
- switching off one bit removes exactly that one term - measured, worst difference
  1.0 level;
- ambient alone is flat (range 3 levels), and specular alone leaves 94% of the scene
  black while still peaking above 250;
- `B` switches between `(R·V)^n` and `(N·H)^n`, and the two put the highlight in the
  same place - measured 0.8 pixels apart - while Blinn's is 4.14x broader at the same
  exponent;
- every earlier key still behaves as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 32

No new feature at all after this - Phase 33 adds **nothing**. It is a verification
phase: the two formal demonstrations get placed, measured and photographed. Demo A is
the grid at low tessellation with `n_s = 160`, where the highlight vanishes in Gouraud
and returns in Phong. Demo B is the cylinder at 6 to 8 segments, where Flat shows Mach
bands, Gouraud smears the highlight and Phong keeps it round.

After Phase 33, nothing in the project may break either demonstration.

---

## Note: verification

### 14 checks, no failures, and nothing to correct

This is the first Stage C phase whose verification was right the first time. That is
worth a note, because it was not luck - it is because this phase's claims are all of
the form "these images sum to that image", which is an arithmetic statement about
pixels and needs no reasoning about geometry, cameras or placement. Every earlier
Stage C phase went wrong on exactly that kind of reasoning:

| Phase | What my test got wrong |
|---|---|
| 26 | demonstrated on an object that physically cannot show the bug |
| 28 | used a control that moves for its own reasons |
| 29 | placed the demo objects where the highlight is invisible |
| 30 | fitted one term of a model whose brightness is mostly another term, and patched a file after writing it |
| 31 | four assumptions about what Flat and Gouraud would look like |

The lesson is not that this phase was easier. It is that **a test comparing images to
each other is far harder to get wrong than a test comparing an image to a prediction**
- and when a prediction is needed, it has to predict the *whole* model, not a part of
it.

### The strongest single measurement in Stage C

```text
ambient + diffuse + specular  vs  all three
mean error 0.192 levels out of 255, worst 1.0, over 404,820 pixels
```

Clipped pixels were excluded, because a channel already at 255 cannot get brighter
and would fail the sum for a reason that has nothing to do with the model.

### Earlier phases still correct

14 draws, 1840 triangles, 1045 vertices - unchanged since Phase 29. The `N` view, `M`,
`L`, `1`/`2`/`3`, `+`/`-`, `W`, `D` and `O` all behave as before.
