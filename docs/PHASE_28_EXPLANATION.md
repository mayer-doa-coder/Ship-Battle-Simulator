# Phase 28 - The Specular Highlight

## Status

Phase 28 is complete and verified. It is a **graded milestone** (`*` in the plan).
Debug and Release build with no warnings, and 17 checks passed with zero failures.

```text
Ship Battle Simulator - Phase 28 | draws 11 | tris 652 | verts 445
```

This is the first thing in the whole project whose appearance depends on **where
you are looking from**. Everything before it looked the same from every angle.

### THE CHECKPOINT: the highlight moves when the camera orbits

Rather than just confirming that something moves, the highlight's position was
**predicted from theory** and compared with the rendered pixels. For a sphere the
specular peak is the surface point whose normal is `normalize(L + V)`:

```text
yaw | measured spot   | predicted spot  | miss
  0 |  663.2,  374.8  |  663.9,  375.4  |  0.9 px
 15 |  654.1,  373.5  |  654.9,  374.1  |  1.0 px
 30 |  644.8,  372.8  |  645.5,  373.4  |  0.9 px
 45 |  635.5,  372.7  |  636.0,  373.3  |  0.8 px
 60 |  626.3,  373.3  |  626.6,  373.9  |  0.6 px
```

**Worst disagreement with theory: 1.0 pixel.** Not merely "it moves" - it is in
exactly the right place at every angle.

And it really is a different *surface point* each time:

```text
yaw  0 : the specular peak is 21.3 deg away from the diffuse peak
yaw 30 : the specular peak is 10.7 deg away from the diffuse peak
yaw 60 : the specular peak is 14.3 deg away from the diffuse peak
```

The diffuse peak never moves on the ball. The specular peak slides over an 11
degree arc as the camera swings through 60 degrees.

## What changed

| File | Change |
|---|---|
| `shaders/basic.vert` | New `out vec3 vWorldPos` |
| `shaders/basic.frag` | New `in vec3 vWorldPos`, `uKs`, `uShininess`, `uViewPos`; the specular term |
| `src/main.cpp` | `SurfaceConfig::SPECULAR_REFLECTANCE` and `SHININESS`; `SceneState::viewPos`; three more uniforms |

Still no new geometry: 11 draws, 652 triangles, 445 vertices, unchanged since
Phase 26.

## The one idea

**Diffuse light scatters everywhere; specular light goes somewhere.**

A diffuse surface looks the same from every direction, because it throws the light
it receives equally in all directions. A *shiny* surface throws it in a preferred
direction - near the mirror bounce - so whether you see a bright spot depends on
whether you happen to be standing in the path of that bounce.

```glsl
vec3  R  = reflect(-L, N);                 // the light, bounced off the surface
vec3  V  = normalize(uViewPos - vWorldPos); // from the surface toward the viewer
float rv = max(dot(R, V), 0.0);
float specular = (lambert > 0.0) ? pow(rv, uShininess) : 0.0;
```

`R · V` asks "how closely does the bounce line up with you?" - 1 if you are exactly
in the path, falling off as you move away. Raise that to a power and you get a
spot.

The full model now has all three of L8's terms:

```text
I = globalAmbient * Ka  +  sunColour * Kd * max(N.L, 0)  +  sunColour * Ks * (R.V)^n
    ^^^^^^^^^^^^^^^^^^     ^^^^^^^^^^^^^^^^^^^^^^^^^^^     ^^^^^^^^^^^^^^^^^^^^^^^^
    flat, everywhere       the shape; same from anywhere    a spot; MOVES with you
```

## Why the shininess exponent makes it *tighter*, not brighter

`uShininess` is `n_s` in L8's notation, and it is easy to misread as a brightness
control. It is not.

`R · V` is at most 1, so it is a number between 0 and 1. Raising a number below 1
to a higher power makes it **smaller**, and it shrinks fastest where it was already
furthest from 1:

| `R·V` | `^2` | `^32` | `^160` |
|---|---|---|---|
| 1.00 | 1.00 | 1.00 | 1.00 |
| 0.95 | 0.90 | 0.19 | 0.0003 |
| 0.80 | 0.64 | 0.001 | ~0 |

At the exact centre of the highlight nothing changes. Everywhere else it collapses.
So a high exponent keeps only the directions almost perfectly aligned with the
bounce - a small hard spot, a polished surface - and a low one leaves a broad
sheen.

Measured against the diffuse-only control, with `n_s = 32`:

```text
            max luminance    bright area
with Ks           244-246       26-33 px
with Ks = 0           119    1536-1999 px
```

Twice as bright, and **53 to 76 times tighter**. That is the difference between a
highlight and a sheen, in two numbers.

## Three details that are each a bug

