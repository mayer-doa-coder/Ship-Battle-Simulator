// Ship Battle Simulator - Phase 33: Stage C complete - the two L9 demonstrations.
//
// Every frame follows the same clear order:
//   1. measure time;
//   2. read input;
//   3. update scene data;
//   4. render the scene;
//   5. show the frame and read window events.
//
// There is a floor now: a flat grid, lying in the XZ plane and facing straight
// up. makeGrid() in src/Mesh.h is the project's first PARAMETERISED generator -
// it is handed GridConfig::CELLS and builds a different amount of geometry
// depending on it, where makeCube and makeQuad always build the same 24 and 4
// vertices. At CELLS = 8 the grid is 81 vertices and 128 triangles.
//
// CELLS decides how finely the grid is DIVIDED. GridConfig::SIZE decides how
// BIG it is, as a draw-time glm::scale, exactly like every other object since
// Phase 16. Keeping those two apart is the whole idea.
//
// This mesh is the sea. Phase 40 displaces its y from the wave functions of x
// and z, which is why it is built horizontal rather than upright like the quad,
// and why its two parameters have to be x and z.
//
// Phase 19 added the way to SEE that CELLS did anything: in solid shading a flat
// grid looks the same however finely it is cut, so press 'W'. The 'W' key also
// prints the counts CELLS predicts, to check against the formula.
//
// Phase 20 adds the first CURVED surface: an open tube standing on the left,
// built by makeCylinder() from a ring of sinf/cosf vertices. It is the first
// mesh whose normals differ from vertex to vertex, each one computed
// analytically as normalize(vec3(x, 0, z)) - straight out from the axis. Press
// 'N' and the colours sweep smoothly round its circumference instead of showing
// one flat value per face.
//
// Phase 21 closed it. Each end got a centre vertex and a triangle fan with a
// FLAT normal - (0, +1, 0) on top and (0, -1, 0) underneath. Those caps cannot
// reuse the wall's rim vertices, because a vertex carries one normal and the
// wall's points sideways, so the mesh grew to 4*segments + 2.
//
// Phase 22 added a sphere, the last of the five meshes the whole project is built
// from. It is the first generator with TWO parameters - stacks for latitude and
// slices for longitude, independent of each other - and the one with the
// simplest normal of all: for a ball centred on its own origin, the direction
// out from the centre IS the surface normal, so the normal is just
// normalize(position). Press 'N' and it renders as the classic RGB ball.
//
// Phase 23 added no sixth shape. Every normal before it was ANALYTIC, written
// down because the shape was known; that phase added the other method -
// computeSmoothNormals(), the L9 slide 20 averaging formula - and a SECOND cube
// built from 8 shared corners to demonstrate it on. The two cubes on the right
// are the same size in the same rotation, differing only in that. The flat cube
// CANNOT be smoothed, and why not is the lesson.
//
// Phase 24 counts the cost. Every draw passes through drawMesh(), so that is
// where the draw calls, triangles and vertices are tallied, and the totals go
// into the window title - a HUD that needs no font, no texture and no extra
// geometry, and that survives into a screen recording. These are MEASUREMENTS of
// what was drawn, not predictions from the config, which makes them the right
// thing to check a hand-worked triangle count against.
//
// Phase 25 finishes Stage B by making the geometry adjustable. '+' and '-' move
// one detail level, and the grid, cylinder and sphere are built again from it
// while the scene keeps turning - six levels spanning 92 to 36,116 triangles. The
// one line that makes that safe is the destroy() at the top of Mesh::upload(),
// written in Phase 14 for exactly this moment: without it every press would
// abandon three GPU buffers with nothing able to free them again.
//
// Two things in it are worth more than the feature. Each level's divisions are
// worked out from the LEVEL-0 constant rather than from the level before, so '-'
// then '+' returns to precisely where it started instead of drifting; and the
// draw count stays at 10 at every level, because detail changes how finely the
// same objects are divided, never how many objects there are.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Lighting.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace AppConfig {

// These values are grouped here so they are easy to find and change during a viva.
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "Ship Battle Simulator - Phase 33";
constexpr const char* VERTEX_SHADER_PATH = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_PATH = "shaders/basic.frag";

// A delayed or dragged window can create one unusually large frame time.
// Limiting dt prevents future movement from jumping a large distance at once.
constexpr float MAX_DELTA_TIME = 0.10f;

// Print one timing report per second instead of printing every frame.
constexpr float REPORT_INTERVAL = 1.0f;

// 1 enables V-sync. Change this to 0 only when measuring uncapped performance.
constexpr int VSYNC_INTERVAL = 1;

// GLM is used here so Phase 0 verifies that the math library is configured too.
const glm::vec3 CLEAR_COLOR(0.82f, 0.66f, 0.04f);

// Phase 3: a colour filter sent to the fragment shader every frame as the
// uniform 'uTint'. Each vertex colour is multiplied by it.
//   (1, 1, 1)    leaves the triangle exactly as its vertex colours describe;
//   below 1      dims that channel;
//   above 1      brightens it, up to the display limit of 1.
// This is the easiest viva value in the phase: change it, rebuild, and the
// triangle changes colour without any edit to shaders/basic.frag.
const glm::vec3 TINT(0.6f, 2.4f, 3.0f);
} // namespace AppConfig

namespace CameraConfig {

// Phase 7 gave the camera a fixed position, EYE. Phase 12 replaced that with
// a real OrbitCamera (src/Camera.h), driven by the mouse since Phase 13, so
// only the point it looks at and which way is "up" stay as constants here.
const glm::vec3 TARGET(0.0f, 0.0f, 0.0f);    // the point it looks at
const glm::vec3 UP(0.0f, 1.0f, 0.0f);        // which way is "up" for this camera

// The viewing frustum: a narrow pyramid of visible space with its point at
// the camera. FIELD_OF_VIEW_DEGREES sets how wide that pyramid opens.
// NEAR_PLANE and FAR_PLANE cut off anything closer or farther than that -
// nothing outside this range is ever drawn, which is why both must comfortably
// contain the triangle's whole depth range (Phase 7 moves it between 2.5 and
// 5.5 units from EYE).
constexpr float FIELD_OF_VIEW_DEGREES = 45.0f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 100.0f;

} // namespace CameraConfig

namespace TriangleConfig {

// Phase 14: each row is now one `Vertex` from src/Mesh.h - a position, a
// normal, and a colour - instead of six loose floats whose meaning depended
// on counting. The position and colour numbers are exactly the ones Phase 2
// typed in; only the way they are written down is new.
//
// The triangle is flat and lies in the XY plane, so all three of its corners
// face the same way: straight out of the screen along +Z. No line of shader
// code reads that normal yet, but the mesh uploads it, so it has to be right
// now rather than guessed at later.
//
// These are still easy viva values: edit positions to reshape/move the
// triangle and edit colours to change its three corners.
const glm::vec3 FLAT_NORMAL(0.0f, 0.0f, 1.0f);

const Vertex VERTICES[] = {
    { {  0.40f,  0.37f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 0.1f } },
    { { -0.40f,  0.37f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 0.1f } },
    { {  0.00f, -0.73f, 0.0f }, FLAT_NORMAL, { 0.1f, 0.1f, 1.0f } },
};

// Phase 14: the triangle needs an index list for the first time. `Mesh`
// always draws with glDrawElements, so even a shape with no shared corners
// needs one - here simply "corner 0, corner 1, corner 2", which is the same
// order glDrawArrays walked through on its own until now.
const unsigned int INDICES[] = { 0, 1, 2 };

constexpr int VERTEX_COUNT = 3;
constexpr int INDEX_COUNT = 3;

} // namespace TriangleConfig

namespace TriangleMotion {

// Phase 4: how the triangle slides. The x offset at any moment is
//     offset = SLIDE_DISTANCE * sin(SLIDE_SPEED * now)
// so it swings between -SLIDE_DISTANCE and +SLIDE_DISTANCE.
// The visible x range is -1 to +1 and the triangle is 0.4 wide on each side of
// its centre, so a distance above 0.6 pushes part of it off the screen.
constexpr float SLIDE_DISTANCE = 0.5f;   // how far each way, in clip-space units
constexpr float SLIDE_SPEED = 1.0f;      // radians per second; one full swing is 2*pi / this

} // namespace TriangleMotion

namespace TriangleSpin {

// Phase 5: how the triangle turns. The angle at any moment is
//     angle = SPIN_SPEED * now
// measured in RADIANS. A full turn is 2*pi = 6.28 radians, so a speed of 2.0
// completes one turn in about 3.14 seconds. A positive angle turns
// counter-clockwise when the axis points toward the viewer.
//
// The turn happens around the model-space ORIGIN (0, 0), which is not exactly
// the visual middle of this triangle (its corners average to y = +0.13), so the
// triangle wobbles a little as it turns. To spin exactly on its middle, edit
// TriangleConfig::VERTICES so the three y values add up to zero.
constexpr float SPIN_SPEED = 0.5f;                        // radians per second
const glm::vec3 SPIN_AXIS(0.0f, 0.0f, 1.0f);              // Z: straight out of the screen

} // namespace TriangleSpin

namespace TriangleScale {

// Phase 6: how the triangle's size pulses. The scale factor at any moment is
//     factor = mid + amp * sin(PULSE_SPEED * now)
// where mid is halfway between PULSE_MIN and PULSE_MAX, and amp is half their
// difference. This keeps the factor inside [PULSE_MIN, PULSE_MAX] without a
// clamp. A factor of 1.0 is the triangle's original size; below 1.0 shrinks
// it, above 1.0 enlarges it.
constexpr float PULSE_MIN = 0.5f;     // smallest size, as a fraction of the original
constexpr float PULSE_MAX = 1.3f;     // largest size, as a fraction of the original
constexpr float PULSE_SPEED = 1.2f;   // radians per second

} // namespace TriangleScale

namespace TriangleDepth {

// Phase 7: how the triangle drifts toward and away from the camera. The world
// z position at any moment is
//     z = DEPTH_AMPLITUDE * sin(DEPTH_SPEED * now)
// which swings between -DEPTH_AMPLITUDE and +DEPTH_AMPLITUDE. The camera
// starts at a distance of 4.0 (OrbitCamera's default radius, Phase 12), so at
// that starting view the triangle's actual distance from the camera swings
// between (4 - DEPTH_AMPLITUDE) when it is nearest and (4 + DEPTH_AMPLITUDE)
// when it is farthest. Orbiting the camera changes the real distance, since
// the camera itself can move now - these two numbers describe the ORIGINAL
// fixed view, not a promise that holds from every angle.
//
// This is the phase's real demonstration. Before Phase 7, changing an
// object's z did nothing useful: with no view or projection matrix, z never
// affected how big anything looked, only whether it was clipped away. Now the
// SAME triangle visibly grows as it nears the camera and shrinks as it
// recedes, because uProjection performs a genuine perspective divide.
constexpr float DEPTH_AMPLITUDE = 1.5f;   // world units nearer/farther than TARGET
constexpr float DEPTH_SPEED = 0.8f;       // radians per second

} // namespace TriangleDepth

namespace DepthTestConfig {

// Phase 8: a second draw of the SAME triangle mesh, shifted this much
// FARTHER from the camera than the first. The camera sits at positive z
// looking toward the origin (CameraConfig), so a NEGATIVE z shift moves an
// object farther away.
//
// This value is fixed, not animated, on purpose: the demonstration only
// works if one copy is reliably nearer than the other at every instant,
// regardless of where TriangleDepth's oscillation happens to be right now.
constexpr float FAR_COPY_Z_OFFSET = -1.2f;   // world units farther than the near copy

// A strong, easily distinguished colour for the far copy, sent as 'uTint'
// just like the near copy's AppConfig::TINT. No second mesh or second vertex
// buffer is needed - the same VAO is bound and drawn twice with a different
// uModel and a different uTint.
const glm::vec3 FAR_COPY_TINT(3.0f, 0.5f, 0.4f);

} // namespace DepthTestConfig

