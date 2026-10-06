#pragma once

// Environment build: DRAWING THE CREW, and the ship's furniture they work with. Included by main.cpp after drawMesh() and Glow (it uses both), the same
// way the rest of the scene is drawn: through drawMesh(), so the draw counters, the mirror pass (the crew is reflected in the sea like everything else)
// and the lighting all apply with no special case.
//
// NO MODELS. A person is built from the unit sphere (head, hands), the unit cylinder (limbs) and the unit cube (torso, hat, boots' cuffs, tools),
// each placed by a matrix that is a chain of joints:
//
//     root (the feet, on the deck, turned to face) -> pelvis (as high as the legs reach) -> torso (leaned, turned, swayed) -> shoulders -> arms
//                                                                                          -> neck -> head (turned, nodded) -> hat, face
//                                                   -> hips -> thighs -> knees -> shins
//
// exactly the hierarchy rule of Stage D: each joint's FRAME holds a position and a rotation only; the size of a limb is applied once, in the matrix handed to
// drawMesh(). A person is attached to the ship by the root: the hull's frame times the person's place in it, so when the ship rolls on a wave, they roll.
//
// One function per role (drawCaptain(), drawSailor(), drawCannonCrew(), drawLookout(), drawRigger(), drawNavigator(), drawCarpenter(), ...) gives that
// role its clothes, hat and tools, and calls the one shared body, drawPerson().
//
// LEVEL OF DETAIL. 0: about 18 parts, with a face and hands' tools; 1: 7 parts; 2: 3 parts (used for the reflection in the sea and for people far away).

#include "Crew.h"
#include "Material.h"
#include "Mesh.h"
#include "Scenery.h"

struct CrewDrawContext {
    ShaderProgram& shader;
    RenderStats& stats;
    const Mesh& sphere;
    const Mesh& cylinder;
    const Mesh& cube;
    float lanternLit;       // 0..1: how bright a held lantern (and the galley fire) is
    float time;             // for the fire's flicker
};

// A cloth material from a colour: matt, with an ambient term a third of the diffuse so the shaded side does not go black.
inline Material crewCloth(const glm::vec3& c)
{
    Material m;
    m.ka = c * 0.38f;
    m.kd = c;
    m.ks = glm::vec3(0.04f);
    m.ns = 6.0f;
    m.name = "crew cloth";
    return m;
}

inline Material crewMetal(const glm::vec3& c)
{
    Material m;
    m.ka = c * 0.25f;
    m.kd = c * 0.8f;
    m.ks = glm::vec3(0.7f);
    m.ns = 48.0f;
    m.name = "crew metal";
    return m;
}

inline Material crewGlow(const glm::vec3& emission)
{
    Material m;
    m.ka = glm::vec3(0.0f); m.kd = glm::vec3(0.0f); m.ks = glm::vec3(0.0f);
    m.ns = 1.0f;
    m.name = "crew glow";
    m.ke = emission;
    return m;
}

// ---- the three primitives, placed by a joint frame ---------------------------------------------------------------------------------

// A limb or post hanging DOWN from a joint: the frame is at its top, it runs along -y for `length`.
inline void crewLimb(const CrewDrawContext& c, const glm::mat4& joint, float length, float diameter, const Material& m)
{
    drawMesh(c.shader, c.stats, c.cylinder,
             joint * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f * length, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(diameter, length, diameter)), m);
}

inline void crewBox(const CrewDrawContext& c, const glm::mat4& frame, const glm::vec3& centre, const glm::vec3& size, const Material& m)
{
    drawMesh(c.shader, c.stats, c.cube, frame * glm::translate(glm::mat4(1.0f), centre) * glm::scale(glm::mat4(1.0f), size), m);
}

inline void crewBall(const CrewDrawContext& c, const glm::mat4& frame, const glm::vec3& centre, const glm::vec3& size, const Material& m)
{
    drawMesh(c.shader, c.stats, c.sphere, frame * glm::translate(glm::mat4(1.0f), centre) * glm::scale(glm::mat4(1.0f), size), m);
}

inline void crewDrum(const CrewDrawContext& c, const glm::mat4& frame, const glm::vec3& centre, float diameter, float height, const Material& m)
{
    drawMesh(c.shader, c.stats, c.cylinder,
             frame * glm::translate(glm::mat4(1.0f), centre) * glm::scale(glm::mat4(1.0f), glm::vec3(diameter, height, diameter)), m);
}

inline glm::mat4 crewRotX(float a) { return glm::rotate(glm::mat4(1.0f), a, glm::vec3(1.0f, 0.0f, 0.0f)); }
inline glm::mat4 crewRotY(float a) { return glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f)); }
inline glm::mat4 crewRotZ(float a) { return glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 0.0f, 1.0f)); }
inline glm::mat4 crewMove(float x, float y, float z) { return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)); }

// Which small extra a role wears or carries.
enum class CrewExtra { NONE, POWDER_HORN, ROPE_COIL, SAW, SPYGLASS, KNIFE };

// ---- the shared body -------------------------------------------------------------------------------------------------------------------

