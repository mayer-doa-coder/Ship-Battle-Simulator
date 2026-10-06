# Phase 35 - The Rigging: Mast, Yard, Sail and Flag

## Status

Phase 35 is complete and verified.

```text
Ship Battle Simulator - Phase 35 | draws 26 | tris 2132 | verts 1381
```

Ten more draws than Phase 34, and **no new mesh**: the masts and yards are the cylinder
from Stage B, the sails and flag are the quad. The counts match a hand calculation:
4 cylinders x 64 triangles + 6 quads x 2 = 268 triangles, plus 24 from Phase 34.

94 checks passed with zero failures: 70 exact checks on the frames and 24 on the
images. The Phase 34 frame test (42 checks) was re-run against the extended header and
still passes.

### THE CHECKPOINT: a recognisable ship silhouette

Two masts, each with a yard and a sail, and a red flag streaming astern from the main
masthead. The fore mast is shorter and nearer the bow; the main mast is taller with the
longer yard. It reads as a ship from any angle.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `MastDimensions`, `MastFrames`; `ShipDimensions` gains the masts and flag; the builder gains the rigging; five new model functions |
| `src/Material.h` | New tuned `FLAG_CLOTH` |
| `src/main.cpp` | `drawShip()` takes the cylinder and quad meshes and draws ten more parts |

## The one idea

**Each part is a child of the one above it, so moving a part moves everything hung
from it - and nothing else.**

```text
deck
 +-- mast (fore)  -->  yard  -->  sail  -->  back sail
 +-- mast (main)  -->  yard  -->  sail  -->  back sail
                        |
                        +-->  flag hoist  -->  flag  -->  back flag
```

### Measured: move the main mast 0.20 along the ship

```text
mast moved by exactly 0.20            yes
yard followed by exactly 0.20         yes
sail followed by exactly 0.20         yes
back sail followed                    yes
flag hoist and flag followed          yes
the FORE mast's chain moved at all    no
anything ABOVE the moved mast moved   no (hull and deck unchanged)
```

Every descendant moved by exactly the amount asked, and nothing else moved - not the
other mast, and not the parents. That is what a hierarchy means, and it is the viva
exercise.

Making the main mast 0.20 taller:

```text
raises its yard by 0.1640   (= 0.20 x 0.82, the yard's fraction of the height)
raises the flag by 0.2000   (the full 0.20: it stays on the masthead)
```

A yard is attached at a **fraction** of its mast's height, so a taller mast carries its
yard up in proportion. The flag stays at the very top.

## Why the flag is a child of the yard and not of the sail

The plan says each part is "a child of the one above", which would make the flag a
child of the sail. The PRD and the reference guide both say otherwise, and for a good
reason that the project will need in Phase 84.

From Phase 84 the sails **swing about their yards**. A flag parented to the sail would
swing with it, which is wrong - a flag has its own motion, driven by the wind and not
by the sail. So the flag is a child of the main **yard**, as a sibling of the sail:

```text
yard  -->  sail           swings about the yard
yard  -->  flag hoist     is carried by the yard, and swings independently
```

This is the one place the implementation departs from the plan's wording, and it follows
the PRD, which is the authoritative spec.

### The hoist and the flag are separate frames

```cpp
glm::mat4 flagHoist;   // at the masthead - the PIVOT Phase 84 will turn
glm::mat4 flag;        // at the cloth's centre - a child of the hoist
glm::mat4 flagBack;    // the flag turned in place
```

They have different jobs. The hoist is the point the flag is flown from and the frame
that will be rotated; the flag is where the cloth actually is. Rotating a frame turns
everything beneath it about **that frame's origin**, so the pivot has to be at the
masthead and the cloth's centre has to be somewhere else.

## Every sail is drawn twice

A quad has one face. With back-face culling on - it has been since Phase 11 - a single
sail would vanish when you walked round to the other side of it. So each sail and the
flag are drawn twice, the second time with a frame turned half a revolution:

```cpp
r.sailBack = r.sail * glm::rotate(glm::mat4(1.0f), SHIP_PI, glm::vec3(0.0f, 1.0f, 0.0f));
```

A rotation is a rigid motion, so this is still a pure frame with no scale. Whichever side
you are on, exactly one of the two faces you.

Measured on the pixels, seen from the stern:

```text
fore sail: 100% filled with the back copy, 15% without it
main sail: 100% filled with the back copy, 15% without it
```

The 15% left without it is just the mast and the yard. That second row is the **negative
control**: it shows the test would notice if the back copy were missing.