namespace QuadConfig {

// Phase 17: the quad's four vertices and six indices are gone from this file.
// Phase 9 wrote them out by hand (4 rows and 6 numbers) and Phase 14 wrapped
// each row in a `Vertex`; makeQuad() in src/Mesh.h now generates exactly the
// same shape, as a unit quad - 1 x 1, centred on its own origin, facing +Z.
//
// What stays here is what is a choice about how the quad LOOKS and where it
// STANDS, which is the same split the cube made in Phase 14:
//
// The four corner colours, in the order makeQuad() walks its corners:
// bottom-left, bottom-right, top-right, top-left. A different colour on every
// corner makes the shared diagonal easy to see, and makes a wrong index
// obvious (the colours would no longer blend the way they are supposed to).
// These are the same four colours the quad has had since Phase 9, now listed
// in the generator's order instead of the old hand-typed one.
const glm::vec3 CORNER_COLORS[4] = {
    { 1.0f, 1.0f, 0.9f },   // 0: bottom-left,  yellow
    { 0.1f, 0.7f, 1.0f },   // 1: bottom-right, blue
    { 0.5f, 1.0f, 0.0f },   // 2: top-right,    green
    { 1.0f, 0.0f, 0.0f },   // 3: top-left,     red
};

// Phase 17: how big the quad is, as a multiple of the 1 x 1 unit mesh. It is
// the quad's width AND its height, and it is the viva value for this phase:
// change this one number and the quad resizes while the four vertices on the
// GPU stay exactly as they were.
//
// 0.8 is not arbitrary. The quad's corners sat at +-0.4 from Phase 9 until
// now, which is 0.8 across, so a scale of 0.8 on the 1 x 1 unit quad puts every
// corner in exactly the place it already was. The picture does not move.
//
// It is one number for both directions on purpose - a uniform scale, like the
// cubes' (Phase 16). A flat quad facing +Z could take separate width and
// height values without disturbing its normal, because the normal lies along
// the one axis left unscaled; it is simply not needed yet.
constexpr float SIZE = 0.8f;

// Phase 9: the quad does not move. It sits to one side, at the same distance
// from the camera as the other two triangles rest at, so it is easy to find
// and does not overlap them. Its job was to prove indexed drawing works (Phase
// 9) and is now to show the simplest generator (Phase 17); giving it motion
// too would blur those ideas with Phases 4-7's.
const glm::vec3 POSITION(-1.8f, 0.0f, 0.0f);

} // namespace QuadConfig

namespace GridConfig {

// Phase 18: how finely the grid is divided. This is the plan's N, and it is
// the viva value for this phase: it is the first number in the project that
// changes how much geometry exists rather than how that geometry looks.
//
//     vertices  = (CELLS + 1) * (CELLS + 1) =  81 at CELLS = 8
//     triangles = 2 * CELLS * CELLS         = 128 at CELLS = 8
//
// Changing it does NOT change the grid's size - that is SIZE below. Division
// and size are two separate ideas, and keeping them apart is the point of the
// unit-mesh rule (Phase 16). Raise this to 32 and the grid is cut into far
// more, far smaller triangles while covering exactly the same ground.
//
// In solid shading a higher CELLS looks almost identical, because the surface
// is flat; press 'W' for wireframe to see what actually changed. Phase 19 is
// the phase that makes that comparison its checkpoint.
constexpr int CELLS = 8;

// Phase 25: CELLS above is now only the division at detail level 0. '+' and '-'
// move the level, and every level's value is worked out from CELLS rather than
// from the previous value.
//
// The floor is not a matter of taste: a grid needs at least one cell to be a
// surface at all, and 2 keeps a visible cross of interior edges so the wireframe
// still shows that neighbouring cells share their corners. See DetailConfig for
// why this is a floor and not also a ceiling.
constexpr int CELLS_MIN = 2;

// Phase 18: how big the grid is, as a multiple of the 1 x 1 unit mesh, applied
// by a glm::scale at draw time like every other object's size.
constexpr float SIZE = 4.0f;

// Where the grid sits: below everything else, as a floor. The lowest point any
// other object reaches is the large cube's bottom corner at y = -1.15, so
// -1.40 keeps the grid clear of all of them with room to spare.
//
// At the default camera position the grid is seen at a shallow angle, so it
// reads as a band of floor across the bottom of the window. Drag the mouse
// upward to lift the camera and look down on it properly - that is the view
// this phase's checkpoint is about.
const glm::vec3 POSITION(0.0f, -1.40f, 0.0f);

// One colour per CORNER OF THE WHOLE GRID, in the order makeGrid walks them:
// (-x,-z), (+x,-z), (+x,+z), (-x,+z). Every vertex between them is blended
// from all four, so the grid is a smooth sheet with no visible seams - which
// is itself the proof that neighbouring cells share their corner vertices.
//
// Blues and teals, as a quiet preview of the sea this mesh becomes in Phase 40,
// and bright enough to read clearly against the golden background.
const glm::vec3 CORNER_COLORS[4] = {
    { 0.10f, 0.25f, 0.55f },   // 0: -x -z, deep blue
    { 0.10f, 0.45f, 0.70f },   // 1: +x -z, mid blue
    { 0.25f, 0.70f, 0.85f },   // 2: +x +z, pale cyan
    { 0.15f, 0.40f, 0.65f },   // 3: -x +z, blue
};

} // namespace GridConfig

namespace CylinderConfig {

// Phase 20: how many flat strips the circle is cut into. Like GridConfig::CELLS
// this changes how finely the surface is DIVIDED, not how big it is, and it is
// the viva value for this phase.
//
//     vertices  = 2 * SEGMENTS = 32 at SEGMENTS = 16
//     triangles = 2 * SEGMENTS = 32 at SEGMENTS = 16
//
// 16 looks convincingly round. Drop it to 6 and the tube is visibly a hexagonal
// prism - every triangle is flat, and a low segment count stops hiding it. That
// is exactly the setup Phase 33's Demo B uses on the cannon barrel.
constexpr int SEGMENTS = 16;

// Phase 25: the fewest segments this mesh may be built with. SEGMENTS above is
// the value at detail level 0.
//
// Three is the fewest that still enclose a volume, but 4 is used as the floor
// because it makes the point better: at 4 the "cylinder" is an obvious square
// prism, so nobody can mistake a round-looking shape for a round one. This is
// the low end of Phase 33's Demo B, where the same faceting is what makes a
// specular highlight break up on the cannon barrel.
constexpr int SEGMENTS_MIN = 4;

// Phase 20: how big it is, as a multiple of the 1 x 1 x 1 unit mesh. The unit
// tube is 1 across and 1 tall, so these are the real diameter and height, and
// the draw-time scale is (DIAMETER, HEIGHT, DIAMETER).
constexpr float DIAMETER = 0.5f;
constexpr float HEIGHT = 0.9f;

// Standing above the quad on the left of the scene, clear of everything: the
// quad's top edge is at y = +0.40 and this tube's base is at +0.50.
const glm::vec3 POSITION(-1.8f, 0.95f, 0.0f);

// A vertical gradient, bottom to top. These are VERTEX COLOURS, not lighting.
// Phase 27 added real light on top of them, so the gradient and the shading are
// now multiplied together - which is exactly what Phase 29 removes, by replacing
// the colour with a real material so that ALL the shading comes from the light.
//
// Steel grey to bright silver, as a quiet preview of the polished-silver
// fittings material (L8 slide 60), and chosen to stand out from the golden
// background rather than blend into it the way a brass colour would.
const glm::vec3 BOTTOM_COLOR(0.20f, 0.20f, 0.24f);
const glm::vec3 TOP_COLOR(0.85f, 0.86f, 0.92f);

} // namespace CylinderConfig

namespace SphereConfig {

// Phase 22: the first shape with TWO division parameters, and they are
// deliberately given DIFFERENT values. If they were both 16, a bug that mixed
// latitude up with longitude would look perfectly fine; at 12 and 18 it shows
// immediately, because the ball would come out subdivided the wrong way round.
//
//     rings     = STACKS - 1                    =   11
//     vertices  = 2 + (STACKS - 1) * SLICES     =  200
//     triangles = 2 * SLICES * (STACKS - 1)     =  396
//
// STACKS is latitude: bands from the north pole to the south.
constexpr int STACKS = 12;
// SLICES is longitude: steps around each ring.
constexpr int SLICES = 18;

// Phase 25: the fewest of each this mesh may be built with. STACKS and SLICES
// above are the values at detail level 0.
//
// Both floors are 3, the fewest either can be and still describe a solid: 2
// slices would be a flat sheet folded in half, and 2 stacks would be two cones
// base to base with no ring between them.
//
// Phase 22 chose 12 and 18 precisely because they are DIFFERENT, so that a bug
// mixing latitude up with longitude cannot hide. Every detail level has to keep
// that true, and it is the reason this phase works out each level's value from
// the number above rather than from the level before it - see DetailConfig.
constexpr int STACKS_MIN = 3;
constexpr int SLICES_MIN = 3;

// Phase 22: how big it is, as a multiple of the 1 x 1 x 1 unit mesh. A sphere is
// round on every axis, so the draw-time scale uses this one number three times -
// the only mesh in the project for which all three factors are the same. Compare
// the quad's (SIZE, SIZE, 1), the grid's (SIZE, 1, SIZE) and the cylinder's
// (DIAMETER, HEIGHT, DIAMETER).
constexpr float DIAMETER = 0.72f;

// Below the quad, completing the left-hand column: cylinder on top, quad in the
// middle, ball at the bottom. It clears the quad's lower edge (y = -0.40) by
// 0.12 and the grid floor (y = -1.40) by 0.16.
const glm::vec3 POSITION(-1.8f, -0.88f, 0.0f);

// A vertical gradient from pole to pole. VERTEX COLOURS, not lighting - the
// shading on this ball comes from Phase 27's sun and is a separate thing
// multiplied on top. Violet to lilac, picked to stand apart from the golden
// background, the blue grid and the silver cylinder.
const glm::vec3 BOTTOM_COLOR(0.18f, 0.08f, 0.30f);
const glm::vec3 TOP_COLOR(0.62f, 0.42f, 0.85f);

} // namespace SphereConfig

namespace LightConfig {

// Phase 27: the project's first light. There will be exactly TWO lights in the
// finished program and no more - this sun, and the muzzle-flash point light that
// arrives in Phase 30. Two is enough to show how contributions SUM, which is the
// lecture's point, and few enough to keep the shader readable.

// The direction the sunlight TRAVELS, not the direction towards the sun. Down and
// across, so it lights the tops and one side of everything and leaves the other
// side in shadow - which is what makes a lit side and a dark side visible.
//
// The fragment shader negates this to get L, the direction from a surface toward
// the light. Doing the negation in one place, named, means it cannot be done twice
// by accident.
const glm::vec3 SUN_DIRECTION(-0.4f, -0.35f, -0.5f);

// Slightly warm white. Sunlight is not pure (1,1,1), and a faint warmth makes the
// difference between the lit and unlit sides read as light rather than as a change
// of colour.
const glm::vec3 SUN_COLOR(1.00f, 0.96f, 0.90f);

// The light that comes from everywhere. It is the stand-in for all the light that
// has already bounced off other surfaces, which a LOCAL illumination model cannot
// compute (L8 s10-11) because it only ever looks at one surface at a time.
//
// Slightly blue, because in a real scene the sky is what fills the shadows. It is
// also why "why are there no shadows?" has a real answer rather than an excuse:
// the pipeline renders each polygon independently, with no knowledge of any other.
const glm::vec3 GLOBAL_AMBIENT(0.15f, 0.15f, 0.18f);

// ---- Phase 30: the second light, and the last one ------------------------
//
// A POINT light, which is a different kind of thing from the sun in one important
// way: it has a POSITION, so it has a distance to every surface, so its light falls
// off. The sun has only a direction and never falls off at all (L8 s19).
//
// Having exactly one of each kind is deliberate. It makes the difference between
// them something you can see rather than something you have to be told, and two
// lights is enough to show how contributions SUM - which is the lecture's point -
// while staying few enough to keep the shader readable.
//
// In the finished project this becomes the MUZZLE FLASH: it will sit at the
// cannon's muzzle and be active for about 0.15 seconds after firing (Phase 49).
// Here it sits still, just above the floor, so that its falloff can be studied in
// a single frame instead of in a fifteen-hundredth of a second.
const glm::vec3 POINT_POSITION(0.0f, -0.95f, 0.60f);

// Warm orange, because it becomes a muzzle flash. It also helps tell the two lights
// apart: the sun is faintly warm white and the ambient is faintly blue, so each of
// the three contributions is a slightly different colour and the 'L' key's
// comparison is easy to read.
const glm::vec3 POINT_COLOR(1.00f, 0.78f, 0.45f);

// Brighter than 1 on purpose. A point light that falls off has to start strong to
// be worth having at any distance, and this is the value that makes the pool of
// light on the floor clearly visible without washing anything out.
const float POINT_INTENSITY = 3.2f;

// The attenuation constants, from L8 s21:
//
//     attenuation = 1 / (a0 + a1 * d + a2 * d * d)
//
// Three terms, each doing a different job:
//
//   a0 = 1.00   the constant term. It stops the division exploding when d is near
//               zero - without it, a surface touching the light would be infinitely
//               bright - and it sets the brightness at the light itself.
//   a1 = 0.09   the linear term.
//   a2 = 0.032  the quadratic term. Real light falls off as 1/d^2 because it
//               spreads over the surface of a sphere, and this is the term that
//               models that. It dominates at long range.
//
// The other two terms are there because a purely quadratic falloff looks harsher
// than real light does - partly because real rooms are full of bounced light that
// this local model cannot compute at all (L8 s10-11), which is the same reason
// there is a global ambient term.
const float ATTENUATION_CONSTANT = 1.000f;
const float ATTENUATION_LINEAR = 0.090f;
const float ATTENUATION_QUADRATIC = 0.032f;

// Phase 30: which lights the 'L' key currently has switched on, as bit flags.
// Bit 0 is the sun, bit 1 is the point light.
constexpr int MASK_SUN = 1;
constexpr int MASK_POINT = 2;
constexpr int MASK_BOTH = MASK_SUN | MASK_POINT;

} // namespace LightConfig