inline void drawPerson(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod, CrewExtra extra)
{
    using namespace CrewBody;
    const float H = look.height;
    const float B = look.build;

    const Material skin = crewCloth(look.skin);
    const Material coat = crewCloth(look.coat);
    const Material trousers = crewCloth(look.trousers);
    const Material boots = crewCloth(look.boots);
    const Material hatMat = crewCloth(look.hat);

    // ---- the joint chain
    const float reach = std::max(crewLegReach(pose.legL), crewLegReach(pose.legR));
    const glm::mat4 pelvis = root * crewMove(0.0f, reach * H + pose.lift * H, 0.0f);
    const glm::mat4 torso = pelvis * crewRotY(pose.twist) * crewRotX(pose.lean) * crewRotZ(-pose.sway);
    const glm::mat4 neck = torso * crewMove(0.0f, TORSO * H, 0.0f);
    const glm::mat4 head = neck * crewRotY(pose.headYaw) * crewRotX(pose.headPitch);

    // ---- LOD 2: a block, a ball and a hat
    if (lod >= 2) {
        crewBox(c, torso, glm::vec3(0.0f, 0.5f * TORSO * H, 0.0f), glm::vec3(0.22f * H * B, TORSO * H, 0.13f * H * B), coat);
        crewBall(c, head, glm::vec3(0.0f, (NECK + 0.5f * HEAD) * H, 0.0f), glm::vec3(HEAD * H), skin);
        crewBox(c, head, glm::vec3(0.0f, (NECK + HEAD) * H, 0.0f), glm::vec3(0.22f * H, 0.05f * H, 0.22f * H), hatMat);
        return;
    }

    // ---- the legs
    const glm::mat4 hips[2] = { pelvis * crewMove(HIP_X * H * B, 0.0f, 0.0f), pelvis * crewMove(-HIP_X * H * B, 0.0f, 0.0f) };
    const Leg* legs[2] = { &pose.legL, &pose.legR };
    for (int i = 0; i < 2; ++i) {
        const glm::mat4 thigh = hips[i] * crewRotX(-legs[i]->pitch);
        const glm::mat4 knee = thigh * crewMove(0.0f, -THIGH * H, 0.0f) * crewRotX(legs[i]->bend);
        if (lod == 0) {
            crewLimb(c, thigh, THIGH * H, 0.085f * H * B, trousers);
            crewLimb(c, knee, SHIN * H, 0.072f * H * B, boots);
        } else {
            // One piece from hip to foot, along the line the two segments make.
            crewLimb(c, thigh, (THIGH + SHIN) * H * 0.95f, 0.08f * H * B, trousers);
        }
    }

    // ---- the torso, belt and coat
    crewBox(c, torso, glm::vec3(0.0f, 0.5f * TORSO * H, 0.0f), glm::vec3(0.22f * H * B, TORSO * H, 0.125f * H * B), coat);
    if (lod == 0) {
        crewBox(c, torso, glm::vec3(0.0f, 0.035f * H, 0.0f), glm::vec3(0.232f * H * B, 0.035f * H, 0.136f * H * B), crewCloth(look.sash ? look.hat : look.belt));
        if (look.coatTails)
            crewBox(c, torso, glm::vec3(0.0f, -0.045f * H, -0.012f * H), glm::vec3(0.21f * H * B, 0.12f * H, 0.115f * H * B), coat);
        if (look.apron)
            crewBox(c, torso, glm::vec3(0.0f, 0.095f * H, 0.066f * H * B), glm::vec3(0.17f * H * B, 0.19f * H, 0.012f * H), crewCloth(glm::vec3(0.90f, 0.88f, 0.82f)));
        if (look.epaulettes)
            for (int s = -1; s <= 1; s += 2)
                crewBox(c, torso, glm::vec3(static_cast<float>(s) * SHOULDER_X * H * B, (TORSO - 0.012f) * H, 0.0f), glm::vec3(0.06f * H, 0.018f * H, 0.07f * H), crewMetal(look.trim));
        if (look.sword)
            crewBox(c, pelvis, glm::vec3(0.13f * H * B, -0.02f * H, -0.03f * H), glm::vec3(0.014f * H, 0.014f * H, 0.30f * H) , crewMetal(glm::vec3(0.75f, 0.75f, 0.78f)));
        if (look.pistol)
            crewBox(c, pelvis, glm::vec3(-0.12f * H * B, 0.03f * H, 0.07f * H), glm::vec3(0.025f * H, 0.045f * H, 0.07f * H), crewCloth(glm::vec3(0.20f, 0.14f, 0.09f)));
        switch (extra) {
        case CrewExtra::POWDER_HORN: crewBall(c, pelvis, glm::vec3(-0.13f * H * B, 0.06f * H, -0.02f * H), glm::vec3(0.04f * H, 0.07f * H, 0.04f * H), crewCloth(glm::vec3(0.55f, 0.45f, 0.30f))); break;
        case CrewExtra::ROPE_COIL:   crewDrum(c, torso, glm::vec3(-0.085f * H * B, 0.19f * H, 0.0f), 0.10f * H, 0.11f * H, crewCloth(glm::vec3(0.62f, 0.52f, 0.34f))); break;
        case CrewExtra::SAW:         crewBox(c, pelvis, glm::vec3(0.13f * H * B, 0.02f * H, -0.03f * H), glm::vec3(0.01f * H, 0.04f * H, 0.24f * H), crewMetal(glm::vec3(0.7f))); break;
        case CrewExtra::SPYGLASS:    crewDrum(c, pelvis * crewRotX(1.57f), glm::vec3(-0.12f * H * B, -0.03f * H, 0.0f), 0.03f * H, 0.17f * H, crewMetal(glm::vec3(0.7f, 0.5f, 0.2f))); break;
        case CrewExtra::KNIFE:       crewBox(c, pelvis, glm::vec3(0.13f * H * B, 0.01f * H, 0.0f), glm::vec3(0.012f * H, 0.012f * H, 0.11f * H), crewMetal(glm::vec3(0.7f))); break;
        default: break;
        }
    }

    // ---- the head, face and hat
    const glm::vec3 headCentre(0.0f, (NECK + 0.5f * HEAD) * H, 0.0f);
    crewBall(c, head, headCentre, glm::vec3(0.98f * HEAD * H, 1.06f * HEAD * H, 0.98f * HEAD * H), skin);
    if (lod == 0) {
        crewBall(c, head, headCentre + glm::vec3(0.0f, -0.012f * H, 0.5f * HEAD * H), glm::vec3(0.028f * H), skin);                       // nose
        const Material dark = crewCloth(glm::vec3(0.04f));
        for (int s = -1; s <= 1; s += 2)
            crewBall(c, head, headCentre + glm::vec3(static_cast<float>(s) * 0.03f * H, 0.018f * H, 0.46f * HEAD * H), glm::vec3(0.016f * H), dark);   // eyes
        if (look.beard)
            crewBox(c, head, headCentre + glm::vec3(0.0f, -0.07f * H, 0.025f * H), glm::vec3(0.12f * H, 0.07f * H, 0.10f * H), crewCloth(glm::vec3(0.14f, 0.10f, 0.07f)));
        if (look.eyepatch)
            crewBox(c, head, headCentre + glm::vec3(0.03f * H, 0.018f * H, 0.47f * HEAD * H), glm::vec3(0.03f * H, 0.025f * H, 0.01f * H), dark);
    }
    const float top = (NECK + HEAD) * H;
    switch (look.hatKind) {
    case HatKind::TRICORN:
        crewDrum(c, head, glm::vec3(0.0f, top + 0.008f * H, 0.0f), 0.17f * H, 0.055f * H, hatMat);
        if (lod == 0) drawMesh(c.shader, c.stats, c.cube, head * crewMove(0.0f, top - 0.012f * H, 0.0f) * crewRotY(0.785f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f * H, 0.014f * H, 0.25f * H)), hatMat);
        break;
    case HatKind::FEATHERED:
        crewDrum(c, head, glm::vec3(0.0f, top + 0.012f * H, 0.0f), 0.18f * H, 0.065f * H, hatMat);
        if (lod == 0) {
            drawMesh(c.shader, c.stats, c.cube, head * crewMove(0.0f, top - 0.012f * H, 0.0f) * crewRotY(0.785f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.30f * H, 0.016f * H, 0.30f * H)), hatMat);
            drawMesh(c.shader, c.stats, c.cube, head * crewMove(0.09f * H, top + 0.05f * H, -0.02f * H) * crewRotZ(-0.6f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.012f * H, 0.15f * H, 0.045f * H)),
                     crewCloth(glm::vec3(0.95f, 0.95f, 0.92f)));
        }
        break;
    case HatKind::BICORN:
        crewBox(c, head, glm::vec3(0.0f, top + 0.012f * H, 0.0f), glm::vec3(0.30f * H, 0.07f * H, 0.14f * H), hatMat);
        break;
    case HatKind::BANDANA:
        crewBall(c, head, glm::vec3(0.0f, top - 0.032f * H, 0.0f), glm::vec3(0.215f * H, 0.115f * H, 0.215f * H), hatMat);
        if (lod == 0) crewBall(c, head, glm::vec3(0.0f, top - 0.04f * H, -0.10f * H), glm::vec3(0.045f * H), hatMat);
        break;
    case HatKind::CAP:
        crewDrum(c, head, glm::vec3(0.0f, top - 0.004f * H, 0.0f), 0.2f * H, 0.045f * H, hatMat);
        if (lod == 0) crewBox(c, head, glm::vec3(0.0f, top - 0.02f * H, 0.1f * H), glm::vec3(0.16f * H, 0.012f * H, 0.07f * H), hatMat);
        break;
    case HatKind::SOUWESTER:
        crewBall(c, head, glm::vec3(0.0f, top - 0.02f * H, 0.0f), glm::vec3(0.215f * H, 0.15f * H, 0.215f * H), hatMat);
        if (lod == 0) {
            crewDrum(c, head, glm::vec3(0.0f, top - 0.04f * H, 0.0f), 0.30f * H, 0.012f * H, hatMat);
            crewBox(c, head, glm::vec3(0.0f, top - 0.07f * H, -0.12f * H), glm::vec3(0.18f * H, 0.1f * H, 0.012f * H), hatMat);   // the flap that keeps rain off the neck
        }
        break;
    case HatKind::COOK_CAP:
        crewDrum(c, head, glm::vec3(0.0f, top + 0.045f * H, 0.0f), 0.15f * H, 0.11f * H, hatMat);
        if (lod == 0) crewBall(c, head, glm::vec3(0.0f, top + 0.10f * H, 0.0f), glm::vec3(0.17f * H, 0.07f * H, 0.17f * H), hatMat);
        break;
    case HatKind::NONE:
        crewBall(c, head, glm::vec3(0.0f, top - 0.025f * H, -0.012f * H), glm::vec3(0.2f * H, 0.1f * H, 0.2f * H), crewCloth(glm::vec3(0.18f, 0.12f, 0.08f)));   // hair
        break;
    }

    // ---- the arms
    const glm::mat4 shoulders[2] = {
        torso * crewMove(SHOULDER_X * H * B, (TORSO - SHOULDER_DOWN) * H, 0.0f),
        torso * crewMove(-SHOULDER_X * H * B, (TORSO - SHOULDER_DOWN) * H, 0.0f),
    };
    const Arm* arms[2] = { &pose.armL, &pose.armR };
    glm::mat4 hand[2];
    for (int i = 0; i < 2; ++i) {
        const float side = (i == 0) ? 1.0f : -1.0f;                       // the left arm is on the +x side
        const glm::mat4 upper = shoulders[i] * crewRotZ(side * arms[i]->roll) * crewRotX(-arms[i]->pitch);
        const glm::mat4 elbow = upper * crewMove(0.0f, -UPPER_ARM * H, 0.0f) * crewRotX(-arms[i]->bend);
        hand[i] = elbow * crewMove(0.0f, -FOREARM * H, 0.0f);
        if (lod == 0) {
            crewLimb(c, upper, UPPER_ARM * H, 0.062f * H * B, coat);
            crewLimb(c, elbow, FOREARM * H, 0.052f * H, coat);
            crewBall(c, hand[i], glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.055f * H), skin);
        } else {
            crewLimb(c, upper, (UPPER_ARM + FOREARM) * H * 0.95f, 0.058f * H * B, coat);
        }
    }
    if (lod >= 1)
        return;

    // ---- what is in the hands (full detail only)
    const glm::mat4& handR = hand[1];
    const glm::mat4& handL = hand[0];
    const Material wood = crewCloth(glm::vec3(0.40f, 0.27f, 0.15f));
    const Material iron = crewMetal(glm::vec3(0.35f, 0.35f, 0.38f));
    const glm::vec3 up(root * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    switch (pose.hold) {
    case CrewHold::NONE: break;
    case CrewHold::BINOCULARS:
        for (int s = -1; s <= 1; s += 2)
            drawMesh(c.shader, c.stats, c.cylinder,
                     head * crewMove(static_cast<float>(s) * 0.03f * H, headCentre.y, 0.115f * H) * crewRotX(1.5708f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.04f * H, 0.085f * H, 0.04f * H)),
                     crewMetal(glm::vec3(0.62f, 0.45f, 0.18f)));
        break;
    case CrewHold::MUSKET_AIMED:
        drawMesh(c.shader, c.stats, c.cube, handR * crewRotX(1.5708f) * crewMove(0.0f, 0.0f, 0.18f * H) * glm::scale(glm::mat4(1.0f), glm::vec3(0.024f * H, 0.03f * H, 0.62f * H)), wood);
        break;
    case CrewHold::MUSKET_SHOULDER:
        drawMesh(c.shader, c.stats, c.cube, handR * crewMove(0.0f, 0.1f * H, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.024f * H, 0.60f * H, 0.03f * H)), wood);
        break;
    case CrewHold::MUSKET_GROUNDED:
        crewBox(c, root, glm::vec3(-0.12f * H * B, 0.30f * H, 0.07f * H), glm::vec3(0.024f * H, 0.60f * H, 0.03f * H), wood);
        break;
    case CrewHold::SWORD_DRAWN:
        drawMesh(c.shader, c.stats, c.cube, handR * crewRotX(1.5708f) * crewMove(0.0f, 0.0f, 0.15f * H) * glm::scale(glm::mat4(1.0f), glm::vec3(0.014f * H, 0.014f * H, 0.34f * H)), iron);
        break;
    case CrewHold::HAMMER:
        crewLimb(c, handR, 0.21f * H, 0.022f * H, wood);
        crewBox(c, handR, glm::vec3(0.0f, -0.205f * H, 0.0f), glm::vec3(0.075f * H, 0.045f * H, 0.045f * H), iron);
        break;
    case CrewHold::LADLE:
        crewLimb(c, handR, 0.24f * H, 0.016f * H, iron);
        crewBall(c, handR, glm::vec3(0.0f, -0.255f * H, 0.0f), glm::vec3(0.06f * H, 0.035f * H, 0.06f * H), iron);
        break;
    case CrewHold::ROPE: {
        // The line the person is hauling: a thin rod from the hands straight up (the rope comes down from the yard).
        const glm::vec3 a(handR * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        drawMesh(c.shader, c.stats, c.cylinder, sceneryBetween(a, a + up * (0.40f + 0.12f * pose.holdParam), 0.014f), crewCloth(glm::vec3(0.62f, 0.52f, 0.34f)));
        break;
    }
    case CrewHold::BARREL:
        crewDrum(c, torso, glm::vec3(0.0f, 0.13f * H, 0.17f * H * B), 0.17f * H, 0.21f * H, wood);
        break;
    case CrewHold::CRATE:
        crewBox(c, torso, glm::vec3(0.0f, 0.13f * H, 0.16f * H * B), glm::vec3(0.17f * H), crewCloth(glm::vec3(0.55f, 0.42f, 0.26f)));
        break;
    case CrewHold::TRAY:
        crewBox(c, torso, glm::vec3(0.0f, 0.12f * H, 0.16f * H * B), glm::vec3(0.17f * H, 0.012f * H, 0.12f * H), iron);
        crewBall(c, torso, glm::vec3(0.03f * H, 0.15f * H, 0.16f * H * B), glm::vec3(0.035f * H), crewCloth(glm::vec3(0.85f, 0.85f, 0.8f)));
        break;
    case CrewHold::MAP_PEN:
        crewLimb(c, handR, 0.13f * H, 0.012f * H, crewMetal(glm::vec3(0.7f, 0.5f, 0.2f)));
        break;
    case CrewHold::LINSTOCK: {
        crewLimb(c, handR, 0.5f * H, 0.014f * H, wood);
        crewBall(c, handR, glm::vec3(0.0f, -0.52f * H, 0.0f), glm::vec3(0.035f * H), crewGlow(glm::vec3(1.0f, 0.55f, 0.15f)));      // the lit match
        break;
    }
    case CrewHold::RAMMER:
        crewLimb(c, handR, 0.62f * H, 0.018f * H, wood);
        crewBall(c, handR, glm::vec3(0.0f, -0.64f * H, 0.0f), glm::vec3(0.05f * H), wood);
        break;
    case CrewHold::SPONGE:
        crewLimb(c, handR, 0.62f * H, 0.018f * H, wood);
        crewBall(c, handR, glm::vec3(0.0f, -0.64f * H, 0.0f), glm::vec3(0.06f * H, 0.07f * H, 0.06f * H), crewCloth(glm::vec3(0.55f, 0.50f, 0.42f)));
        break;
    case CrewHold::BALL:
        crewBall(c, torso, glm::vec3(0.0f, 0.17f * H, 0.15f * H * B), glm::vec3(0.06f * H), BLACK_PLASTIC);
        break;
    case CrewHold::LANTERN:
        break;
    }
    if (pose.lampInHand) {
        crewBox(c, handL, glm::vec3(0.0f, -0.025f * H, 0.0f), glm::vec3(0.03f * H, 0.012f * H, 0.03f * H), BRASS);
        Material glass = LANTERN_GLASS;
        glass.ke = LANTERN_GLASS.ke * (0.25f + 0.75f * c.lanternLit);
        crewBall(c, handL, glm::vec3(0.0f, -0.05f * H, 0.0f), glm::vec3(0.045f * H, 0.055f * H, 0.045f * H), glass);
    }
}

