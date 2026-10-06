#pragma once

// Environment build: DRAWING THE TREASURE, THE WILDLIFE AND THE SHIP'S HATCHES. Included by main.cpp after CrewDraw.h (it uses its CrewDrawContext and its little
// box / ball / drum helpers, and drawMesh() and Glow behind them), so everything here is drawn the way the rest of the scene is: through drawMesh(), from the same three
// unit meshes - a cube, a sphere, a cylinder - each placed by a matrix and sized by a scale applied at the draw call. No models.
//
// Nothing here stores anything. A chest is drawn from `(lid, goldLeft, glow)`, which the caller works out from the clock (src/Treasure.h); a bird, a fish and a dolphin from the
// pose the closed-form path gives (src/Wildlife.h).

#include "CrewDraw.h"
#include "Interior.h"
#include "Treasure.h"
#include "Wildlife.h"

// ---- the chest ------------------------------------------------------------------------------------------------------------------------

// A treasure chest, standing with its base on the origin of `frame` and its front towards +z. `scale` sizes the whole chest (uniformly, at the draw call). `lid` is
// how far open it is, 0 to 1; `goldLeft` how much gold is in it, 0 to 1; `glow` 0 to 1 how much it shines (the wood, the bands and the coins take emission from it).
// A halo is added to `glows` for the glow, and a thin shaft of light rises from a chest that is worth looking for.
inline void drawChest(const CrewDrawContext& c, const glm::mat4& frame, float scale, float lid, float goldLeft, float glow, std::vector<Glow>* glows)
{
    const glm::mat4 F = frame * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    Material wood = CHEST_WOOD, iron = IRON_DARK, gold = GOLD, brass = BRASS;
    wood.ke = glm::vec3(0.30f, 0.17f, 0.04f) * glow;
    iron.ke = glm::vec3(0.12f, 0.10f, 0.05f) * glow;
    gold.ke = GOLD.ke + glm::vec3(0.55f, 0.40f, 0.06f) * glow;
    brass.ke = glm::vec3(0.35f, 0.25f, 0.04f) * glow;

    // The body: a box with iron straps round it and iron at the corners, and a brass lock on the front.
    crewBox(c, F, glm::vec3(0.0f, 0.08f, 0.0f), glm::vec3(0.30f, 0.16f, 0.20f), wood);
    for (int s = -1; s <= 1; s += 2) {
        const float x = 0.095f * static_cast<float>(s);
        crewBox(c, F, glm::vec3(x, 0.08f, 0.0f), glm::vec3(0.030f, 0.162f, 0.204f), iron);
    }
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            crewBox(c, F, glm::vec3(0.147f * static_cast<float>(sx), 0.075f, 0.098f * static_cast<float>(sz)), glm::vec3(0.012f, 0.15f, 0.012f), iron);
    crewBox(c, F, glm::vec3(0.0f, 0.12f, 0.102f), glm::vec3(0.04f, 0.05f, 0.008f), brass);

    // The gold: coins heaped in the open top. It is only there to see once the lid is up, and it shrinks as it is gathered.
    if (goldLeft > 0.01f) {
        const float g = goldLeft;
        crewBall(c, F, glm::vec3(0.0f, 0.158f, 0.0f), glm::vec3(0.24f, 0.07f * g, 0.15f), gold);
        crewBall(c, F, glm::vec3(-0.06f, 0.168f, 0.02f), glm::vec3(0.10f, 0.06f * g, 0.08f), gold);
        crewBall(c, F, glm::vec3(0.07f, 0.165f, -0.02f), glm::vec3(0.09f, 0.055f * g, 0.08f), gold);
        for (int k = 0; k < 4; ++k) {                                                                                  // loose coins on top
            const float a = 1.7f * static_cast<float>(k) + 0.6f;
            crewDrum(c, F * crewRotX(0.35f * std::sin(a * 3.0f)), glm::vec3(0.07f * std::cos(a), 0.19f * g + 0.01f, 0.05f * std::sin(a)), 0.034f, 0.006f, gold);
        }
    }

    // The lid hinges along the back top edge and swings up and back; it is an arch (a box with a half-barrel on it).
    const glm::mat4 hinge = F * crewMove(0.0f, 0.16f, -0.10f) * crewRotX(-1.95f * lid);
    crewBox(c, hinge, glm::vec3(0.0f, 0.03f, 0.10f), glm::vec3(0.30f, 0.06f, 0.20f), wood);
    crewDrum(c, hinge * crewMove(0.0f, 0.06f, 0.10f) * crewRotZ(1.5708f), glm::vec3(0.0f), 0.20f, 0.30f, wood);
    for (int s = -1; s <= 1; s += 2) {
        crewBox(c, hinge, glm::vec3(0.095f * static_cast<float>(s), 0.03f, 0.10f), glm::vec3(0.030f, 0.062f, 0.204f), iron);
        crewDrum(c, hinge * crewMove(0.095f * static_cast<float>(s), 0.06f, 0.10f) * crewRotZ(1.5708f), glm::vec3(0.0f), 0.206f, 0.030f, iron);
    }

    if (glows != nullptr && glow > 0.02f) {
        const glm::vec3 centre = glm::vec3(F * glm::vec4(0.0f, 0.12f, 0.0f, 1.0f));
        glows->push_back({ centre, 0.9f * scale, 0.55f * glow, 0.0f });
        if (goldLeft > 0.01f && lid > 0.5f)
            glows->push_back({ centre + glm::vec3(0.0f, 0.10f * scale, 0.0f), 0.6f * scale, 0.5f * glow * goldLeft, 0.0f });
    }
}

