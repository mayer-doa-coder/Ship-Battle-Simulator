# Phase 27 - The First Light: Ambient and Diffuse

## Status

Phase 27 is complete and verified. **This is the first phase with light in it.**
Debug and Release build with no warnings, and 14 image checks passed with zero
failures.

```text
Ship Battle Simulator - Phase 27 | draws 11 | tris 652 | verts 445
```

The counts are unchanged from Phase 26. Lighting adds no geometry at all - it only
changes what colour each pixel comes out.

### THE CHECKPOINT: a sphere with a clear lit side and dark side

Measured on the ball alone, as luminance out of 255:

```text
            min   mean   max    pure-black pixels
ambient only   8   14.5    21            0
diffuse only   0   61.4   102          268
both           8   76.0   119            0
```

Three things in that table are the whole phase:

1. **`both` has a range of 8 to 119.** A clear lit side and a clear dark side.
2. **`14.5 + 61.4 = 75.9`, and `both` measures `76.0`.** The two terms *add*. That
   is not a figure of speech - it is what the illumination model does, and you can
   see it in the arithmetic.
3. **`diffuse only` has 268 pure-black pixels; `both` has none.** The ambient term
   is the only thing stopping the unlit side from being a black hole.

## What changed

| File | Change |
|---|---|
| `shaders/basic.frag` | The ambient and diffuse terms; `uGlobalAmbient`, `uSunDirection`, `uSunColor`, `uKa`, `uKd`; `normalize()` on the interpolated normal |
| `src/main.cpp` | New `LightConfig` and `SurfaceConfig`; five uniforms set once per frame |

No new geometry, no new key, no change to any mesh.

## The one idea

**Lambert's cosine law: how bright a surface looks depends on the angle between
the surface and the light.**

```glsl
vec3  L       = -normalize(uSunDirection);
float lambert = max(dot(N, L), 0.0);
```

A surface facing the light square-on catches the most light. One at a glancing
angle spreads the same light over more area, so each patch of it gets less. The
cosine of the angle between them is exactly that falloff - and because `N` and `L`
are both unit vectors, **`N · L` IS that cosine**. No trigonometry needed; a dot
product does it.

The full model, with this phase's two terms:

```text
I = globalAmbient * Ka  +  sunColour * Kd * max(N . L, 0)
    ^^^^^^^^^^^^^^^^^^     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    everywhere, flat       directional, gives the shape
```

### Three details that are each a bug if you get them wrong

**`-uSunDirection`, not `uSunDirection`.** `L` has to point *from the surface
toward the light*. The config stores the direction the light **travels**, which is
the opposite. Getting this backwards lights precisely the wrong side of everything
- which is at least an obvious kind of wrong.

**`max(..., 0.0)` is not tidiness.** A surface facing away gives a *negative* dot
product. Without the clamp that negative number would be multiplied by the colour
and **subtracted** from the ambient term, so the dark side would end up darker than
black and would lose its ambient light entirely. The 268-black-pixel measurement
above is what the diffuse term looks like on its own; without the clamp it would
eat into the ambient as well.

**`normalize(vNormal)` in the fragment shader.** Phase 26 already normalised in the
*vertex* shader, and that is still needed - but it is not enough. Interpolating
between two unit vectors across a triangle gives something **shorter than 1**:
picture the straight line between two points on a circle, which passes inside the
circle. On a flat face whose corners share one normal this costs nothing. On the
cylinder and the sphere it would make the surface slightly too dark.

That last one was an **acknowledged debt**. The comment that used to sit in
`basic.frag` said normalize "belongs here from that point on", meaning from
Phase 20 when curved surfaces arrived, and it was never added. It is added now,
because lighting is the point at which it stops being cosmetic.

## Why the sun has no position and no attenuation

```cpp
const glm::vec3 SUN_DIRECTION(-0.4f, -0.35f, -0.5f);
```

A **directional** light is modelled as being infinitely far away, so its rays are
parallel everywhere in the scene. There is no position, so there is no distance,
so there is nothing to fall off over (L8 s19). The sun is not measurably closer to
one ship than to another.

The point light in Phase 30 is the one that needs `1/(a0 + a1·d + a2·d²)`, and
having one light of each kind is deliberate: it makes the difference between them
something you can see rather than something you have to be told.

## Why there is an ambient term at all

This is worth being able to answer properly, because it is really a question about
what this whole pipeline can and cannot do.

