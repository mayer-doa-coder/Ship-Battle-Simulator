# Phase 36 - The Cannon Chain: Mount, Yoke, Barrel

## Status

Phase 36 is complete and verified.

```text
Ship Battle Simulator - Phase 36 | draws 28 | tris 2208 | verts 1471
```

Two more draws than Phase 35 - the mount block and the barrel - and no new mesh. The
counts match a hand calculation: a cube (12 triangles, 24 vertices) and a cylinder
(64 triangles, 66 vertices).

65 new checks passed with zero failures (46 exact on the frames, 19 on the images), and
the 112 earlier Stage D frame checks were re-run against the extended header and still
pass.

### THE CHECKPOINT: editing `azimuth` and `elevation` swings the gun correctly

The muzzle's position was compared against a closed form worked out **independently of
the matrix code**. With the bow towards `+z`, an azimuth `az` about the vertical and an
elevation `el` about the horizontal, the muzzle's offset from the barrel's pivot must be

```text
L * ( sin(az) cos(el),   sin(el),   cos(az) cos(el) )
```

Seventy-eight aims - every 30 degrees of azimuth from -180 to +180, against elevations
from -5 to 45 degrees - were checked:

```text
   az   el |  measured offset              |  closed form                 | miss
    90   15 | (+0.3284, +0.0880, +0.0000)   | (+0.3284, +0.0880, -0.0000)  | 4.0e-08
     0   45 | (+0.0000, +0.2404, +0.2404)   | (+0.0000, +0.2404, +0.2404)  | 2.1e-08
  -180   -5 | (+0.0000, -0.0296, -0.3387)   | (+0.0000, -0.0296, -0.3387)  | 6.6e-08

78 aims compared; worst miss anywhere: 6.83e-08
```

**The worst disagreement anywhere is seven hundredths of a millionth of a unit.**

And the same thing on the pixels: the barrel *as rendered*, from above, at ten different
aims, against where the projected frames say it should be:

```text
worst axis-angle error 0.4 degrees, worst centroid error 2.0 px   (barrel ~27 px thick)
```

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `CannonDimensions`, `CannonPose`; four new frames; `shipMuzzlePosition()`, `shipMuzzleDirection()`; two model functions |
| `src/main.cpp` | Two more draws in `drawShip()` |

The default aim - **30 degrees off the bow, 12 degrees up** - is in
`ShipConfig::DEFAULT_CANNON_POSE`. Editing those two numbers is the viva exercise.

## The one idea

**One rotation per job, each in its own frame, so the rotations never tangle.**

```text
deck
 +-- mount          a block standing on the deck
      +-- yoke       turns about the VERTICAL        <- azimuth lives here
           +-- barrel   tilts about a HORIZONTAL axis  <- elevation lives here
                +-- muzzle   an empty frame at the tip
```

The barrel is a child of the yoke, so it inherits the azimuth **for free**. The cannon
swings round and tilts up as two independent moves, because each is applied about its own
axis in its own frame.

### Why it has to be a chain and not one matrix

The two rotations are about **different axes**, and rotations do not commute. If both
were applied to the barrel in one step, the horizontal axis it tilts about would be the
*ship's* x axis rather than the barrel's own - so after turning the gun 90 degrees to
face outboard, elevating it would roll it sideways instead of raising it. By putting the
azimuth on the yoke and the elevation on the barrel beneath it, "tilt about x" always
means "tilt about the axis perpendicular to wherever the gun is currently pointing".

Measured, the two behave independently:

```text
changing the AZIMUTH:     muzzle height unchanged, barrel pivot unmoved,
                          horizontal reach unchanged at L cos(el),
                          bearing from the pivot equals the azimuth that was set
changing the ELEVATION:   bearing unchanged, height set to exactly L sin(el),
                          and the muzzle always lies on a sphere of radius MUZZLE_Z
```

## The sign that is easy to get wrong

```cpp
f.barrel = f.yoke
         * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, c.trunnionHeight, 0.0f))
         * glm::rotate(glm::mat4(1.0f), -aim.elevation, glm::vec3(1.0f, 0.0f, 0.0f));
```