### And the lighting on the back is right

The back face's normal points the opposite way, because the normal is carried through
the same rotation. Both lights are on the bow side, so for the back of a sail `N.L` is
negative for both, and the only light left is ambient. The prediction is
`GLOBAL_AMBIENT x ka x 255 = (0.15, 0.15, 0.18) x (0.20, 0.19, 0.17) x 255`:

```text
back of the fore sail, median colour (8, 7, 8)
ambient-only prediction              (7.7, 7.3, 7.8)
```

Exactly as the model says. That is why the ship looks dark from behind and bright from
the front - not a bug, just the sun being in front of it.

## A bug caught on the way: two flags

The first version made the back flag by turning the flag's frame half a revolution. But
that frame sat at the **pole**, and the flag streams astern **from** the pole. Turning
about the pole puts the back copy on the *opposite* side of the mast, pointing forward,
and the ship ends up with two flags.

The sails never showed it because a sail is symmetric about its frame. The CPU test
caught it before any picture was drawn:

```text
FAIL back flag covers the same rectangle
```

The fix is the hoist-and-flag split above: with the flag's frame at the cloth's own
centre, turning it in place leaves the cloth where it was. The images now confirm it:

```text
the stern-side rectangle is 100% red; the bow-side mirror rectangle 0.0% red
```

One flag, and nothing on the other side.

## No part inherits another's size

Five different ships were built - the default, a hull 4x longer, a thick short deck, a
mast 3x taller and a yard 2x longer - and every mast, yard, sail and the flag was
measured at the size it was drawn:

```text
every mast, yard, sail and the flag is drawn at exactly its OWN size   - all five ships
```

And the same ship built the **naive** way, with the mast a child of the deck's scaled
matrix:

```text
mast meant to be (0.030, 0.60, 0.030)
the naive tree draws it (0.0108, 0.0180, 0.0372)
```

A 0.60-tall mast comes out 0.018 tall - 3% of what it should be. (Its length along z is
only 1.24x too long, because the deck is 1.24 long and close to the 1.0 it multiplies by;
the damage is in x and y, where the deck is small.)

## Where each part sits

Every position is checked against the geometry it should produce:

- each **mast** stands exactly on the deck's top surface and is exactly its height tall
  and its diameter across;
- each **yard** crosses its mast at 82% of the height, lies across the ship, and is
  exactly its length;
- each **sail** has its top edge at the yard, hangs down exactly its own height, is
  exactly its own width, clears the deck, and is no wider than the ship;
- the **flag**'s top edge is level with the main masthead and it streams astern by
  exactly its own length.

## The ship is rotated about its root, and everything follows

A preview of Phase 37, since it falls straight out of the structure: with the root rolled
45 degrees, **every** rigging frame equals the level frame rotated by hand about the root
- the worst difference anywhere is `2.4e-07`. Nothing was told about the rotation; the
tree carried it.

## Likely teacher questions

### Move the mast and show the sail and flag follow.

Change `masts[MAIN_MAST].z` in [src/Ship.h](../src/Ship.h). Measured: the yard, the sail,
the back sail, the flag hoist and the flag all move by exactly the amount the mast did,
and the fore mast and the hull do not move at all.

### Why is the flag not a child of the sail?

Because from Phase 84 the sail swings about its yard, and a flag parented to the sail
would swing with it. The flag has its own motion, so it hangs from the yard as a sibling
of the sail. That is what the PRD specifies, and it departs from the plan's wording.

### Why does each sail get drawn twice?

A quad has one face and back-face culling is on, so a single sail vanishes from behind.
The second draw uses a frame turned half a revolution. Measured, a sail seen from the
stern is 100% filled with the back copy and 15% without it.

### Why is the ship dark from behind?

Both lights are on the bow side. The back of a sail has its normal turned the other way,
so the dot product is negative for both, and only ambient is left. Predicted
`(7.7, 7.3, 7.8)`, measured `(8, 7, 8)`.

### Why is the yard attached at a fraction of the mast's height?

So a taller mast carries its yard up with it. Raising the main mast by 0.20 raised its
yard by 0.164, which is 0.20 x 0.82, and the flag by the full 0.20.

### What stops a mast inheriting the deck's size?

The same rule as Phase 34: frames hold translation and rotation only, and scale is applied
once, in the matrix handed to `drawMesh`. Measured with a hull 4x longer, a thick short
deck, a taller mast and a longer yard, every part is drawn at exactly its own size.

