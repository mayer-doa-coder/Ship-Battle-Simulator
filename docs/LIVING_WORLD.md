# Living world expansion

The outer world is layered over the existing campaign, battle, crew, weather, wildlife and underwater code. `LivingWorldSystem` owns simulation state and contains no OpenGL. `drawLivingWorld` reads that state and draws it with the project's existing sphere, cube, cylinder and hull meshes. No model, texture, engine or new dependency is used.

## Stable world structure

Eight regional ports form two wide 430/620-unit rings around five campaign ports separated by more than 150 units. Their regions are tropical, volcanic, rocky, mountainous, swamp, ruined, abandoned and mysterious. Each region deterministically generates six sites from stable integer hashes, for forty-eight sites in total. Sites have 95-160-unit approaches so neighbouring discoveries do not merge into a single crowded coast. The sites distribute ancient ruins, hideouts, forts, lighthouses, wrecks, villages, caves, temples, camps, outposts, treasure islands and haunted shrines. Three overlapping land lobes create non-circular shores and leave a navigable cove. The exact same lobes are passed to ship collision, so visible land and solid land agree.

The renderer uses three distance bands. Far sites retain their land and landmark silhouette, middle sites add beaches, and near sites add buildings, paired clue stones, vegetation, reef life, seabed and physical caches. A fixed registry and range checks bound update and drawing cost. Traffic outside the local 115-unit simulation radius sleeps without losing state.

## Exploration and secrets

Sailing within discovery range permanently reveals a site and grants a small discovery reward. An island cache is a three-step physical search:

1. Anchor near the cove, press `Tab`, and inspect the shore marker with `F`.
2. Find the paired stones inland and inspect them.
3. Search the described side of the island, or dive to the indicated wreck, and open the physical cache.

The cache then disappears, adds a relic and gold, and can unlock a cannon improvement or gold figurehead. After three minutes the cleared site produces its region's rare resource, giving the player a reason to return. Underwater sites have seabed, coral, plants, fish, wreck geometry and a cache at depth; the existing underwater fog and depth lighting provide visibility variation.

## Traffic, encounters and factions

Thirty-two bounded traffic vessels travel between actual ports. Their departure lanes are staggered rather than stacked at berths. Merchants, pirates, navy ships, smugglers, neutral sailors and fishing craft use the predictive hull-wide navigation planner, retain damage, and sleep outside the active radius. Hostility follows faction reputation. The principal battle enemy starts over 120 units away, while hostile encounters form 70-110 units from the player and approach before their 46/62-unit firing ranges. Their cannon fire uses the existing ballistic projectile and particle systems; player cannonballs can damage and sink them. A destroyed vessel stays destroyed for ten minutes, leaves recoverable cargo, and respawns only while well away from the player.

The encounter manager evaluates region, weather, night, reputation and campaign combat state before selecting an event. It can produce assistance calls, survivors, ambushes, inspections, distress signals, floating treasure, local squalls, hostile convoys, smugglers, discoveries, map clues and burning ships. Events use world positions checked against land. Assistance, inspections and smuggling update reputation and cargo; ambushes can place a port under attack; burning ships use the pooled smoke system; squalls feed wind, rain, waves and haze into the existing atmosphere.

Reputation is tracked separately for pirates, navy, merchants, smugglers, neutral sailors and settlements. Attacks change the struck faction's standing, assistance improves merchant standing, the port-defence campaign reward improves navy standing, and regional discoveries improve settlement standing. Port relationships alter prices, hostile ports refuse trade, under-attack ports close their market, and guards visually react.

## Economy and port life

The cargo table adds pearls, obsidian, relics and contraband to the original ten goods. Regional ports produce distinct rare resources. Prices use current stock, demand, production, port damage and relationship. Markets consume stock, rebuild local exports over time and increase demand for unavailable imports. At a dock, `[` and `]` select a good and `F` buys or sells it; the compact prompt shows the action and price.

Nearby ports render market stalls, warehouses, a tavern sign, blacksmith, repair stocks, cargo and a moored ship. Thirteen NPCs per port follow role-specific routes between those facilities. Workers retain the existing timed carry animation, while merchants, guards, sailors, fishermen, civilians and smiths follow simple day/night schedules.

## Persistent state and verification

Discovery, solved clues, collected caches, traffic damage, destroyed-ship timers, floating cargo, faction reputation, port attack/damage, market stock and campaign consequences all live in persistent runtime state. The CPU campaign test verifies port/site/treasure/traffic spacing, the enemy's preparation distance, eight regions, forty-eight sites, bounded traffic, deterministic generation, shared collision footprints, persistent discovery, the complete clue chain, destroyed-ship persistence and salvage, and supply-driven prices. Existing campaign, dolphin, enemy-navigation and camera suites remain part of the same regression gate.