Look at the minus sign. A rotation about `+x` by a positive angle carries a vector along
`+z` **downwards** - `z` turns into `-y`. So with the barrel lying along `+z`, a positive
elevation needs the angle **negated** to raise the muzzle. The reference guide's snippet
omits it. Without it the gun depresses when told to elevate.

This is exactly the kind of mistake that stays hidden when the angle is zero, which is why
the default aim is deliberately **not** zero in either angle: a gun pointing dead ahead and
perfectly level would look right under almost any error in the two rotations.

It was tested three ways, so it cannot slip through:

- the barrel's local rotation is compared against `R_x(-elevation)` exactly;
- positive elevation is checked to **raise** the muzzle;
- on the pixels, the barrel's highest rendered point is **row 551 at elevation 45 and row
  424 at elevation -5** - a difference of 127 pixels, in the right direction.

## Position takes `w = 1` and direction takes `w = 0`

```cpp
inline glm::vec3 shipMuzzlePosition(const ShipFrames& f)
{
    return glm::vec3(f.muzzle * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));   // w = 1: a POSITION
}

inline glm::vec3 shipMuzzleDirection(const ShipFrames& f)
{
    return glm::normalize(glm::vec3(f.barrel * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));   // w = 0: a DIRECTION
}
```

A **position** has a place, so the translation in the matrix must move it: `w = 1`. A
**direction** is only an orientation - it has no place - so the translation must **not**
touch it: `w = 0`.

Measured by moving the whole ship by `(5, 2, -3)`:

```text
muzzle POSITION   moved by exactly (5, 2, -3)
muzzle DIRECTION  did not change at all
```

And what the wrong way gives - taking the direction with `w = 1`:

```text
w = 1 gives (5.39, 1.10, -3.27), length 6.40
w = 0 gives a unit vector
```

The answer is dominated by the barrel's *world position*, so a cannonball launched that way
heads for the world origin instead of along the barrel. These two functions exist now, with
a test, because Phase 86 launches a cannonball from them.

## The muzzle is a frame with no mesh

The yoke and the muzzle have nothing to draw. The yoke's whole job is to carry a rotation
to its child; the muzzle's is to be **read**. An empty frame is the cleanest way to mark
"the place a cannonball will be born", because it inherits the whole hierarchy - the
ship's root, its roll, the azimuth, the elevation - without any special-case code.

## `MUZZLE_Z` is the barrel's length

The barrel runs from its pivot to the muzzle, so the distance to the muzzle **is** the
barrel's length. It is stored once, as `muzzleZ`, and the drawn barrel is measured to
agree: one end of the drawn tube is exactly the pivot and the other is exactly the muzzle
point.

How the tube is placed:

```cpp
return f.barrel
     * glm::translate(...(0, 0, muzzleZ * 0.5f))        // 3. move half its length along +z
     * glm::rotate(..., SHIP_PI * 0.5f, (1, 0, 0))      // 2. stand it along +z instead of +y
     * glm::scale(...(diameter, muzzleZ, diameter));    // 1. size it
```

Read from the bottom up, because the rightmost matrix is applied first. Step 3 matters: the
unit cylinder is centred on its origin, so without it the pivot would be at the barrel's
middle and the barrel would stick out of the back of the cannon as well as the front.

## The trunnion height

```cpp
0.045f,    // trunnion height above the mount's top
```

The barrel's pivot is a little above the mount, because a tube has a radius. A pivot sitting
**on** the mount's top would bury the lower half of the barrel in the block. It is checked:
`trunnionHeight >= barrelDiameter / 2`.

## No part inherits another's size

A ship was built with a hull 4x longer, a different deck and a flat, wide mount:

```text
the barrel is still drawn at exactly (0.050, 0.340, 0.050)
```

And built the naive way, with the barrel hung from the mount's **scaled** matrix:

```text
barrel meant to be (0.050, 0.340, 0.050)
the naive tree draws it (0.0150, 0.0068, 0.0150)
```

A 0.34-long barrel comes out 0.0068 long: **two percent of what it should be**.

