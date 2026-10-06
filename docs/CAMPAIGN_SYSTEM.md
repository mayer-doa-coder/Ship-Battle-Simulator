# Campaign, ports, cargo and navigation

The living-world implementation is split into simulation and drawing. `src/Campaign.h` has no OpenGL calls. It owns mission progression, port markets, cargo capacity, timed transfers, docking, discovery, treasure clues, NPC routes and navigation bearings. `src/CampaignDraw.h` reads that state and builds every visible port, warehouse, pier, crate, barrel, worker, guard, merchant, moored ship, mission vessel and treasure landmark from the project's existing cube, cylinder and sphere meshes.

## Campaign route

| # | Mission | Main integration |
|---:|---|---|
| 1 | Relief for Port Royal | Load food at Nassau, sail, and unload at Port Royal |
| 2 | The Spice Convoy | Remain near a moving merchant from Port Royal to Tortuga |
| 3 | Caribbean Exchange | Export cloth and import weapons across two markets |
| 4 | No Sail on the Horizon | Find the stranded vessel and remain alongside while survivors board |
| 5 | The Black Squall | Carry spices through sufficiently severe wind/rain and dock at Kingston |
| 6 | The Cartographer's Secret | Follow an uncertain clue, refine it at close range, go ashore and recover the chest |
| 7 | Break Havana's Supply Line | Use the existing combat/damage system against Havana's supply ship |
| 8 | The Red Jackal | Hunt and sink Captain Silas Vane's flagship |
| 9 | Stand at Port Royal | Destroy the attacker inside the port-defence area |
| 10 | Run the Devil's Channel | Open the range from pursuit and dock safely at Nassau |

Each `MissionDefinition` stores its type, title, objective, required action, start, destination, cargo, reward, success description and failure description. `MissionSystem` stores the active stage, progress, goal, elapsed time and terminal state. A success waits five seconds, grants coin/reputation and unlocks the next mission. A sinking fails the current mission and restarts that mission rather than skipping progression. The final reward is guarded by `campaignComplete`, so it can be applied only once.

## Ports, docking and cargo

Nassau, Port Royal, Tortuga, Havana and Kingston retain their campaign roles. Eight outer regional ports extend the same `PortSystem`, each with a faction, colour, position, rare export and demand. The port renderer has two levels of detail: distant ports retain the coastline, warehouse silhouettes, pier, tower and loading ship; nearby ports add cargo stacks, market stalls, tavern, smithy, repair yard and thirteen scheduled NPCs.

`DockingSystem` moves through `SAILING -> APPROACH -> MOORING -> DOCKED/WORKING -> DEPARTING`. Entering a harbour limits speed, the mooring state eases position and heading onto the berth, and arrival lowers the existing animated anchor. Raising it after transfers finish starts departure.

`CargoSystem` has a 24-unit hold and separate counts for the original ten goods plus pearls, obsidian, relics and contraband. `TradingSystem` derives prices from stock, demand, local production, port damage and faction relationship. `[` and `]` select the intended good and `F` buys or sells it. A `CargoTransfer` takes 3.2 seconds per unit. Stock and inventory change only when a worker completes a trip; they never change at the moment a command is issued. Workers walk warehouse-to-pier routes in the existing `CARRY` pose, and the corresponding coloured crate or barrel is drawn in their hands. Loaded units also appear as bounded cargo stacks on the player's deck.

## Navigation and treasure

The normal HUD shows heading, cardinal rose, mission bearing/distance and nearest-port bearing. F10 opens the chart. It shows the player, discovered and unknown ports, islands, current destination, discovered enemy contact and treasure uncertainty with distinct procedural symbols. Arrow keys pan and plus/minus zoom while the chart owns sailing/fire input.

A treasure clue first produces an approximate marker with a large uncertainty circle. Approaching the site reduces the uncertainty and reveals the exact landmark. After anchoring nearby, TAB enters world-space shore exploration. The player must walk to the physical chest and press F at arm's reach; the campaign inventory, treasure gold and mission progress then update together.

## Verification

Run the CPU-only state checks without creating an OpenGL window:

```powershell
build/Release/ship_battle_simulator.exe --campaigntest
```

The suite checks all ten distinct mission types, port-specific markets, cargo capacity, timed stock transfer, docking detection, map discovery, clue refinement and compass/destination updates. The existing `--viewtest` suite remains the regression gate for camera and battle-view behavior.
