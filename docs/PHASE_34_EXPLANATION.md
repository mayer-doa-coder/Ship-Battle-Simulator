# Phase 34 - The Ship's Root, Hull and Deck

## Status

Phase 34 is complete and verified. It is a **graded milestone** (`*` in the plan) and
the **first phase of Stage D**, the ship hierarchy.

```text
Ship Battle Simulator - Phase 34 | draws 16 | tris 1864 | verts 1093
```

Two more draws than Phase 33 - the hull and the deck - and **no new mesh**. Both are
the same unit cube from Phase 14, at two different sizes.

58 checks passed with zero failures: 42 exact checks on the frames, 16 on the images.

### THE CHECKPOINT: a hull and a deck, and the deck is not stretched like the hull

The deck was built beside hulls of six very different sizes, and the size at which the
deck was actually *drawn* was measured each time:

```text
hull size asked for              | applied hull size       | applied DECK size
default       (0.42, 0.20, 1.40) | (0.420, 0.200, 1.400)   | (0.360, 0.030, 1.240)
4x longer     (0.42, 0.20, 5.60) | (0.420, 0.200, 5.600)   | (0.360, 0.030, 1.240)
3x wider      (1.26, 0.20, 1.40) | (1.260, 0.200, 1.400)   | (0.360, 0.030, 1.240)
5x deeper     (0.42, 1.00, 1.40) | (0.420, 1.000, 1.400)   | (0.360, 0.030, 1.240)
squashed      (0.08, 0.04, 0.28) | (0.084, 0.040, 0.280)   | (0.360, 0.030, 1.240)
absurd        (20.0, 0.10, 0.30) | (20.00, 0.100, 0.300)   | (0.360, 0.030, 1.240)
```

**The last column never changes.** That is the checkpoint, stated as a number.

And the same test run against a ship built the *wrong* way:

```text
deck meant to be (0.36, 0.03, 1.24)
the naive tree draws it (0.151, 0.006, 6.944)
```

The deck comes out nearly **seven units long** - a 4x stretch it should never have
received - and a fifth of its intended thickness.

## What changed

| File | Change |
|---|---|
| `src/Ship.h` | **New.** `ShipDimensions`, `ShipFrames`, `buildShipFrames()`, `shipHullModel()`, `shipDeckModel()`. No OpenGL in it |
| `src/Material.h` | New tuned `DECK_WOOD` |
| `src/main.cpp` | `ShipPlacement::POSITION`; `SceneState::shipFrames`; `drawShip()`; two draws |

## The one idea

**A frame says where something is. A size says how big it is. Only the frame is
inherited.**

```cpp
// WRONG: the hull's matrix includes its scale, and the deck is built from it.
glm::mat4 hull = root * glm::scale(glm::mat4(1.0f), hullSize);
glm::mat4 deck = hull * glm::translate(...);        // the deck is now hull-shaped

// RIGHT: store the frame without any scale, and scale only when drawing.
f.hull = f.root * glm::translate(glm::mat4(1.0f), liftToHullCentre);
f.deck = f.hull * glm::translate(glm::mat4(1.0f), liftToDeckCentre);
...
drawMesh(..., f.hull * glm::scale(glm::mat4(1.0f), hullSize), ...);   // scale HERE only
```

Phase 16 introduced this rule for single objects. A ship is a **tree**, and a tree
inherits, so the rule stops being tidy and becomes the thing that decides whether the
ship works. A hull stored as `root * scale(0.42, 0.20, 1.40)` would pass that scale to
the deck, the deck to the mast, the mast to the yard, the yard to the sail. Every part
of the ship would be squashed to a fifth of its height and stretched 1.4 times along
its length.

### How do you tell, mechanically, that a frame holds no scale?

A matrix that holds only translation and rotation is a **rigid motion**. Its three
basis vectors are all exactly length 1 and at right angles, and its determinant is +1.
Any scale breaks the first condition, so that is what was tested:

```text
placed:             root, hull, deck  - all rigid, error 0.000000
rolled 45 deg:      root, hull, deck  - all rigid, error 0.000000
arbitrary rotation: root, hull, deck  - all rigid, error 0.000000
```

## The deck depends on the hull, but only by one number

This is the subtle part, and it is worth being precise about.

The deck has to sit **on** the hull. So its height above the hull's centre depends on
how deep the hull is:

```cpp
f.deck = f.hull * glm::translate(glm::mat4(1.0f),
             glm::vec3(0.0f, d.hullSize.y * 0.5f + d.deckSize.y * 0.5f, 0.0f));
```

The hull's *size* appears here - as a **number**, used to work out how high to lift
the deck. The hull's *scale matrix* does not appear, because there is none to multiply
by. The deck's position depends on the hull, which is correct, and its size does not,
which is the checkpoint. That is exactly the right amount of dependence.