OpenGL's raster pipeline is a **local** illumination model (L8 s10-11). It renders
each polygon independently, with no knowledge of any other polygon in the scene.
So it cannot know that light bounced off the deck and up onto the underside of the
cannon - that would need a *global* technique.

The ambient term is the stand-in for all of that bounced light. It is one flat
number per channel, slightly blue here `(0.15, 0.15, 0.18)` because in a real scene
the sky is what fills the shadows.

This is also the honest answer to "why are there no shadows?". Not an oversight -
shadows require knowing what is between a surface and a light, and a local model by
definition does not look.

Measured proof that the ambient really is directionless: on its own it spans
luminance **8 to 21** across the whole ball, a range of 13 out of 255. Essentially
flat, as something with no direction must be.

## `uKa` and `uKd`, and the thing they are standing in for

```glsl
vec3 base    = vColor * uTint;
vec3 ambient = uGlobalAmbient * uKa * base;
vec3 diffuse = uSunColor * uKd * base * lambert;
```

`Ka` and `Kd` are L8's notation for how strongly a surface reflects ambient and
diffuse light. Right now they are **one pair of numbers for the whole scene**, both
`(1,1,1)`, and the per-vertex colour is still standing in for the rest of a
material.

That is a deliberate half-step, and it is honest about it:

| | Now (Phase 27) | After Phase 29 |
|---|---|---|
| Where the colour comes from | the vertex's own colour | `src/Material.h`, the L8 slide 60 table |
| Where `Ka`/`Kd` come from | one scene-wide constant | per object, in `drawMesh()` |
| Set how often | once per frame | once per object |

Introducing the uniform *names* now, alongside the lighting that uses them, means
Phase 29 changes only where the values come **from** - not the shader. And the
per-vertex colours are what keep the cube's six faces and the quad's blended
corners visible in the meantime.

## Why the uniforms are set once per frame

```cpp
shader.setVec3("uGlobalAmbient", LightConfig::GLOBAL_AMBIENT);
shader.setVec3("uSunDirection", LightConfig::SUN_DIRECTION);
shader.setVec3("uSunColor", LightConfig::SUN_COLOR);
```

Neither the sun nor the global ambient belongs to any one object, so sending them
inside the per-object loop would be eleven identical uploads per frame instead of
one. This is the "hoist uniforms out of the per-object loop" point that Phase 62
measures, applied as soon as there is something to hoist.

## The `N` debug view stays unlit

```glsl
if (uDebugNormals == 1) {
    FragColor = vec4(N * 0.5 + 0.5, 1.0);
    return;
}
```

The whole job of that view is to be **read accurately**. Shading it, or tinting it,
would make the colours mean two things at once. The early `return` keeps it
completely outside the lighting - which also means the Phase 26 `M` comparison
still works exactly as it did.

## Likely teacher questions

### What are the two terms, and what does each one do?

Ambient is flat and comes from everywhere - measured range 8 to 21 across the whole
ball, so essentially no direction. Diffuse is `max(N·L, 0)` and carries all of the
shape - measured range 0 to 102. Together: 8 to 119.

### Why `N · L` rather than an angle?

Because for two unit vectors the dot product *is* the cosine of the angle between
them, and Lambert's law says brightness follows that cosine. A dot product is three
multiplies and two adds; computing an angle and then its cosine would be far more
work for the same answer.

### Why negate the sun direction?

The config stores the direction the light travels. `L` has to point from the
surface toward the light, which is the opposite. Negating in one named place means
it cannot be done twice by accident.

### What happens without the `max(..., 0)`?

A surface facing away gives a negative dot product, which would be subtracted from
the ambient term. The dark side would go darker than black and lose its ambient
light entirely.

### Why does the sun have no attenuation?

Because it has no position. A directional light is infinitely far away and its rays
are parallel, so there is no distance to fall off over (L8 s19). Phase 30's point
light is the one with attenuation.

### Why is there an ambient term? Isn't it fake?

It is a deliberate approximation. OpenGL's pipeline is a *local* model - it renders
each polygon with no knowledge of any other (L8 s10-11), so it cannot compute light
that bounced off something else. Ambient stands in for that. Without it, every
surface facing away from the sun would be pure black: measured, 268 pixels of the
ball.

### Why is it slightly blue?

Because in a real scene the sky fills the shadows.

### Why are there no shadows?

Same reason there has to be an ambient term. Shadows need to know what lies between
a surface and a light, and a local illumination model never looks at anything but
the surface in front of it. It is intentional, not a shortcoming.