## The barrel inherits the ship's roll for free

This is the PRD's critical property, and it falls straight out of the structure. With the
root rolled 45 degrees:

```text
level direction (0.489, 0.208, 0.847)  ->  rolled 45 deg (0.199, 0.493, 0.847)
```

The muzzle direction rotated by **exactly** that roll, and the muzzle position by exactly
the rotation about the root. Nothing in the cannon was told the ship was rolling. That is
why, once Stage F makes the ship ride the waves, a shot fired at the top of a roll will fly
differently from one fired at the bottom - with no special code anywhere.

## Where the cannon sits

```cpp
glm::vec2(0.10f, 0.08f),        // mount position on the deck (x, z)
```

On the `+x` side, between the masts, so the barrel can look out over the side. By default
the muzzle is at ship-local `(0.266, 0.366, 0.368)` against a hull half-beam of `0.210` - it
overhangs the side. Its height above the deck is `0.186`, with the fore sail's lower edge at
`0.192`; the barrel passes outside the sail's width rather than under it, which the test
accepts as clear.

(The `+x` side is **port**. With the bow towards `+z` and `y` up, the right-hand side is
forward times up, which is `-x`.)

## Likely teacher questions

### Why is it three nodes: mount, yoke, barrel?

Because azimuth and elevation are about different axes and rotations do not commute. The
yoke turns about the vertical; the barrel tilts about an axis in the yoke's own frame, so
it is always perpendicular to wherever the gun points. In one matrix, elevating a gun that
had been turned 90 degrees would roll it sideways.

### Show me that editing the angles swings it correctly.

Change `DEFAULT_CANNON_POSE` in [src/Ship.h](../src/Ship.h). Measured: across 78 aims the
muzzle is where `L(sin az cos el, sin el, cos az cos el)` says, worst error 6.8e-08. On the
pixels, the rendered barrel's axis is within 0.4 degrees of the prediction at ten aims.

### Why is there a minus sign on the elevation?

A rotation about `+x` by a positive angle carries a vector along `+z` downwards. With the
barrel along `+z`, the angle has to be negated for positive elevation to raise the muzzle.
Measured on the pixels: the highest barrel point is 127 rows higher at +45 than at -5.

### What is the muzzle, and why does it have no mesh?

An empty frame at the barrel's tip, `MUZZLE_Z` along its axis. It exists to be read: it is
where the cannonball is born in Phase 86, and because it is a child of the whole chain it
inherits the ship's roll, the azimuth and the elevation automatically.

### Why does the position use `w = 1` and the direction `w = 0`?

A position has a place and must be moved by the matrix's translation; a direction is only
an orientation and must not be. Moving the ship by `(5, 2, -3)` moves the muzzle position by
exactly that and leaves the direction unchanged. With `w = 1` on the direction it comes out
as `(5.39, 1.10, -3.27)` - aimed at the world origin.

### Why is the default aim not zero?

Because a gun pointing dead ahead and level would look right under almost any error in the
two rotations. 30 degrees of azimuth and 12 of elevation make a swapped axis or a wrong sign
show immediately.

### How does the cannon know the ship is rolling?

It does not, and that is the point. The barrel's world transform is the root times every
parent above it times the azimuth times the elevation, so a roll in the root is inherited
automatically. Rolled 45 degrees, the muzzle direction rotates by exactly that roll.

### Why does the barrel's pivot sit above the mount?

A tube has a radius. A pivot on the mount's top would bury half the barrel in the block.

## Simple viva modifications

- **The named exercise: change the mount position on the deck.** `cannon.mountXZ` in
  [src/Ship.h](../src/Ship.h), for example `(-0.10, -0.30)` to move it aft and to the other
  side. The yoke, barrel and muzzle all go with it.
- **Swing the gun:** change `DEFAULT_CANNON_POSE`. Try azimuth 90 to point straight over the
  side, and elevation 45 for maximum lift.
- **Lengthen the barrel:** `muzzleZ`. The muzzle moves out with it, and the drawn barrel is
  still exactly that long.