**`reflect(-L, N)`, not `reflect(L, N)`.** GLSL's `reflect` wants the *incoming*
direction - the way the light travels toward the surface. `L` points the other way,
from the surface toward the light, so it is negated.

**Gate the specular on the surface being lit.**

```glsl
float specular = (lambert > 0.0) ? pow(rv, uShininess) : 0.0;
```

This is not in the bare formula, and leaving it out produces a real artefact: `R`
and `V` can line up on a surface that faces **away** from the light, so a shiny
object glints on its own dark side. Checking that the surface is actually receiving
light first removes it.

**`uViewPos` must be refreshed every frame.** This is one of the project's standard
bugs, and it is nasty precisely because nothing breaks: the scene renders perfectly
and the highlight simply refuses to move. It looks like the specular term not
working rather than like one value being out of date.

The defence is structural - `updateScene()` already works out the camera position
to build the view matrix, and the *same variable* is stored:

```cpp
glm::vec3 eye = orbitCameraPosition(scene.camera);
scene.view = glm::lookAt(eye, CameraConfig::TARGET, CameraConfig::UP);
scene.viewPos = eye;        // the identical value, so they cannot disagree
```

## Why the highlight is not the object's colour

```glsl
vec3 diffuse = uSunColor * uKd * base * lambert;   // base = the surface's colour
vec3 spec    = uSunColor * uKs * specular;         // no base
```

Look at what `spec` is **not** multiplied by: the surface's own colour. A highlight
is the colour of the **light**, not of the object - which is why a shiny red ball
has a *white* highlight, not a red one. It is light bouncing straight off the
surface without being absorbed into it.

`Ks` is there to let a surface tint it slightly if it should, which is how metals
differ from plastics. Look at L8 slide 60, arriving in Phase 29: black plastic has
`k_d` of `(0.01, 0.01, 0.01)` - essentially no diffuse colour at all - but `k_s` of
`(0.50, 0.50, 0.50)`. It is black and still shiny.

## Why the fragment needs its world position

The specular term needs `V`, the direction from *this surface point* to the camera.
That means the surface point itself must be available per fragment, so the vertex
shader passes it on:

```glsl
vWorldPos = vec3(uModel * vec4(aPosition, 1.0));
```

Two things to notice. **`w = 1.0`**, because this is a position and the translation
part of `uModel` must apply to it - compare `vNormal` just above it, which is a
direction and uses a `mat3` precisely so no translation can reach it. And **only
`uModel`**, not `uView` or `uProjection`: the sun direction, the normal and the
view position must all live in the same space, and this project uses world space
throughout. Mixing spaces is the classic way to get a highlight that drifts as the
camera turns.

## A result that looks backwards, and why it is right

Over 60 degrees of orbit:

```text
specular peak travels  37.5 px across the screen
diffuse  peak travels  73.3 px across the screen
```

The specular highlight moves **less** on screen than the fixed diffuse peak. That
seems wrong until you see why: **a highlight tends to stay facing you.** As you
orbit, it slides across the surface to keep pointing roughly your way, so it hugs
the middle of the visible disc - while a point glued to the surface swings right
across it and out of sight.

So the signature of view dependence is not "it moves a lot". It is "**it moves
differently from a point stuck to the surface**". That is what the checkpoint
measures, and it is what you are actually watching when you drag the mouse.

## Likely teacher questions

### What is the specular term for?

Shininess. A diffuse surface scatters light equally in every direction so it looks
the same from anywhere; a shiny one throws light near the mirror bounce, so the
bright spot depends on where the viewer is.

### Why `(R·V)^n`?

`R` is the light bounced off the surface and `V` points at the viewer, so `R · V`
measures how closely you are standing in the path of the bounce. The exponent
decides how quickly it falls off as you move out of that path.

### What does raising the shininess do - make it brighter?

No, **tighter**. `R·V` is at most 1, so a higher power shrinks everything except the
exact centre. At `n = 32`, `R·V = 0.95` becomes 0.19; at `n = 160` it becomes
0.0003. The peak stays at 1 either way.

### Why multiply by `lambert`?

To stop a shiny object glinting on its own dark side. `R` and `V` can happen to
line up on a surface facing away from the light, which would produce a highlight
where no light is arriving.

### Why is the highlight white rather than the object's colour?

Because `spec` is not multiplied by the surface colour. A highlight is light
bouncing off the surface rather than being absorbed into it, so it takes the colour
of the light. That is why a shiny red ball has a white highlight.

### Why does the fragment shader need the world position?

To work out `V`, the direction from this particular surface point to the camera.
Only `uModel` is applied - not view or projection - because the sun, the normal and
the view position all have to be in the same space for the dot products to mean
anything.

### What goes wrong if `uViewPos` is stale?

Nothing visible breaks. The scene renders and the highlight just stops moving,
which looks like the specular term failing rather than one value being out of date.
It is stored from the same variable the view matrix is built from, so the two
cannot disagree.

