#pragma once
// One crew member, built the same way as the ship: named local positions
// measured from a root, drawn with the project's reusable unit meshes (a
// cube and a sphere), each tinted its own colour. No pose and no role yet -
// this file only builds the still, standing body; giving it something to do
// is later work.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Shader.h"

namespace CrewShape {

// Legs: two separate boxes with a small gap between them, instead of one
// wide box standing in for both - a person has two legs, and building them
// as two objects is what lets each one eventually move independently
// (walking, climbing rigging) once Stage K gives the crew something to do.
constexpr float LEG_WIDTH = 0.10f;
constexpr float LEG_HEIGHT = 0.35f;
constexpr float LEG_DEPTH = 0.10f;
constexpr float LEG_GAP = 0.03f;
const glm::vec3 PANTS_COLOR(0.15f, 0.20f, 0.35f);

constexpr float TORSO_WIDTH = 0.22f;
constexpr float TORSO_HEIGHT = 0.32f;
constexpr float TORSO_DEPTH = 0.14f;

constexpr float ARM_WIDTH = 0.08f;
constexpr float ARM_HEIGHT = 0.28f;
constexpr float ARM_DEPTH = 0.08f;

// Hands: a small sphere at the end of each arm.
constexpr float HAND_RADIUS = 0.045f;

constexpr float HEAD_RADIUS = 0.11f;
const glm::vec3 SKIN_COLOR(0.85f, 0.68f, 0.53f);

// The nose and ears are the same material as the head, but NOT the exact
// same colour - with no lighting in this project yet, a same-coloured bump
// on a same-coloured sphere has no shading to reveal its shape and simply
// disappears. A slightly different shade is what actually makes them
// visible, standing in for the shading Stage C will eventually add.
const glm::vec3 NOSE_COLOR(0.90f, 0.64f, 0.52f);
const glm::vec3 EAR_COLOR(0.78f, 0.60f, 0.46f);

// Face: eyes, ears, a nose, and lips, all placed relative to the head's own
// centre. "Front" is +Z and "up" is +Y, the same axes as everywhere else in
// this project - there is nothing face-specific about the directions, only
// about how small these parts are.
constexpr float EYE_RADIUS = 0.018f;
constexpr float EYE_OFFSET_X = 0.045f;
constexpr float EYE_OFFSET_Y = 0.015f;
const glm::vec3 EYE_COLOR(0.05f, 0.05f, 0.08f);

constexpr float EAR_RADIUS = 0.028f;

constexpr float NOSE_RADIUS = 0.022f;

constexpr float LIPS_WIDTH = 0.05f;
constexpr float LIPS_HEIGHT = 0.014f;
constexpr float LIPS_DEPTH = 0.012f;
const glm::vec3 LIPS_COLOR(0.55f, 0.25f, 0.28f);

// The default shirt colour, used when a caller does not ask for a
// particular one - see drawCrewMember()'s 'shirtColor' argument, which lets
// main.cpp give each of several crew members their own colour.
const glm::vec3 DEFAULT_SHIRT_COLOR(0.25f, 0.35f, 0.65f);

} // namespace CrewShape