- **Remove the minus sign** on the elevation and watch the gun depress when told to elevate.
- **Put the translation after the rotation** in the yoke and watch the gun orbit the ship's
  origin instead of turning on its mount.
- **Take the direction with `w = 1`** and print it.
- **Roll the ship:** multiply the root by a rotation in `updateScene()` and watch the cannon
  go with it, aimed differently.

## Checkpoint

Phase 36 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the cannon chain is mount, then yoke (azimuth), then barrel (elevation), then muzzle, each
  frame one local step from its parent, and all of them rigid at three roots and at 54
  aims;
- the muzzle matches the closed form `L(sin az cos el, sin el, cos az cos el)` at 78 aims -
  measured, worst miss 6.8e-08;
- positive elevation **raises** the muzzle, in the frames and on the pixels;
- the rendered barrel's axis is within 0.4 degrees of the prediction at ten aims;
- the barrel is drawn from exactly its pivot to exactly its muzzle, at exactly its own size,
  and a naively built cannon fails the same test;
- the muzzle position takes `w = 1` and the direction `w = 0`, and the wrong way is shown to
  aim at the origin;
- rolling the root rolls the gun by exactly that amount;
- with the ship hidden, ten views are **byte-for-byte identical to the real Phase 33**;
- the cannon cannot hide Demo A's highlight;
- the scene is 28 draws / 2208 triangles / 1471 vertices, matching a hand calculation.

## What is not part of Phase 36

No firing: there is no cannonball, no `SPACE`, no gravity - Stage H. No aiming keys: the
azimuth and elevation are fixed numbers you edit by hand, and Phase 85 puts them on the
arrow keys. No auto-track, no reload, no muzzle flash. The muzzle *position and direction*
exist as functions, ready for Phase 86 - but nothing calls them yet.

Phase 37 is the **proof** of the whole stage: rotating the ship root 45 degrees carries every
child, and the `H` key clears the **root's** rotation only, leaving every child's own local
transform alone.

---

## Note: verification

### 65 new checks, no failures

| Suite | What it does | Checks |
|---|---|---:|
| Cannon frames | The real `Ship.h`: rigidity at 54 aims, the chain, a closed-form comparison at 78 aims, the drawn barrel, size independence, `w = 1` against `w = 0`, the rolled root, and sensible defaults | 46 |
| Phase 34 frames | Re-run unchanged against the extended header | 42 |
| Phase 35 rigging | Re-run unchanged against the extended header | 70 |
| Images | Byte-for-byte regression against Phase 33, the barrel at ten aims, elevation on screen, the Demo A sight line and the detail extremes | 19 |

### The closed form was worked out first

The comparison at 78 aims is meaningful because the formula
`L(sin az cos el, sin el, cos az cos el)` was derived from the geometry - a unit vector at
bearing `az` and elevation `el` - and not by reading the matrix code back. If the matrices
had the rotations in the wrong order, or the wrong sign, or the translation on the wrong
side, they could not have matched it at 78 points to 7e-08.

### One observation about a number in the elevation table

The rendered "tip height above pivot" runs 8 to 12 pixels higher than the predicted figure
at every elevation, including a constant `12.4` against `0.0` at -5 and 0 degrees. That is
the barrel's **radius** (about 13 pixels at that zoom): the highest *pixel* of a tube is its
top edge, not its axis. It is a measurement artefact and it is constant, so it does not
affect the result - but it is why the tolerance there is a barrel radius, not a pixel.

### No mistakes to correct this time

Unlike Phases 34 and 35, nothing had to be fixed. The elevation sign and the order of the
three matrices in the barrel model were worked out before writing, and the closed-form test
confirmed them first time. That is worth saying plainly: the two things most likely to be
wrong in a cannon were reasoned through and then verified, rather than discovered.

### Earlier phases still correct

With the ship hidden the output is identical to the real Phase 33 across ten views and
modes, and the 112 earlier frame checks pass unchanged. The cannon's cylinder is the shared
mesh, so at the lowest detail the barrel becomes square - the ship's silhouette changes by
under 8% across the whole detail range.