Measured across four very different hull depths, the deck's underside meets the hull's
top surface **exactly** every time:

```text
hull depth 0.20: deck underside -1.25000 meets hull top -1.25000
hull depth 1.00: deck underside -0.45000 meets hull top -0.45000
hull depth 0.05: deck underside -1.40000 meets hull top -1.40000
```

## Why the dimensions are an argument, not a constant

```cpp
inline ShipFrames buildShipFrames(const glm::mat4& root, const ShipDimensions& d)
```

The task is to "change the hull dimensions and show the deck is unaffected". If the
sizes were `constexpr` values read from inside the builder, the only way to try a
different hull would be to edit the source and rebuild. Passing them in as a struct
means the very same function runs with a different hull in the same program - which
is how the six-hull table above was produced, in one run.

The same reason is why the model functions live in `Ship.h` rather than inline in
`main.cpp`:

```cpp
inline glm::mat4 shipHullModel(const ShipFrames& f, const ShipDimensions& d)
{
    return f.hull * glm::scale(glm::mat4(1.0f), d.hullSize);
}
```

The test calls this **very function** the renderer calls, so the thing that was
measured is the thing that is drawn. There is no copy that could drift.

## `Ship.h` contains no OpenGL

That is deliberate. It means every claim above could be tested **exactly**, on the CPU,
with no window, no driver and no pixels - 42 checks in a fraction of a second. The
image checks then only need to confirm that what was measured is what appears.

## Where the ship sits, and why

```cpp
const glm::vec3 POSITION(-0.20f, -1.40f, -1.20f);
```

At the back of the sea, which is the one clear patch of the gallery, and well away from
the front edge where Demo A's highlight lands. Demo A's brightest pixel was
unprojected onto the sea's plane:

```text
Demo A's brightest pixel is at world (-0.03, -1.40, 0.88)
the ship's footprint, with a 0.3 margin: x in [-0.71, 0.31], z in [-2.20, -0.20]
the peak is 1.08 units clear of it
```

**Nothing about the ship may interfere with either Stage C demonstration**, and that
is now a measured margin rather than an intention.

### The root is at the waterline

Its y is the sea's own y, so the hull is lowered below it by a **draft** of 0.05. A hull
centred on the root would float half out of the water, on a ship that should sit in it.

### An honest note about visibility

From the default front camera the ship is mostly **hidden behind the test triangle and
the quad**, which sit between the camera and the back of the sea. This is the cost of
adding a ship to a scene that is still a test gallery. To inspect it, drag the camera
round behind it (a yaw of 180 degrees) and zoom out. Phase 80 makes the camera follow
the player ship, which removes the problem for good.

## Where each job happens

| Place | Job |
|---|---|
| `buildShipFrames()` | Builds the root, hull and deck frames - rigid motions only |
| `shipHullModel()`, `shipDeckModel()` | The only place a ship part's scale is applied |
| `drawShip()` | Two `drawMesh` calls, from the one cube mesh |
| `updateScene()` | Rebuilds the frames each frame from the root |

## Likely teacher questions

### What is a frame, and why does it not store a scale?

A frame is where a node is and how it is turned - a translation and a rotation. It does
not store a size because a node's children are built from its frame, so any size stored
in it would be inherited by every descendant.

### What would go wrong if the hull stored its scale?

The deck would be built from the scaled hull matrix and come out hull-shaped. Measured
with a hull 4x longer: the deck meant to be 0.36 x 0.03 x 1.24 comes out
0.151 x 0.006 x 6.944. And the mast on the deck would inherit that too, as would
everything above it.

### How do you know a stored frame has no scale?

It is a rigid motion, so its three basis vectors are exactly length 1, mutually
perpendicular, with determinant +1. Any scale breaks that. All three frames pass at
three different roots, including a 45 degree roll and an arbitrary rotation, with an
error of zero.

### The deck uses the hull's size in its offset. Isn't that inheriting?

It uses the hull's **size as a number** to work out how high to lift the deck, because
the deck must sit on whatever hull it is on. It does not use the hull's **scale
matrix**. The deck's position depends on the hull and its size does not - and that is
exactly right.

### Show me that changing the hull does not affect the deck.

Six hulls, from a fifth of the normal size to 20 units wide: the deck is drawn at
0.360 x 0.030 x 1.240 every single time, and its underside meets the hull's top
exactly.

### Why is there no new mesh for the ship?

The hull and the deck are both the unit cube from Phase 14, drawn at two sizes. The
ship adds two draw calls and nothing to graphics memory. That is the mesh-reuse rule
the whole project is built on.

### Why is `Ship.h` free of OpenGL?

So the frames can be tested exactly on the CPU. Every claim in this phase was checked
without a window or a single pixel.