### Show me that it moves.

Drag the mouse and watch the ball and the tube. The spot slides across them.
Measured: it sits within 1 pixel of where the maths predicts at every angle, and
the surface point it sits on moves over an 11 degree arc during a 60 degree orbit.

### Why does the highlight move less on screen than the diffuse bright area?

Because a highlight stays roughly facing you - it slides across the surface to keep
pointing your way, so it stays near the middle of the visible disc. A point glued to
the surface swings right across. The measured travel is 37 pixels against 73.

## Simple viva modifications

- **The named exercise: change the shininess and watch the highlight tighten.**
  `SurfaceConfig::SHININESS` in [src/main.cpp](../src/main.cpp). Try `4` for a
  broad sheen and `160` for a pinpoint. The measured bright area goes from
  thousands of pixels to tens.
- **Change `SPECULAR_REFLECTANCE`** to `(0,0,0)` and the shine disappears entirely -
  the scene drops back to exactly its Phase 27 appearance.
- **Tint the highlight:** set it to `(0.8, 0.5, 0.2)` for a brassy look, and explain
  that this is how a metal differs from a plastic.
- **Break the gate:** remove the `lambert > 0.0` check and look for glints on the
  dark sides of things.
- **Freeze the camera position:** comment out `scene.viewPos = eye;`. The scene looks
  perfect and the highlight stops moving - the exact shape of the standard bug.
- **Reflect the wrong vector:** change `reflect(-L, N)` to `reflect(L, N)` and the
  highlight moves to the wrong place.

## Checkpoint

Phase 28 passes when:

- Debug and Release build with no compiler warnings;
- both shaders compile and link with no missing-uniform warning;
- the highlight **moves when the camera orbits**, and sits within 1 pixel of the
  position theory predicts at every tested angle;
- it is a tight spot, not a sheen - measured 53 to 76 times tighter than the diffuse
  bright region, and twice as bright;
- with `Ks = 0` the peak brightness is identical from every angle, confirming that
  all of the view dependence comes from this term;
- the highlight is the colour of the light, not of the surface;
- `uViewPos` is refreshed every frame from the same variable the view matrix uses;
- every earlier object and key still behaves as before;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 28

No real materials - one `Ks` and one shininess for the whole scene. Phase 29 adds
`src/Material.h` and the L8 slide 60 table, where the shininess range is
deliberately enormous: sailcloth at 4 against ocean at 160, a 40x spread in a
single frame. The single `32` here is explicitly a placeholder, not a tuning
decision.

No Blinn-Phong `(N·H)^n` comparison - that is Phase 32, along with the `B` key.
No second light and no attenuation: Phase 30. No Flat or Gouraud: Phase 31.

Phase 29 adds **`src/Material.h`**, and its checkpoint is three spheres side by side
that are obviously different materials - the screenshot the report needs for the
`n_s` comparison.

---

## Note: verification

### 17 checks, no failures

The highlight's position was **predicted** rather than merely observed. The test
replicates the camera setup with glm, solves for the surface point where `(R·V)^n`
peaks - the point whose normal is `normalize(L + V)`, found by a few refinement
passes because `V` depends on the point being solved for - projects it to a pixel,
and compares against the rendered image. Worst miss: 1.0 pixel.

That is a much stronger claim than "the highlight moves", and it would have caught
a wrong `reflect`, a missing negation, a space mix-up or a stale `uViewPos`, none
of which a visual check reliably catches.

### Two wrong assumptions of mine, found by the test

Both were in the *test*, not the shader, and both are worth recording because each
produced a confident-looking result that meant nothing.

1. **The first version orbited the world origin.** The ball sits off-centre, so
   orbiting swung it toward and away from the camera and **halved its on-screen
   size** - 19,631 pixels down to 9,559. Any measurement expressed as a fraction of
   the silhouette was therefore meaningless. Fixed by aiming the camera at the ball,
   after which the silhouette holds at 19,046 to 19,066 pixels.

2. **The diffuse peak is not a valid control.** I assumed that with `Ks = 0` the
   bright region would stay put, so that any movement would be attributable to
   specular. It does not stay put: the diffuse peak is a fixed point *on the
   surface*, but orbiting changes its **projection**, so it moves on screen too -
   and in fact it moves *further* than the highlight does. The control "failed" by
   moving 0.671 against 0.476, which is the correct physical answer to the wrong
   question. Replaced with the analytic prediction.

The second mistake is the instructive one. It would have been easy to read the first
result as a bug in the shader and start changing working code.

### Earlier phases still correct

11 draws, 652 triangles, 445 vertices - unchanged, as expected. The `N` view stays
unlit and unaffected, the Phase 26 `M` toggle still works, and the ambient and
diffuse terms still sum as Phase 27 measured.
