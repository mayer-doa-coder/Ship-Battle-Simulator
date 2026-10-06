# Phase 37 - The Hierarchy Proof: Rotate the Root, Press `H`

## Status

Phase 37 is complete and verified. It is a **graded milestone** (`*` in the plan) and it
**completes the first part of Stage D, the hierarchy method**. (This document was written when Stage D ended here. Stage D was later widened into a full pirate-galleon build, Phases 38 to 78, and the statements below about what comes next have been updated to match.)

```text
Ship Battle Simulator - Phase 37 | draws 28 | tris 2208 | verts 1471
```

No new geometry and no new draw call. This phase adds a root *pose*, a rotation to apply
to it, and a key.

46 new checks passed with zero failures: 24 exact on the frames and 22 on the images.

### THE CHECKPOINT: with `H`, the children keep their own local transforms while the root is the identity

**The ship starts rolled 45 degrees. That is deliberate - it is the proof.** Every mast,
sail, flag and the cannon came with it, because every one of them is a descendant of a
single root matrix. Press `H` and the ship rights itself; press it again and it tilts back.

Two precise claims were tested on **every parent-child link on the ship** (17 of them):

**(a) Each child's local transform did not change.**

```text
node               | local transform differs between tilted and level by
hull               | 1.2e-07
main mast          | 1.2e-07
main sail          | 1.2e-07
flag               | 1.2e-07
cannon barrel      | 1.2e-07
...                (all 17 the same)

17 parent-child pairs; worst difference 1.19e-07
```

`H` did not touch anything below the root. The differences are float rounding.

**(b) Each child's world transform followed the root exactly.**

```text
18 frames; worst difference between 'tilted' and 'level, rotated by hand': 2.38e-07
```

Every frame on the ship is exactly the level frame, rotated 45 degrees about the root.
Nothing was told about the rotation; the tree carried it.

And it is not a trivial proof. The tilt moves the main masthead by **0.735 units** and
the muzzle by **0.346** on a ship 1.40 long, and the main mast's axis in the world goes
from `(0, 1, 0)` to `(-0.707, 0.707, 0)`. Yet in the deck's own axes it still points
straight up, because it was carried and not bent.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | `ShipPose`, `shipRootMatrix()`, `shipPoseWithoutSeaRotation()`; `ShipConfig::PROOF_ROLL` |
| `src/main.cpp` | `SceneState::rootTiltEnabled`; the root is now built from a pose; the `H` key |

`buildShipFrames()` is **unchanged** - and that is itself the point. The frame builder
takes a root matrix and does not care where it came from.

## The one idea

**Change one matrix and everything moves; change nothing else.**

The whole ship is built from one root:

```text
root --+-- hull -- deck --+-- mast -- yard -- sail
                          +-- mount -- yoke -- barrel -- muzzle
```

Everything on the ship is a descendant of that one `mat4`. So rolling the ship is not a
matter of rolling 17 parts and keeping them in step - it is **one** change, to the root,
and the other 17 follow because they were never given a position of their own, only a
position *relative to their parent*.

That is what `H` demonstrates. It does not move a mast, a sail or the cannon. It only
swaps the root, and the rest follows it back to level.

## The root has four numbers, and they have different owners

```cpp
struct ShipPose {
    glm::vec3 position;   // world position of the root, at the waterline
    float heading;        // radians about the vertical: where the bow points
    float roll;           // radians about the ship's own long axis (z)
    float pitch;          // radians about the ship's own cross axis (x)
};
```

They are kept separate rather than baked into one matrix because of **who controls them**:

| Number | Owner | Phase |
|---|---|---|
| `position`, `heading` | the **player** | 38: steering |
| `roll`, `pitch` | the **sea** | 42: waves |

`H` clears the sea's contribution and **must leave the player's alone**. It can only do
that if the two were never mixed together. Measured, on a ship at `(7, -1.4, -3)` heading
30 degrees and rolled and pitched:

```text
after H:   position untouched, heading untouched, roll = 0, pitch = 0
           the root is exactly T * R_y(heading)
           the bow's compass bearing is EXACTLY the heading
           the bow is level
```

Pressing `H` mid-voyage straightens the ship **without turning it round or moving it**.

### The order of the rotations

```cpp
T(position) * R_y(heading) * R_z(roll) * R_x(pitch)
```

Read right to left: pitch first, then roll, then heading, then move into place. So roll and
pitch are about the **ship's own** axes, and only afterwards is the whole thing turned to its
heading. That is how a vessel behaves - a ship pointing east still rolls about its own length,
not about the world's. Tested directly:

```text
a ship facing east, rolled 45 degrees: its up leans along the world z axis, not x
```

## The tilt is a roll, and why it is 45 degrees

```cpp
const float PROOF_ROLL = 45.0f * SHIP_PI / 180.0f;
```