### Where is the ship in the scene, and can you see it?

At the back of the sea, clear of Demo A by 1.08 units. From the default camera it is
mostly hidden by the test triangle; drag round behind it or zoom out. Phase 80 makes
the camera follow the player.

## Simple viva modifications

- **The named exercise: change the hull dimensions and show the deck is
  unaffected.** `ShipConfig::DEFAULT_DIMENSIONS.hullSize` in
  [src/Ship.h](../src/Ship.h). Try `(0.42, 0.20, 4.0)` and the hull stretches to four
  units while the deck stays exactly the shape it was.
- **Show the wrong way:** in `drawShip()` replace `shipDeckModel(frames, dims)` with
  `shipHullModel(frames, dims) * glm::scale(glm::mat4(1.0f), dims.deckSize)` and watch
  the deck turn into a long thin needle.
- **Change the draft:** `hullDraft`. A larger value sinks the hull into the sea; zero
  floats it on top.
- **Change the deck's size:** `deckSize`. The hull does not change at all - the
  independence works in both directions.
- **Prove the frames are rigid:** print the length of `frames.deck[0]`. It is exactly
  1.0, whatever the hull.
- **Move the ship:** `ShipPlacement::POSITION`. One edit, and both parts follow.

## Checkpoint

Phase 34 passes when:

- Debug, Release and strict `/W4` build with no warnings of any kind;
- `src/Ship.h` exists, contains no OpenGL, and builds the root, hull and deck frames;
- **every stored frame is a pure rigid motion** - measured, error 0 at three roots;
- `glm::scale` appears only in the two model functions that feed `drawMesh`;
- the deck is drawn at **exactly its own size** beside every hull tried, from a fifth
  of normal to 20 units wide;
- a ship built the naive way fails the same test, so the test genuinely detects the
  bug;
- the deck's underside meets the hull's top exactly, whatever the hull's depth;
- the silhouette has the proportions the frames say - measured, deck/hull length 0.877
  against 0.886 expected, and length-to-beam 3.26 against 3.33;
- with the ship hidden, **ten different views are byte-for-byte identical to the real
  Phase 33**;
- Demo A's highlight is 1.08 units clear of the ship;
- the scene is 16 draws / 1864 triangles / 1093 vertices, +2 / +24 / +48 on Phase 33.

## What is not part of Phase 34

No mast, no sail, no cannon, no crew: Phases 35 and 36. No movement, no rocking and no
animation of any kind - the root is a fixed placement. No `H` key yet: Phase 37.

Phase 35 adds the **rigging chain** - mast, yard, sail and flag, each a child of the one
above - and its checkpoint is a recognisable ship silhouette.

---

## Note: verification

### 58 checks, no failures

| Suite | What it does | Checks |
|---|---|---:|
| Frames | Compiles the real `Ship.h` and tests the frames exactly: rigidity, applied sizes across six hulls, a deliberately wrong ship, seating, and root translation | 42 |
| Images | Byte-for-byte regression against the real Phase 33, the Demo A clearance, and the silhouette proportions | 16 |

### A regression test against the real previous phase

The strongest check in this phase is not about the ship. With the ship hidden, ten
renders - default, from above, Flat, Gouraud, lowest detail, highest detail, from
behind, the normals view, specular-only and Blinn-Phong - were each compared with the
same view rendered by the **actual Phase 33 code**, which was saved as a patched copy
at the time. All ten are identical to the last byte.

That is a much stronger statement than "the old tests still pass". It says nothing in
Stage C moved.

### Two mistakes of mine in the tests, both caught

1. **A dangling `else`.** The first regression check reported every comparison as
   `-1 differing bytes`. The cause was an `else` that bound to the *inner* `if`
   instead of the outer one, so every byte that was *equal* set the counter to -1. The
   rendering was fine; the test was reporting its own bug.
2. **A check that asked a question the scene answered trivially.** The first version of
   the Demo A clearance check asked whether the ship changed any pixels in the
   pitch-32 view. It changed none - the test triangle hides the ship from that angle -
   so "the ship does not overlap the highlight" passed for the wrong reason. Replaced
   with the highlight's **world-space position**, found by unprojecting its brightest
   pixel onto the sea's plane, and a measured clearance of 1.08 units.

### Looking at the images, for the first time

Every earlier Stage C check read pixels numerically. For this phase the renders were
converted to PNG and **looked at**. That turned up the visibility problem described
above - the ship is mostly hidden from the default camera - which no number would have
shown, because the pixel checks all passed.

### Earlier phases still correct

Every key from every earlier stage behaves as before, and the ship is lit by the same
two lights and shaded by the same mode as everything else: `1`/`2`/`3`, `K`, `B`, `L`,
`N`, `M`, `W`, `D`, `O`, `+`/`-`.
