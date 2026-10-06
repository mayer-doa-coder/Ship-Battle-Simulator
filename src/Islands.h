#pragma once

// Phase 46: distant islands and sea stacks.
//
// An empty horizon looks like a render; a faint dark coastline on it looks like a place. This is
// scenery and nothing more: nothing collides with it, nothing aims at it, and it is not part of
// the ship. It is a TABLE of nine shapes placed far out on the sea, drawn with meshes that already
// exist - the sphere flattened into a hill, the cylinder stood up as a rock - so it costs nine
// draw calls and no new geometry.
//
// Why they look like "dark hazed silhouettes" without any code that says so: they are 38 to 56
// units from the ship, and the Phase 45 haze is 76% to 93% complete over that range. Whatever the
// island's own colour, most of what reaches the eye is the horizon's colour, and the island shows
// only as a slightly darker, slightly bluer shape against it. The haze does the work.
//
// There is no OpenGL here, so the placement rules can be checked exactly on the CPU.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

enum class IslandKind {
    HILL,   // the unit sphere, flattened and half-sunk: a rounded island
    ROCK,   // the unit cylinder, stood up and mostly above the water: a sea stack
};

struct Island {
    IslandKind kind;

    // Where it stands on the sea, as a compass bearing and a distance FROM THE SHIP. Bearing 0
    // is straight ahead of the bow (+z) and it turns towards +x, the same convention the camera's
    // yaw and the sky use. A bearing and a distance read better in a table than a pair of x and z
    // numbers, and "move one island" is then a change to one of two familiar numbers.
    float bearingDegrees;
    float distance;

    // How wide it is across, and how high it rises ABOVE the waterline, in world units.
    float width;
    float height;
};

namespace IslandConfig {

constexpr int COUNT = 15;

// The table. Bearings are spread round the whole compass so that every camera preset has some
// coast in view: F1 looks towards bearing 215, F2 towards 180, F3 towards 270 and F4 towards 0.
// Distances stay between 38 and 56 so that they are far from the ship (which is 5.6 long) and
// well inside the sea (80 to its edge) and the sky dome (90).
const Island ISLANDS[COUNT] = {
    //  kind              bearing  distance  width  height
    { IslandKind::HILL,    190.0f,  46.0f,   14.0f,  2.6f },
    { IslandKind::ROCK,    205.0f,  38.0f,    1.6f,  4.2f },
    { IslandKind::HILL,    232.0f,  54.0f,   22.0f,  4.4f },
    { IslandKind::HILL,    258.0f,  44.0f,    9.0f,  1.8f },
    { IslandKind::HILL,    300.0f,  50.0f,   18.0f,  3.2f },
    { IslandKind::ROCK,    340.0f,  41.0f,    2.2f,  5.0f },
    { IslandKind::HILL,     20.0f,  56.0f,   20.0f,  3.8f },
    { IslandKind::HILL,     75.0f,  47.0f,   12.0f,  2.2f },
    { IslandKind::HILL,    160.0f,  52.0f,   16.0f,  3.0f },
    // Added with the playable build: nearer scenery, so that sailing past something shows you are moving. These are 24 to 36 units from the
    // start, where the haze is lighter. The island at bearing 310 carries the lighthouse (drawn in renderScene from LIGHTHOUSE_*).
    { IslandKind::HILL,     70.0f,  30.0f,    9.0f,  1.6f },
    { IslandKind::ROCK,    100.0f,  24.0f,    1.4f,  3.0f },
    { IslandKind::HILL,    135.0f,  36.0f,   12.0f,  2.4f },
    { IslandKind::HILL,    310.2f,  34.1f,   10.0f,  1.4f },
    { IslandKind::ROCK,    285.0f,  26.0f,    1.8f,  3.6f },
    { IslandKind::HILL,    335.0f,  42.0f,   14.0f,  2.8f },
};

// The lighthouse stands on the island at bearing 310.2, distance 34.1 from the start: x = -26, z = 22.
constexpr float LIGHTHOUSE_X = -26.0f;
constexpr float LIGHTHOUSE_Z = 22.0f;
constexpr float LIGHTHOUSE_BASE_HEIGHT = 1.2f;      // above the sea: the island's top
constexpr float LIGHTHOUSE_HEIGHT = 7.0f;

// How far a ROCK's cylinder reaches BELOW the waterline, so that it rises out of the sea instead
// of standing on it with a perfect circle round its foot.
constexpr float ROCK_DEPTH_BELOW_WATER = 0.3f;

} // namespace IslandConfig

// Where an island's centre is on the sea, as x and z, measured from the ship's root.
inline glm::vec2 islandOffsetFromShip(const Island& island)
{
    const float a = glm::radians(island.bearingDegrees);
    return glm::vec2(std::sin(a) * island.distance, std::cos(a) * island.distance);
}

// The radius of the circle the island's foot occupies on the water.
inline float islandRadius(const Island& island)
{
    return island.width * 0.5f;
}

// The matrix handed to drawMesh(): the island's frame times its draw-time scale, as every object
// in the project is built. `shipRoot` is the ship's position at the waterline, so the sea level is
// its y.
//
// HILL  - the unit sphere is 1 across and centred on its origin. Scaled to (width, 2 x height, width)
//         and centred ON the waterline, its upper half rises `height` above the water and its lower
//         half is under the sea, hidden by the sea's own surface.
// ROCK  - the unit cylinder is 1 across, 1 tall and centred on its origin. Scaled to (width,
//         height + ROCK_DEPTH, width) it is lifted so that its top is `height` above the water.
inline glm::mat4 islandModel(const Island& island, const glm::vec3& shipRoot)
{
    const glm::vec2 o = islandOffsetFromShip(island);
    if (island.kind == IslandKind::HILL) {
        return glm::translate(glm::mat4(1.0f), glm::vec3(shipRoot.x + o.x, shipRoot.y, shipRoot.z + o.y))
             * glm::scale(glm::mat4(1.0f), glm::vec3(island.width, 2.0f * island.height, island.width));
    }
    const float total = island.height + IslandConfig::ROCK_DEPTH_BELOW_WATER;
    return glm::translate(glm::mat4(1.0f),
                          glm::vec3(shipRoot.x + o.x,
                                    shipRoot.y + (island.height - IslandConfig::ROCK_DEPTH_BELOW_WATER) * 0.5f,
                                    shipRoot.z + o.y))
         * glm::scale(glm::mat4(1.0f), glm::vec3(island.width, total, island.width));
}