// ---- the birds -------------------------------------------------------------------------------------------------------------------------

// A gull or a tern. `lod` 0 full (eight parts), 1 a body and two wings. Wings flap about the body's length axis; a banking bird leans into its turn.
inline void drawBird(const CrewDrawContext& c, const BirdPose& b, int lod)
{
    if (!b.visible)
        return;
    const glm::mat4 F = glm::translate(glm::mat4(1.0f), b.pos) * crewRotY(b.heading) * crewRotX(-b.pitch) * crewRotZ(b.bank);
    const glm::mat4 S = F * glm::scale(glm::mat4(1.0f), glm::vec3(b.size));
    const Material& body = (b.type == 0) ? GULL_WHITE : GULL_GREY;
    drawMesh(c.shader, c.stats, c.sphere, S * glm::scale(glm::mat4(1.0f), glm::vec3(0.22f, 0.18f, 0.55f)), body);
    const float flap = 0.62f * b.flap;
    for (int side = -1; side <= 1; side += 2) {
        const float s = static_cast<float>(side);
        const glm::mat4 W = S * crewRotZ(s * flap) * crewMove(s * 0.45f, 0.0f, 0.0f);
        drawMesh(c.shader, c.stats, c.cube, W * glm::scale(glm::mat4(1.0f), glm::vec3(0.85f, 0.035f, 0.30f)), body);
        if (lod == 0)
            drawMesh(c.shader, c.stats, c.cube, W * crewMove(s * 0.38f, 0.0f, -0.04f) * crewRotZ(s * 0.25f * b.flap) * glm::scale(glm::mat4(1.0f), glm::vec3(0.34f, 0.03f, 0.24f)), GULL_GREY);
    }
    if (lod == 0) {
        drawMesh(c.shader, c.stats, c.sphere, S * crewMove(0.0f, 0.04f, 0.30f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.13f)), body);                        // head
        drawMesh(c.shader, c.stats, c.cube, S * crewMove(0.0f, 0.03f, 0.40f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.035f, 0.03f, 0.10f)), BRASS);            // beak
        drawMesh(c.shader, c.stats, c.cube, S * crewMove(0.0f, 0.0f, -0.34f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.16f, 0.02f, 0.20f)), body);              // tail
    }
}

// ---- fish ------------------------------------------------------------------------------------------------------------------------------

inline void drawFish(const CrewDrawContext& c, const FishPose& f)
{
    const glm::mat4 F = glm::translate(glm::mat4(1.0f), f.pos) * crewRotY(f.heading) * crewRotX(-f.pitch) * glm::scale(glm::mat4(1.0f), glm::vec3(f.size));
    const Material& m = (f.type == 0) ? FISH_SILVER : (f.type == 1 ? FISH_ORANGE : FISH_BLUE);
    drawMesh(c.shader, c.stats, c.sphere, F * glm::scale(glm::mat4(1.0f), glm::vec3(0.085f, 0.11f, 0.30f)), m);
    drawMesh(c.shader, c.stats, c.cube, F * crewMove(0.0f, 0.0f, -0.17f) * crewRotY(0.5f * f.wag) * crewMove(0.0f, 0.0f, -0.05f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.13f, 0.10f)), m);
}

// ---- dolphins --------------------------------------------------------------------------------------------------------------------------

