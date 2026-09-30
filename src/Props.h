#pragma once
// Small reusable objects for a ship battle scene - a barrel, a crate, and a
// cannonball - built the exact same way as the ship and the crew member: one
// named local transform from a root, drawn with the project's existing
// reusable unit meshes, tinted with 'uTint'. No new GPU geometry, no new
// technique - just three more things a scene can be built from.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Shader.h"

namespace PropShape {

constexpr float BARREL_RADIUS = 0.14f;
constexpr float BARREL_HEIGHT = 0.30f;
const glm::vec3 BARREL_COLOR(0.36f, 0.22f, 0.10f);

// Hoops: two thin, slightly wider bands near the top and bottom of the
// barrel - the detail that tells a barrel apart from a plain cylinder.
constexpr float HOOP_HEIGHT = 0.03f;
constexpr float HOOP_RADIUS_SCALE = 1.08f;   // slightly wider than the barrel itself
const glm::vec3 HOOP_COLOR(0.18f, 0.14f, 0.09f);

constexpr float CRATE_SIZE = 0.22f;
const glm::vec3 CRATE_COLOR(0.55f, 0.40f, 0.22f);

// Straps: two thin crossing bands on the crate's lid - the detail that
// tells a crate apart from a plain cube.
constexpr float STRAP_WIDTH = 0.03f;
constexpr float STRAP_THICKNESS = 0.02f;
const glm::vec3 STRAP_COLOR(0.30f, 0.20f, 0.10f);

constexpr float CANNONBALL_RADIUS = 0.09f;
const glm::vec3 CANNONBALL_COLOR(0.08f, 0.08f, 0.09f);

} // namespace PropShape

// A barrel standing upright, with two hoop bands. 'root' is the surface it
// stands on - its own origin is the barrel's base, not its centre, so the
// barrel never needs repositioning to sit flush on a deck or the ground.
inline void drawBarrel(ShaderProgram& shader, const Mesh& unitCylinder, const glm::mat4& root)
{
    using namespace PropShape;

    const glm::mat4 bodyFrame = glm::translate(root, glm::vec3(0.0f, BARREL_HEIGHT * 0.5f, 0.0f));
    shader.setMat4("uModel", glm::scale(bodyFrame, glm::vec3(BARREL_RADIUS * 2.0f, BARREL_HEIGHT, BARREL_RADIUS * 2.0f)));
    shader.setVec3("uTint", BARREL_COLOR);
    unitCylinder.draw();

    const float hoopDiameter = BARREL_RADIUS * 2.0f * HOOP_RADIUS_SCALE;
    const float hoopYs[2] = { BARREL_HEIGHT * 0.18f, BARREL_HEIGHT * 0.82f };
    for (float hoopY : hoopYs) {
        const glm::mat4 hoopFrame = glm::translate(root, glm::vec3(0.0f, hoopY, 0.0f));
        shader.setMat4("uModel", glm::scale(hoopFrame, glm::vec3(hoopDiameter, HOOP_HEIGHT, hoopDiameter)));
        shader.setVec3("uTint", HOOP_COLOR);
        unitCylinder.draw();
    }
}

// A crate with two crossing lid straps, its base at 'root's own origin - the
// same "stands on this surface" convention as the barrel.
inline void drawCrate(ShaderProgram& shader, const Mesh& unitCube, const glm::mat4& root)
{
    using namespace PropShape;

    const glm::mat4 bodyFrame = glm::translate(root, glm::vec3(0.0f, CRATE_SIZE * 0.5f, 0.0f));
    shader.setMat4("uModel", glm::scale(bodyFrame, glm::vec3(CRATE_SIZE)));
    shader.setVec3("uTint", CRATE_COLOR);
    unitCube.draw();

    // The straps sit just above the lid, not exactly on it, so they never
    // fight the lid's own surface for the same pixels.
    const float strapY = CRATE_SIZE + STRAP_THICKNESS * 0.5f;
    const glm::mat4 strapFrame = glm::translate(root, glm::vec3(0.0f, strapY, 0.0f));

    shader.setMat4("uModel", glm::scale(strapFrame, glm::vec3(CRATE_SIZE, STRAP_THICKNESS, STRAP_WIDTH)));
    shader.setVec3("uTint", STRAP_COLOR);
    unitCube.draw();

    shader.setMat4("uModel", glm::scale(strapFrame, glm::vec3(STRAP_WIDTH, STRAP_THICKNESS, CRATE_SIZE)));
    shader.setVec3("uTint", STRAP_COLOR);
    unitCube.draw();
}

// A cannonball, resting on 'root's own surface rather than half-buried in it -
// the one difference from the ship's cannon, which fires a ball rather than
// storing one, is a later phase (Stage H).
inline void drawCannonball(ShaderProgram& shader, const Mesh& unitSphere, const glm::mat4& root)
{
    using namespace PropShape;
    const glm::mat4 frame = glm::translate(root, glm::vec3(0.0f, CANNONBALL_RADIUS, 0.0f));
    shader.setMat4("uModel", glm::scale(frame, glm::vec3(CANNONBALL_RADIUS * 2.0f)));
    shader.setVec3("uTint", CANNONBALL_COLOR);
    unitSphere.draw();
}