### Why is the flag split into a hoist and a flag?

They have different jobs. The hoist is the pivot at the masthead that Phase 84 will turn;
the flag is the cloth, centred half its length astern. Rotating a frame turns everything
beneath it about that frame's origin, so the two origins have to differ.

### Does the rigging cost any new geometry?

No. Masts and yards are the cylinder, sails and flag are the quad. The ship is ten more
draw calls and nothing more in graphics memory.

## Simple viva modifications

- **The named exercise: move the mast and watch the sail and flag follow.** Change
  `masts[MAIN_MAST].z` in [src/Ship.h](../src/Ship.h), for example from `-0.18` to `0.10`.
- **Make a mast taller:** `masts[MAIN_MAST].height`. The yard and the flag rise with it.
- **Move a yard down the mast:** `yardHeightFraction`. The sail comes with it.
- **Lengthen a yard:** `yardLength`. Nothing else changes.
- **Resize the sail:** `sailSize`. It stays hanging from the yard.
- **Show the wrong way:** build a mast from `shipDeckModel(...)` and watch it shrink to a
  stub.
- **Remove the back copy:** delete the `sailBack` draw and walk round to the stern.
- **Drag the camera** round behind the ship (yaw 180) and zoom out to see the whole thing
  in the scene.

## Checkpoint

Phase 35 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the ship has two masts, each with a yard and a sail, and a flag at the main masthead;
- each part is a child of the one above, and **moving a part moves exactly its
  descendants** - measured, every descendant by exactly 0.20 and nothing else;
- every new frame is a pure rigid motion, at three different roots;
- every part is drawn at exactly its own size across five different ships, and a naively
  built ship fails the same test;
- every sail is visible from both sides - 100% filled, against 15% without the back copy -
  and the back is lit exactly as the lighting model predicts;
- there is exactly one flag, on the stern side of the main mast;
- with the ship hidden, ten views are **byte-for-byte identical to the real Phase 33**;
- the rigging cannot hide Demo A's highlight - the sight line crosses 0 of 4 ship boxes;
- the scene is 26 draws / 2132 triangles / 1381 vertices, matching a hand calculation.

## What is not part of Phase 35

No cannon: Phase 36. No movement, no rocking, and no swinging sails - the sails and the
flag are still. Phase 84 gives them their idle motion, which is why the frames are laid
out the way they are. No `H` key yet: Phase 37.

Phase 36 adds the **cannon chain**: mount, yoke (azimuth) and barrel (elevation), with a
named `MUZZLE_Z` tip. Its checkpoint is a cannon on the deck that swings correctly when
`azimuth` and `elevation` are edited by hand.

---

## Note: verification

### 94 checks, no failures

| Suite | What it does | Checks |
|---|---|---:|
| Rigging frames | The real `Ship.h`: rigidity, the chain, the geometry of every part, moving a mast, no size inheritance, a naive ship, silhouette, and the rolled root | 70 |
| Phase 34 frames | Re-run unchanged against the extended header | 42 |
| Images | Byte-for-byte regression against Phase 33, both-sides visibility with a negative control, back-face lighting, one flag, and the Demo A sight line | 24 |

### Mistakes caught

1. **Two flags.** Described above. A real bug in the code, found by the CPU test before
   anything was rendered, and then confirmed gone in the pixels.
2. **A wrong expectation in my own test.** I asserted the naive mast would be "smeared
   along the ship by the deck's length". It is not: the deck is 1.24 long, so the
   naive mast is only 1.24x too long in that direction. The damage is in x and y. The
   assertion was replaced with the true one, and the measured number is printed so the
   reason is visible.
3. **Two editing slips**, both mine and both caught at compile time: a `\n` that collapsed
   into a real newline inside a string, and a brace dropped while replacing a line. Neither
   reached the project.

### One honest observation about the look

The front of the main sail measures a median luminance of **255**: pure white. The point
light is a permanent fixture at the moment, with intensity 3.2, and in front of the ship it
saturates the cloth. That is not a defect in this phase - the light's strength was tuned in
Phase 30 so its pool on the sea would be obvious - but it does mean the sails lose their
shading from the front. In Phase 90 the point light becomes the 0.15-second muzzle flash,
and the sails will stop being blown out.

### Earlier phases still correct

With the ship hidden the output is identical to the last byte to the real Phase 33 across
ten views and modes, so nothing in Stage C moved. The mast and yard meshes are the shared
cylinder, so the `+` and `-` detail keys change their roundness too: at the lowest detail
they become square.