// A dolphin about 3 units long (the body below is 1.7 long, drawn at 1.7 times that, so it reads from the ship): a body and a paler belly, a head with its beak, a dorsal fin, two flippers and the flukes, which beat up and down with `wag`.
inline void drawDolphin(const CrewDrawContext& c, const DolphinPose& d)
{
    const glm::mat4 F = glm::translate(glm::mat4(1.0f), d.pos) * crewRotY(d.heading) * crewRotX(-d.pitch) * glm::scale(glm::mat4(1.0f), glm::vec3(1.4f));
    const auto blob = [&](const glm::mat4& frame, const glm::vec3& centre, const glm::vec3& size, const Material& m) {
        drawMesh(c.shader, c.stats, c.sphere, frame * glm::translate(glm::mat4(1.0f), centre) * glm::scale(glm::mat4(1.0f), size), m);
    };
    blob(F, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.34f, 0.34f, 1.30f), DOLPHIN_BACK);                                           // the body
    blob(F, glm::vec3(0.0f, -0.07f, 0.06f), glm::vec3(0.30f, 0.24f, 1.18f), DOLPHIN_BELLY);                                      // the pale underside
    blob(F, glm::vec3(0.0f, 0.01f, 0.58f), glm::vec3(0.28f, 0.27f, 0.42f), DOLPHIN_BACK);                                        // the forehead (the melon)
    blob(F, glm::vec3(0.0f, -0.045f, 0.80f), glm::vec3(0.10f, 0.085f, 0.30f), DOLPHIN_BELLY);                                    // the beak, tapering
    blob(F * crewMove(0.0f, 0.18f, -0.10f) * crewRotX(0.65f), glm::vec3(0.0f), glm::vec3(0.045f, 0.34f, 0.22f), DOLPHIN_BACK);   // the dorsal fin, swept back
    for (int s = -1; s <= 1; s += 2)
        blob(F * crewMove(0.17f * static_cast<float>(s), -0.12f, 0.34f) * crewRotZ(-0.8f * static_cast<float>(s)) * crewRotX(0.4f), glm::vec3(0.0f), glm::vec3(0.20f, 0.03f, 0.17f), DOLPHIN_BACK);   // flippers
    const glm::mat4 tail = F * crewMove(0.0f, 0.0f, -0.62f) * crewRotX(0.35f * d.wag);
    blob(tail, glm::vec3(0.0f, 0.0f, -0.10f), glm::vec3(0.19f, 0.19f, 0.62f), DOLPHIN_BACK);                                    // the tail stock
    const glm::mat4 flukes = tail * crewMove(0.0f, 0.0f, -0.40f) * crewRotX(0.25f * d.wag);
    for (int s = -1; s <= 1; s += 2)
        blob(flukes * crewMove(0.14f * static_cast<float>(s), 0.0f, -0.03f) * crewRotY(0.5f * static_cast<float>(s)), glm::vec3(0.0f), glm::vec3(0.30f, 0.03f, 0.13f), DOLPHIN_BACK);     // the flukes
}

// ---- the hatches and the companionway on the main deck ----------------------------------------------------------------------------------------

// Where the stairs and hatches from the hold come up through the main deck: a raised coaming round a black opening, the cover propped open beside it, and the hooded
// door of the companionway. `hull` is the hull's frame; the connectors are in it. Hatches and the door are what you see from outside, and what you use to go below.
inline void drawDeckOpenings(const CrewDrawContext& c, const glm::mat4& hull, const InteriorLayout& L)
{
    for (const Connector& k : L.connectors) {
        if (k.lower != LVL_HOLD)
            continue;
        const float y = k.upperFoot.y;
        if (k.kind == ConnectorKind::HATCH) {
            const glm::mat4 H = hull * crewMove(k.upperFoot.x, y, k.upperFoot.z);
            crewBox(c, H, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.30f, 0.012f, 0.26f), GUNPORT_DARK);                                   // the opening
            for (int s = -1; s <= 1; s += 2) {
                crewBox(c, H, glm::vec3(0.15f * static_cast<float>(s), 0.02f, 0.0f), glm::vec3(0.025f, 0.04f, 0.29f), CASTLE_WOOD);       // the coaming
                crewBox(c, H, glm::vec3(0.0f, 0.02f, 0.13f * static_cast<float>(s)), glm::vec3(0.30f, 0.04f, 0.025f), CASTLE_WOOD);
            }
            crewBox(c, H * crewMove(0.0f, 0.04f, -0.13f) * crewRotX(-1.25f), glm::vec3(0.0f, 0.0f, 0.13f), glm::vec3(0.28f, 0.02f, 0.26f), DECK_PLANK);   // the cover, propped up
            for (int g = 0; g < 4; ++g)
                crewBox(c, H * crewMove(0.0f, 0.04f, -0.13f) * crewRotX(-1.25f), glm::vec3(0.0f, 0.012f, 0.05f + 0.06f * static_cast<float>(g)), glm::vec3(0.285f, 0.006f, 0.012f), IRON_DARK);
        } else if (k.kind == ConnectorKind::STAIRS) {
            const glm::mat4 H = hull * crewMove(k.upperFoot.x, y, k.upperFoot.z);
            crewBox(c, H, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.26f, 0.012f, 0.30f), GUNPORT_DARK);
            for (int s = -1; s <= 1; s += 2) {
                crewBox(c, H, glm::vec3(0.14f * static_cast<float>(s), 0.03f, 0.0f), glm::vec3(0.025f, 0.06f, 0.32f), CASTLE_WOOD);       // the coaming of the companionway
                crewBox(c, H, glm::vec3(0.0f, 0.03f, 0.16f * static_cast<float>(s)), glm::vec3(0.30f, 0.06f, 0.025f), CASTLE_WOOD);
            }
        }
    }
}