// Phase 32: which terms of the illumination model are on, as bit flags. L8 slide 54.
namespace TermMask {
constexpr int AMBIENT = 1;
constexpr int DIFFUSE = 2;
constexpr int SPECULAR = 4;
constexpr int ALL = AMBIENT | DIFFUSE | SPECULAR;

// The cycle the 'K' key walks, and it is deliberately not just counting upward.
//
//   ambient            what a surface looks like with no directional light at all
//   ambient + diffuse  the shape appears
//   all three          the finished picture
//   specular ALONE     the highlight on its own, against black
//
// Ending on specular-alone is the useful part: it is the one term you cannot pick
// out by eye once it has been added to the others, so seeing it isolated is what
// makes the sum believable. Then it returns to ambient and the walk repeats.
const int CYCLE[4] = { AMBIENT, AMBIENT | DIFFUSE, ALL, SPECULAR };
constexpr int CYCLE_COUNT = 4;

inline const char* name(int mask)
{
    switch (mask) {
    case AMBIENT:           return "ambient only";
    case AMBIENT | DIFFUSE: return "ambient + diffuse";
    case ALL:               return "all three terms - the finished picture";
    case SPECULAR:          return "specular ONLY - the highlight against black";
    default:                return "custom";
    }
}
} // namespace TermMask

namespace MaterialDemoConfig {

// Phase 29: three spheres side by side, in brass, polished silver and black
// plastic. This is the report screenshot for the shininess comparison.
//
// They are the SAME sphere mesh as the one in the left-hand column, drawn three
// more times - so the whole demonstration costs three draw calls and no extra
// memory on the graphics card. That is the Phase 16 mesh-reuse rule paying for
// itself in a place where it is easy to see.
//
// What to look for, from left to right:
//
//   brass            n_s = 27.9   a broad warm highlight on a strongly coloured
//                                 surface - metal tints what it absorbs
//   polished silver  n_s = 89.6   a much TIGHTER highlight on a nearly grey
//                                 surface - silver has little colour of its own
//   black plastic    n_s = 32     almost no diffuse light at all (k_d = 0.01) and
//                                 yet an obvious shine (k_s = 0.50)
//
// The third one is the argument for materials existing. "Black but polished"
// cannot be expressed as a single colour, which is exactly why a surface needs
// four numbers rather than one.
// WHERE these sit is not a cosmetic choice, and getting it wrong broke the phase
// once. They were first placed high up at y = 1.35, where the default camera sees
// their UNDERSIDES - which face away from both the sun and the mirror direction. The
// black plastic ball rendered pure black, max luminance 0, because specular is the
// only light it reflects at all. A material demo that shows no highlight is not a
// material demo.
//
// So they sit at eye level and in front, where the sun-plus-viewer half-vector lands
// squarely on the face you are looking at.
const glm::vec3 POSITIONS[3] = {
    { -0.95f, 0.00f, 1.90f },
    {  0.00f, 0.00f, 1.90f },
    {  0.95f, 0.00f, 1.90f },
};

// Noticeably bigger than the left-hand column's ball, because the thing being
// compared is a highlight and a highlight needs room to be seen.
const float DIAMETER = 0.55f;

constexpr int COUNT = static_cast<int>(sizeof(POSITIONS) / sizeof(POSITIONS[0]));

} // namespace MaterialDemoConfig

namespace DetailConfig {

// Phase 25: ONE number decides how finely every parameterised mesh is divided.
// '+' raises it, '-' lowers it, and each mesh works out its own divisions from
// it. Level 0 is the detail every phase up to 24 shipped with.
//
// Each level DOUBLES the divisions, because detail is about whether a surface
// looks smooth and going from 8 cells to 9 is invisible while 8 to 16 is
// obvious. Each press therefore has to be worth pressing, and the whole range is
// reachable in a few presses instead of a hundred.
//
// These counts were MEASURED with the Phase 24 counters, not worked out by hand:
//
//   level | grid  cylinder   sphere  | triangles  vertices submitted
//   ------|--------------------------|-----------------------------
//     -2  |    2        4    3 x   4 |        92              127
//     -1  |    4        8    6 x   9 |       206              196
//      0  |    8       16   12 x  18 |       640              437
//     +1  |   16       32   24 x  36 |     2,348            1,339
//     +2  |   32       64   48 x  72 |     9,124            4,823
//     +3  |   64      128   96 x 144 |    36,116           18,511
//
// The draw count is 10 at every one of them. Detail changes how finely the same
// objects are divided, never how many objects there are, and that is the clearest
// thing the window title shows while '+' is pressed.
//
// THE IMPORTANT PART: a level's values are worked out from the level-0 numbers,
// never from the level before. Repeatedly halving a running value loses the
// remainder - 18 becomes 9, then 4 - so '-' twice and '+' twice would leave the
// sphere at 12 x 12 instead of 12 x 18. That is not just untidy. Phase 22 chose
// 12 and 18 BECAUSE they differ, so that a bug confusing latitude with longitude
// cannot hide; 12 x 12 would hide exactly that. Deriving each level from the
// start instead makes '-' then '+' return to precisely where it began, and keeps
// STACKS and SLICES unequal at every level.
//
// The level is clamped rather than the individual values, so the ceiling is one
// number instead of four. The per-mesh _MIN constants stay as a floor: they are
// what stops a shape being asked for fewer divisions than it needs to be a
// surface at all, and they would matter immediately if a level-0 value were
// lowered or LEVEL_MIN widened.
constexpr int LEVEL_START = 0;
constexpr int LEVEL_MIN = -2;
constexpr int LEVEL_MAX = 3;

} // namespace DetailConfig

namespace SmoothCubeConfig {

// Phase 23: a second cube, built from 8 SHARED corners and smoothed by
// computeSmoothNormals(), placed for a side-by-side comparison with the flat
// 24-vertex one.
//
// The SCALE deliberately matches CubeConfig::SCALES[1] exactly, and the position
// sits just above that cube, so the two are the same size and adjacent. The only
// difference between them is how their normals were decided - which is the whole
// point of the phase, and would be muddied by comparing two different sizes.
constexpr float SCALE = 0.48f;

// Just above flat cube 1 at (2.20, 0.10). All four cubes share one rotation, so
// two of them overlap only if their centres are closer than sqrt(3) * (h1 + h2);
// here that is 0.831 and the distance is 0.922.
const glm::vec3 POSITION(2.00f, 1.00f, 0.0f);

// Coloured by corner HEIGHT, not by face - because it cannot be coloured by face.
// A shared corner carries one colour for all three faces that meet there, exactly
// as it carries one normal. That impossibility is the Phase 10 lesson seen from
// the other side, so it is worth leaving visible rather than working around.
const glm::vec3 BOTTOM_COLOR(0.30f, 0.12f, 0.06f);
const glm::vec3 TOP_COLOR(0.95f, 0.72f, 0.45f);

} // namespace SmoothCubeConfig

namespace StretchedCubeConfig {

// Phase 26: a cube deliberately squashed and stretched by DIFFERENT amounts on
// each axis, and built from SHARED corners. Both of those matter, and the second
// one is the subtle part.
//
// THE TRAP. A non-uniform scale is not enough on its own. The flat 24-vertex
// cube's normals are axis-aligned - (1,0,0), (0,1,0) and so on - and an
// axis-aligned scale leaves such a normal pointing exactly where it started:
//
//     diag(sx,sy,sz) * (1,0,0) = (sx,0,0)   ->  normalize  ->  (1,0,0)
//
// Only one component is non-zero, so scaling changes the vector's LENGTH and not
// its DIRECTION, and normalize() throws the length away. The naive matrix and the
// normal matrix agree, and the bug is invisible. Measured: 0.000 degrees of
// difference on all 12 triangles, at every rotation.
//
// So this cube uses the Phase 23 SHARED-corner mesh instead. Its normals are the
// corner diagonals, roughly (1,1,1)/sqrt(3), which have all three components
// non-zero - and now the two matrices disagree badly:
//
//     naive       (1.30, 0.22, 0.55) * (1,1,1)  ->  mostly +X
//     (M^-1)^T    (1/1.30, 1/0.22, 1/0.55)      ->  mostly +Y
//
// Those are about 60 degrees apart. The same is true of a squashed ball or any
// other surface whose normals point in directions the scale axes do not.
//
// The general rule, worth saying out loud: the normal matrix matters when a
// normal is NOT aligned with the axes being scaled unevenly. That is why the
// cylinder, scaled (DIAMETER, HEIGHT, DIAMETER), also shows no error - its wall
// normals are (x, 0, z) and the x and z factors are equal.
const glm::vec3 SCALE(1.30f, 0.22f, 0.55f);

// Front and centre, below the cubes and above the grid floor, where it is easy to
// orbit around and look at from the side.
const glm::vec3 POSITION(0.55f, -0.95f, 0.9f);

// It shares the flat cube's rotation so it keeps presenting new faces to the
// camera; a static one would only ever prove the normal matrix on two of them.

} // namespace StretchedCubeConfig

namespace CubeConfig {

// Phase 16: HALF_SIZE used to live here, and makeCube() used to be given it.
// Both are gone. The mesh is now a UNIT cube - exactly 1 x 1 x 1, centred on
// its own origin - and a cube's real size is the SCALES entry below, applied
// by a glm::scale inside the drawMesh call. The vertex data on the GPU no
// longer knows how big any cube on screen is.
//
// Phase 14: the cube's six face colours, one per face, in the same order
// makeCube() walks its faces: +Z, -Z, +X, -X, +Y, -Y.
//
// Phase 10 wrote out all 24 vertices and all 36 indices by hand, repeating
// each face's colour on four rows. Those two arrays are gone: makeCube() in
// src/Mesh.h generates exactly the same numbers from a loop. The colours stay
// here, in main.cpp, because they are a choice about how the cube LOOKS,
// while the generator's job is only its SHAPE. That split is the same one
// materials formalise in Phase 29.
//
// Each face still gets its own 4 vertices rather than sharing the cube's 8
// real corners, for the reason Phase 10 gave: a vertex carries one colour and
// one normal, and a cube corner belongs to three faces that need different
// values for both.
const glm::vec3 FACE_COLORS[6] = {
    { 0.0f, 0.0f, 1.0f },   // +Z front  - blue
    { 1.0f, 1.0f, 0.0f },   // -Z back   - yellow
    { 1.0f, 0.0f, 0.0f },   // +X right  - red
    { 0.0f, 1.0f, 1.0f },   // -X left   - cyan
    { 0.0f, 1.0f, 0.0f },   // +Y top    - green
    { 1.0f, 0.0f, 1.0f },   // -Y bottom - magenta
};

// Phase 16: three cubes, drawn from the ONE unit mesh above.
//
// Where each cube sits. They are kept on the right of the scene, clear of the
// quad on the left and the triangles in the middle, and spread far enough
// apart that none of them touches another even at the widest point of its
// spin (a turning cube reaches 0.866 * its size from its own centre, because
// its corner is at sqrt(3)/2 of its width).
//
// To add or remove a cube, edit THIS list and SCALES below. Nothing else needs
// touching: COUNT counts this list for you, and no mesh is uploaded per cube.
const glm::vec3 POSITIONS[] = {
    {  1.10f, -0.50f, 0.0f },   // large
    {  2.20f,  0.10f, 0.0f },   // medium
    {  1.25f,  0.85f, 0.0f },   // small
};

// How many cubes exist. It is COUNTED from the list above rather than typed
// in, so the two can never disagree. Writing it by hand meant a wrong number
// here described cubes that had no position and no size.
constexpr int COUNT = static_cast<int>(sizeof(POSITIONS) / sizeof(POSITIONS[0]));

// How big each cube is, as a multiple of the unit mesh. This is the phase's
// key viva value: change one of these numbers and that one cube resizes, while
// the mesh data on the GPU is not touched at all.
//
// They are uniform scales - the same factor on x, y, and z - on purpose.
// A NON-uniform scale breaks the naive transformation of a normal and needs
// the normal matrix to correct it, which is Phase 26's lesson, so it is left
// out until the phase that can explain it.
constexpr float SCALES[] = { 0.75f, 0.48f, 0.30f };

// Every cube needs BOTH a position and a size, so the two lists must be the
// same length. The compiler checks it here rather than the program reading a
// size that was never written.
static_assert(sizeof(SCALES) / sizeof(SCALES[0]) == sizeof(POSITIONS) / sizeof(POSITIONS[0]),
              "CubeConfig: POSITIONS and SCALES must have the same number of "
              "entries - every cube needs both a place and a size.");

// How the cube turns. The axis is deliberately NOT one of X, Y, or Z alone -
// a tilted axis means every face eventually faces the camera as the cube
// spins, which is the real proof that this is a solid 3D object and not six
// flat squares that happen to be glued together.
constexpr float SPIN_SPEED = 0.6f;   // radians per second

// glm::rotate expects its axis to already be unit length; glm::normalize is
// not a compile-time function, so this cannot be constexpr like SPIN_SPEED,
// but it only ever runs once, at program startup.
const glm::vec3 SPIN_AXIS = glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f));

} // namespace CubeConfig

