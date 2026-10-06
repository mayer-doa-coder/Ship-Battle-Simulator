# Ship Reference Review - the prototype against the ships it is meant to imitate

Written 5 October 2026, before Phase 38. It does three things: records what could and could
not be established about the two reference ships, judges the Phase 34-37 prototype against
them, and lists the changes it caused to the plan.

**What this is not.** It is not a measurement of any film or game model. No model files were
available, and every number below comes from a web page that is linked. Where a page did not
say something, this document says "not stated" and does not fill the gap from memory.

## 1. What the references say

### The Jackdaw (Assassin's Creed IV: Black Flag)

| Fact | What was found | Source quality |
|---|---|---|
| Class | A **brig**; one source adds that her main sail makes her closer to a *snow* | Fetched page: [AC wiki (Jackdaw)](https://assassinscreedwiki.accesstheanimus.com/wiki/Jackdaw). The snow remark is from a search summary only |
| Masts | **Two**, both square-rigged. The page lists "twenty-six sails" but does not say how many masts or which is tallest | Two masts: search summary of [the Brig article](https://assassinscreed.fandom.com/wiki/Brig) (that page itself could not be fetched, HTTP 402). Mast order: **not verified** - one search summary said the foremast is taller, the page I could fetch did not confirm it, so it is not used |
| Size | "60 meters long from her stern to the tip of her bowsprit", "48.5 meters high", "11.9 meters" wide | Fetched page. What "high" is measured from is not stated |
| Guns | The fetched page lists "forty-six broadside cannons", four chasers and two swivel guns (the page does not say this is the fully upgraded figure); a search summary quoted 22 guns | **Sources disagree or are unclear.** Not used as a target |
| Flag | "plain black flag, then a pirate flag with just a skull" | Fetched page. Note: a skull alone, not skull and crossbones |
| Other | "iron-plated naval ram"; gold and iron plating at the top upgrade level | Fetched page |

Hull colour, sail colour and figurehead shape are **not stated** on any page I could read.

### The Black Pearl (Pirates of the Caribbean)

| Fact | What was found | Source |
|---|---|---|
| Type | "East Indiaman" and "Galleon" (a hybrid) | [Wikipedia](https://en.wikipedia.org/wiki/Black_Pearl_(Pirates_of_the_Caribbean)) |
| Length | 165 ft (50.292 m) | same |
| Masts | "three large masts" | [SlashGear](https://www.slashgear.com/1780698/black-pearl-pirates-caribbean-ship-type/); a search summary names foremast, mainmast and mizzenmast |
| Guns | 32 twelve-pounders: 18 on the gun deck, 14 on the upper deck | Wikipedia |
| Look | "distinctive black hull and sails"; sails "tattered ... ripped in many places" | Wikipedia |
| How it was filmed | First film: a steel barge with wooden structures on top. Later films: a set built on the hull of the cargo ship *Sunset* (109 ft), "not an authentic tall ship" | Wikipedia and SlashGear |
| Beam, mast heights, flag design | **Not stated** | - |

## 2. What follows for this project

1. **The three-mast layout is the Pearl's, not the Jackdaw's.** The Jackdaw is a two-masted
   brig. The plan said "in the style of the Jackdaw ... and the ships of Pirates of the
   Caribbean" and asked for three masts, so the honest description is: *three masts, black
   tattered sails and black hull from the Pearl; the brig-sized, heavily gunned, gold-trimmed
   working ship feel from the Jackdaw.* The plan now says so, and nothing in the project may
   claim a three-masted Jackdaw.
2. **"At least six gunports a side" is safe.** The Pearl's 32 guns are 16 a side if split
   evenly (an inference, not a stated layout). The build uses 8 a side on one row.
3. **The proportion targets stay as design targets.** The only hard ratio the sources give is
   the Jackdaw's 60 : 11.9, which is 5.0 : 1 *including the bowsprit*. A hull without its
   bowsprit is shorter, so a hull-only 3.6 to 4.4 : 1 is consistent with it, but that is an
   inference. No source gives a Pearl beam or any mast height, so the mast ratios
   (1.00 : 0.85 : 0.65) remain the project's own choice.
4. **The flag is the generic Jolly Roger.** The Jackdaw's is a skull alone; the plan's skull
   and crossbones is therefore a design decision, not a copy.

## 3. The prototype, judged

Rendered from the front-quarter and the side with the Phase 37 code (`tilt` off). It is a
two-masted barge: a dark brown box with a lighter slab on it, two thin masts with flat white
rectangles, a red rectangle at the top of one mast, and one small cannon.

| Feature | References | Prototype (measured from the Phase 37 code) | Verdict |
|---|---|---|---|
| Hull shape | Curved hull, pointed bow, raised stern | A box. Both ends square. Length to beam 1.40 / 0.42 = **3.33** | **Fails** |
| Masts | Jackdaw 2, Pearl 3 | 2 | Matches the Jackdaw's count, not the plan's three |
| Mast height | Tall: Jackdaw "48.5 m high" on a 60 m hull | Main mast 0.78 above the deck on a 1.40 hull = **0.56** of hull length | **Fails** the 0.95 to 1.15 target |
| Sails | Pearl: black, torn, billowing | Flat cream quads (`SAILCLOTH`), straight edges, no belly | **Fails** |
| Rigging | Many ropes (Jackdaw lists 26 sails alone) | None | Fails |
| Guns | 16 or more a side | 1 cannon on the deck | **Fails** |
| Stern and bow | Raised stern, bowsprit, figurehead | None of them | Fails |
| Flag | Black with a white skull | Red rectangle | **Fails** |
| Palette | Black and dark brown, bronze and gold | Mid brown, silver, cream, red | **Fails** |
| Hierarchy and frames | n/a | Rigid frames, one root, `H` proof | **Passes - and is the reason it exists** |

**Verdict.** As a ship, the prototype resembles neither reference; it was built to prove the
method (Phases 34 to 37) and it did. As a *foundation* it is sound, because every rule that
the galleon needs - unscaled frames, size applied at draw time, a muzzle readable as a
position and a direction, one root - is already in place and tested. So the prototype is kept
as the base of the build and refined, not discarded: Phases 47 to 74 replace its parts one
at a time while the checks from 34 to 37 keep passing.

## 4. Changes made to the plan because of this review

| Change | Why |
|---|---|
| Plan, "what looks like the movie or game" table, Masts row, now says the three-mast layout follows the Pearl and that the Jackdaw has two | Sources 1 and 2 above |
| Plan states a build target of **8 gunports a side** | Sources 2 above; keeps the "at least six" row |
| Phase 39's checkpoint wording is tightened (see the plan) | A tall narrow ship seen end-on cannot fill 40% of a 16:9 window's width |
| Phase 45 gains a distance haze | The plan's own Phase 45 and 46 checkpoints ("continuous with the sea's haze", "hazed silhouettes") need one, and no phase added it |

Phase 78 repeats this judgement on the finished ship, with screenshots from all five camera
presets.