// ---- one function per role ---------------------------------------------------------------------------------------------------------------
//
// Each is the shared body with that role's look and tools. They exist as separate functions so a role can be changed (a different coat for the carpenter, a
// spyglass for the navigator) in one place and so a viva question - "show me how the captain differs from a sailor" - has one function to point at.

inline void drawCaptain(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)     { drawPerson(c, root, look, pose, lod, CrewExtra::SPYGLASS); }
inline void drawFirstMate(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)   { drawPerson(c, root, look, pose, lod, CrewExtra::NONE); }
inline void drawHelmsman(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)    { drawPerson(c, root, look, pose, lod, CrewExtra::KNIFE); }
inline void drawLookout(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)     { drawPerson(c, root, look, pose, lod, CrewExtra::NONE); }
inline void drawSailor(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)      { drawPerson(c, root, look, pose, lod, CrewExtra::KNIFE); }
inline void drawRigger(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)      { drawPerson(c, root, look, pose, lod, CrewExtra::ROPE_COIL); }
inline void drawCannonCrew(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)  { drawPerson(c, root, look, pose, lod, CrewExtra::POWDER_HORN); }
inline void drawDeckCrew(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)    { drawPerson(c, root, look, pose, lod, CrewExtra::NONE); }
inline void drawCarpenter(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)   { drawPerson(c, root, look, pose, lod, CrewExtra::SAW); }
inline void drawCook(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)        { drawPerson(c, root, look, pose, lod, CrewExtra::KNIFE); }
inline void drawNavigator(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)   { drawPerson(c, root, look, pose, lod, CrewExtra::SPYGLASS); }
inline void drawMarine(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)      { drawPerson(c, root, look, pose, lod, CrewExtra::POWDER_HORN); }
inline void drawCabinCrew(const CrewDrawContext& c, const glm::mat4& root, const CrewLook& look, const CrewPose& pose, int lod)   { drawPerson(c, root, look, pose, lod, CrewExtra::NONE); }