// Time values needed by one frame. Keeping them together makes it clear which
// time is absolute and which value describes only the previous frame.
struct FrameClock {
    float now = 0.0f;
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
};

// Phase 24: what ONE frame actually cost, counted as it is drawn.
//
// These are MEASUREMENTS, not predictions. Every draw in the project goes
// through drawMesh(), so counting there cannot drift away from what really
// happened: if a draw is added, removed, or accidentally issued twice, these
// numbers change by themselves.
//
// That is the difference between this and the line the 'W' key prints
// (Phase 19), which works the numbers out from GridConfig. A prediction tells
// you what the code INTENDS; a measurement tells you what it DID. Having both
// is how you find out they disagree.
struct RenderStats {
    int drawCalls = 0;

    // Vertices and triangles SUBMITTED this frame, which is not the same as the
    // amount stored on the graphics card: a mesh drawn three times is counted
    // three times here but uploaded once. That gap is the mesh-reuse argument,
    // and Phase 62 measures it properly.
    int vertices = 0;
    int triangles = 0;
};

// Small counters used only for the once-per-second console report.
struct FrameStats {
    float reportStartTime = 0.0f;
    int renderedFrames = 0;
};

// Phase 14: `TriangleGpu`, `QuadGpu`, and `CubeGpu` used to sit here - three
// structs of raw GLuint handles, the second and third identical to each
// other. The `Mesh` class in src/Mesh.h replaced all three. It holds the same
// handles, but it also knows how many indices to draw and deletes its own
// buffers, which a bare struct of handles could never do on its own.

// Phase 25: the first numbers in the project that decide how much geometry
// exists AND are allowed to change while the program runs.
//
// Until now every division parameter was a `constexpr int` in a config
// namespace, which means the compiler burns it into the executable and nothing
// can alter it. These four are ordinary variables, seeded from those config
// values, so the config constants become the STARTING detail rather than the
// only detail.
//
// They are grouped in their own struct rather than loose in SceneState because
// they share one job: any change to any of them means the three parameterised
// meshes have to be built again. `needsRebuild` is that message. processInput()
// sets it when a key is pressed and main() clears it once the meshes are back,
// which keeps the input code free of mesh handles and keeps the expensive work
// out of the key handler.
//
// `level` is the only one a key changes. The other four are worked out from it by
// applyDetailLevel(), so they are a cache of that arithmetic rather than four
// independent values that could drift apart.
struct MeshDetail {
    int level = DetailConfig::LEVEL_START;

    int cells = GridConfig::CELLS;
    int segments = CylinderConfig::SEGMENTS;
    int stacks = SphereConfig::STACKS;
    int slices = SphereConfig::SLICES;

    bool needsRebuild = false;
};

// Data that changes as the scene changes. updateScene() writes it and
// renderScene() reads it, so neither function needs to know about the other.
struct SceneState {
    // Phase 12: the camera's own state - a distance and two angles, turned
    // by mouse drag and scroll since Phase 13. Its default values give the
    // exact same starting view Phases 7-11 used, so nothing changes on
    // screen until the mouse is actually used.
    OrbitCamera camera;

    // glm::mat4(1.0f) is the identity matrix: it moves nothing. Writing the 1.0f
    // explicitly keeps this correct in every glm version.
    glm::mat4 triangleModel = glm::mat4(1.0f);

    // Phase 7: the camera's two matrices. Unlike triangleModel these do not
    // depend on 'now' yet, because the camera itself does not move until
    // Phase 12. They ARE rebuilt every frame, because uProjection depends on
    // the window's aspect ratio, and the window can be resized at any time.
    glm::mat4 view = glm::mat4(1.0f);

    // Phase 28: where the camera is, in world space. The specular term needs it, and
    // it has to be refreshed every frame from the orbit camera - a stale uViewPos is
    // a standard bug here, because the scene keeps rendering and only the highlight
    // silently stops moving. It is stored rather than recomputed in renderScene()
    // because updateScene() already works it out to build the view matrix.
    glm::vec3 viewPos = glm::vec3(0.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    // Phase 8: the second, reused draw of the same mesh. It is always the
    // first triangle's own model matrix, shifted farther from the camera, so
    // the two stay overlapping on screen no matter how the first one moves.
    glm::mat4 farCopyModel = glm::mat4(1.0f);

    // Phase 9: the quad's matrix. It never changes shape, only where it
    // sits, so this is really just QuadConfig::POSITION turned into a matrix.
    // It is still rebuilt every frame, for the same reason as everything
    // else here: renderScene() should only ever read scene state, never
    // calculate it.
    //
    // Phase 17: renamed from quadModel to quadFrame, because it now follows
    // the unit-mesh rule (Phase 16) like the cubes do. It holds the quad's
    // PLACE only - a translation - and no scale. The quad's size,
    // QuadConfig::SIZE, is multiplied on at draw time in renderScene().
    glm::mat4 quadFrame = glm::mat4(1.0f);

    // Phase 18: the grid's frame - a translation to GridConfig::POSITION and
    // nothing else, following the same rule as the quad's and the cubes'. Its
    // size, GridConfig::SIZE, is multiplied on at draw time.
    glm::mat4 gridFrame = glm::mat4(1.0f);

    // Phase 20: the cylinder's frame - a translation to CylinderConfig::POSITION
    // and nothing else. Its diameter and height are multiplied on at draw time.
    glm::mat4 cylinderFrame = glm::mat4(1.0f);

    // Phase 22: the sphere's frame - a translation to SphereConfig::POSITION and
    // nothing else. Its diameter is multiplied on at draw time.
    glm::mat4 sphereFrame = glm::mat4(1.0f);

    // Phase 23: the smooth cube's frame. It shares the flat cubes' rotation, so
    // the two turn in perfect step and the only difference between them is the
    // normals.
    glm::mat4 smoothCubeFrame = glm::mat4(1.0f);

    // Phase 26: the stretched cube's frame. Like every other frame it holds a
    // translation and a rotation and NO scale - the uneven scale this phase is
    // about is applied at draw time, which is exactly where the normal matrix has
    // to be computed from.
    glm::mat4 stretchedCubeFrame = glm::mat4(1.0f);

    // Phase 16: one FRAME per cube. A frame holds where the cube is and how
    // it is turned - a translation and a rotation - and deliberately NO
    // scale. The scale is multiplied on at the last possible moment, inside
    // the drawMesh call in renderScene().
    //
    // Keeping scale out of a stored matrix looks like a small detail now,
    // with three separate cubes. It becomes the single most important rule in
    // the project at Stage D, when the cannon is a child of the deck and the
    // deck is a child of the hull: a scale left in a parent's matrix is
    // inherited by every child beneath it, so a stretched hull would stretch
    // the cannon, the masts, and the crew standing on it. Storing frames
    // unscaled makes that impossible by construction.
    //
    // Like the old single cubeModel, these DO depend on 'now', because the
    // cubes spin.
    //
    // The `= {}` matters. glm::mat4's default constructor is `= default`, so
    // an array declared without it would hold whatever happened to be in
    // memory. `= {}` value-initialises every element, which for a type like
    // this fills it with zeros - not the identity, but a definite, repeatable
    // value rather than garbage. updateScene() overwrites all of them every
    // frame before renderScene() reads any, so zero is never what gets drawn.
    //
    // This deliberately does NOT name each element. An initialiser list with
    // one entry per cube would have to be kept in step with CubeConfig by hand,
    // which is exactly the kind of duplication that lets a half-finished edit
    // stop the project compiling. Written this way, any number of cubes works.
    glm::mat4 cubeFrames[CubeConfig::COUNT] = {};

    // Phase 8: toggled by the 'D' key. True matches the driver's normal
    // behaviour: the nearer fragment wins regardless of draw order. False
    // disables GL_DEPTH_TEST, so whichever triangle is drawn LAST simply
    // overwrites the other's pixels, correct or not.
    bool depthTestEnabled = true;
    bool depthKeyWasDown = false;

    // Phase 11: toggled by the 'W' key. False is the normal, solid view:
    // GL_CULL_FACE stays on, so a wrongly-wound face is silently discarded.
    // True switches to line-only rendering AND switches culling off, so
    // every triangle's outline is visible, including one that solid mode
    // would have thrown away.
    bool wireframeEnabled = false;
    bool wireframeKeyWasDown = false;

    // Phase 6: toggled by the 'O' key. False builds the correct T * R * S
    // order; true builds the same three matrices back to front, on purpose,
    // so the two can be compared live.
    bool reverseOrder = false;

    // Remembers last frame's key state so a held-down key flips the toggle
    // only once, on the frame it is first pressed, instead of roughly 120
    // times a second for as long as it is held.
    bool orderKeyWasDown = false;

    // Phase 15: toggled by the 'N' key. False is the normal picture, coloured
    // from each vertex's own colour. True makes the fragment shader ignore
    // colour and tint completely and paint each pixel from the NORMAL
    // instead, so every normal in the scene can be checked by eye.
    bool debugNormalsEnabled = false;
    bool debugNormalsKeyWasDown = false;

    // Phase 25: how finely the parameterised meshes are currently divided,
    // changed by '+' and '-'.
    MeshDetail detail;
    bool detailUpKeyWasDown = false;
    bool detailDownKeyWasDown = false;

    // Phase 26: toggled by the 'M' key. True is correct: normals are carried into
    // world space by the normal matrix (M^-1)^T. False deliberately uses
    // mat3(uModel) instead - the naive, wrong way - so the two can be compared
    // live on the stretched cube. Same idea as the 'O' key for transform order and
    // the 'D' key for depth testing: the project shows the wrong way too.
    bool normalMatrixEnabled = true;
    bool normalMatrixKeyWasDown = false;

    // Phase 30: which lights are on, cycled by the 'L' key. Both to begin with -
    // isolating a light is a thing you do to investigate, not the normal view.
    int lightMask = LightConfig::MASK_BOTH;
    bool lightKeyWasDown = false;

    // Phase 31: which shading model is in use - Flat, Gouraud or Phong - set by keys
    // 1, 2 and 3. Phong to begin with, because it is the correct one; the other two
    // exist to be compared against it.
    int shadingMode = ShadingMode::PHONG;
    bool shadingKeyWasDown[ShadingMode::COUNT] = {};

    // Phase 32: which terms of the illumination model are on, walked by the 'K' key.
    // All three to begin with - the finished picture is the normal view.
    int termCycleIndex = 2;   // TermMask::CYCLE[2] is ALL
    bool termKeyWasDown = false;

    // Phase 32: toggled by 'B'. False is Phong's (R.V)^n, true is Blinn-Phong's
    // (N.H)^n. Phong first because that is what Phase 28 built and what the specular
    // checkpoint was measured against.
    bool useBlinn = false;
    bool blinnKeyWasDown = false;
};

static void glfwErrorCallback(int errorCode, const char* description)
{
    std::fprintf(stderr, "[GLFW error %d] %s\n", errorCode, description);
}

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    // OpenGL draws into the framebuffer, whose pixel size can differ from the
    // window size on high-DPI displays. Updating the viewport prevents stretching.
    glViewport(0, 0, width, height);
}

// Phase 25: one mesh's divisions at a given detail level.
//
// Each level doubles, so the arithmetic is a shift: level +3 is "times 8" and
// level -2 is "divided by 4". Using a shift rather than a loop of multiplications
// says that directly, and a right shift rounds down exactly as integer division
// would.
//
// `startValue` is always the mesh's level-0 constant, never the value from the
// previous level. That is what makes the whole scheme reversible: level 0 gives
// the level-0 constant back exactly, however many presses it took to get there.
// constexpr so the static_asserts below can run it at compile time.
static constexpr int detailAtLevel(int startValue, int level, int minimum)
{
    const int scaled = (level >= 0) ? (startValue << level) : (startValue >> -level);
    return (scaled < minimum) ? minimum : scaled;
}

// Phase 25: the two properties the detail scheme has to have, checked by the
// compiler rather than trusted.
//
// First: level 0 must reproduce each mesh's level-0 constant exactly. If it did
// not, MeshDetail's default values and this function would disagree about what
// the program starts with, and the startup build would differ from the build you
// get by pressing '+' then '-'.
static_assert(detailAtLevel(GridConfig::CELLS, 0, GridConfig::CELLS_MIN)
                  == GridConfig::CELLS, "level 0 must give GridConfig::CELLS");