A **roll**, because roll is what the sea applies in Stage F and what `H` will keep clearing.
Large, because a gentle tilt could hide a part that failed to follow, and 45 degrees
cannot. When Phase 83 sets the roll from the waves under the hull this constant goes away;
until then it is "the root rotation angle" the viva asks you to change.

The proof holds at **any** angle. Eight were tested, including negative, 90 and 180:

```text
-60 deg: worst world error 1.7e-07, worst local change 2.4e-07
 45 deg: worst world error 2.4e-07, worst local change 1.2e-07
180 deg: worst world error 3.0e-08, worst local change 2.4e-07
```

And with heading, roll **and** pitch all set at once, every frame is still rigid and
clearing roll and pitch still changes no child's local transform.

## Why the ship starts tilted

So that `H` has something to **clear**. In the finished program the sea rocks the ship by
default and `H` is how you stop it; a ship that started level would make the key's first
press do the opposite of what its description says.

The startup banner and the console message both read the angle from the constant, so
they cannot go stale when the angle is changed. (An early version of the banner printed
"rolled 0 degrees" - a `printf` with a format and no argument. The strict build warned
about it immediately, which is exactly what that build exists for.)

## `H` through the project's real `processInput()`

The key handler was tested by driving the project's own `processInput()` with a fake
keyboard, so the key constant, the held-key edge detection and the toggle are the
project's, not a copy:

```text
H once          ->  byte-identical to the level render
H twice         ->  byte-identical to the tilted render
H three times   ->  level again
H held 5 frames ->  toggled ONCE, not five times
an unrelated key->  the ship is untouched
```

The **hold test** matters most. Without the "was it down last frame" check a held key flips
the state about 120 times a second, so you would see the ship flicker rather than right
itself.

## The rendering matches the frames to one pixel

The predicted screen bounds of the ship were computed by projecting the corners of every
part's mesh through the camera, and compared with the rendered pixels:

```text
tilted: rendered x 528..845  y 177..479    predicted x 528..846  y 177..480
level : rendered x 522..794  y 186..523    predicted x 522..795  y 186..524
```

Within a single pixel on every edge, in both states. The picture is what the matrices say.

## What `H` does not yet show

Honestly: in this phase **nothing on the ship moves by itself**. The sails hang still, the
flag does not stream, the gun does not swing. So the proof is that every child's local
transform is *unchanged* and its world transform *follows the root* - not that a part is
visibly doing something of its own while the hull stays still.

That visible version arrives in two steps. Phase 83 makes the root rock with the waves, and
Phase 84 gives the sails and flag their own idle motion. From then on pressing `H` freezes
the hull while the sails and flag **keep moving on a still hull** - the same proof, with
something visibly alive underneath it. The structure that makes that work is the one tested
here.

## Likely teacher questions

### Show me hierarchical transforms.

Press `H`. The ship rolls 45 degrees and every mast, sail, flag and the cannon come with it,
though only one matrix changed. Measured over all 17 parent-child links: no child's local
transform changed (worst 1.2e-07), and all 18 frames are exactly the level frame rotated
about the root (worst 2.4e-07).

### What does `H` actually change?

Only the root's roll and pitch. It does not touch a single child. Every child is built from
the root, so it simply follows.

### Why does it clear only the root's rotation?

Because the root's *position* and *heading* belong to the player - Phase 79 steers with them.
`H` removes only what belongs to the sea. Measured, after `H` the ship's position and heading
are untouched and the bow's bearing is exactly the heading.

### Why are position, heading, roll and pitch four separate numbers?

Because they have different owners: the player controls two and the sea controls two. Mixed
into one matrix, `H` could not clear one pair without the other.

### What order are the rotations applied in, and why?

`R_y(heading) * R_z(roll) * R_x(pitch)`, read right to left: pitch, then roll, then heading.
Roll and pitch are about the ship's own axes, so a ship facing east still rolls about its
own length.

### Does it work for any angle?

Yes. Eight angles were tested, from -60 to 180 degrees: every child follows exactly and none
changes locally.

### Why does the ship start tilted?

So `H` has something to clear. In the finished program the sea rocks the ship by default and
`H` stops it. The tilt is `PROOF_ROLL`, 45 degrees.

### Does holding `H` flicker it?

No. The handler only toggles on the frame the key first goes down. Holding it for five
frames toggles it exactly once, which was tested through the real `processInput()`.

### What will make this proof more visible?

Phases 83 and 84. Once the sea rocks the root and the sails and flag move on their own,
pressing `H` freezes the hull while the rigging keeps moving.

## Simple viva modifications

- **The named exercise: change the root rotation angle.** `PROOF_ROLL` in
  [src/Ship.h](../src/Ship.h). Try 20 degrees for a gentle heel, or 90 to lay the ship on
  its side. Every part follows at every angle.
