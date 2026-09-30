#pragma once
// Background scenery: a mountain on the horizon, a few drifting clouds, and
// rocks poking out of the water. Every shape here is still one of the
// project's existing reusable meshes - nothing new is invented, only placed
// and coloured differently, the same rule every object in this project
// follows.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Shader.h"

namespace SceneryShape {

const glm::vec3 MOUNTAIN_COLOR(0.42f, 0.42f, 0.46f);
const glm::vec3 MOUNTAIN_SNOW_COLOR(0.93f, 0.94f, 0.96f);
const glm::vec3 CLOUD_COLOR(0.98f, 0.98f, 1.0f);
const glm::vec3 ROCK_COLOR(0.35f, 0.34f, 0.33f);

} // namespace SceneryShape

// A mountain. 'mountainShape' is makeUnitTaperedBox() (src/Mesh.h) with a
// small 'bottomScale', drawn with a NEGATIVE height in its scale - the
// exact same shape the ship's hull uses to go narrow at the BOTTOM, but
// flipping the sign of the height turns "narrow at the bottom" into
// "narrow at the top" for free, with no new mesh and no rotation: a
// negative scale still spans the same [-0.5, +0.5] unit range, it just
// swaps which end is which. That is exactly a mountain's silhouette - wide
// base, narrow peak - built from a shape this project already has.
inline void drawMountain(
    ShaderProgram& shader,
    const Mesh& mountainShape,
    const Mesh& unitSphere,
    const glm::mat4& root,
    float width,
    float height,
    float depth)
{
    using namespace SceneryShape;

    const glm::mat4 frame = glm::translate(root, glm::vec3(0.0f, height * 0.5f, 0.0f));
    shader.setMat4("uModel", glm::scale(frame, glm::vec3(width, -height, depth)));
    shader.setVec3("uTint", MOUNTAIN_COLOR);
    mountainShape.draw();

    // A small, lighter cap near the peak, suggesting snow.
    const glm::mat4 capFrame = glm::translate(root, glm::vec3(0.0f, height * 0.92f, 0.0f));
    shader.setMat4("uModel", glm::scale(capFrame, glm::vec3(width * 0.22f, height * 0.20f, depth * 0.22f)));
    shader.setVec3("uTint", MOUNTAIN_SNOW_COLOR);
    unitSphere.draw();
}

// A cloud: a small cluster of overlapping spheres at different sizes and
// offsets - the same "several simple shapes read as one object" idea the
// crew and the ship's cannon already use, just with spheres instead of
// boxes and cylinders.
inline void drawCloud(ShaderProgram& shader, const Mesh& unitSphere, const glm::mat4& root, float scale)
{
    using namespace SceneryShape;

    struct Puff {
        glm::vec3 offset;
        float radius;
    };
    const Puff puffs[4] = {
        { glm::vec3(0.0f, 0.0f, 0.0f), 1.0f },
        { glm::vec3(0.75f, 0.08f, 0.05f), 0.75f },
        { glm::vec3(-0.75f, 0.05f, -0.05f), 0.70f },
        { glm::vec3(0.10f, 0.32f, 0.05f), 0.62f },
    };
    for (const Puff& puff : puffs) {
        const glm::mat4 puffFrame = glm::translate(root, puff.offset * scale);
        shader.setMat4("uModel", glm::scale(puffFrame, glm::vec3(puff.radius * scale * 2.0f)));
        shader.setVec3("uTint", CLOUD_COLOR);
        unitSphere.draw();
    }
}

// A rock: the same "a few overlapping spheres" idea as the cloud, in a
// dark grey, sitting low enough that it reads as breaking the water's
// surface rather than floating above it.
inline void drawRock(ShaderProgram& shader, const Mesh& unitSphere, const glm::mat4& root, float scale)
{
    using namespace SceneryShape;

    struct Chunk {
        glm::vec3 offset;
        float radius;
    };
    const Chunk chunks[3] = {
        { glm::vec3(0.0f, 0.0f, 0.0f), 1.0f },
        { glm::vec3(0.55f, -0.15f, 0.25f), 0.62f },
        { glm::vec3(-0.5f, -0.20f, -0.2f), 0.55f },
    };
    for (const Chunk& chunk : chunks) {
        const glm::mat4 chunkFrame = glm::translate(root, chunk.offset * scale);
        shader.setMat4("uModel", glm::scale(chunkFrame, glm::vec3(chunk.radius * scale * 2.0f)));
        shader.setVec3("uTint", ROCK_COLOR);
        unitSphere.draw();
    }
}
