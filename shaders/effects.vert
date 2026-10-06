#version 330 core
// The effects program: everything that is NOT lit geometry. One tiny program, three jobs, chosen by uEffectMode:
//
//   0  the on-screen control panel and HUD - positions arrive in WINDOW pixels and become clip space here;
//   1  rain - thousands of streaks whose positions are a closed form of uTime (nothing is stored per frame);
//   2  stars - points on a sphere round the camera, twinkling as a function of uTime;
//   3  glow - soft round halos round lanterns, lamps and the sun or moon (each a camera-facing square, faded to nothing at its edge).
//
// The lit scene keeps its single shader (basic.vert / basic.frag) and its single branch for Flat, Gouraud and Phong. This
// program draws things that have no surface to light: a rectangle of the menu, a raindrop, a star.

layout(location = 0) in vec3 aPosition;   // mode 0: x, y in window pixels | mode 1: a point in the unit cube | mode 2: a unit direction
layout(location = 1) in vec4 aColor;      // mode 0: the colour | mode 1: (_, speed variation, _, 0 head / 1 tail) | mode 2: (size, twinkle rate, phase, _)
layout(location = 2) in vec4 aExtra;      // mode 4 (particles): u, v, size, kind

uniform int uEffectMode;
uniform vec2 uScreen;            // mode 0: the window size in pixels
uniform mat4 uViewProjection;    // modes 1 and 2
uniform float uTime;             // seconds - the clock the closed forms are functions of
uniform vec3 uCamRight;           // mode 4: the camera's right and up, for billboards
uniform vec3 uCamUp;
uniform vec2 uParticleFade;        // mode 4: distance at which particles start to fade and at which they are gone
uniform vec3 uEye;               // mode 2: the camera (the star sphere is centred on it)

// Rain
uniform vec3 uRainCentre;        // the middle of the box the rain falls in (the ship, at the waterline)
uniform vec3 uRainBox;           // the box's size
uniform vec3 uRainFall;          // the unit direction a drop travels (down, leaning with the wind)
uniform float uRainSpeed;
uniform float uRainLength;
uniform vec3 uRainColor;
uniform float uRainAlpha;        // the peak alpha

// Stars
uniform float uStarAlpha;
uniform float uStarRadius;
uniform float uPixelScale;

out vec4 vColor;
out vec4 vExtra;

void main()
{
    vExtra = vec4(0.0);
    if (uEffectMode == 0) {
        // Window pixels (y grows downward) to clip space (y grows upward).
        vec2 ndc = vec2(aPosition.x / uScreen.x * 2.0 - 1.0, 1.0 - aPosition.y / uScreen.y * 2.0);
        gl_Position = vec4(ndc, 0.0, 1.0);
        vColor = aColor;
    } else if (uEffectMode == 1) {
        // A drop starts at a fixed random point of the box and moves along uRainFall at its own speed. The position is
        // wrapped back into the box round the centre, so the shower follows the ship forever and the drop count never
        // changes. The only thing that moves is uTime: this is a closed form, not an animation table.
        float speed = uRainSpeed * (0.8 + 0.4 * aColor.g);
        vec3 p = aPosition * uRainBox + uRainFall * speed * uTime;
        vec3 rel = p - uRainCentre;
        rel -= uRainBox * floor(rel / uRainBox + 0.5);          // into [-box/2, +box/2) on every axis
        vec3 world = uRainCentre + rel + vec3(0.0, 0.5 * uRainBox.y, 0.0);

        // The head is the lower end of the streak; the tail trails back up the way the drop came from.
        if (aColor.a > 0.5)
            world -= uRainFall * uRainLength;

        gl_Position = uViewProjection * vec4(world, 1.0);

        // Bright at the head, gone at the tail, and fading with distance so the far shower is a soft haze.
        float distanceFade = clamp(1.0 - length(world - uEye) / 70.0, 0.0, 1.0);
        vColor = vec4(uRainColor, uRainAlpha * (1.0 - aColor.a) * distanceFade);
    } else if (uEffectMode == 2) {
        // Stars: a point on the sphere of radius uStarRadius round the eye.
        vec3 world = uEye + aPosition * uStarRadius;
        gl_Position = uViewProjection * vec4(world, 1.0);
        gl_PointSize = aColor.r * uPixelScale;
        float twinkle = 0.62 + 0.38 * sin(uTime * aColor.g + aColor.b * 40.0);
        vColor = vec4(0.82, 0.88, 1.0, uStarAlpha * twinkle);
    } else if (uEffectMode == 4) {
        // Mode 4, PARTICLES. kind 0 puff, 2 spark, 5 bubble: a square facing the camera; 1 ring, 3 foam: a square lying flat on the sea; 4 ray: a strip
        // whose corners the C++ side has already placed. aExtra = (u, v, size, kind), size being the half-extent in world units.
        vec2 q = aExtra.xy * 2.0 - 1.0;
        int kind = int(aExtra.w + 0.5);
        vec3 world;
        if (kind == 4)
            world = aPosition;
        else if (kind == 1 || kind == 3)
            world = aPosition + vec3(q.x, 0.0, q.y) * aExtra.z;
        else
            world = aPosition + (uCamRight * q.x + uCamUp * q.y) * aExtra.z;
        gl_Position = uViewProjection * vec4(world, 1.0);
        float fade = 1.0 - smoothstep(uParticleFade.x, uParticleFade.y, length(aPosition - uEye));
        vColor = vec4(aColor.rgb, aColor.a * fade);
        vExtra = aExtra;
    } else {
        // Mode 3, GLOW: a camera-facing square the C++ side has already placed in the world. The fragment shader turns it into a soft
        // round halo. aColor = (u, v inside the square, 0 warm or 1 cool, strength).
        gl_Position = uViewProjection * vec4(aPosition, 1.0);
        vColor = aColor;
    }
}