static_assert(detailAtLevel(CylinderConfig::SEGMENTS, 0, CylinderConfig::SEGMENTS_MIN)
                  == CylinderConfig::SEGMENTS, "level 0 must give CylinderConfig::SEGMENTS");
static_assert(detailAtLevel(SphereConfig::STACKS, 0, SphereConfig::STACKS_MIN)
                  == SphereConfig::STACKS, "level 0 must give SphereConfig::STACKS");
static_assert(detailAtLevel(SphereConfig::SLICES, 0, SphereConfig::SLICES_MIN)
                  == SphereConfig::SLICES, "level 0 must give SphereConfig::SLICES");

// Second: the sphere's two division counts must stay DIFFERENT at every level
// the keys can reach. Phase 22 picked 12 and 18 so that a bug swapping latitude
// for longitude could not hide behind matching numbers, and a detail scheme that
// let them become equal at some level would quietly undo that.
static constexpr bool sphereDivisionsDifferAt(int level)
{
    return detailAtLevel(SphereConfig::STACKS, level, SphereConfig::STACKS_MIN)
        != detailAtLevel(SphereConfig::SLICES, level, SphereConfig::SLICES_MIN);
}

static_assert(sphereDivisionsDifferAt(-2) && sphereDivisionsDifferAt(-1)
                  && sphereDivisionsDifferAt(0) && sphereDivisionsDifferAt(1)
                  && sphereDivisionsDifferAt(2) && sphereDivisionsDifferAt(3),
              "sphere STACKS and SLICES must differ at every reachable detail level");

// Phase 25: works out every parameterised mesh's divisions from detail.level.
// Called whenever the level changes, so the four cached values are never stale.
static void applyDetailLevel(MeshDetail& detail)
{
    detail.cells = detailAtLevel(GridConfig::CELLS, detail.level, GridConfig::CELLS_MIN);
    detail.segments = detailAtLevel(CylinderConfig::SEGMENTS, detail.level,
                                    CylinderConfig::SEGMENTS_MIN);
    detail.stacks = detailAtLevel(SphereConfig::STACKS, detail.level, SphereConfig::STACKS_MIN);
    detail.slices = detailAtLevel(SphereConfig::SLICES, detail.level, SphereConfig::SLICES_MIN);
}

// Phase 25: moves the detail level one step and asks for a rebuild, unless the
// level is already at the end of its range.
static void changeDetail(SceneState& scene, bool up)
{
    MeshDetail& detail = scene.detail;

    const int wanted = detail.level + (up ? 1 : -1);
    const int clamped = std::max(DetailConfig::LEVEL_MIN,
                                 std::min(DetailConfig::LEVEL_MAX, wanted));

    if (clamped == detail.level) {
        std::printf("[detail] already at the %s level (%d)\n",
                    up ? "highest" : "lowest", detail.level);
        return;
    }

    detail.level = clamped;
    applyDetailLevel(detail);

    // The prediction, printed from the detail values. The [mesh] lines the
    // rebuild prints next are the measurement, and the window title's triangle
    // count is the measurement for the whole frame - the same
    // prediction-against-measurement pairing Phase 24 is built on.
    std::printf("[detail] level %+d: grid CELLS %d, cylinder SEGMENTS %d, "
                "sphere STACKS %d SLICES %d\n",
                detail.level,
                detail.cells,
                detail.segments,
                detail.stacks,
                detail.slices);

    detail.needsRebuild = true;
}

static void processInput(GLFWwindow* window, SceneState& scene)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Phase 13: the camera is no longer read here at all. It is driven by
    // two GLFW callbacks in src/Camera.h instead, which GLFW calls directly
    // from glfwPollEvents() whenever the mouse actually moves or scrolls -
    // there is nothing for processInput() to poll every frame any more.

    // Phase 8: 'D' switches GL_DEPTH_TEST off and on. Same edge-detection
    // reason as 'O' below: without the "was it already down" check, holding
    // the key would flip the state roughly 120 times a second.
    const bool depthKeyIsDown = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    if (depthKeyIsDown && !scene.depthKeyWasDown) {
        scene.depthTestEnabled = !scene.depthTestEnabled;
        std::printf(
            "[depth] GL_DEPTH_TEST %s\n",
            scene.depthTestEnabled
                ? "ON (the nearer triangle wins, regardless of draw order)"
                : "OFF (whichever triangle is drawn LAST wins, correct or not)");
    }
    scene.depthKeyWasDown = depthKeyIsDown;

    // Phase 11: 'W' switches between solid and wireframe rendering, and
    // between culling on and culling off. Same edge-detection reason as
    // 'D' and 'O': without the "was it already down" check, holding the key
    // would flip the state roughly 120 times a second.
    const bool wireframeKeyIsDown = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    if (wireframeKeyIsDown && !scene.wireframeKeyWasDown) {
        scene.wireframeEnabled = !scene.wireframeEnabled;
        std::printf(
            "[wireframe] %s\n",
            scene.wireframeEnabled
                ? "ON, culling OFF (every triangle's outline is visible, front and back)"
                : "OFF, culling ON (the normal solid view)");

        // Phase 19: when the outlines come on, print the number they should add
        // up to. The point of the wireframe view on a parameterised mesh is to
        // check the count by eye against the formula, and that is much easier
        // with the arithmetic written out beside it.
        //
        // These numbers come from the detail values, not from the Mesh, so they
        // are the PREDICTION. The actual counts the mesh reported when it was
        // uploaded are what they should match. (The live per-frame counters in
        // the window title are a different thing and belong to Phase 24.)
        //
        // Phase 25: this reads scene.detail.cells, NOT GridConfig::CELLS. The
        // config constant is only the starting value now, so predicting from it
        // would keep printing 8 after '+' had already rebuilt the grid at 16 -
        // a prediction that cannot be wrong is worthless.
        if (scene.wireframeEnabled) {
            const int n = scene.detail.cells;
            std::printf(
                "            grid: CELLS = %d, so expect %d x %d = %d quads, "
                "2 x %d x %d = %d triangles, and (%d + 1)^2 = %d vertices\n",
                n, n, n, n * n, n, n, 2 * n * n, n, (n + 1) * (n + 1));
        }
    }
    scene.wireframeKeyWasDown = wireframeKeyIsDown;

    // Phase 15: 'N' switches the normals debug view on and off. Same
    // edge-detection reason as every other toggle in this function.
    const bool debugNormalsKeyIsDown = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    if (debugNormalsKeyIsDown && !scene.debugNormalsKeyWasDown) {
        scene.debugNormalsEnabled = !scene.debugNormalsEnabled;
        std::printf(
            "[normals] debug view %s\n",
            scene.debugNormalsEnabled
                ? "ON  (each pixel is painted from its normal: N * 0.5 + 0.5)"
                : "OFF (back to the vertex colours, filtered by uTint)");
    }
    scene.debugNormalsKeyWasDown = debugNormalsKeyIsDown;

    // Phase 6: 'O' compares the correct T * R * S order against the same
    // three matrices multiplied back to front. glfwGetKey reports the key as
    // PRESSED for every frame it is held down, so without the "was it already
    // down" check the order would flip roughly 120 times a second while the
    // key is held, which looks like it does nothing.
    const bool orderKeyIsDown = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
    if (orderKeyIsDown && !scene.orderKeyWasDown) {
        scene.reverseOrder = !scene.reverseOrder;
        std::printf(
            "[order] %s\n",
            scene.reverseOrder
                ? "S * R * T (reversed on purpose - watch it smear)"
                : "T * R * S (correct)");
    }
    scene.orderKeyWasDown = orderKeyIsDown;

    // Phase 26: 'M' switches the normal matrix off and on. Same edge detection as
    // every other toggle here.
    //
    // This is the clearest evidence in the phase. The correction is invisible on a
    // rotated or uniformly scaled object - (M^-1)^T and M point the same way - so
    // without a before/after on a deliberately stretched object there would be
    // nothing to show, and a grader would have to take the maths on trust.
    const bool normalMatrixKeyIsDown = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (normalMatrixKeyIsDown && !scene.normalMatrixKeyWasDown) {
        scene.normalMatrixEnabled = !scene.normalMatrixEnabled;
        std::printf(
            "[normals] transform %s\n",
            scene.normalMatrixEnabled
                ? "(M^-1)^T  - CORRECT; the stretched cube's faces read their true axis colours"
                : "mat3(uModel) - WRONG on purpose; watch the stretched cube's normals lean");
    }
    scene.normalMatrixKeyWasDown = normalMatrixKeyIsDown;

    // Phase 32: 'K' walks the term mask - L8 slide 54, live and in this scene.
    const bool termKeyIsDown = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    if (termKeyIsDown && !scene.termKeyWasDown) {
        scene.termCycleIndex = (scene.termCycleIndex + 1) % TermMask::CYCLE_COUNT;
        const int mask = TermMask::CYCLE[scene.termCycleIndex];
        std::printf("[terms] %s  (mask %d)\n", TermMask::name(mask), mask);
    }
    scene.termKeyWasDown = termKeyIsDown;

    // Phase 32: 'B' switches between Phong's (R.V)^n and Blinn-Phong's (N.H)^n.
    //
    // For the SAME exponent Blinn's highlight is broader, because the angle between
    // N and H is about half the angle between R and V. That is not a bug in either
    // one - it is why the two are not interchangeable without rescaling n.
    const bool blinnKeyIsDown = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
    if (blinnKeyIsDown && !scene.blinnKeyWasDown) {
        scene.useBlinn = !scene.useBlinn;
        std::printf("[specular] %s\n",
                    scene.useBlinn
                        ? "Blinn-Phong (N.H)^n - cheaper, and broader for the same n"
                        : "Phong (R.V)^n - what Phase 28 built");
    }
    scene.blinnKeyWasDown = blinnKeyIsDown;

    // Phase 31: keys 1, 2 and 3 choose Flat, Gouraud or Phong.
    //
    // They are separate keys rather than a cycle because a comparison wants direct
    // access: when a grader asks "now show me Gouraud", hunting round a cycle is
    // worse than pressing one key. The 'K' and 'L' cycles exist because those walk
    // through a sequence on purpose.
    for (int mode = 0; mode < ShadingMode::COUNT; ++mode) {
        const bool isDown = glfwGetKey(window, GLFW_KEY_1 + mode) == GLFW_PRESS;
        if (isDown && !scene.shadingKeyWasDown[mode] && scene.shadingMode != mode) {
            scene.shadingMode = mode;
            std::printf("[shading] %s\n", ShadingMode::name(mode));
        }
        scene.shadingKeyWasDown[mode] = isDown;
    }

    // Phase 30: 'L' cycles which lights are on - both, sun only, point only.
    //
    // This is the only way to see what a light is actually contributing once two of
    // them have been summed into one number per pixel. The eye is very bad at
    // separating two overlapping light sources, and very good at spotting what
    // changed when one is removed.
    //
    // The order is deliberate: both first, so the normal view is what you return to.
    const bool lightKeyIsDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lightKeyIsDown && !scene.lightKeyWasDown) {
        if (scene.lightMask == LightConfig::MASK_BOTH)
            scene.lightMask = LightConfig::MASK_SUN;
        else if (scene.lightMask == LightConfig::MASK_SUN)
            scene.lightMask = LightConfig::MASK_POINT;
        else
            scene.lightMask = LightConfig::MASK_BOTH;

        std::printf("[lights] %s\n",
                    scene.lightMask == LightConfig::MASK_BOTH
                        ? "BOTH - the sun and the point light, summed per fragment"
                        : (scene.lightMask == LightConfig::MASK_SUN
                               ? "SUN only - directional, no falloff anywhere"
                               : "POINT only - watch it fall off with distance"));
    }
    scene.lightKeyWasDown = lightKeyIsDown;

    // Phase 25: '+' raises the detail level and '-' lowers it, rebuilding every
    // parameterised mesh. Same edge-detection as every other key here, and it
    // matters more for these two than for any of the others: a held key would
    // rebuild three meshes on the graphics card roughly 120 times a second.
    //
    // On most keyboards '+' needs Shift, so GLFW_KEY_EQUAL - the unshifted key
    // with '+' printed on it - is accepted as well, and the numeric keypad's own
    // '+' and '-' alongside both. GLFW reports physical keys, not the characters
    // they would type, so all four have to be named explicitly.
    const bool detailUpIsDown =
        glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS;
    if (detailUpIsDown && !scene.detailUpKeyWasDown)
        changeDetail(scene, true);
    scene.detailUpKeyWasDown = detailUpIsDown;

    const bool detailDownIsDown =
        glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;
    if (detailDownIsDown && !scene.detailDownKeyWasDown)
        changeDetail(scene, false);
    scene.detailDownKeyWasDown = detailDownIsDown;
}