// Draws one crew member standing at 'crewRoot', feet at its local origin -
// so 'crewRoot' should already be positioned exactly on the deck (or ground)
// it is meant to stand on. 'shirtColor' tints the torso, arms, and hands'
// sleeves are not modelled separately, so this is really "the shirt and the
// exposed collar together" - everything is one colour, per crew member.
inline void drawCrewMember(
    ShaderProgram& shader,
    const Mesh& unitCube,
    const Mesh& unitSphere,
    const glm::mat4& crewRoot,
    const glm::vec3& shirtColor = CrewShape::DEFAULT_SHIRT_COLOR)
{
    using namespace CrewShape;

    auto drawPart = [&](const Mesh& mesh, const glm::mat4& model, const glm::vec3& color) {
        shader.setMat4("uModel", model);
        shader.setVec3("uTint", color);
        mesh.draw();
    };

    // Legs: the feet are at y = 0, crewRoot's own origin, so each leg box's
    // centre sits at half its own height above that. Two separate boxes,
    // offset left and right of the centreline by half the gap plus half a
    // leg's own width.
    const float legOffsetX = LEG_GAP * 0.5f + LEG_WIDTH * 0.5f;
    const float legSides[2] = { -legOffsetX, legOffsetX };
    for (float legX : legSides) {
        const glm::mat4 legFrame =
            glm::translate(crewRoot, glm::vec3(legX, LEG_HEIGHT * 0.5f, 0.0f));
        drawPart(unitCube, glm::scale(legFrame, glm::vec3(LEG_WIDTH, LEG_HEIGHT, LEG_DEPTH)), PANTS_COLOR);
    }

    // Torso: sits right on top of the legs.
    const float torsoCenterY = LEG_HEIGHT + TORSO_HEIGHT * 0.5f;
    const glm::mat4 torsoFrame = glm::translate(crewRoot, glm::vec3(0.0f, torsoCenterY, 0.0f));
    drawPart(unitCube, glm::scale(torsoFrame, glm::vec3(TORSO_WIDTH, TORSO_HEIGHT, TORSO_DEPTH)), shirtColor);

    // Arms and hands: one of each on either side of the torso, at the
    // torso's own height. The hand sits just past the bottom of its arm.
    const float armOffsetX = TORSO_WIDTH * 0.5f + ARM_WIDTH * 0.5f;
    const float armSides[2] = { -armOffsetX, armOffsetX };
    for (float armX : armSides) {
        const glm::mat4 armFrame = glm::translate(crewRoot, glm::vec3(armX, torsoCenterY, 0.0f));
        drawPart(unitCube, glm::scale(armFrame, glm::vec3(ARM_WIDTH, ARM_HEIGHT, ARM_DEPTH)), shirtColor);

        const float handCenterY = torsoCenterY - ARM_HEIGHT * 0.5f - HAND_RADIUS * 0.6f;
        const glm::mat4 handFrame = glm::translate(crewRoot, glm::vec3(armX, handCenterY, 0.0f));
        drawPart(unitSphere, glm::scale(handFrame, glm::vec3(HAND_RADIUS * 2.0f)), SKIN_COLOR);
    }

    // Head: a sphere sitting on top of the torso.
    const float headCenterY = LEG_HEIGHT + TORSO_HEIGHT + HEAD_RADIUS;
    const glm::vec3 headCenter(0.0f, headCenterY, 0.0f);
    const glm::mat4 headFrame = glm::translate(crewRoot, headCenter);
    drawPart(unitSphere, glm::scale(headFrame, glm::vec3(HEAD_RADIUS * 2.0f)), SKIN_COLOR);

    // Eyes: two small dark spheres near the front of the head.
    const float eyeZ = HEAD_RADIUS * 0.92f;
    const float eyeSides[2] = { -EYE_OFFSET_X, EYE_OFFSET_X };
    for (float eyeX : eyeSides) {
        const glm::mat4 eyeFrame = glm::translate(
            crewRoot, headCenter + glm::vec3(eyeX, EYE_OFFSET_Y, eyeZ));
        drawPart(unitSphere, glm::scale(eyeFrame, glm::vec3(EYE_RADIUS * 2.0f)), EYE_COLOR);
    }

    // Ears: two small spheres at the head's own left and right extremes.
    const float earSides[2] = { -HEAD_RADIUS * 0.95f, HEAD_RADIUS * 0.95f };
    for (float earX : earSides) {
        const glm::mat4 earFrame = glm::translate(crewRoot, headCenter + glm::vec3(earX, 0.0f, 0.0f));
        drawPart(unitSphere, glm::scale(earFrame, glm::vec3(EAR_RADIUS * 2.0f)), EAR_COLOR);
    }

    // Nose: a small sphere at the front of the head, between and below the eyes.
    const glm::mat4 noseFrame = glm::translate(
        crewRoot, headCenter + glm::vec3(0.0f, -0.010f, HEAD_RADIUS * 0.95f));
    drawPart(unitSphere, glm::scale(noseFrame, glm::vec3(NOSE_RADIUS * 2.0f)), NOSE_COLOR);

    // Lips: a small flat box below the nose.
    const glm::mat4 lipsFrame = glm::translate(
        crewRoot, headCenter + glm::vec3(0.0f, -0.045f, HEAD_RADIUS * 0.90f));
    drawPart(unitCube, glm::scale(lipsFrame, glm::vec3(LIPS_WIDTH, LIPS_HEIGHT, LIPS_DEPTH)), LIPS_COLOR);
}
