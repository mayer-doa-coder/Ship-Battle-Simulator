# Stage D completion record — Phases 53–78

Stage D is implemented in C++17/OpenGL 3.3 code only. It uses the existing GLFW/GLAD/GLM project dependencies; no model, texture, physics, particle, or rendering library was added.

## Delivered work

| Phases | Implementation | Evidence |
|---|---|---|
| 53–54 | Eight recessed gunports per side and 16 port-aligned batteries. Each battery has a rigid port/carriage/barrel/muzzle chain, four wheels, recoil, player and AI ballistic firing. | `Ship.h` (`BroadsideCannonFrames`); `main.cpp` (`fireBroadside`, battery render). |
| 55–58 | Symmetric stern windows and doors, raised gold trim, a 15-degree bowsprit with lashings, and a centred procedural figurehead. | `drawStageDShipDetails`. |
| 59–61 | Fore/main/mizzen layout (0.85/1.00/0.65 height ratio), three tapered yards each, `makeSail`, analytic normals, both faces, deterministic holes and torn lower edge. | `Ship.h`, `Mesh.h::buildSailGeometry`. |
| 62–65 | Stays, backstays, paired shrouds, ratline ladders, braces and lifts. | `drawStageDShipDetails`, `CrewDraw.h`. |
| 66–68 | Bowed black main flag, ensign, two pennants, and skull/crossbones geometry. | `drawStageDShipDetails`. |
| 69–74 | Eight-spoke crew-operated helm/rudder, placed cargo/coils, emissive lanterns with the existing two-light rule, deck ladders/hatches/companionway, and animated anchor/chain. | Existing integrated `Crew*.h`, `Fx*.h`, `WorldDraw.h`. |
| 75 | Named pirate palette: mahogany, weathered brown, charcoal, aged beige, muted gold, aged iron and hemp. | `Material.h`. |
| 76 | Startup CPU batches for static port frames, openings, carriages and wheels; animated barrels, damage, crew and effects remain separate. `P` toggles the normal and batched paths; `--batch` starts on the batched path. | `ShipStaticBatchSet`, `makeShipStaticBatches`. |
| 77 | Release build and the existing camera view test were re-run; gallery/Demo paths remain selectable with `G`. | Build log; `--viewtest` command below. |
| 78 | Five screenshots have been captured for review. | `stage_d_evidence/preset_f1.png` through `preset_f5.png`. |

## Material audit

| Ship part | Material |
|---|---|
| Lofted hull / planks / castles | `HULL_TIMBER`, `DECK_PLANK`, `CASTLE_WOOD`, `MAHOGANY`, `WEATHERED_BROWN` |
| Sails / pirate flag | `SAILCLOTH`, `SAIL_BLACK`, `CHARCOAL` |
| Gun barrels / wheels | `IRON_DARK`, `AGED_IRON` |
| Trim / figurehead | `BRASS`, `MUTED_GOLD` |
| Rigging / lashings | `ROPE_MAT`, `HEMP` |
| Windows / lanterns | `LANTERN_GLASS` (emissive only; no third light) |

## Measured run

On the 960×540 capture runs, the renderer reported 2,862–3,333 draw calls and 344,764–442,440 triangles depending on the camera and visible LOD. In the final controlled 640×360 side-view run, static battery batching reduced draw calls from **3,121 to 2,689** while keeping submitted triangles at **413,320**. Batching is disabled during recoil so moving carriages and barrels keep their original hierarchy behavior.

## Reproduction

```powershell
cmake --build build --config Release
.\build\Release\ship_battle_simulator.exe --viewtest --shot viewtest.png --frames 90
.\build\Release\ship_battle_simulator.exe --batch --camera 90 4 8.12 --shot batch.png --frames 1
```

Controls added for the combat completion are `Y` to cycle `ROUND` / `CHAIN` / `GRAPE`, Left or Right `Ctrl` to fire the broadside toward the enemy, and `P` to compare static battery batching. Chain shot applies extra mast damage; at three mast-damage steps a mast falls and sailing drive is reduced.

## Student review

The engineering checklist and the five captures are complete. The requested final likeness judgement is deliberately left to the student, as required by Phase 78: review the five captures against the chosen reference and record approval or the remaining silhouette changes here.