// Draws one member: finds their place in the world (the hull's frame times where they stand, turned to face), their look and their pose, and calls their
// role's function.
inline void drawCrewMember(const CrewDrawContext& c, const glm::mat4& hull, const CrewMember& m, const CrewContext& ctx, const ShipCrew& crew, int lod, bool enemy)
{
    const glm::mat4 root = hull * crewMove(m.pos.x, m.pos.y, m.pos.z) * crewRotY(m.facing);
    const CrewLook look = crewLookFor(m.role, m.variant, crew.gear, enemy);
    CrewPose pose = crewPoseOf(m, ctx, crew);

    // A lookout in a fog or at night keeps his glasses to his eyes; a gunner's linstock burns only when the guns are manned.
    switch (m.role) {
    case CrewRole::CAPTAIN:     drawCaptain(c, root, look, pose, lod); break;
    case CrewRole::FIRST_MATE:  drawFirstMate(c, root, look, pose, lod); break;
    case CrewRole::HELMSMAN:    drawHelmsman(c, root, look, pose, lod); break;
    case CrewRole::LOOKOUT:     drawLookout(c, root, look, pose, lod); break;
    case CrewRole::SAILOR:      drawSailor(c, root, look, pose, lod); break;
    case CrewRole::RIGGER:      drawRigger(c, root, look, pose, lod); break;
    case CrewRole::CANNON_CREW: drawCannonCrew(c, root, look, pose, lod); break;
    case CrewRole::DECK_CREW:   drawDeckCrew(c, root, look, pose, lod); break;
    case CrewRole::CARPENTER:   drawCarpenter(c, root, look, pose, lod); break;
    case CrewRole::COOK:        drawCook(c, root, look, pose, lod); break;
    case CrewRole::NAVIGATOR:   drawNavigator(c, root, look, pose, lod); break;
    case CrewRole::MARINE:      drawMarine(c, root, look, pose, lod); break;
    case CrewRole::CABIN_CREW:  drawCabinCrew(c, root, look, pose, lod); break;
    }
}