static void startClock(FrameClock& clock)
{
    clock.now = static_cast<float>(glfwGetTime());
    clock.lastFrameTime = clock.now;
    clock.deltaTime = 0.0f;
}

static void updateClock(FrameClock& clock)
{
    clock.now = static_cast<float>(glfwGetTime());

    const float measuredDelta = clock.now - clock.lastFrameTime;
    clock.deltaTime = std::min(measuredDelta, AppConfig::MAX_DELTA_TIME);
    clock.lastFrameTime = clock.now;
}

static void updateScene(
    SceneState& scene,
    float now,
    float deltaTime,
    int framebufferWidth,
    int framebufferHeight)
{
    // 'now' drives motion that follows a formula, like this slide.
    // Nothing is stored between frames: the position is recalculated from the
    // clock every time, so there is no table of positions to pre-compute.
    const float offsetX =
        TriangleMotion::SLIDE_DISTANCE * std::sin(TriangleMotion::SLIDE_SPEED * now);

    // Phase 7: the triangle's world-space depth. Positive moves it toward the
    // camera's starting position (nearer, so it looks bigger); negative moves
    // it away.
    const float offsetZ =
        TriangleDepth::DEPTH_AMPLITUDE * std::sin(TriangleDepth::DEPTH_SPEED * now);

    // glm::translate(matrix, vector) returns 'matrix' with a move of 'vector'
    // added. Starting from the identity matrix gives a pure translation.
    const glm::mat4 slide =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, offsetX, offsetZ));

    // Phase 5: glm::rotate(matrix, angle, axis) works the same way, but adds a
    // turn. The angle is in radians and comes from the clock, like the slide.
    const float spinAngle = TriangleSpin::SPIN_SPEED * now;
    const glm::mat4 spin =
        glm::rotate(glm::mat4(1.0f), spinAngle, TriangleSpin::SPIN_AXIS);

    // Phase 6: glm::scale(matrix, vector) works the same way again, but
    // resizes instead of moving or turning. A pulsing factor between
    // PULSE_MIN and PULSE_MAX, applied equally on x and y, keeps the
    // triangle's proportions correct while it grows and shrinks.
    // The 0.5f here is a MIDPOINT: halfway between PULSE_MIN and PULSE_MAX.
    // It read 0.1f from Phase 12 until Phase 19, which made the midpoint 0.18
    // instead of 0.9, so the factor swung from -0.22 to +0.58 - the triangle
    // was under half its intended size, collapsed to a point twice per period,
    // and came back inside out. See the Stage A review, finding F1.
    const float scalePulseMid =
        (TriangleScale::PULSE_MIN + TriangleScale::PULSE_MAX) * 0.5f;
    const float scalePulseAmp =
        (TriangleScale::PULSE_MAX - TriangleScale::PULSE_MIN) * 0.5f;
    const float scaleFactor =
        scalePulseMid + scalePulseAmp * std::sin(TriangleScale::PULSE_SPEED * now);
    const glm::mat4 scaleMat =
        glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor, scaleFactor, 1.0f));

    // Join all three by multiplying. Read the product from RIGHT to LEFT,
    // because the vertex meets the rightmost matrix first. The correct order
    // is T * R * S:
    //   1. scale - resize around the origin, where the corners sit;
    //   2. spin  - turn the resized triangle around the origin;
    //   3. slide - carry the turned, resized triangle to its place on screen.
    // Each step only ever acts on the origin-centred result of the step
    // before it, so the triangle grows and shrinks on the spot, spins on the
    // spot, and both of those together slide as one rigid trip.
    //
    // 'O' rebuilds the same three matrices back to front: S * R * T. That
    // order slides first, so a factor meant to resize the shape instead
    // stretches how FAR it slides, and a spin meant to turn the shape instead
    // swings the whole slid-out trip around the origin. The shape looks like
    // it smears through a wide, pulsing loop instead of pulsing and spinning
    // on the spot. Nothing here is broken; only the sequence of operations,
    // read right to left, is different.
    scene.triangleModel = scene.reverseOrder
        ? scaleMat * spin * slide    // S * R * T, deliberately backwards
        : slide * spin * scaleMat;   // T * R * S, correct

    // Phase 8: build the far copy by adding ONE more translation on the LEFT
    // of the already-finished near-copy matrix. Left means "done last", so
    // this shifts the whole placed-turned-scaled triangle straight along
    // world z, without touching its shape, rotation, or size at all - the
    // same "translate the finished result" idea Phase 4 introduced, just
    // applied to a matrix instead of a raw vertex.
    const glm::mat4 farCopyShift = glm::translate(
        glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, DepthTestConfig::FAR_COPY_Z_OFFSET));
    scene.farCopyModel = farCopyShift * scene.triangleModel;

    // Phase 9: the quad's matrix is a single, unmoving translation. Phase 17:
    // that is all it is - no glm::scale here, for the same reason as the
    // cubes' frames. QuadConfig::SIZE is applied in renderScene().
    scene.quadFrame = glm::translate(glm::mat4(1.0f), QuadConfig::POSITION);

    // Phase 18: the grid does not move either. Like the quad's, its frame is a
    // single translation, with no glm::scale in it.
    scene.gridFrame = glm::translate(glm::mat4(1.0f), GridConfig::POSITION);

    // Phase 20: the cylinder does not move either - another plain translation,
    // with no glm::scale stored in it.
    scene.cylinderFrame = glm::translate(glm::mat4(1.0f), CylinderConfig::POSITION);

    // Phase 22: the sphere does not move either.
    scene.sphereFrame = glm::translate(glm::mat4(1.0f), SphereConfig::POSITION);

    // Phase 10: each cube's matrix is T * R, the same pattern Phase 5
    // introduced: spin the cube around its own centre first (the origin,
    // where every one of its 24 vertices is measured from), THEN carry the
    // already-turning cube out to its resting place. Doing it the other way
    // round would make the cube orbit its position instead of spinning on the
    // spot - exactly Phase 5's wobble lesson, at a larger scale.
    //
    // Phase 16: the spin is worked out ONCE and shared by all three cubes, so
    // they turn in perfect step. That is deliberate: it leaves the scale as
    // the only thing that differs between them besides where they stand, which
    // is exactly what this phase is trying to show.
    const float cubeSpinAngle = CubeConfig::SPIN_SPEED * now;
    const glm::mat4 cubeSpin =
        glm::rotate(glm::mat4(1.0f), cubeSpinAngle, CubeConfig::SPIN_AXIS);

    // Phase 16: note what is NOT here - there is no glm::scale in this loop.
    // A frame is a place and a rotation, nothing more. CubeConfig::SCALES is
    // never read in updateScene() at all; it is read once, in renderScene(),
    // at the moment of drawing.
    for (int i = 0; i < CubeConfig::COUNT; ++i) {
        const glm::mat4 cubeSlide =
            glm::translate(glm::mat4(1.0f), CubeConfig::POSITIONS[i]);
        scene.cubeFrames[i] = cubeSlide * cubeSpin;
    }

    // Phase 23: the smooth cube uses the SAME cubeSpin as the flat ones. Two
    // cubes turning identically, differing only in how their normals were
    // decided, is what makes the comparison fair.
    scene.smoothCubeFrame =
        glm::translate(glm::mat4(1.0f), SmoothCubeConfig::POSITION) * cubeSpin;

    // Phase 26: the stretched cube shares cubeSpin too, so it keeps turning new
    // faces toward the camera. A static cube would only ever demonstrate the normal
    // matrix on the two or three faces that happened to be visible.
    scene.stretchedCubeFrame =
        glm::translate(glm::mat4(1.0f), StretchedCubeConfig::POSITION) * cubeSpin;

    // Phase 7: glm::lookAt(eye, target, up) builds the view matrix from three
    // vectors instead of a translate/rotate/scale recipe. It re-measures every
    // WORLD position as seen from the camera, so the camera can stay at the
    // origin of its own space while everything else moves around it.
    const glm::vec3 eye = orbitCameraPosition(scene.camera);
    scene.view = glm::lookAt(eye, CameraConfig::TARGET, CameraConfig::UP);

    // Phase 28: the same eye the view matrix was built from, kept for the specular
    // term. Taking it from the identical variable - rather than working it out again
    // in renderScene() - makes it impossible for the lighting and the camera to
    // disagree about where the viewer is.
    scene.viewPos = eye;

    // The projection depends on the window's shape, not the clock, so it is
    // rebuilt from the CURRENT framebuffer size every frame. A minimised
    // window can report a height of 0, and dividing by that would be
    // undefined, so a height of at least 1 is always used for the aspect
    // ratio.
    const int safeHeight = std::max(framebufferHeight, 1);
    const float aspectRatio =
        static_cast<float>(framebufferWidth) / static_cast<float>(safeHeight);
    scene.projection = glm::perspective(
        glm::radians(CameraConfig::FIELD_OF_VIEW_DEGREES),
        aspectRatio,
        CameraConfig::NEAR_PLANE,
        CameraConfig::FAR_PLANE);

    // deltaTime is for input-driven motion such as steering (a later phase).
    // This cast tells the compiler that leaving it unused is intentional.
    static_cast<void>(deltaTime);
}

// Phase 14: one function replaces createTriangle/destroyTriangle,
// createQuad/destroyQuad, and createCube/destroyCube - six functions and
// about 210 lines of near-identical glGen/glBind/glBufferData/
// glVertexAttribPointer calls. All of that work now happens once, inside
// Mesh::upload(), and each Mesh frees itself in Mesh::destroy(), so the
// three destroy functions have no work left to do at all.
//
// Only the triangle still carries its vertices written out by hand, exactly as
// Phase 2 wrote them. The cube (Phase 14), the quad (Phase 17), and the grid
// (Phase 18) are GENERATED; makeCylinder and makeSphere arrive in Phases 20-22.
// Phase 25: builds the three meshes whose geometry depends on a detail value,
// and ONLY those three. The triangle, quad, cube and smooth cube are always the
// same handful of vertices, so there would be nothing to rebuild.
//
// This is safe to call over and over. Mesh::upload() calls destroy() before it
// uploads anything, so the previous VAO, VBO and EBO are handed back to the
// driver first and the three handles are replaced rather than added to. That one
// line in src/Mesh.h is the whole reason this phase does not leak: without it,
// every press of '+' would abandon three GPU buffers that nothing could ever
// free again.
//
// createMeshes() calls this for the startup build too, so the upload arguments
// for these three meshes are written in exactly one place. A second copy inside
// createMeshes() would be free to drift out of step with this one.
static bool rebuildMeshes(Mesh& gridMesh,
                          Mesh& cylinderMesh,
                          Mesh& sphereMesh,
                          const MeshDetail& detail)
{
    // Phase 18: the grid is the first mesh whose SIZE in memory depends on a
    // number we choose. Everything about it - 81 vertices and 128 triangles at
    // CELLS = 8 - comes out of that one value.
    if (!makeGrid(gridMesh, detail.cells, GridConfig::CORNER_COLORS))
        return false;

    // Phase 20: the first curved surface. Its ring of vertices comes from
    // sinf/cosf, and every one of them carries its own analytic normal.
    if (!makeCylinder(cylinderMesh,
                      detail.segments,
                      CylinderConfig::BOTTOM_COLOR,
                      CylinderConfig::TOP_COLOR))
        return false;

    // Phase 22: the first TWO-parameter shape. Its normal is the simplest in the
    // project - for a ball centred on its own origin, normalize(position) IS the
    // surface normal.
    return makeSphere(sphereMesh,
                      detail.stacks,
                      detail.slices,
                      SphereConfig::BOTTOM_COLOR,
                      SphereConfig::TOP_COLOR);
}

static bool createMeshes(Mesh& triangleMesh,
                         Mesh& quadMesh,
                         Mesh& gridMesh,
                         Mesh& cylinderMesh,
                         Mesh& sphereMesh,
                         Mesh& cubeMesh,
                         Mesh& smoothCubeMesh,
                         const MeshDetail& detail)
{
    // Mesh::upload takes std::vector, and the triangle's config arrays are
    // fixed-size C arrays, so each one is copied into a vector by naming its
    // first element and one-past-its-last. The copy happens once, at startup.
    const std::vector<Vertex> triangleVertices(
        TriangleConfig::VERTICES,
        TriangleConfig::VERTICES + TriangleConfig::VERTEX_COUNT);
    const std::vector<unsigned int> triangleIndices(
        TriangleConfig::INDICES,
        TriangleConfig::INDICES + TriangleConfig::INDEX_COUNT);

    if (!triangleMesh.upload("triangle", triangleVertices, triangleIndices))
        return false;

    // Phase 17: the quad's 4 vertices and 6 indices are built in src/Mesh.h,
    // as a 1 x 1 unit quad facing +Z, instead of being typed out here.
    if (!makeQuad(quadMesh, QuadConfig::CORNER_COLORS))
        return false;

    // Phase 25: the three parameterised meshes are built by the same function
    // that '+' and '-' call later, so startup and every rebuild go down one path.
    if (!rebuildMeshes(gridMesh, cylinderMesh, sphereMesh, detail))
        return false;

    // The cube's 24 vertices and 36 indices are built by a loop in
    // src/Mesh.h instead of typed out here. makeCube() produces the same
    // numbers, in the same order, that Phase 10 wrote by hand.
    if (!makeCube(cubeMesh, CubeConfig::FACE_COLORS))
        return false;

    // Phase 23: the same cube shape from 8 shared corners instead of 24 separate
    // ones, with its normals AVERAGED rather than written down. Same 12 triangles,
    // a third of the vertices, and a completely different look in the 'N' view.
    return makeSmoothCube(smoothCubeMesh,
                          SmoothCubeConfig::BOTTOM_COLOR,
                          SmoothCubeConfig::TOP_COLOR);
}

