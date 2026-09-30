# The Object Scene - Ship, Enemy Ship, Crew, Sun, Sea, and Props

This is separate from [docs/PHASE_PLAN.md](PHASE_PLAN.md), which is still
paused at the end of Phase 14. This document is about a different piece of
work: building real, detailed objects now - a ship, an enemy ship, a
five-person crew, the sun, the sea, and some deck props - so there is
something a teacher can actually look at and be shown today.

## Three fixes made after the first look

Looking at the first version, three things needed fixing:

1. **A crew member's head was overlapping the flag's sail.** The crew
   member standing near the back mast was placed too close to it - close
   enough that, from most angles, their head appeared to poke straight
   through the sail instead of standing clearly in front of or beside it.
   Every crew member's spot on deck has been moved to keep clear space
   between their head and both sails.

2. **The wheel was just a plain solid circle, with no spokes.** A real
   ship's wheel is recognisable because of its spokes - the lines
   radiating from the centre out to the rim. The wheel now has a small
   hub in the middle and six spokes radiating out to the edge, in a
   lighter wood colour than the dark disc behind them, so the spokes are
   actually visible instead of blending into one plain shape. (From
   square-on behind the wheel, one or two spokes point almost straight at
   the camera and look shorter than the rest - that is normal, expected
   perspective, the same way a real wheel's spokes look different lengths
   from different angles. Drag to orbit around it and every spoke is
   there.)

3. **The bottom of the hull was a plain rectangular box.** A real ship's
   hull is wider at deck level and narrows toward the bottom (the keel).
   The hull is now built from a new tapered shape instead of a plain box -
   full width at the top, narrower at the bottom - so it reads as a hull
   instead of a floating brick.

## Turning the box into an actual pirate ship

The fix above only made the hull narrower at the bottom - it was still a
plain rectangle when seen from above or from the side, the same width from
bow to stern. A real sailing ship is not that shape at all, so the hull,
and the ship built on top of it, were reworked properly:

- **The hull now has a pointed bow and a flat stern**, not the same width
  all along its length. It is built from five cross-sections spaced along
  its length - almost a point at the front, full width a little past the
  middle, and a flat (not pointed) face at the back, the way a real ship's
  stern is usually built. Both ships use this same shape.
- **The ship is about a third longer** than before, to comfortably fit
  three masts, a raised rear deck, and the crew without feeling cramped.
- **A third mast was added.** Real sailing ships of this kind have three -
  fore, main, and mizzen - not two. The flag now flies from the back-most
  one (the mizzen), as it would on a real ship.
- **A bowsprit** - the pole that angles up and out from the bow - was
  added. It is one of the most recognisable parts of a sailing ship's
  silhouette, and the ship did not have one before.
- **A raised quarterdeck** was added at the stern, a step above the main
  deck, which is where the wheel now stands - on a real ship, the
  helmsman stood on a raised deck, not down among the rest of the crew.
- **The wheel now has eight spokes** instead of six, better filling its
  now-larger size.

None of this needed any lighting or new drawing technique - it is still
flat colours and the same "translate, then scale, then tint" recipe every
object in this project already uses. What changed is the SHAPE the hull is
built from: a new generator, `makeUnitShipHull()`, replacing the plain
tapered box.

## New scenery: mountains, clouds, and rocks

The scene no longer floats in an empty sky over an empty sea:

- **A full ring of twelve mountains** surrounds the scene on every side,
  each with a small lighter "snow cap" near its peak, and each a little
  different in size and distance from the next so the range does not look
  like the same shape copied and evenly spaced. The first version of this
  only put two mountains behind the ships, which meant orbiting the camera
  the other way around showed empty sky - a full ring fixes that.
- **Three clouds** drift in the sky, each built from a small cluster of
  overlapping spheres rather than one plain ball, so they read as fluffy
  rather than as obviously round.
- **Three rocks** break the water's surface, the same "cluster of
  overlapping spheres" idea as the clouds, in a dark grey instead of white.