// ---- the furniture: what the crew works with --------------------------------------------------------------------------------------------

// A ladder or shroud between two hull-local points: two rails and rungs between them. `rungSpacing` is the gap between rungs.
inline void drawLadder(const CrewDrawContext& c, const glm::mat4& hull, const glm::vec3& foot, const glm::vec3& head, float width, float rungSpacing, bool rungs)
{
    const glm::vec3 along = head - foot;
    const float length = glm::length(along);
    if (length < 1e-4f)
        return;
    glm::vec3 side = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), along / length);
    side = (glm::dot(side, side) > 1e-6f) ? glm::normalize(side) : glm::vec3(1.0f, 0.0f, 0.0f);
    const Material rope = crewCloth(glm::vec3(0.60f, 0.50f, 0.32f));
    const Material wood = crewCloth(glm::vec3(0.38f, 0.26f, 0.15f));
    for (int s = -1; s <= 1; s += 2) {
        const glm::vec3 a = glm::vec3(hull * glm::vec4(foot + side * (0.5f * width * static_cast<float>(s)), 1.0f));
        const glm::vec3 b = glm::vec3(hull * glm::vec4(head + side * (0.5f * width * static_cast<float>(s)), 1.0f));
        drawMesh(c.shader, c.stats, c.cylinder, sceneryBetween(a, b, rungs ? 0.020f : 0.014f), rungs ? wood : rope);
    }
    if (!rungs)
        return;
    const int n = std::max(2, static_cast<int>(length / rungSpacing));
    const glm::vec3 dir = along / length;
    // A rung is a bar across the ladder: along `side`, so its model is built from the three axes.
    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(side, 0.0f);
    basis[1] = glm::vec4(glm::normalize(glm::cross(dir, side)), 0.0f);
    basis[2] = glm::vec4(dir, 0.0f);
    for (int i = 1; i <= n; ++i) {
        const glm::vec3 p = foot + dir * (length * static_cast<float>(i) / static_cast<float>(n + 1));
        basis[3] = glm::vec4(p, 1.0f);
        drawMesh(c.shader, c.stats, c.cube, hull * basis * glm::scale(glm::mat4(1.0f), glm::vec3(width, 0.014f, 0.014f)), wood);
    }
}