// Phase 16: draws one mesh, with one model matrix and one tint. Everything a
// single object needs is now in one call, and the per-object uniforms live in
// exactly one place instead of being repeated above every draw.
//
// The `model` argument is where the unit-mesh rule is actually enforced. A
// caller that wants a cube three units wide does not ask for a bigger mesh; it
// hands in a matrix with a glm::scale in it:
//
//     drawMesh(shader, stats, cubeMesh, frame * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f)), tint);
//
// Note that the scale sits on the RIGHT of the frame, so (reading right to
// left, Phase 6) the mesh is resized FIRST, about its own origin, and only
// then placed and turned by the frame. A scale written on the left would
// resize the finished placement instead, stretching how far the object sits
// from the origin - Phase 6's smear, in three dimensions.
//
// This signature grows into the reference project's drawMesh(mesh, model,
// material) once Phase 29 replaces the tint with a real material.
static void drawMesh(ShaderProgram& shader,
                     RenderStats& stats,
                     const Mesh& mesh,
                     const glm::mat4& model,
                     const Material& material)
{
    // Phase 29: the material, set per object. This is the one place that knows which
    // object is being drawn, so it is the only place that can know its material.
    shader.setVec3("uKa", material.ka);
    shader.setVec3("uKd", material.kd);
    shader.setVec3("uKs", material.ks);
    shader.setFloat("uShininess", material.ns);
    shader.setMat4("uModel", model);

    // Phase 26: the normal matrix, (M^-1)^T, computed HERE because this is the one
    // place that knows an object's final model matrix - scale included. It is done
    // on the CPU, once per object per frame, rather than in the shader: inverting a
    // matrix per VERTEX would repeat identical work thousands of times for one
    // object, and GLSL's inverse() is not available in every 3.3 profile anyway.
    //
    // glm::mat3(...) takes the top-left 3x3 AFTER the inverse and transpose, which
    // is the right order: the translation has to be inverted along with everything
    // else before it is discarded.
    shader.setMat3("uNormalMatrix",
                   glm::mat3(glm::transpose(glm::inverse(model))));
    mesh.draw();

    // Phase 24: counted HERE, at the one place every draw in the project passes
    // through, so the totals cannot get out of step with what was drawn.
    ++stats.drawCalls;
    stats.vertices += mesh.vertexCount();
    stats.triangles += mesh.triangleCount();
}