**A real bug the ring found:** zooming out and orbiting far enough could
put the camera INSIDE a mountain, filling the whole screen with one flat
colour (the mountain's own colour, with no picture left to make sense of).
The ring's distance from the centre of the scene was smaller than the
camera's own farthest possible zoom - so at the right combination of angle
and distance, the two occupied the same space. The fix was to move the
whole ring much farther out - safely beyond the camera's maximum zoom
distance, with real room to spare - and make the mountains bigger to match,
so they still read clearly from that greater distance.

**Another gap the ring left behind:** moving the mountains much farther out
left a wide ring of bare, empty background between the edge of the sea and
the foot of every mountain, since the sea was only ever sized to comfortably
fit the two ships. The sea is now sized to reach past the mountains' own
outer edge, so every mountain now appears to rise directly out of the water
instead of floating above empty space behind it.

These are backdrop, not objects a teacher would usually ask to inspect up
close, so none of them got their own number-key view the way the ship,
the cannon, and the props did - they only appear as part of the full `0`
scene. [src/Scenery.h](../src/Scenery.h) holds all three, and each one is
still built entirely from the project's existing cube, cylinder, and sphere
shapes - the mountains reuse the same tapered-box shape an earlier version
of the hull used, just flipped upside down and drawn with a much sharper
taper.

## A live coordinate readout in the title bar

The window's title bar now shows, updated every frame, where your mouse is
actually pointing in the 3D scene - not on screen, but the real
`X`/`Z` position in the world (or in whatever single object you're
currently looking at). Two readings are shown together: one at ground
level (`y = 0`, which is also every object's own natural base - a hull's
bottom, a crew member's feet, a prop's base, since every isolated view
draws its object sitting exactly at the world's origin), and one at
whatever height the camera is currently looking at. Either can say "not
looking at this height" - that just means the mouse is pointed somewhere
(the sky, say) that never reaches that particular height at all.

This is meant as a practical aid for the exact thing this project already
does everywhere: every position in every file is a named constant
(`SceneConfig::CREW_LOCAL_POSITIONS`, `ShipShape::MOUNT_X`, and so on) that
gets typed in, then the program is rebuilt to see the result. Hovering the
mouse over the spot you want something to sit gives you real numbers to
type in, instead of guessing and rebuilding repeatedly.

**This is a coordinate reader, not a coordinate editor.** Dragging an
object around live on screen - the way a proper 3D editor like Blender
lets you grab and move something with the mouse - is a much bigger, separate
feature: it would need every object's position to become something that
can change while the program runs (instead of a fixed constant), plus
on-screen handles to grab and drag, plus hit-testing to know which handle
the mouse is over. That has not been built, and is worth planning
separately if it's wanted - the reader here is the first, smaller, useful
piece of that idea.

## What you can see right now

Run the program and you see the whole scene: your ship sailing near an enemy
ship, five crew members standing on your ship's deck, the sun in the sky,
the sea all around, and three props (a barrel, a crate, a cannonball) on
deck.

**Every object can also be shown by itself.** Press a key to switch:

| Key | Shows |
|---|---|
| `0` | Everything together (the starting view) |
| `1` | Your ship, up close |
| `2` | Just the cannon |
| `3` | Just the flag |
| `4` | One crew member, up close (good for showing the face) |
| `5` | The sun, up close |
| `6` | The sea |
| `7` | A barrel |
| `8` | A crate |
| `9` | A cannonball |
| `E` | The enemy ship, up close |

Every time you switch, the camera jumps to a good distance and angle for
that object automatically - nothing appears as a tiny dot or a paper-thin
sliver. You can still drag with the left mouse button to look around it,
and scroll to zoom closer or farther. Press `W` to see the wireframe
(the triangles each shape is built from).

Nothing moves by itself yet, and nothing is lit yet (no shadows, no shiny
highlights) - those are separate ideas that come later in the step-by-step
plan. Every object here is solid, flat colour.

## What is new or improved, in plain terms

### The ship is bigger, and has a wheel now

The ship grew to about one and a half times its old size, so five people
fit comfortably on deck instead of one. It also has a real wheel (the
steering wheel, sometimes called the helm) near the back - a post with a
round wheel on top, standing where a helmsman would use it.

### The ship also has round windows (portholes)

Six small dark circles, three down each side of the hull, so the hull
reads as a real ship's side instead of a plain brown box.

### There is now an enemy ship

It is built exactly the same way as your ship - same hull, same cannon,
same wheel - but painted in dark, near-black colours instead of warm wood
tones, so the two are easy to tell apart at a glance. It is turned to face
your ship, like two ships meeting at sea.

### There are five crew members now, not one

Each one stands at a different spot on deck (one at the wheel, one at the
cannon, and so on), and each wears a different coloured shirt (blue, red,
green, grey, tan) so they are easy to count and tell apart, even though
none of them have a job to do yet - that comes later.

### Each crew member has a real face and a real body now

Before, a crew member was five plain boxes. Now each one has:

- **two separate legs**, with a small gap between them, instead of one
  wide block for both legs;
- **hands** - a small round hand at the end of each arm;
- **two eyes**, **two ears**, a **nose**, and **lips** on the head.

The nose and ears are a slightly different shade of skin colour than the
rest of the head. This matters because the scene has no lighting yet - a
same-coloured bump on a same-coloured head would be invisible, since there
is no shading to show its shape. A different shade is what makes them
actually visible.

### The barrel and the crate look like a barrel and a crate now

The barrel has two dark bands wrapped around it, near the top and the
bottom, the way a real wooden barrel is held together. The crate has two
straps crossing on its lid, like a strapped-shut chest. Before this, both
were just a plain cylinder and a plain cube.

### The sun is a proper circle now, and looks brighter

The sun used to sit far off to one side of the view, and a sphere viewed
from near the edge of the camera's view gets stretched into an oval - this
is just how camera perspective works, the same way a very wide-angle photo
stretches things near its edges. Moving the sun closer to the middle of the
view fixed this, and it is now a clean circle. It is also built from two
spheres, one smaller and brighter sitting inside the bigger one, so it
reads as a glowing ball instead of one flat-coloured disc.

### Everything looks smoother

Two changes together make every round shape (masts, barrels, heads, the
sun) look noticeably rounder and smoother instead of having visible flat
edges:

- the round shapes are now built from twice as many slices as before;
- the picture itself is smoothed at every edge (called anti-aliasing),
  which removes the small stair-step jaggedness a screenshot would
  otherwise show.

## A real bug this work found and fixed

While testing the flag's own close-up view, it showed up as a paper-thin
red line instead of a red rectangle. The cause: the camera math was always
circling around the exact centre of the world, no matter what it was
supposed to be looking at. That never showed up as a problem before,
because everything the camera had ever looked at happened to sit close to
the centre of the world. The flag sits high up, far from the centre, so the
mistake finally became visible. It is fixed now - the camera correctly
circles around whatever object is actually being shown. This also made the
starting view of the whole scene look slightly better than before, even
though nothing there looked obviously wrong.

## What changed, file by file

| File | Change |
|---|---|
| [src/Mesh.h](../src/Mesh.h) | Added `makeUnitTaperedBox()` (narrower at one end than the other - used for the mountains) and `makeUnitShipHull()` (a proper pointed-bow, flat-stern boat shape, replacing the tapered box as the hull) |
| [src/Ship.h](../src/Ship.h) | Ship enlarged and rebuilt on the new hull shape; a third mast (mizzen) added; a bowsprit and a raised quarterdeck added; the wheel rebuilt with a hub and spokes instead of a plain disc; portholes added; every part's colour comes from a "palette," so a second, differently coloured ship can reuse the same building code |
| [src/Crew.h](../src/Crew.h) | Separate legs, hands, eyes, ears, nose, and lips added; each crew member can now be given its own shirt colour |
| [src/Props.h](../src/Props.h) | The barrel gained two bands; the crate gained two crossed straps |
| [src/Scenery.h](../src/Scenery.h) | New. `drawMountain()`, `drawCloud()`, `drawRock()` - background scenery built from the project's existing shapes |
| [src/main.cpp](../src/main.cpp) | Added the enemy ship and four more crew members, repositioned for the larger ship and to keep clear of the sails; added two mountains, three clouds, and three rocks; fixed the sun's position and gave it a brighter core; fixed the camera bug described above; raised the round shapes' detail; turned on anti-aliasing; added the live mouse-coordinate readout in the title bar (`mouseToWorldPoint()`, `updateMouseCoordinateTitle()`) |

## Easy changes for a viva

| Want to change | Edit |
|---|---|
| The ship's size | `ShipShape::HULL_WIDTH` / `HULL_HEIGHT` / `HULL_LENGTH` in [src/Ship.h](../src/Ship.h) |
| Any ship part's colour | `ShipShape::PLAYER_PALETTE` or `ENEMY_PALETTE` in the same file |
| The hull's silhouette (how pointed the bow is, how wide the stern is) | The `stations` table inside `makeUnitShipHull()`, [src/Mesh.h](../src/Mesh.h) |
| How narrow the hull's bottom is | `ShipShape::HULL_BOTTOM_SCALE` (0 to 1; 1 gives back a flat-bottomed hull) |
| A mast's position | `FORE_MAST_Z` / `MAIN_MAST_Z` / `MIZZEN_MAST_Z` in [src/Ship.h](../src/Ship.h) |
| The bowsprit's length or angle | `BOWSPRIT_LENGTH` / `BOWSPRIT_PITCH_DEGREES` |
| The quarterdeck's size or height | `QUARTERDECK_LENGTH` / `QUARTERDECK_WIDTH` / `QUARTERDECK_HEIGHT` |
| The wheel's size or position | `WHEEL_RADIUS`, `WHEEL_POST_HEIGHT`, `WHEEL_Z` |
| How many spokes the wheel has | `spokeCount` inside `drawWheel()`, same file |
| How many portholes, or where | `PORTHOLE_ZS` |
| A crew member's shirt colour | `SceneConfig::CREW_SHIRT_COLORS` in [src/main.cpp](../src/main.cpp) |
| Where a crew member stands | `SceneConfig::CREW_LOCAL_POSITIONS` |
| Where the enemy ship sits | `SceneConfig::ENEMY_SHIP_POSITION` |
| A face feature's size (eyes, nose, ears, lips) | Its constant in `CrewShape`, [src/Crew.h](../src/Crew.h) |
| The barrel's bands or the crate's straps | `HOOP_COLOR`/`HOOP_RADIUS_SCALE` or `STRAP_COLOR`/`STRAP_WIDTH` in [src/Props.h](../src/Props.h) |
| The sun's position, size, or colour | `SceneConfig::SUN_POSITION` / `SUN_RADIUS` / `SUN_COLOR` in [src/main.cpp](../src/main.cpp) |
| The sea's size or colour | `SceneConfig::WATER_WIDTH` / `WATER_LENGTH` / `WATER_COLOR` (keep it reaching past the mountain ring, or the mountains stop looking like they rise from the water) |
| How many mountains ring the scene, or how far away | `SceneConfig::MOUNTAIN_RING_COUNT` / `MOUNTAIN_RING_RADIUS` (keep this well beyond `Camera.h`'s `MAX_RADIUS`, or the camera can end up inside one) |
| A mountain's size, or how much peaks vary | `SceneConfig::MOUNTAIN_BASE_WIDTH` / `MOUNTAIN_BASE_HEIGHT` / `MOUNTAIN_DEPTH` / `MOUNTAIN_SIZE_VARIATION` |
| How many clouds or rocks, or where | `SceneConfig::CLOUD_POSITIONS` / `ROCK_POSITIONS` and their matching `_COUNT` |
| How round everything looks | `SceneConfig::CYLINDER_SEGMENTS`, `SPHERE_STACKS`, `SPHERE_SLICES` |
| How smooth the picture's edges look | `AppConfig::MSAA_SAMPLES` (try `0` to see the difference) |
| What a single object's close-up view looks like | Its line in `viewPresetFor()`, `src/main.cpp` |

## What you can demonstrate to a teacher

- **Press `0`**, then orbit around: a full little sea battle scene, two
  ships facing each other, a crew, a sun, and props on deck.
- **Press `E`** right after `1`: the same ship-building code, just
  recoloured, proving the ship and the enemy ship are not two separate,
  hand-built things.
- **Press `4`**, then scroll in close: a crew member's face - eyes, ears,
  nose, and lips - all built from nothing but boxes and spheres.
- **Press `W`** on any object: the wireframe shows exactly how many
  triangles make up a "round" shape like the wheel or a head.
- **Press `7`** and **`8`**: the barrel's bands and the crate's straps -
  small details that stop a shape from looking like a plain primitive.

## What is not part of this work

- **No lighting.** Every object is one flat colour; there is no shading,
  no shadows, and no shiny highlights yet. That comes later, in Stage C of
  the phase plan.
- **No motion.** Nothing sails, nothing aims, nothing fires, and the crew
  do not move or have jobs yet. That comes later too, in Stages E through K.
- **The phase plan itself is unchanged.** It still resumes at Phase 15
  whenever that step-by-step work continues; this scene exists alongside
  it, not instead of it.