### Why normalize the normal twice - in both shaders?

Two different reasons. The vertex shader normalizes because the normal matrix
changes a normal's length while fixing its direction. The fragment shader
normalizes because interpolating between two unit normals across a triangle gives
something shorter than 1 - the straight line between two points on a circle passes
inside it. Skipping the second one makes curved surfaces slightly too dark.

### Why is the debug view not lit?

Its only job is to be read accurately. Shading it would make each colour mean two
things at once.

## Simple viva modifications

- **The named exercise: change the sun direction.** `LightConfig::SUN_DIRECTION` in
  [src/main.cpp](../src/main.cpp). Try `(0.4, -0.35, 0.5)` and the lit side swaps
  to the other side of the ball. Try `(0, -1, 0)` for noon, lighting only the tops.
- **Prove each term separately:** set `GLOBAL_AMBIENT` to `(0,0,0)` and the dark
  side goes pure black. Set `DIFFUSE_REFLECTANCE` to `(0,0,0)` and the scene goes
  flat and shapeless. (Phase 32 adds a `K` key that does this live.)
- **Make the sun coloured:** set `SUN_COLOR` to `(1.0, 0.6, 0.3)` for a sunset. The
  lit side warms and the shadow stays blue, which shows the two terms are separate.
- **Break the clamp:** remove `max(..., 0.0)` from the fragment shader and watch the
  dark side lose its ambient light.
- **Break the normalize:** delete `normalize(vNormal)` in the fragment shader. The
  cylinder and the sphere go slightly dark; the flat objects do not change at all,
  which explains exactly why.
- **Reverse L:** drop the minus sign on `uSunDirection` and the wrong side lights.

## Checkpoint

Phase 27 passes when:

- Debug and Release build with no compiler warnings;
- both shaders compile and link with no missing-uniform warning;
- the ball has an obvious **lit side and dark side** - measured luminance 8 to 119;
- the dark side is not pure black, because ambient reaches it - measured 0 black
  pixels with both terms, 268 with diffuse alone;
- the ambient term on its own is nearly flat - measured range 13 out of 255;
- the two terms **sum** - measured 14.5 + 61.4 against 76.0 for both;
- changing the sun direction visibly moves the lit side;
- the `N` debug view is unaffected, and the Phase 26 `M` toggle still works;
- the geometry counts are unchanged at 11 draws / 652 triangles / 445 vertices;
- the window reports frame timing and closes cleanly with every GPU object freed.

## What is not part of Phase 27

No specular highlight - `(R·V)^n`, `uKs`, `uShininess` and `uViewPos` are Phase 28,
and until they arrive nothing in the scene is shiny, so nothing changes when the
camera moves. No real materials: Phase 29. No second light and no attenuation:
Phase 30. No Flat or Gouraud comparison: Phase 31. No `K` term mask: Phase 32.

Phase 28 adds the **specular term**, and its checkpoint is that the highlight
*moves* when the camera orbits - the first thing in the project whose appearance
depends on where you are looking from.

---

## Note: verification

### 14 image checks, no failures

The lighting was verified by rendering the ball five ways and measuring the pixels:
with both terms, with the sun reversed, with ambient only, with diffuse only, and
the whole scene. Each term was switched off by driving the real uniform to zero, so
the code under test is the project's own shader.

The strongest evidence is the one that was not asked for: **the measured means add
up.** 14.5 for ambient, 61.4 for diffuse, 76.0 for both. If the shader were doing
anything other than summing two independent terms, that would not hold.

### One pre-existing debt paid

`normalize()` on the interpolated normal in the fragment shader. The old comment in
`basic.frag` stated it was owed from Phase 20, when curved surfaces arrived, and it
had never been added. Harmless while nothing was lit; a brightness error on every
curved surface as soon as something is.

### One weaker result, stated honestly

Reversing the sun moves the centroid of the brightest tenth by 28 pixels, which
passes but is less dramatic than it sounds. The reason is that with the sun
reversed the lit side faces *away* from the camera, so what remains visible is a
rim rather than a broad highlight. The per-term isolation in section 3 is the
better evidence for this phase, and Phase 28's moving highlight will be the
stronger demonstration of direction.

### Earlier phases still correct

11 draws, 652 triangles, 445 vertices - unchanged from Phase 26, as expected, since
lighting adds no geometry. The `N` view, the `M` toggle, the `+`/`-` detail levels
and every Stage B key behave as before.
