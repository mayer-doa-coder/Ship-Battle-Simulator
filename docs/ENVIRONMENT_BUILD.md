# Environment build - locations, weather, a dynamic day and weather, a pause menu, solid ships

Added after the playable build, on top of Phase 53 and everything before it. Like the playable build it is a separate step, not a numbered
phase: the ship's own phases (Stage D) stay paused at Phase 53. All the code is in new headers ([src/Environment.h](../src/Environment.h),
[src/Scenery.h](../src/Scenery.h), [src/Overlay.h](../src/Overlay.h), [src/Screenshot.h](../src/Screenshot.h)), a small second shader
([shaders/effects.vert](../shaders/effects.vert), [shaders/effects.frag](../shaders/effects.frag)) and the places in
[src/main.cpp](../src/main.cpp) that call them (search for "Environment build").

**The program opens exactly as before**: open ocean, sunny, sunset - the golden hour of Phase 43. The numbers of that atmosphere are the old
profile's, and a CPU check confirms `composeAtmosphere()` of the default choice equals them bit for bit.

## What you can choose

While you **play** there is no menu: the screen shows the world, a small battle HUD (bottom left) and, for a moment, a line naming whatever you just
changed. **ESC pauses** the game and opens the **options menu**; ESC again, or the RESUME button, carries on. QUIT closes the program. (In the Stage C
gallery, which has no menu, ESC still just closes it.)

| Group | Options | Keys (work in play, and in the menu) |
|---|---|---|
| Location | Open Ocean, Jungle Island, Mountain Coast, Rocky Islands, Foggy Coast | `I` (Shift: back) |
| Weather | Sunny, Cloudy, Rainy, Misty, Storm, **Dynamic** | `C` (Shift: back) |
| Time of day | Day, Sunset, Night, **Dynamic** | `T` (Shift: back) |