// Everything fixed to the deck that the crew uses. `engaged` runs the guns out; `lanterns` (0..1) lights the lanterns; `lod` 1 or more skips the small things.
inline void drawShipFurniture(const CrewDrawContext& c, const ShipFrames& f, const CrewLayout& L, const ShipDimensions& d, const ShipCrew& crew,
                              float lanterns, bool engaged, std::vector<Glow>* glows, int lod)
{
    const glm::mat4& hull = f.hull;
    const Material wood = crewCloth(glm::vec3(0.40f, 0.27f, 0.15f));
    const Material darkWood = crewCloth(glm::vec3(0.26f, 0.17f, 0.10f));
    const Material iron = crewMetal(glm::vec3(0.30f, 0.30f, 0.33f));
    const Material rope = crewCloth(glm::vec3(0.62f, 0.52f, 0.34f));
    const float lit = std::clamp((lanterns - 0.3f) / 0.7f, 0.0f, 1.0f);
    const auto lantern = [&](const glm::vec3& at, float glowSize) {
        Material glass = LANTERN_GLASS;
        glass.ke = LANTERN_GLASS.ke * lanterns;
        crewBox(c, hull, at + glm::vec3(0.0f, 0.07f, 0.0f), glm::vec3(0.07f, 0.02f, 0.07f), BRASS);
        crewBall(c, hull, at, glm::vec3(0.075f, 0.095f, 0.075f), glass);
        if (glows != nullptr)
            glows->push_back({ glm::vec3(hull * glm::vec4(at, 1.0f)), glowSize, lit });
    };

    // ---- the wheel, on the poop, which turns with the helm
    {
        const glm::mat4 wheel = hull * crewMove(L.wheelHub.x, L.wheelHub.y, L.wheelHub.z) * crewRotZ(crew.wheelAngle);
        crewBox(c, hull, glm::vec3(L.wheelHub.x, 0.5f * (L.wheelHub.y - L.poopY), L.wheelHub.z), glm::vec3(0.12f, L.wheelHub.y - L.poopY, 0.10f), darkWood);      // the pedestal
        crewDrum(c, wheel * crewRotX(1.5708f), glm::vec3(0.0f), 0.05f, 0.07f, BRASS);                                                                              // the hub
        for (int k = 0; k < 4; ++k)                                                                                                                                  // the spokes, with their handles
            drawMesh(c.shader, c.stats, c.cube, wheel * crewRotZ(0.7854f * static_cast<float>(k)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f * (L.wheelRadius + 0.045f), 0.02f, 0.02f)), wood);
        for (int i = 0; i < 8; ++i) {                                                                                                                                // the rim
            const float a = 0.7854f * static_cast<float>(i);
            drawMesh(c.shader, c.stats, c.cube,
                     wheel * crewMove(L.wheelRadius * std::cos(a), L.wheelRadius * std::sin(a), 0.0f) * crewRotZ(a + 1.5708f)
                           * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f * 3.14159f * L.wheelRadius / 8.0f * 1.12f, 0.024f, 0.034f)), darkWood);
        }
        // The lantern on its post beside the helm.
        crewLimb(c, hull * crewMove(L.helmLantern.x, L.helmLantern.y - 0.06f, L.helmLantern.z), L.helmLantern.y - 0.06f - L.poopY, 0.02f, iron);
        lantern(L.helmLantern, 1.5f);
    }

    // ---- the crow's nest: a floor and a low wall round the mast head
    {
        crewDrum(c, hull, L.nestFloor - glm::vec3(0.0f, 0.02f, 0.0f), 2.0f * L.nestRadius, 0.04f, darkWood);
        for (int i = 0; i < 8; ++i) {
            const float a = 0.7854f * static_cast<float>(i);
            drawMesh(c.shader, c.stats, c.cube,
                     hull * crewMove(L.nestFloor.x + L.nestRadius * std::cos(a), L.nestFloor.y + 0.12f, L.nestFloor.z + L.nestRadius * std::sin(a)) * crewRotY(-a - 1.5708f)
                           * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f * 3.14159f * L.nestRadius / 8.0f * 1.05f, 0.22f, 0.025f)), wood);
        }
        lantern(L.nestFloor + glm::vec3(0.22f, 0.30f, 0.0f), 1.2f);
    }

    // ---- the captain's cabin on the quarterdeck: low walls, a table with a chart on it, a stool, a shelf
    {
        const float h = L.cabinWallHeight, y = L.quarterY + 0.5f * h;
        const float x0 = L.cabinMin.x, x1 = L.cabinMax.x, z0 = L.cabinMin.y, z1 = L.cabinMax.y;
        crewBox(c, hull, glm::vec3(0.5f * (x0 + x1), y, z1), glm::vec3(x1 - x0, h, 0.03f), wood);                             // the front wall
        crewBox(c, hull, glm::vec3(x0, y, 0.5f * (z0 + z1)), glm::vec3(0.03f, h, z1 - z0), wood);                             // the port wall
        const float gap0 = z0 + 0.5f * (z1 - z0) - 0.07f, gap1 = gap0 + 0.14f;                                                // the doorway in the inboard wall
        crewBox(c, hull, glm::vec3(x1, y, 0.5f * (z0 + gap0)), glm::vec3(0.03f, h, gap0 - z0), wood);
        crewBox(c, hull, glm::vec3(x1, y, 0.5f * (gap1 + z1)), glm::vec3(0.03f, h, z1 - gap1), wood);
        crewBox(c, hull, glm::vec3(0.5f * (x0 + x1), L.quarterY + 0.004f, 0.5f * (z0 + z1)), glm::vec3(x1 - x0 - 0.05f, 0.008f, z1 - z0 - 0.05f), crewCloth(glm::vec3(0.55f, 0.12f, 0.10f)));   // the rug
        // the table
        crewBox(c, hull, L.tablePos + glm::vec3(0.0f, 0.15f, 0.0f), glm::vec3(L.tableSize.x, 0.02f, L.tableSize.y), darkWood);
        crewBox(c, hull, L.tablePos + glm::vec3(0.0f, 0.075f, 0.0f), glm::vec3(0.06f, 0.15f, 0.06f), darkWood);
        crewBox(c, hull, L.tablePos + glm::vec3(0.0f, 0.162f, 0.0f), glm::vec3(0.20f, 0.004f, 0.14f), crewCloth(glm::vec3(0.88f, 0.82f, 0.62f)));                                    // the chart
        if (lod == 0) {
            crewBox(c, hull, glm::vec3(L.shelfPos.x - 0.03f, L.quarterY + 0.15f, L.shelfPos.z), glm::vec3(0.06f, 0.02f, 0.26f), darkWood);
            crewDrum(c, hull, glm::vec3(L.shelfPos.x - 0.03f, L.quarterY + 0.20f, L.shelfPos.z - 0.06f), 0.03f, 0.09f, crewCloth(glm::vec3(0.1f, 0.35f, 0.15f)));
            crewDrum(c, hull, glm::vec3(L.shelfPos.x - 0.03f, L.quarterY + 0.20f, L.shelfPos.z + 0.05f), 0.03f, 0.09f, crewCloth(glm::vec3(0.4f, 0.2f, 0.08f)));
        }
        lantern(L.cabinLantern, 1.2f);
    }

    // ---- the galley: a stone hut with a chimney, a cauldron on a tripod over a fire
    {
        const glm::vec3 g = L.galleyPos;
        const float flicker = 0.78f + 0.22f * std::sin(11.0f * c.time) * std::sin(7.3f * c.time + 1.0f);
        crewBox(c, hull, g + glm::vec3(0.0f, 0.5f * L.galleySize.y, 0.0f), L.galleySize, crewCloth(glm::vec3(0.46f, 0.43f, 0.40f)));
        crewBox(c, hull, g + glm::vec3(0.0f, L.galleySize.y + 0.02f, 0.0f), glm::vec3(L.galleySize.x + 0.06f, 0.04f, L.galleySize.z + 0.06f), darkWood);
        crewDrum(c, hull, g + glm::vec3(-0.08f, L.galleySize.y + 0.14f, -0.05f), 0.07f, 0.22f, crewCloth(glm::vec3(0.34f, 0.28f, 0.26f)));
        crewBox(c, hull, g + glm::vec3(0.5f * L.galleySize.x + 0.004f, 0.12f, 0.0f), glm::vec3(0.012f, 0.18f, 0.15f), GUNPORT_DARK);                          // the fire door
        crewBox(c, hull, g + glm::vec3(0.5f * L.galleySize.x + 0.01f, 0.07f, 0.0f), glm::vec3(0.012f, 0.07f, 0.11f), crewGlow(glm::vec3(1.0f, 0.42f, 0.10f) * flicker));       // the fire inside
        // the cauldron over its own small fire
        const glm::vec3 p = L.cauldronPos;
        for (int k = 0; k < 3; ++k) {
            const float a = 2.094f * static_cast<float>(k);
            drawMesh(c.shader, c.stats, c.cylinder,
                     sceneryBetween(glm::vec3(hull * glm::vec4(p + glm::vec3(0.09f * std::cos(a), 0.0f, 0.09f * std::sin(a)), 1.0f)),
                                    glm::vec3(hull * glm::vec4(p + glm::vec3(0.02f * std::cos(a), 0.17f, 0.02f * std::sin(a)), 1.0f)), 0.014f), iron);
        }
        crewDrum(c, hull, p + glm::vec3(0.0f, 0.12f, 0.0f), 0.16f, 0.10f, iron);
        crewDrum(c, hull, p + glm::vec3(0.0f, 0.168f, 0.0f), 0.14f, 0.006f, crewCloth(glm::vec3(0.45f, 0.30f, 0.12f)));                                           // the stew
        crewBall(c, hull, p + glm::vec3(0.0f, 0.03f, 0.0f), glm::vec3(0.08f, 0.06f, 0.08f), crewGlow(glm::vec3(1.0f, 0.38f, 0.08f) * flicker));                    // the embers
        crewBox(c, hull, p + glm::vec3(0.0f, 0.012f, 0.0f), glm::vec3(0.16f, 0.024f, 0.04f), darkWood);
        if (glows != nullptr)
            glows->push_back({ glm::vec3(hull * glm::vec4(p + glm::vec3(0.0f, 0.06f, 0.0f), 1.0f)), 0.55f, 0.5f * flicker * std::clamp(0.35f + 0.65f * lit, 0.0f, 1.0f) });
        lantern(L.galleyLantern, 1.1f);
    }

    // ---- ladders and shrouds
    drawLadder(c, hull, L.quarterLadder[0], L.quarterLadder[1], 0.15f, 0.07f, true);
    drawLadder(c, hull, L.poopLadder[0], L.poopLadder[1], 0.15f, 0.07f, true);
    drawLadder(c, hull, L.foreLadder[0], L.foreLadder[1], 0.15f, 0.07f, true);
    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        drawLadder(c, hull, L.shroud[i][0], L.shroud[i][1], 0.20f, 0.22f, lod == 0);                                                              // starboard: the way up
        drawLadder(c, hull, glm::vec3(-L.shroud[i][0].x, L.shroud[i][0].y, L.shroud[i][0].z), glm::vec3(-L.shroud[i][1].x, L.shroud[i][1].y, L.shroud[i][1].z), 0.20f, 0.22f, false);
    }

    // ---- the running rigging: a line from each end of each yard down to a belaying point on the rail
    for (int i = 0; i < SHIP_MAST_COUNT; ++i) {
        for (int s = -1; s <= 1; s += 2) {
            const glm::vec3 top(static_cast<float>(s) * L.yardHalfLength[i], L.yardY[i], L.mastFoot[i].z);
            const Station& b = L.belay[(i == MAIN_MAST ? 0 : 2) + (s > 0 ? 0 : 1)];
            drawMesh(c.shader, c.stats, c.cylinder,
                     sceneryBetween(glm::vec3(hull * glm::vec4(top, 1.0f)), glm::vec3(hull * glm::vec4(b.pos + glm::vec3(0.0f, 0.12f, 0.0f), 1.0f)), 0.012f), rope);
        }
    }

    // ---- the other guns, on the main deck by their ports; in action they are run out
    for (int i = 0; i < 4; ++i) {
        const DummyGun& g = L.dummyGun[i];
        const glm::mat4 gun = hull * crewMove(g.pos.x, g.pos.y, g.pos.z) * crewRotY(g.facing);
        const float run = engaged ? 0.10f : 0.0f;
        crewBox(c, gun, glm::vec3(0.0f, 0.065f, 0.0f), glm::vec3(0.22f, 0.10f, 0.32f), darkWood);
        drawMesh(c.shader, c.stats, c.cylinder, gun * crewMove(0.0f, 0.15f, 0.04f + run) * crewRotX(1.5708f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.085f, 0.42f, 0.085f)), BLACK_PLASTIC);
    }

    // ---- cargo: barrels, crates, coils of rope, the shot rack, and the carpenter's bench
    for (int s = 0; s < 2; ++s) {
        const glm::vec3 b = L.barrelStack[s];
        crewDrum(c, hull, b + glm::vec3(0.08f, 0.10f, 0.0f), 0.16f, 0.20f, wood);
        crewDrum(c, hull, b + glm::vec3(-0.08f, 0.10f, 0.0f), 0.16f, 0.20f, wood);
        crewDrum(c, hull, b + glm::vec3(0.0f, 0.30f, 0.0f), 0.16f, 0.20f, wood);
        if (lod == 0)
            for (int h = 0; h < 2; ++h)
                crewDrum(c, hull, b + glm::vec3(0.0f, 0.06f + 0.14f * static_cast<float>(h) + 0.20f, 0.0f), 0.172f, 0.014f, iron);
        const glm::vec3 k = L.crateStack[s];
        crewBox(c, hull, k + glm::vec3(0.0f, 0.075f, 0.0f), glm::vec3(0.17f), crewCloth(glm::vec3(0.55f, 0.42f, 0.26f)));
        crewBox(c, hull, k + glm::vec3(0.0f, 0.075f, 0.18f), glm::vec3(0.15f), crewCloth(glm::vec3(0.50f, 0.38f, 0.24f)));
        crewBox(c, hull, k + glm::vec3(0.0f, 0.225f, 0.07f), glm::vec3(0.15f), crewCloth(glm::vec3(0.58f, 0.45f, 0.28f)));
    }
    for (int i = 0; i < 3; ++i)
        crewDrum(c, hull, L.ropeCoil[i] + glm::vec3(0.0f, 0.02f, 0.0f), 0.20f, 0.04f, rope);
    crewBox(c, hull, L.shotRack + glm::vec3(0.0f, 0.03f, 0.0f), glm::vec3(0.14f, 0.06f, 0.14f), darkWood);
    for (int i = 0; i < 6; ++i) {
        const int row = (i < 3) ? 0 : 1;
        const int col = (i < 3) ? i : i - 3;
        crewBall(c, hull, L.shotRack + glm::vec3(-0.035f + 0.035f * static_cast<float>(col) + (row ? 0.018f : 0.0f) - 0.02f, 0.085f + 0.05f * static_cast<float>(row), 0.0f), glm::vec3(0.06f), BLACK_PLASTIC);
    }
    {
        const glm::vec3 w = L.workbench;
        crewBox(c, hull, w + glm::vec3(0.0f, 0.13f, 0.0f), glm::vec3(0.16f, 0.025f, 0.34f), darkWood);
        crewBox(c, hull, w + glm::vec3(0.0f, 0.065f, 0.13f), glm::vec3(0.12f, 0.13f, 0.03f), darkWood);
        crewBox(c, hull, w + glm::vec3(0.0f, 0.065f, -0.13f), glm::vec3(0.12f, 0.13f, 0.03f), darkWood);
        crewBox(c, hull, w + glm::vec3(0.0f, 0.15f, 0.05f), glm::vec3(0.10f, 0.014f, 0.16f), wood);                      // a plank waiting to be cut
        // The patch on the planking, if the ship has been hit: a pale new board where the carpenter is working.
    }
    if (crew.repairTimer > 0.0f) {
        const glm::vec3 s = crew.repairSpot;
        crewBox(c, hull, s + glm::vec3(0.0f, 0.004f, 0.0f), glm::vec3(0.22f, 0.008f, 0.14f), crewCloth(glm::vec3(0.78f, 0.62f, 0.38f)));
    }
}