// The shader is no longer passed as const: setting a uniform changes the
// shader program, so this function can no longer promise to leave it alone.
// Phase 24: this now RETURNS what it drew. The counters are a local, built up
// from zero every frame, so a stale total from a previous frame is impossible.
static RenderStats renderScene(
    ShaderProgram& shader,
    const Mesh& triangleMesh,
    const Mesh& quadMesh,
    const Mesh& gridMesh,
    const Mesh& cylinderMesh,
    const Mesh& sphereMesh,
    const Mesh& cubeMesh,
    const Mesh& smoothCubeMesh,
    const SceneState& scene)
{
    glClearColor(
        AppConfig::CLEAR_COLOR.r,
        AppConfig::CLEAR_COLOR.g,
        AppConfig::CLEAR_COLOR.b,
        1.0f);

    // Clear both buffers every frame. The colour buffer holds visible pixels;
    // the depth buffer will decide which 3D surfaces are closest in later phases.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use() first: a uniform is written into whichever program is currently
    // in use, so setting it before use() would send the value nowhere.
    shader.use();

    // Phase 7: the camera matrices are the same for every object drawn this
    // frame, so they are uploaded once, before either draw call.
    shader.setMat4("uView", scene.view);
    shader.setMat4("uProjection", scene.projection);

    // Phase 15: the debug view applies to the whole scene, not to one object,
    // so like the camera matrices it is uploaded once per frame rather than
    // once per draw. setInt() is used here for the first time; it has existed,
    // unused, since Phase 3.
    shader.setInt("uDebugNormals", scene.debugNormalsEnabled ? 1 : 0);

    // Phase 26: which matrix the vertex shader uses on the normal. Set once per
    // frame, not per object, because it is a comparison the viewer chooses rather
    // than a property of any one object.
    shader.setInt("uUseNormalMatrix", scene.normalMatrixEnabled ? 1 : 0);

    // Phase 27: the lighting uniforms. Set once per FRAME, not once per object,
    // because neither the sun nor the global ambient belongs to any one object -
    // this is the "hoist uniforms out of the per-object loop" point Phase 62 measures.
    shader.setVec3("uGlobalAmbient", LightConfig::GLOBAL_AMBIENT);
    shader.setVec3("uSunDirection", LightConfig::SUN_DIRECTION);
    shader.setVec3("uSunColor", LightConfig::SUN_COLOR);

    // Phase 30: the second light. It has a POSITION, which is the whole difference
    // between it and the sun - a position means a distance, and a distance means the
    // light falls off.
    shader.setVec3("uPointPosition", LightConfig::POINT_POSITION);
    shader.setVec3("uPointColor", LightConfig::POINT_COLOR);
    shader.setFloat("uPointIntensity", LightConfig::POINT_INTENSITY);
    shader.setVec3("uAttenuation", glm::vec3(LightConfig::ATTENUATION_CONSTANT,
                                             LightConfig::ATTENUATION_LINEAR,
                                             LightConfig::ATTENUATION_QUADRATIC));
    shader.setInt("uLightMask", scene.lightMask);

    // Phase 31: one integer decides which of the three shading models runs. Switching
    // mode costs exactly this upload - no program change, no pipeline flush.
    shader.setInt("uShadingMode", scene.shadingMode);

    // Phase 32: the term mask and the specular formula.
    shader.setInt("uTermMask", TermMask::CYCLE[scene.termCycleIndex]);
    shader.setInt("uUseBlinn", scene.useBlinn ? 1 : 0);

    // Phase 29: uKa, uKd, uKs and uShininess are no longer set here. They moved into
    // drawMesh(), because a material belongs to an OBJECT the way a model matrix does.
    // What stays here is only what belongs to the whole frame: the two lights and the
    // camera position.

    // Phase 28: the camera's world position, refreshed from updateScene() every
    // frame. If this ever stops being sent the highlight freezes in place.
    shader.setVec3("uViewPos", scene.viewPos);

    // Phase 8: this ONE line decides whether depth is honoured at all. It is
    // set fresh every frame from the 'D' key's state, rather than relying on
    // whatever main() enabled once at startup, so the effect is visible the
    // instant the key is pressed.
    if (scene.depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    // Phase 11: 'W' controls TWO pieces of GL state together, for one reason.
    // glPolygonMode alone would not be enough: a triangle that GL_CULL_FACE
    // discards for facing the wrong way is thrown away BEFORE the polygon
    // mode ever gets a chance to draw its outline. Switching culling off at
    // the same moment as switching to line mode is what lets a culled
    // face's edges actually appear.
    if (scene.wireframeEnabled) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Phase 16: every draw below is now a single drawMesh() call. Phase 14
    // moved "which VAO, which draw call, how many indices" into the mesh;
    // this phase moves "which matrix, which material" into one function too, so a
    // draw reads as one line that says what is different about this object.
    //
    // Phase 29: the tint argument became a Material. Every draw below now names a
    // material from src/Material.h, and several of them are previews of the finished
    // project: the grid is OCEAN because it becomes the sea, the ball is
    // BLACK_PLASTIC because it becomes the cannonball, the tube is BRASS because it
    // becomes the barrel.

    // Phase 24: zeroed every frame, filled in by drawMesh, returned below.
    RenderStats stats;

    // Draw 1: the grid, drawn BEFORE everything else, on purpose.
    //
    // With depth testing on, draw order makes no difference (Phase 8), so this
    // choice only matters when 'D' switches it off. The grid is by far the
    // largest thing on screen; drawn last with depth off it would simply paint
    // over the two triangles and wreck Phase 8's demonstration. Drawn first,
    // the triangles still land on top of it and that demonstration is
    // untouched. An earlier phase's evidence must keep working.
    //
    // The y factor of the scale is 1, not SIZE: the grid is flat in y, exactly
    // as the quad is flat in z, so there is no depth to scale.
    const glm::mat4 gridScale = glm::scale(
        glm::mat4(1.0f), glm::vec3(GridConfig::SIZE, 1.0f, GridConfig::SIZE));
    drawMesh(shader, stats, gridMesh, scene.gridFrame * gridScale, OCEAN);

    // Draw 2: the near copy of the triangle. Drawn BEFORE the far copy below,
    // which is what Phase 8's demonstration depends on.
    drawMesh(shader, stats, triangleMesh, scene.triangleModel, BRASS);

    // Draw 3: the SAME mesh again, at DepthTestConfig::FAR_COPY_Z_OFFSET
    // farther away, drawn AFTER the near copy. Nothing here is duplicated except the draw
    // call itself: same mesh, same vertex data, just a different uModel and
    // uTint. Drawing the farther copy AFTER the nearer one is deliberate -
    // with depth testing off, its "wrong" pixels are the ones that end up on
    // screen, which is exactly what makes GL_DEPTH_TEST worth having.
    drawMesh(shader, stats, triangleMesh, scene.farCopyModel, SAILCLOTH);

    // Draw 4: the quad.
    //
    // Phase 29: SAILCLOTH, the matt end of the material table - n_s = 4, almost no
    // specular at all. A flat sheet of cloth is the right thing for the shape, and it
    // is the low end of the 40x shininess spread the ocean sits at the top of.
    //
    // Its four corner colours are still in the mesh data and are no longer drawn. The
    // material decides its appearance now.
    //
    // Phase 17: the quad is a unit mesh now, so its size is applied here, at
    // draw time, exactly as the cubes' sizes are. The z factor is 1, not
    // SIZE: a quad has no depth to scale, and 1 leaves it flat.
    const glm::mat4 quadScale = glm::scale(
        glm::mat4(1.0f), glm::vec3(QuadConfig::SIZE, QuadConfig::SIZE, 1.0f));
    drawMesh(shader, stats, quadMesh, scene.quadFrame * quadScale, SAILCLOTH);

    // Draws 5, 6, and 7: THREE cubes, from ONE mesh and ONE VAO.
    //
    // This loop is the whole of Phase 16. `cubeMesh` is uploaded once, at
    // startup, and holds a single 1 x 1 x 1 cube. Each pass through this loop
    // multiplies that cube's unscaled frame by a different glm::scale and
    // hands the result to drawMesh. Nothing about the mesh changes between
    // passes - no re-upload, no second VAO, not one byte of vertex data
    // touched - and three cubes of three different sizes appear.
    //
    // Read the product right to left, as always (Phase 6):
    //   1. glm::scale - resize the unit cube about its own origin;
    //   2. the frame   - spin it, then carry it to its place.
    //
    // The scale is applied HERE, in the draw call, and nowhere else. It is
    // never stored back into scene.cubeFrames, so it can never be inherited by
    // anything - which is the habit Stage D's ship hierarchy depends on
    // completely.
    //
    // Phase 29: HULL_WOOD for all three, which is what they become in Stage D. Their
    // 24 per-face colours are still in the mesh and are no longer drawn - press N to
    // see that the six faces are all still distinct where it matters, in the normals.
    for (int i = 0; i < CubeConfig::COUNT; ++i) {
        const glm::mat4 cubeScale = glm::scale(
            glm::mat4(1.0f), glm::vec3(CubeConfig::SCALES[i]));
        drawMesh(shader, stats, cubeMesh, scene.cubeFrames[i] * cubeScale, HULL_WOOD);
    }

    // Draw 8: the cylinder - side wall and both caps, one mesh, one draw call.
    //
    // The scale is (DIAMETER, HEIGHT, DIAMETER): the unit tube is 1 across and
    // 1 tall, so the two across-axes take the diameter and the up-axis takes the
    // height. Compare the quad's (SIZE, SIZE, 1) and the grid's (SIZE, 1, SIZE):
    // in each case the mesh's own flat or round directions decide which of the
    // three factors is which, and getting it wrong makes a tube into a disc.
    //
    // Drawn last, which is safe here: it stands alone on the left and overlaps
    // nothing else on screen, so even with depth testing off (the 'D' key) there
    // is nothing for it to paint over. The grid is the draw whose order really
    // matters - see Draw 1.
    const glm::mat4 cylinderScale = glm::scale(
        glm::mat4(1.0f),
        glm::vec3(CylinderConfig::DIAMETER, CylinderConfig::HEIGHT, CylinderConfig::DIAMETER));
    drawMesh(shader, stats, cylinderMesh, scene.cylinderFrame * cylinderScale, BRASS);

    // Draw 9: the sphere.
    //
    // The only scale in the project where all three factors are the same, because
    // a ball is round on every axis. Each mesh's own shape decides which factor
    // goes where, and this is the simplest case of that rule.
    const glm::mat4 sphereScale = glm::scale(
        glm::mat4(1.0f), glm::vec3(SphereConfig::DIAMETER));
    drawMesh(shader, stats, sphereMesh, scene.sphereFrame * sphereScale, BLACK_PLASTIC);

    // Draw 10: the smooth cube, the same size as flat cube 1 and just above it.
    //
    // Press 'N' and compare the two directly: the flat cube shows six hard,
    // unchanging face colours, and this one shows each face blending between its
    // four corners. Identical geometry, identical rotation, identical scale - the
    // only difference is whether each face got its own vertices.
    const glm::mat4 smoothCubeScale = glm::scale(
        glm::mat4(1.0f), glm::vec3(SmoothCubeConfig::SCALE));
    drawMesh(shader, stats, smoothCubeMesh, scene.smoothCubeFrame * smoothCubeScale, POLISHED_SILVER);

    // Draw 11: Phase 26's stretched cube - the object the normal matrix is for.
    //
    // It uses smoothCubeMesh, NOT cubeMesh, and that is the whole trick. The flat
    // cube's normals are axis-aligned, so an axis-aligned scale cannot turn them
    // (see StretchedCubeConfig) - it would show nothing. The shared-corner cube's
    // normals are corner diagonals, so they go badly wrong without the correction.
    //
    // Press 'N' to see the normals, then 'M' to switch the correction off. This
    // cube's faces swing by about 60 degrees; nothing else in the scene moves at
    // all, which is itself the lesson about when this bug can bite.
    const glm::mat4 stretchedCubeScale = glm::scale(
        glm::mat4(1.0f), StretchedCubeConfig::SCALE);
    drawMesh(shader, stats, smoothCubeMesh,
             scene.stretchedCubeFrame * stretchedCubeScale, POLISHED_SILVER);

    // Draws 12, 13 and 14: Phase 29's material comparison - the report screenshot.
    //
    // Three spheres, one mesh, three materials. Nothing else differs between them:
    // same geometry, same size, same position in a row, same light. Every visible
    // difference between them comes from four numbers.
    //
    // Brass has a broad warm highlight; polished silver's is far tighter at n_s 89.6
    // against brass's 27.9; and black plastic is almost pure black and obviously
    // shiny at the same time, which is the one thing a single colour could never say.
    const glm::mat4 demoScale = glm::scale(
        glm::mat4(1.0f), glm::vec3(MaterialDemoConfig::DIAMETER));
    const Material* demoMaterials[MaterialDemoConfig::COUNT] = {
        &BRASS, &POLISHED_SILVER, &BLACK_PLASTIC
    };
    for (int i = 0; i < MaterialDemoConfig::COUNT; ++i) {
        const glm::mat4 frame =
            glm::translate(glm::mat4(1.0f), MaterialDemoConfig::POSITIONS[i]);
        drawMesh(shader, stats, sphereMesh, frame * demoScale, *demoMaterials[i]);
    }

    return stats;
}

// Phase 24: put the frame's measured cost in the window title.
//
// The title bar is a free HUD: no font, no texture, no extra geometry, no second
// shader - and it shows up in a screen recording, which a console window does
// not. The whole mechanism is snprintf into a buffer and one GLFW call.
//
// It is only rewritten when a number actually CHANGES. glfwSetWindowTitle goes
// through to the operating system, and asking Windows to re-set the same string
// 120 times a second is pure waste. In this scene the geometry is fixed, so the
// title is written once and then left alone; Phase 25's '+' and '-' keys are what
// will make it visibly move.
static void updateWindowTitle(GLFWwindow* window,
                              const RenderStats& stats,
                              RenderStats& lastShown)
{
    if (stats.drawCalls == lastShown.drawCalls &&
        stats.vertices == lastShown.vertices &&
        stats.triangles == lastShown.triangles) {
        return;
    }

    char title[256];
    std::snprintf(title, sizeof(title),
                  "%s | draws %d | tris %d | verts %d",
                  AppConfig::WINDOW_TITLE, stats.drawCalls, stats.triangles, stats.vertices);
    glfwSetWindowTitle(window, title);

    // Also print it once, so the numbers are in the console log of a recording
    // even if the title bar is cropped out of shot.
    std::printf("[counters] draw calls %d, triangles %d, vertices submitted %d\n",
                stats.drawCalls, stats.triangles, stats.vertices);

    lastShown = stats;
}

static void reportFrame(FrameStats& stats, const FrameClock& clock)
{
    ++stats.renderedFrames;

    const float reportDuration = clock.now - stats.reportStartTime;
    if (reportDuration < AppConfig::REPORT_INTERVAL)
        return;

    const float averageFps = static_cast<float>(stats.renderedFrames) / reportDuration;
    const float averageDelta = reportDuration / static_cast<float>(stats.renderedFrames);

    std::printf(
        "[frame] time=%7.2f s  dt=%7.4f s  average dt=%7.4f s  fps=%6.1f\n",
        clock.now,
        clock.deltaTime,
        averageDelta,
        averageFps);
    std::fflush(stdout);

    stats.reportStartTime = clock.now;
    stats.renderedFrames = 0;
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (glfwInit() != GLFW_TRUE) {
        std::fprintf(stderr, "Failed to initialize GLFW.\n");
        return 1;
    }

    // Ask the driver for modern OpenGL 3.3 Core. Later phases will use GLSL 330.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        AppConfig::WINDOW_WIDTH,
        AppConfig::WINDOW_HEIGHT,
        AppConfig::WINDOW_TITLE,
        nullptr,
        nullptr);

    if (window == nullptr) {
        std::fprintf(stderr, "Failed to create the OpenGL window.\n");
        glfwTerminate();
        return 1;
    }

    // OpenGL functions belong to a context, so the context must be current first.
    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        std::fprintf(stderr, "Failed to load OpenGL functions with GLAD.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    if (GLAD_GL_VERSION_3_3 == 0) {
        std::fprintf(stderr, "OpenGL 3.3 is not available on this computer.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(AppConfig::VSYNC_INTERVAL);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // Both of these are only the startup defaults now. GL_DEPTH_TEST has been
    // dynamic since Phase 8, and GL_CULL_FACE joins it in Phase 11:
    // renderScene() sets both fresh every frame from the 'D' and 'W' keys'
    // state, so these two lines matter only for the very first frame, before
    // either key has been read.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    std::printf("OpenGL   : %s\n", glGetString(GL_VERSION));
    std::printf("GLSL     : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::printf("Renderer : %s\n", glGetString(GL_RENDERER));
    ShaderProgram shader;
    // Phase 31: the third argument is the shared lighting code from src/Lighting.h,
    // spliced into BOTH stages. Gouraud runs it per vertex and Phong per fragment, and
    // the comparison between them is only honest because it is literally the same
    // text in both.
    if (!shader.loadFromFiles(
            AppConfig::VERTEX_SHADER_PATH,
            AppConfig::FRAGMENT_SHADER_PATH,
            sharedLightingSource())) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Phase 25: declared before the meshes now, because the detail values it
    // holds decide how much geometry the first build produces. Its mouse
    // callbacks are still wired up further down, once the meshes are known to
    // have worked.
    SceneState scene;

    // Phase 14: Mesh objects replace structs of raw handles and create/destroy
    // pairs. Each one owns its own VAO, VBO, and EBO. They are declared here,
    // before the loop, so they live for as long as the window does.
    Mesh triangleMesh;
    Mesh quadMesh;
    Mesh gridMesh;
    Mesh cylinderMesh;
    Mesh sphereMesh;
    Mesh cubeMesh;
    Mesh smoothCubeMesh;

    if (!createMeshes(triangleMesh, quadMesh, gridMesh, cylinderMesh, sphereMesh,
                      cubeMesh, smoothCubeMesh, scene.detail)) {
        std::fprintf(stderr, "Failed to create the scene meshes.\n");
        smoothCubeMesh.destroy();
        cubeMesh.destroy();
        sphereMesh.destroy();
        cylinderMesh.destroy();
        gridMesh.destroy();
        quadMesh.destroy();
        triangleMesh.destroy();
        shader.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("Phase 33 ready - STAGE C COMPLETE. No new feature; this phase is the two demonstrations.\n");
    std::printf("  DEMO A (L9 s28): look at the sea. Press 2 then 3. Gouraud's sun streak peaks at 186 and Phong's at 255, on identical geometry. Press + twice and Gouraud catches up - the failure was coarseness.\n");
    std::printf("  DEMO B (L9 s27): press - once for an 8-segment tube, then 1, 2, 3. Flat uses 42 brightness levels where Phong uses 216: that is the faceting and the Mach banding.\n");
    std::printf("  Also: K walks L8 s54, B swaps Blinn-Phong, L isolates each light, N shows normals, M the normal matrix, W wireframe, D depth, O transform order. ESC to close.\n");

    // Phase 13: give the two mouse callbacks in src/Camera.h a way to reach
    // this camera. GLFW's callbacks are plain C function pointers - they
    // cannot capture 'scene' the way a lambda could - so a pointer to the
    // one camera they need is attached to the window itself, and each
    // callback reads it back out with glfwGetWindowUserPointer.
    //
    // Seeding lastCursorX/Y from the REAL current cursor position, rather
    // than leaving them at their 0.0 default, stops the very first drag from
    // jumping by however far the cursor's true starting position happens to
    // be from the corner of the screen.
    glfwSetWindowUserPointer(window, &scene.camera);
    glfwGetCursorPos(window, &scene.camera.lastCursorX, &scene.camera.lastCursorY);
    glfwSetCursorPosCallback(window, handleOrbitCameraCursorMove);
    glfwSetScrollCallback(window, handleOrbitCameraScroll);

    FrameClock clock;
    startClock(clock);

    FrameStats stats;
    stats.reportStartTime = clock.now;

    // Phase 24: what the title bar is currently showing. Starting it at -1 forces
    // the first frame to write the title, whatever the real counts turn out to be.
    RenderStats lastShownStats;
    lastShownStats.drawCalls = -1;

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        updateClock(clock);
        processInput(window, scene);

        // Phase 25: rebuild here, between reading input and using the meshes.
        // Doing it in processInput() would mean the key handler owned GPU
        // objects; doing it after renderScene() would draw one frame at the old
        // detail after the new detail was announced.
        //
        // A failed rebuild leaves the meshes empty, so there is nothing sensible
        // to draw and the loop ends. The cleanup below still runs, because this
        // breaks out of the loop rather than returning.
        if (scene.detail.needsRebuild) {
            if (!rebuildMeshes(gridMesh, cylinderMesh, sphereMesh, scene.detail)) {
                std::fprintf(stderr, "Failed to rebuild the scene meshes.\n");
                break;
            }
            scene.detail.needsRebuild = false;
        }

        // Phase 7: read the CURRENT framebuffer size every frame, not just
        // once at startup, so uProjection keeps a correct aspect ratio if the
        // window is resized while the program runs.
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        updateScene(scene, clock.now, clock.deltaTime, framebufferWidth, framebufferHeight);
        // Named frameCost, not stats: main already has a FrameStats called
        // 'stats' for the timing report, and shadowing it here would quietly
        // hand the wrong one to reportFrame() below.
        const RenderStats frameCost =
            renderScene(shader, triangleMesh, quadMesh, gridMesh, cylinderMesh, sphereMesh,
                        cubeMesh, smoothCubeMesh, scene);
        updateWindowTitle(window, frameCost, lastShownStats);

        glfwSwapBuffers(window);
        glfwPollEvents();

        reportFrame(stats, clock);
    }

    // OpenGL resources must be deleted while the context still exists.
    //
    // Phase 14: each Mesh also frees itself in its destructor, but a Mesh
    // declared in main() is destroyed AFTER glfwTerminate() has already
    // destroyed the context, and deleting a GPU object with no context is
    // not valid. Calling destroy() here, explicitly, deletes the buffers at
    // the right moment and leaves every handle at 0, so the later destructor
    // finds nothing to do. This is the same arrangement ShaderProgram has
    // used since Phase 2.
    smoothCubeMesh.destroy();
    cubeMesh.destroy();
    sphereMesh.destroy();
    cylinderMesh.destroy();
    gridMesh.destroy();
    quadMesh.destroy();
    triangleMesh.destroy();
    shader.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
