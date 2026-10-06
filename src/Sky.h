#pragma once

// Phase 45: the sky - the arithmetic that places the dome, the sun and the gradient.
//
// There is no OpenGL in this file, so every claim about the sky can be checked on the CPU against
// the very functions the renderer calls: that the sun's disc lies EXACTLY along the direction its
// light comes from, that the gradient starts at the horizon's own colour, and that the dome is
// inside the far plane. main.cpp does the drawing; this file only does the sums.
//
// WHAT THE SKY IS, AND IS NOT
//
// It is the existing sphere mesh (the same makeSphere() as the ball and the cannonball), seen
// from inside, centred on the camera, with a colour gradient painted into its vertices and drawn
// as pure emission. It is not a skybox: a skybox is a cube of six TEXTURES, and this project has
// none. There is no image anywhere - the "picture" of the sky is a linear blend between two
// colours.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace SkyConfig {

// The dome's radius. It must be inside the far plane (100) or the far side would be clipped, and
// outside everything the scene draws that matters. (The ship is never more than about 14 units
// from the camera in any preset.)
constexpr float RADIUS = 90.0f;

// How finely the dome is cut. Its colour is linear in elevation, so the gradient is smooth for
// any value; these only decide how round the dome's silhouette is and how many triangles it costs
// (2 * SLICES * (STACKS - 1) = 1656). They do NOT follow the '+' and '-' detail level: the dome
// is not one of the three parameterised meshes.
constexpr int STACKS = 24;
constexpr int SLICES = 36;

// The sun's disc is a small sphere placed this far from the camera, along the direction the
// sunlight comes FROM, so it appears exactly where the light says the sun is. It is inside the
// dome (80 < 90) and drawn after it.
constexpr float SUN_DISTANCE = 80.0f;

// How big the disc looks, as the angle it covers. The real sun covers about half a degree; a
// disc that small would be a few pixels here, so this is deliberately large - 3 degrees reads
// clearly as a sun without swallowing the sky around it.
constexpr float SUN_ANGULAR_DIAMETER_DEGREES = 3.0f;

} // namespace SkyConfig

// The direction FROM the scene TOWARDS the sun. A light's `sunDirection` is the way the light
// TRAVELS, so the sun lies the opposite way: this is L in the lighting equation, negated once,
// here, and nowhere else.
inline glm::vec3 skyDirectionToSun(const glm::vec3& sunTravelDirection)
{
    return -glm::normalize(sunTravelDirection);
}

// Where the sun's disc is centred, in the world: the eye, plus the direction to the sun, times a
// distance. The dome is centred on the eye too, so the sun keeps the same place in the sky however
// the camera moves - exactly as the real one does.
inline glm::vec3 skySunPosition(const glm::vec3& eye, const glm::vec3& sunTravelDirection, float distance)
{
    return eye + skyDirectionToSun(sunTravelDirection) * distance;
}

// The disc's diameter for a given distance, so that it COVERS a given angle as seen from the eye:
// a sphere of diameter D at distance d spans 2 * atan(D / 2d). Solving for D:
inline float skySunDiscDiameter(float distance, float angularDiameterDegrees)
{
    return 2.0f * distance * std::tan(glm::radians(angularDiameterDegrees) * 0.5f);
}

// The two colours to hand to makeSphere() so that the dome shows `horizon` EXACTLY at elevation
// 0 and `zenith` exactly straight up.
//
// makeSphere() blends its two colours LINEARLY in the polar angle: the south pole gets `bottom`,
// the north pole `top`, and the equator - the horizon - gets their midpoint. So:
//
//     top    = zenith
//     bottom = 2 * horizon - zenith           so that  (bottom + top) / 2 = horizon
//
// `bottom` can have a channel above 1 (red 1.76 for the golden-hour sky). That is fine in a vertex:
// it is a float, and it is only ever seen far below the horizon, behind the sea.
inline glm::vec3 skyDomeBottomColor(const glm::vec3& horizon, const glm::vec3& zenith)
{
    return 2.0f * horizon - zenith;
}

// What the dome shows at a given elevation (radians above the horizon, -pi/2 to +pi/2): the
// blend the vertices carry. Used by the tests to check the mesh against the formula it is meant
// to follow:   horizon + (zenith - horizon) * (elevation / 90 degrees)
inline glm::vec3 skyColorAtElevation(const glm::vec3& horizon, const glm::vec3& zenith, float elevationRadians)
{
    constexpr float HALF_PI = 1.57079632679489662f;
    return horizon + (zenith - horizon) * (elevationRadians / HALF_PI);
}