- **Roll about a different axis:** in `shipRootMatrix`, swap which rotation `roll` uses.
- **Add a pitch:** set `pose.pitch` in `updateScene()` and the bow rises.
- **Give it a heading:** set `pose.heading` to `0.8f` and the whole ship turns - and `H`
  still leaves it facing that way.
- **Move the ship:** change `ShipPlacement::POSITION`. Everything goes with it.
- **Show that only the root changes:** print `frames.mount` with `H` on and off. It differs.
  Print `glm::inverse(frames.deck) * frames.mount`. It does not.
- **Break the hierarchy on purpose:** build one part directly from the world instead of
  from its parent, and press `H`. That part is left behind.

## Checkpoint

Phase 37 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- the root is built from a pose of position, heading, roll and pitch;
- rotating the root 45 degrees carries **every one of the 18 frames** - measured, exactly
  the level frame rotated about the root, worst error 2.4e-07;
- **no child's local transform changes** across all 17 links, worst 1.2e-07;
- `H` clears the root's roll and pitch and nothing else - position and heading untouched,
  and the bow's bearing exactly the heading;
- the proof holds at eight angles and with roll and pitch together;
- `H` toggles once per press, is not fooled by holding, and two presses restore the start,
  tested through the project's own `processInput()`;
- the rendered ship's bounds match the projected frames to within one pixel, tilted and
  level;
- with the ship hidden, ten views are **byte-for-byte identical to the real Phase 33**;
- the tilt cannot hide Demo A's highlight;
- the scene is still 28 draws / 2208 triangles / 1471 vertices.

## What is not part of Phase 37

No steering and no movement: the ship has a pose but nothing changes it - `W`, `S`, `A` and
`D` are still the camera's, until Phase 79. No waves: the roll is a constant, not the sea
under the hull. No idle motion in the sails or flag. And `H` does not yet freeze anything
that is moving, because nothing on the ship moves.

**The first part of Stage D is finished.** The prototype ship has a root, a hull and a deck, two masts with yards, sails
and a flag, and a cannon with an azimuth and an elevation - all from five meshes - and a
proof that they are one connected hierarchy.

**Phase 38 is next**: the showcase scene for the pirate galleon, which hides the test gallery with a `G` key and shows the ship at galleon scale. The prototype proves the *method*; Phases 38 to 78 build the real ship with it. Player movement - `PlayerMotion { position, heading, speed }` on `W`, `S`, `A` and `D` - is now Stage E, **Phase 79**, after the ship is built.

---

## Note: verification

### 46 new checks, no failures

| Suite | What it does | Checks |
|---|---|---:|
| Hierarchy proof | The real `Ship.h`: all 17 links' local transforms, all 18 frames' world transforms, rigidity, `H` against position and heading, rotation order, eight angles, roll and pitch together | 24 |
| Images | Byte-for-byte regression against Phase 33; `H` through the real `processInput()`; rendered bounds against projected frames; Demo A clearance | 22 |

### Every Stage D test re-run against the final header

```text
Phase 34 frames  42 checks   0 failures
Phase 35 rigging 70 checks   0 failures
Phase 36 cannon  46 checks   0 failures
Phase 37 proof   24 checks   0 failures
                -----
                182 checks   0 failures
```

### Mistakes caught

1. **A bounds check that inflated flat shapes.** The first run of the "rendered pixels match
   the predicted bounds" check failed on the tilted ship: predicted right edge 911, rendered
   845. The cause was in my test, not the ship: it treated the flat quads (sails, flag) as
   boxes with `z = +/-0.5`, adding half a unit of depth that does not exist. With quads
   treated as flat, rendered and predicted agree to a pixel.
2. **A `printf` with no argument.** The banner's `%.0f` was added and its argument was not,
   so it printed "rolled 0 degrees". The strict-warnings build flagged `C4473` at once. It
   is the clearest recent example of why that build is run every phase.
3. **A vague assertion.** My first test of "H keeps the ship's course" compared two bearings
   without an absolute value and with a loose tolerance, so it could not have failed in the
   direction that matters. Replaced with the exact statement: after `H`, the bow's bearing
   *equals the heading*, to 1e-5.

### A design decision worth recording

The ship starts tilted 45 degrees, which looks odd for the first few seconds of every
session until you press `H`. The alternative - starting level and having `H` apply the tilt -
reads more naturally for a demo, but it makes the key do the opposite of what the plan, the
PRD and the finished program say it does. The ship starting tilted matches the finished
behaviour, where the sea rocks it from the first frame. If it proves annoying in rehearsal,
it is one word: `rootTiltEnabled = false`.

### Earlier phases still correct

With the ship hidden the output is identical to the last byte to the real Phase 33 across ten
views and modes, so nothing in Stage C moved. The ship is lit by the same two lights and
shaded by the same mode as everything else.