The same lists are buttons in the menu; click one. The land changes at once; the light **glides** over 1.4 s (the atmosphere on screen when you clicked
is the start of a blend towards the new one - the same idea as the camera's glide in Phase 40). All fixed combinations work (75 of them), and
DYNAMIC can be set for the weather, the time of day, or both.

## Dynamic day and night, dynamic weather

| | What happens |
|---|---|
| Time = Dynamic | the sun and moon move: a whole day takes `DynamicConfig::DAY_SECONDS` (240 s) of play. The sun rises, crosses the front of the ship, sets in golden light; the stars come out and the moon crosses; the lanterns light as it darkens; dawn comes round again |
| Weather = Dynamic | a new spell of weather every `DynamicConfig::WEATHER_SECONDS` (40 s), the last 10 s of each spent changing into the next: sunny, cloudy, rain, mist or a storm with lightning, fair weather the most common and a storm the rarest |

Both start from where the scene is: choosing Dynamic when it is sunset begins a day at sunset, and the first spell of weather is the weather you were
looking at, so nothing jumps. They stop with the game when you pause, and carry on from the same instant.

**How, with no stored animation (Requirement 12).** The sun's elevation is `50 degrees x cos(2 pi u)` for the day's phase `u`, which is the clock
divided by the day's length; its bearing is `50 degrees x sin(2 pi u)`. The moon is the other half: up when the sun is down (peak 30 degrees). The light
slot holds whichever is up, and when it changes hands the light dips - dusk. What the sky looks like is read off the *sun's elevation*, not the clock:
above 38 degrees it is the DAY row of the look table, at 14 degrees the golden hour (Phase 43's colours), below -8 degrees NIGHT, blended between - so a low
sun is golden whether rising or setting. The weather in force is a hash of the number of the spell the clock is in (`dynamicWeatherOfSpell`), so the
same instant always gives the same sky. Everything is in `composeAtmosphereLive()` in [src/Environment.h](../src/Environment.h); it is still the same
two lights and the same shader, fed different numbers each frame.

## Pause

The game's clock (`updateClock` in main.cpp) is the real time minus all the time spent paused, and its frame length is zero while paused. Everything that
is a closed form of that clock (rain, clouds, lightning, the day, the weather, gulls, sway, the ship's rocking) and everything stepped by the frame length
(the ships, the cannonballs, the reload) therefore holds still, and resumes from the same instant. While paused, steering, firing, the restart key and
aiming are ignored; the camera can still be orbited, and a change made in the menu still settles (it advances by the real frame length).

## Controls added or changed

| Input | Action |
|---|---|
| `ESC` | pause + open the options menu / resume (was: quit) |
| Click a menu button | choose that location / weather / time, or RESUME / QUIT |
| `I` `C` `T` (+Shift) | step location / weather / time forward (back); weather and time end with DYNAMIC |
| Right mouse button | fire (as `SPACE`); one shot per press, same reload |
| Mouse over the menu | the menu owns the mouse: no aiming, no camera drag, no zoom |
| Scroll | zoom, now out to 30 units (was 15); the step grows with the distance beyond 15 so the long range is not ten notches |
| `F11` | full screen on / off |
| `F12` | save `screenshot_NNNN.png` (beside the program) |

Everything else is unchanged: `W A S D`/arrows sail, the mouse aims, `SPACE` fires, left-drag orbits, `, . PgUp PgDn Q E` and `F1`-`F5`/`R`
move the camera, `G` is the Stage C gallery, `H` is the hierarchy proof, `K L B N M X V O 1 2 3 + -` are the lecture demonstrations.
The gallery shows none of this: it is Stage C's evidence and is drawn exactly as before.

## Ships have 20 hit points; solid things do not overlap

`PlayConfig::MAX_HEALTH` is 20 (it was 5): it takes **twenty cannonballs** to sink a ship, yours or the enemy's. The health bars above the ships and
the HUD scale with it.

**Everything solid pushes back.** A ship's shape for collision is a chain of four circles down its length, placed from its position and heading
(`SceneryConfig::HULL_CIRCLES`; the hull is 5.6 long and 1.44 wide). Land - islands, mountains, cliffs, reefs, the pier's planks, **and now the open
ocean's buoys and sea stacks** - is a list of circles seen from above.

| Meeting | What happens |
|---|---|
| ship and ship | `separateShips()`: every circle of one tested against every circle of the other; overlaps are pushed apart along the line between the circles, each ship by half, repeated until clear. Two hulls crossing like an X (whose pushes cancel) are pushed apart along the line between the ships' centres. Both lose way (`[navigation] COLLISION`). Run twice a frame with the land push, so a ram that shoves a ship into a rock is corrected too |
| ship and land | `pushShipOffLand()`: each overlapping circle asks for the push that clears it; the requests are averaged. A search for the nearest open water settles the rare wedge between circles that nearly touch (`[navigation] AGROUND`) |
| cannonball and ship | unchanged (the hull box) |
| cannonball and land | the ball stops (`[combat] the shot struck the land`) |

A ship that has finished sinking is gone and no longer in the way.

## How it works

**One equation, three choices.** A choice is only data. `composeAtmosphere()` ([src/Environment.h](../src/Environment.h)) turns
(location, weather, time) into an `Atmosphere`: the sun's (or moon's) direction and colour, the ambient light, the horizon and zenith colours,
the haze, the clouds, rain, wind, lightning, the sea's roughness, the lanterns. The renderer pours those into the *same* two lights and the
*same* shader (`LightProfile`, Phase 43). Each time of day supplies a clear-sky base **and** an overcast base; each weather says how overcast,
how misty, how dark, how wet; the two are blended. A storm at midnight is therefore the night's overcast colours darkened by the storm's
factor, not a separately painted scene. Sunny is built so every modifier is exactly neutral (a blend of 0, a scale of 1).

| Weather | Overcast | Sun let through | Mist | Clouds | Rain | Notes |
|---|---|---|---|---|---|---|
| Sunny | 0 | 100% | 0 | 12 | - | the previous build |
| Cloudy | 0.75 | 45% | low | 24, bigger | - | grey lid, soft light |
| Rainy | 0.92 | 22% | some | 24, bigger | 65% of 7000 drops | leaning with the wind |
| Misty | 0.35 | 60% | 0.85 | 6, thin | - | haze density about 2.5x |
| Storm | 1.0 | 10% | some | 24, large, dark | 100% | gale, lightning, ship rocks +-6 degrees |

**Exactly two lights, always.** At night the "sun" slot carries the **moon**: the same directional light, bluer and dimmer, with the moon's disc
placed along `-L` exactly as the sun's was (Phase 45). Lanterns are **emissive geometry** (Phase 44), never light sources; their soft halos are
translucent camera-facing squares. Nothing here adds a third light, a texture, a shadow or a skybox: the sky is still the gradient dome, now
rebuilt (888 vertices) only on frames where its colours changed.

**Nothing is pre-computed animation (Requirement 12).** Rain is ~7000 streaks whose positions are a closed form of `uTime` evaluated in the
vertex shader (one draw call, no per-frame data); lightning is a function of the clock (6-second periods, a strike in two of three, at a time the
period number chooses); clouds drift, trees sway, gulls circle and the ship rocks as sines of `glfwGetTime()`. The fixed random points (the
raindrops' start positions, the stars, the scenery) are made once from fixed seeds and are the same on every run.

**Scenery from the meshes the project already has** ([src/Scenery.h](../src/Scenery.h)): no new generator. A hill is the unit sphere flattened;
a trunk, tower or post is the unit cylinder; a boulder, cliff slab, plank or roof is the unit cube turned and stretched; a crag is the cube
stood on a corner and stretched along the world's vertical. One reusable function per object (`addPalm`, `addBroadleaf`, `addPine`,
`addBoulder`, `addCrag`, `addCliff`, `addMountain`, `addLighthouse`, `addTropicalIsland`, `addRockMass`) and one builder per location
(`buildJungleIslands`, `buildMountainCoast`, `buildRockyIslands`, `buildFoggyCoast`; the open ocean keeps the Phase 46 island table, the buoys,
sea stacks and lighthouse). A prop is a leaf of the scene, so the "scale only at the draw call" rule has nothing to leak into; a palm's fronds
are built from the *top of the trunk as a point*, never from the trunk's scaled matrix. Props outside a cone round the view direction, or beyond
the far plane, are skipped before `drawMesh`.

| Location | What it holds | Props |
|---|---|---|
| Open Ocean | the previous scenery: nine-plus islands, buoys, sea stacks, lighthouse | table |
| Jungle Island | 7 dense jungle islands (sand skirt, lumpy hill, broadleaf forest, beach palms, boulders) and sandbars | ~1300 |
| Mountain Coast | 12 mountains (snow caps on the tall ones), green foothills with pines, sea cliffs, a headland, sea stacks | ~320 |
| Rocky Islands | 13 crag-and-boulder masses, reef rocks, and a channel about 11 units wide (ship centre) straight on from the start | ~290 |
| Foggy Coast | dunes and dark pines, a pier with a hut and a lantern, a lighthouse on a headland, pilings, buoys; fog of its own | ~250 |

**Land** is a list of circles (seen from above) that every location yields. A ship that touches one is pushed back out and
loses way (`[navigation] AGROUND`); the enemy looks 14 units ahead and turns away from land; a cannonball stops when it enters land lower than its
top (`[combat] the shot struck the land`). Where circles overlap the pushes are averaged, and a search for the nearest open water settles the
rare wedge a ship cannot fit in. Choosing a new location builds its land round the player, turned to face the way they face, so you never
choose "Jungle Island" and find only empty sea; `ENTER` starts a new fight and lays it out round the start again.

**The sea follows the place and the weather**: tropical turquoise, slate blue, grey-green; lifted by day (the old colours are tuned for the golden
hour), darker and rougher in a storm, and giving back the grey or night sky's light when overcast. The ship rolls, pitches and heaves on a sine of the
clock with an amplitude the weather chooses (0.7 degrees sunny, 6 in a storm); `H` takes all of it away along with the heel, as the hierarchy proof requires.

**The panel and HUD** use no font file, texture or GUI library: each letter is a 5 x 7 grid of bits and a lit run of pixels is one rectangle, so the
whole overlay is one vertex buffer and one draw call. The layout and hit-testing are plain rectangle arithmetic, kept apart from OpenGL.

## Files changed in earlier code

| File | Change |
|---|---|
| [shaders/basic.frag](../shaders/basic.frag) | `uniform float uAlpha` (1 everywhere except clouds), `FragColor = vec4(lit, uAlpha)`; blending is off unless a translucent pass turns it on |
| [src/Camera.h](../src/Camera.h) | `OrbitCamera::uiCapture` (the callbacks ignore the mouse while the panel owns it); max radius 15 -> 30; scroll step scales beyond 15 |
| [src/Material.h](../src/Material.h) | new terrain, vegetation and emissive materials appended; the verbatim L8 materials untouched |
| [src/Shader.h](../src/Shader.h) | `setVec2` |
| [src/main.cpp](../src/main.cpp) | environment state, input, update, translucent pass, rain, stars, overlay, command-line options |

## Checks that were run

CPU (`env_test`, built outside the repository; no OpenGL context): default atmosphere equals the Phase 43 golden hour bit for bit; all 75
combinations are finite and sensible; storm darker than clear, mist at least doubles the haze, stars and lanterns at night only, lightning only in a
storm (399 strikes in 600 periods, flash never above 1); blends are exact at both ends; both fight start points are at least 7 units from land in
every location; 600 random ships (with random headings) dropped on each location all end on open water; the rocky channel is open from the start to z = 66 and about 14 units
wide at its throat (for the ship's centre); 2000 random pairs of ships dropped on top of each other (909 touching) are all separated, and pairs that were not touching are never moved; a head-on ram is symmetric; the dynamic day sampled over three days is finite, bright at noon and dark at midnight, has stars, rain, a sun and a moon, and its sky colour never jumps; the dynamic weather visits all five weathers (fair weather most often), is the same at the same instant and repeats daily; scenery is identical on every build; every button's centre hits itself at five window sizes, buttons never overlap and stay inside
their panel; text stays inside its box. Rendered and inspected: all five locations by day, all five weathers at sunset, clear night with moon and
stars, storm with lightning, jungle at night, the gallery (Demo A and B scenes unchanged, 142 draws with the ship). Frame rate stayed at the 120 Hz
v-sync cap in the heaviest scene (jungle in rain: 912 draws, 206k triangles).

**Not checked by me**: the live mouse and keyboard paths (ESC pause and resume, clicking menu buttons, `I C T`, right-click fire, `F11`, ramming and running aground by hand) were left for you to try. I rendered the pause menu, a dynamic noon, dusk and night, and ran the program in dynamic mode for six seconds at 120 fps.

## Command-line options (for reproducing a view)

`--env L W T` (indices from 0), `--camera YAW PITCH RADIUS`, `--ui 0|1|2`, `--clock SECONDS`, `--shot FILE` (save a PNG after `--frames N` frames, default 90,
then quit), `--size W H`, `--gallery`. With no arguments the program runs as always.

## Limits

- The sea is still a flat grid (the wave displacement is Phase 81 in the plan); the weather roughens its colour and rocks the ship, not the surface.
- No shadows (L8 s10-11): a lantern lights nothing around it, it only glows.
- Scenery is hazed: at 40+ units most of what reaches the eye is the horizon's colour, which is the point of the haze but means distant mountains are silhouettes.
- Props are static apart from sway; the flag does not yet flutter harder in a gale.

---

# Addendum: the living crew, the dynamic ocean, lightning, illumination and reflection

Files: [src/Crew.h](../src/Crew.h) (roles, layout, state machines, poses - no OpenGL), [src/CrewDraw.h](../src/CrewDraw.h) (the people and the ship's furniture, from the
existing sphere, cylinder and cube), [src/Waves.h](../src/Waves.h) (the waves). Keys: `Y` reflections on/off, `U` crew on/off.

## The crew

Twenty-one people on your ship, thirteen on the enemy's, each with a permanent post worked out from the ship's own dimensions and kept in the **hull's frame**, so they
roll and pitch with the ship. Each person is a joint chain (pelvis, torso, neck, head, shoulders, elbows, hips, knees) of primitives; limb sizes are applied only in the
draw matrix. One function per role: `drawCaptain`, `drawFirstMate`, `drawHelmsman`, `drawLookout`, `drawSailor`, `drawRigger`, `drawCannonCrew`, `drawDeckCrew`,
`drawCarpenter`, `drawCook`, `drawNavigator`, `drawMarine`, `drawCabinCrew` over one shared body (`drawPerson`).

| Role | Post and job |
|---|---|
| Captain | poop deck beside the wheel: points, looks to sea with hand to brow; directs the guns in a fight. Tall, red coat, feathered hat, sword, epaulettes |
| First mate | quarterdeck beat; talks toward the captain; points at the enemy |
| Helmsman | both hands on the wheel, which really turns with your steering (and the enemy's) |
| Lookout | crow's nest, binoculars; sweeps faster and leans out in fog |
| Sailors | haul ropes hand over hand at the belaying points; lash them in a storm |
| Riggers | climb the shroud to the yard and work the sail; come down in a storm or fight |
| Cannon crew | at the guns: swab, load ball, ram, then stand clear as it fires |
| Deck crew | carry barrels and crates between stacks; carry shot to the gun in a fight |
| Carpenter | workbench; goes to the damage and hammers after a hit |
| Cook | galley hut with a glowing fire door, stirs a cauldron |
| Navigator | chart table in the open captain's cabin, reads the map |
| Marines | patrol the waist and quarterdeck (climbing the ladder), muskets up at the rail in a fight |
| Steward | carries a tray from the cabin shelf to the table |

Idle animation (breathing, sway, head turns) runs for everyone. **Reactions:** rain puts on oilskins and sou'westers and slows everyone; a storm sends sailors running to
lash ropes, reefs the sails to 42%, lashes the guns, brings riggers down; night sends most of the company to sit against the rail, leaving the helm, lookout, navigator,
a marine, a gun crew and a sailor on watch; fog sharpens the lookout; **action stations** (enemy in range and a shot fired in the last 14 s, or within 16 units) man the guns
and rails; a hit makes nearby people flinch, may leave some kneeling for six seconds, and sends the carpenter and a sailor to mend the spot; a sinking ship panics.
Reload is now 1.6 s so the gun drill is visible.

## Dynamic ocean

The sea is four travelling sine waves (swell 24 units to ripple 4.6, deep-water speeds) in **one table** in `Waves.h`. The vertex shader lifts a 128 x 128 sea mesh with
them and takes the normal from their slope, and the *GLSL text is generated from the same table* the CPU reads (a test parses the text back and compares: worst difference
1e-5). The ship's heave, roll and pitch come from the water under its bow, stern and sides, so it rises to swells; buoys and cannonball splashes use the same function. The
weather sets the sea state (0.45 fair, 2.0 storm), and crests foam in storms. `H` still removes the roll and pitch. The gallery's sea stays flat and coarse (Demo A).

## Lightning, illumination, reflection

- **Lightning:** a branching bolt (main stroke plus two forks) that thins as it fades, reflected in the water; cold glows where it lands and at the cloud; clouds light up from
  inside; the whole sky flashes (as before).
- **Illumination:** still exactly two lights. The point light is lent each frame: a gun flash (0.18 s, orange, dying off) for any cannon, otherwise the lantern over the helm at
  night, so the poop deck is really lit by its lamp. Lanterns, the galley fire, the lit match and officers' hand lanterns are emissive with soft glows.
- **Reflection:** a planar mirror. The sky, ships (low-detail crew), land and bolt are drawn again upside down below the sea (reflected model matrices, flipped winding,
  reflected lights, a clip plane), then the sea is blended over it by the **Fresnel term** (Schlick, F0 = 0.03), so water mirrors at grazing angles and is clear looking down;
  a rough sea is a poorer mirror.

## Checks

CPU (all pass): every post inside the hull and on its deck; all 13 roles present; 150 s of fair weather sees every role do its job and nobody exceed walking speed; rain,
storm, night, fog, combat, hit, injury and sinking behave as above; 118,000 sampled poses keep every joint within human range and the pelvis above the floor; waves: GLSL
equals CPU, slope is the derivative, ship tilt limited to 17 degrees with the right sign. Rendered and inspected: side, top and rear views, storm, night, a bolt. Speed:
120 fps (v-sync cap) in fair weather and in a jungle storm, 1670 draws.

**Not checked by me:** live play (firing, being hit, the enemy's crew in action, ESC while crews work) and the look of ships far away.

## Limits

Crew do not collide with each other or the furniture (posts and routes are chosen to avoid it). The reflection is of one flat plane under the player's ship, so it is slightly
off under the enemy's. Ropes are straight rods. The enemy's crew is smaller and dressed darker.

---

# Addendum 2: cannon effects, ship wakes, and a proper underwater camera

Files: [src/Particles.h](../src/Particles.h) (no OpenGL), the particle mode (4) of [shaders/effects.vert](../shaders/effects.vert) / [effects.frag](../shaders/effects.frag), the sea's
underside and caustics in [shaders/basic.frag](../shaders/basic.frag), and `drawParticles()`, `renderUnderwaterScene()`, `applyUnderwater()` in [src/main.cpp](../src/main.cpp).
Test options: `--fire N` fires the player's cannon on frame N, `--drive` holds forward, `--splash` makes a splash and a ship impact, `--camera YAW PITCH RADIUS` with a negative pitch puts the camera under water.

| System | What it does |
|---|---|
| `ParticleSystem` / `updateParticles()` | one fixed pool (3200); closed forms of age; dt-based so 30 and 144 fps agree; expired particles are removed at once; a full pool makes no new ones; dt = 0 (paused) freezes everything |
| `CannonSystem` | muzzle flash, fireball, 16 sparks, 14 smoke puffs that billow to ~3 units and fade over 3-5.5 s, wisps; a thin smoke trail along the ball's flight; ship impact (flash, sparks, wood splinters, smoke, plus water if at the waterline); land impact (dust, stones). Used for BOTH ships |
| `WaterSplash` | three delayed ripples that spread and fade, a crown of droplets that fall back and vanish into the sea, mist, and a patch of foam on the surface |
| `ShipWake` | stern wash in a V, foam along the sides, bow spray and bow-wave foam, bubbles under the hull; rates scale with speed (bow spray with its square) and the sea state; a rough sea slaps even a stationary hull. Placed from the ship's position and heading |
| `UnderwaterRenderer` | the camera's depth below the *actual wave surface* above it, the smooth 0..1 blend across a 0.6-unit band, bubbles and drifting specks, and the light shafts |
| Recoil, shake, flash | the barrel slides back along its own axis (0.32 units, eased out over ~0.45 s); the view shakes by an amount falling with distance squared; the scene's one point light is lent to every flash (muzzle 7, impact 4.5, land 3) |

## The underwater camera

The blank screen came from three things: the sea was a single-sided surface (invisible from below), the sky dome still drew below it, and nothing gave the water any depth. Now:

- **Detect:** `depth = surfaceHeight(eye.x, eye.z) - eye.y`; `underwater = smoothstep` of that across +-0.3. A function of the eye only, so it works whatever the weather, time or ship.
- **The surface from below** is the real sea mesh, drawn without culling and with depth testing on. Seen from below (decided per fragment: eye lower than the surface point) it shows **Snell's window**: straight up you see the sky (brightened, with a glint of the sun or moon), outside a 97-degree cone the surface is a dark mirror (total internal reflection).
- **Fog:** the shader's haze becomes the water: teal, density 0.045 (about 25 units of sight), darker and bluer the deeper the eye, and dimmer at night. The sun loses red with depth, ambient takes the water's colour. Everything moves smoothly with `underwater`.
- **No sky below the surface:** the sky dome is drawn with alpha `1 - underwater` over the water-coloured clear colour, and is gone when fully under. Stars, clouds, rain, the reflection pass and the above-water glows are skipped.
- **Depth:** a sandy seabed 14 units down, the ship's full hull (it was always modelled below the waterline), caustic light patterns on everything under the surface that fade with depth and with fog, 11 slanting light shafts, bubbles, and drifting specks.

## Checks

CPU (all pass): every emitter's burst expires completely; 100 blasts stay within the pool; paused means frozen; a spark is in the same place after one second at 30 and 144 fps (within 0.1 unit);
droplets vanish in the sea, bubbles at the surface, foam rides the waves; wake output is zero when still and calm and grows with speed and storm; foam is astern; the underwater blend is smooth
(no step over 0.08), 0 above and 1 below, and counts a point just under a wave crest as underwater; bubbles never spawn above the surface. Rendered and inspected: muzzle blast and smoke, splash
with ripple and droplets, ship impact, storm wake, the camera below the surface at three pitches. 120 fps in a jungle storm while driving. The gallery is unchanged (142 draws).

**Not checked by me:** live play (being hit, ramming, crossing the surface by dragging the camera) and a splash from an actual shot beyond the test option.

## Limits

No refraction (the world below does not bend at the surface), no depth sorting between particles, the reflection is a single plane, and the enemy's guns other than the aimable cannon do not fire.

---

# Addendum 3: spray that matches the light, a steadier enemy, and five camera views

**Splash colour.** White water is the colour of the light on it. `ParticleSystem::waterTint` (ambient x 1.25 + sun x 0.5, clamped below white) is set every frame from the atmosphere and
multiplies every splash, ripple, foam, mist, bow-spray and bubble colour, so spray is gold at sunset, dim blue by moonlight, grey in a storm and bright (never pure white) at noon. Flash and sparks stay bright: they are fire.

**Enemy.** The dice are gone from its movement: it closes on you in a straight line, and as the range falls it turns its heading further to one side until, at about 19 units, it is broadside
and circling you (`wanted = bearing + side x offset(range)`; the side is chosen once, as the smaller turn). Too close it opens the range by turning away; land and the arena edge still override.
It fires on a steady 4.5 s beat when you are in range, aiming at where you will be when the ball arrives (your position + velocity x flight time), with a small scatter that grows gently with range
(0.5 + 0.035 x range units). Constants: `AI_FIRE_INTERVAL`, `AI_ORBIT_FAR/NEAR`, `AI_MAX_OFFSET`, `AI_AIM_SCATTER_*`.

**Camera views** ([src/Views.h](../src/Views.h)): `F6` free, `F7` first person, `F8` cinematic, `F9` captain (the same key again, or `F1`-`F5` / `R`, returns to the chase camera); also the VIEW buttons in the pause menu.

| View | How it works |
|---|---|
| Chase | the original orbit camera, restored exactly as you left it |
| Free | a detached camera: W A S D / arrows fly along where you look, E up, Q down, Shift 3x, drag or `, . PgUp PgDn` to look; it may go under the sea. The ship is not steered meanwhile (it is still fired with Space / right click) |
| First person | the eye is a point in the ship's own frame on the main deck below the forecastle, so it pitches and rolls with the waves (60% of the roll is kept); drag to look round; 64 degree FOV |
| Cinematic | five nine-second shots (orbit, low bow, crane, side tracking, chase), each a closed form of its own clock, eased into by an exponential approach so cuts become swings; within 75 units of the enemy after a recent shot it shows a wide duel shot instead; 36 degree FOV |
| Captain | the eye is at the captain's head, looking where his animation looks (at the sea, at the enemy when he directs the fight); he is not drawn; drag to look round |

Test option: `--view N` (1 free, 2 first person, 3 cinematic, 4 captain). Checks: all CPU tests pass (the menu now has 29 buttons incl. the five views); rendered every view and the splash at sunset and at night.
Not checked by me: flying the free camera by hand, the F-keys, and a long fight against the new enemy.

---

# Addendum 4: camera fixes

Found by running each view for 70 s with `--trace` (prints every frame the view turns more than 1 degree or moves more than 0.4 units) and by scripting view changes inside the program (`--viewtest`).

| Problem found | Cause | Fix |
|---|---|---|
| Captain view snapped by up to 90 degrees in a frame | the camera took the captain's body twist and head turn raw, and they change instantly when his task changes | the view follows half his twist and a third of his head turn, eased at no more than 1.1 rad/s (`stepCaptainView`) |
| Film camera whipped across the sea (up to 1.7 units a frame) and flickered between the duel shot and normal shots | an exponential ease with no speed cap; the duel test could change every frame, and its side flipped every 9 s | speed caps (eye 5.5, aim 4.5 units/s), a far initial aim point, the duel shot held 7 s after the last shot and always seen from the same side |
| Starting a new fight (ENTER) turned the free, first-person and captain views | `resetBattle` rotated the camera's yaw, which those views use as their look angle | it now does so only in the chase view |

`--viewtest` checks: chase to free changes nothing on screen; free to chase restores exactly the picture that was left (also after film and first-person); W flies the way the camera looks and the ship stays put; first-person and captain eyes sit on the main deck and the poop in the ship's own frame; the film camera starts gently; ENTER does not turn a look-around view. All 10 pass. Measured over 70 s of play: chase and free 0 jumps, first person 0.1 deg/frame worst, film 0.2, captain 0.9 (its limit). CPU tests in `vtest.cpp` cover the look conventions, flight, the 60% roll, the film camera's speed (5.5 units/s, 41 deg/s over 300 s with the duel switching on and off) and the captain's turn limit.

---

# Addendum 5: treasure, wildlife and the ship's interior

New files: [src/Treasure.h](../src/Treasure.h) (chest state and reach rules), [src/Wildlife.h](../src/Wildlife.h) (paths of birds, fish and dolphins), [src/Interior.h](../src/Interior.h) (the rooms of the hold, the walker), [src/WorldDraw.h](../src/WorldDraw.h) (drawing chests, animals, hatches). None of them has OpenGL in it except WorldDraw.h, which only calls `drawMesh()` with the existing cube, sphere and cylinder. One new shader uniform, `uFlipNormals` (the hull drawn from inside). Still two lights.

## Treasure

| Where | Kind | How you collect it |
|---|---|---|
| beach, wreck, cave, pier | `BEACH`, `WRECK`, `CAVE` | sail within 11 units, press `F` |
| summit / sea stack | `SUMMIT` | within 17 units |
| seabed | `UNDERWATER` | the camera must be under the sea within 4 units (`F6`, then `Q` to sink) |
| in your ship (5 chests: captain's cabin, crew quarters, two in the cargo hold, behind the powder) | `SHIP` | walk to it in the below-deck view (`TAB`), within 0.55 units |

Every location has 4 or 5 chests (about half in plain sight, the rest in caves, behind islands, on summits or on the seabed) and the hold has 5, so a place counts `x/9` or `x/10`. A chest glows more the nearer you are; a hidden one only shows its glow from 24 units. Opening one: the lid swings up over 1.25 s, the gold is gathered over the next 0.9 s (both closed forms of `now - openedAt`), coins and glints are thrown up, a flash lights the scene, and the counter at the top of the screen (`GOLD n  CHESTS x/y`) flashes. What persists is only `collected[]` and `openedAt[]`; a collected chest stays collected when you leave a location and come back.

## Wildlife

28 gulls and terns (their number follows the weather: none at night, in rain or in fog), 3 dolphins and 5 shoals of 9 fish. Every pose is a closed form of the clock: a bird is on a circle round the ship or an island (or a lazy figure of eight high up), a school drifts on an ellipse with each fish on its own Lissajous curve inside it, a dolphin circles ahead of the ship and every 7 to 11 s leaves the water along a parabola 2.4 units high (a splash is emitted on take-off and on re-entry). Fish are only drawn when the camera is under the sea.
Reactions: `startle()` pushes every animal within range away from a bang (a gun 1.0, a hit 0.9, a ball striking land 0.7 or sea 0.6); the push is an offset that decays by `exp(-0.45 t)`, so a flock scatters, flaps hard, and drifts back to its circle in a few seconds.

## The interior

`TAB` (or the BELOW DECK button in the pause menu) puts you on foot on the main deck. `W A S D` walk (Shift runs), the mouse looks, `F` uses what is in reach: a ladder up to the quarterdeck, the poop deck or the forecastle; the cargo hatch and the companionway down into the hold; a chest.

| Room (stern to bow) | Contents |
|---|---|
| captain's cabin | bed, desk and chair, books, ink and quill, candle, chart table with map, dividers, compass and sextant, globe, bookshelf, a chest |
| crew quarters | hammocks (three with sleepers), mess table, crates, barrels, sacks, rope, the companionway stairs, a chest |
| cargo hold | the hatch ladder, crates and barrels along both walls, flour, a weapons rack (muskets and cutlasses), rope, two chests |
| galley | stove with a cook, cauldron, work table, barrels, shelves of pots, utensils and dried fish |
| powder magazine | kegs, round shot, a crate, a chest; its lamp is behind glass in the bulkhead |

The hold is bounded by the real hull (`InteriorLayout::halfWidthAt` walks the hull's section curve), is drawn in the hull's own frame, and so rolls and pitches with the ship. Lighting: no sun, ambient (0.085, 0.068, 0.055), and the scene's one point light is lent to the lantern nearest the walker (smoothed, so it glides from one lantern to the next); the lanterns are emissive geometry with halos. The outside world is not drawn while you are below decks, and climbing through the roof fades to black and back.
Climbing through the main deck, being below the sea level or being in the hold never turns the underwater look on (it is forced off while the eye is in the hull).

## Checks

- `itest.cpp` (outside the repo): flood fill and then real `stepExplorer()` walking from the companionway to every chest and every ladder foot in the hold, and on the main deck from the spawn to every ladder and hatch; ladder feet are standable; the walker never leaves the hull; every world chest in every location has open water within reach (so the ship can legally be there), is not within reach of the start, and the sunken ones are 2+ units down and inside the free camera's range; startle decays; bird poses are finite. All pass. This test found real faults (furniture blocking doorways, a chest under the bed, ladder feet inside crates, a wreck too close to the enemy's start) which were fixed.
- the earlier CPU tests (env, particles, views) still pass.
- Rendered: all five rooms, the main deck on foot, chests closed and open on a beach, gold in the chest, dolphins and gulls, fish under the sea, the pause menu with the new rows.
- Frame rate: Release 120 fps (the monitor's limit) on the jungle island at 1280 x 720, 2458 draw calls.

Test options: `--at LEVEL X Z YAW PITCH` (stand on foot at a place; levels 0 hold, 1 main deck, 2 quarterdeck, 3 poop, 4 forecastle), `--freecam X Y Z YAW PITCH`, `--collect` (open all the world's chests), `--use N` (press `F` on frame N).

## Limits

- Walking, climbing and the chest keys were driven by the test options and the CPU tests, not by hand at the keyboard.
- No shadows (the project has none): the hold is lit by one lantern at a time plus ambient and the lanterns' own glow.
- The crew does not collide with you; people on deck walk through the player.
- The climb through the roof is a fade, not an animation of the hatch opening.
- Chests in caves, on summits and on wrecks are reached by sailing near them, not by landing: the land is not walkable.

---

# Addendum 6: world realism pass (scenery, shore, weather, sky)

Only the 3D world changed; the options menu, ship, crew, combat, wildlife, underwater look and cameras are as they were. New code: `addRockCluster`, `addCrag` (now a group), `addBush`, `addFern`, `addBroadLeafPlant`, `addOceanIsland`, a new `addMountain` and `addCliff` in [src/Scenery.h](../src/Scenery.h); `updateAmbientEffects`, `drawWaterfalls`, a new `makeSkyMesh`, and wind on sails and flag in [src/main.cpp](../src/main.cpp); `uWet` in [src/Lighting.h](../src/Lighting.h); the shoreline in [shaders/basic.frag](../shaders/basic.frag); `Atmosphere::mist` in [src/Environment.h](../src/Environment.h).

| Place | What changed |
|---|---|
| all | rocks are heaps of 3-5 blocks turned about all three axes plus a rounded boulder (never one box); crags are a main spire, one or two leaning spires and a rubble skirt; every shape is seeded from its own position, so no two match |
| open ocean | the 15 islands of the table are built from the same parts (sand skirt, grassy back, palms, bushes, stones) or crag groups instead of plain grey ellipsoids and cylinders; sea stacks are leaning piles of blocks |
| jungle | a skin of canopy blobs in three greens over each hill, emergent giant trees, ferns, banana-leaf plants, bushes at the sand's edge, eight far hills in the haze |
| mountain coast | green forested foot, bare-rock tiers, 3-5 spurs on the flanks, a jagged crown of spires, snow on the tall ones, outcrops on the slopes, a **waterfall** down the seaward flank of the three tallest; cliffs are ragged walls of leaning blocks with ledges and a grass cap |
| rocky islands | irregular stacks and boulder skirts; a pale beach with a palm on the small and grassy ones; caves are heaps of stone under fallen slabs |
| foggy coast | a second, higher ridge behind the first, stones on the shore |

**Shore.** The sea shader knows the nearest 24 land circles (`uShore`): over the shelf the water turns clear green-blue and loses its mirror, at the edge it shows sand, and a line of surf breathes in and out; foam particles are also shed along the beaches of the round islands. **Rain** wets the world: `scene.wetness` rises in a few seconds of rain and dries over half a minute; wet surfaces are up to 38% darker, their specular exponent is pulled up to 70+ and a sheen is added (the sea, sky and gallery are drawn dry), and rain pocks the sea with rings and tiny spouts. **Mist** is now a field of the atmosphere; misty weather has drifting pale puffs at the water and a lower haze, so near things stay clear while the distance goes. **Storm:** sails billow about their yards and the flag streams, both scaled by the wind. **Night jungle:** fireflies over the nearer islands. **Waterfalls** are three lanes of rods with a travelling brightness, a foam pool and spray, dimmed with the light of the hour.
**Sky.** The dome is built vertex by vertex: the colour rises from horizon to zenith along a curve (a thin bright band under a deep vault), brightens round the sun or moon in the light's own colour (a tight core in a broad glow that hugs the horizon when the sun is low), and is darker and cooler on the side away from a low sun; it follows the sun in a dynamic day. Clouds are clumps of four lumps (underside darker), alternate ones on a higher, thinner deck. Dolphins and gulls were redrawn/lightened (they read as black shapes).

**Checks.** CPU tests all pass (the default's haze is now 0.022: the sunset is lighter); 17 renders across the five places, the three times and the weathers; Release 120 fps (the vsync limit) in the heaviest places (up to 2,445 draw calls, 494k triangles). The gallery is unchanged (142 draws).

**Not done, and why.** Real shadows: the project is a local-illumination renderer by design (no shadow maps), so there are no cast shadows; the time of day shows in the light's direction and colour and the sky. Thunder: the project has no audio. freeGLUT/GLU: the project uses GLFW and GLM, not changed. Instancing: props are drawn one by one (distance-culled at 98 units); a few thousand draws are within budget and a batching change was not needed. The real fireflies, mist and surf are checked by rate arithmetic and a few renders, not frame by frame.
