#version 330 core
// ^ src/Lighting.h's shared lighting block is spliced in immediately below this
//   line, by ShaderProgram::loadFromFiles(). It brings in uShadingMode, the
//   material uniforms, the light uniforms, lightContribution() and
//   computeLighting(). Do NOT declare those again here.

// Phase 29: the per-vertex colour still arrives but is no longer read. The material
// decides an object's appearance now.
//
// Phase 45: it is read again, by exactly one thing - a material whose emission is marked
// uKeFromVertexColor (the sky dome's gradient). Every other draw ignores it, as before.
in vec3 vColor;

// Phase 15/26: the interpolated normal, in world space.
//
// INTERPOLATED is the key word for Phase 31. In Phong mode this is what gets blended
// across the triangle, and the lighting is then worked out per pixel from it. In
// Gouraud mode the colour was blended instead and this is unused.
in vec3 vNormal;

// Phase 28: this fragment's world position, for the view direction and for the point
// light's distance.
in vec3 vWorldPos;

// Phase 31: the Gouraud result, already lit in the vertex shader and blended here.
in vec3 vLitColor;

// Environment build: white water on the crests (0 for everything but the sea).
in float vSeaFoam;

// Phase 15: 0 draws the normal picture, 1 paints normals as colours.
uniform int uDebugNormals;

// Environment build: the surface's opacity. 1 for everything the project drew before, so with blending off (which
// it is, except for clouds, glows and the sun's halo) the picture is unchanged by this uniform. The C++ side sets it
// to 1 at the start of every frame and puts it back after each translucent draw.
uniform float uAlpha;

// Environment build: UNDERWATER. uUnderWater is the colour of the water seen from inside; uUnderSky what the sky looks like through the surface (Snell's window);
// uCaustic the strength of the dancing light patterns on whatever is below the surface (0 above water, so nothing changes there); uWaterY the mean sea level.
uniform vec3 uUnderWater;
uniform vec3 uUnderSky;
uniform float uCaustic;
uniform float uWaterY;

// Environment build: THE SHORE. The land seen from above as circles (x, z, radius) - the nearest few, set every frame from the scenery. The sea reads its distance to the
// nearest shore to turn from deep water to shallows: lighter and greener over the shelf, sandy at the edge, with a line of surf that breathes in and out.
uniform int uShoreCount;
uniform vec3 uShore[24];

// Hybrid ray tracing: ordinary geometry is still rasterized. Only an above-water
// sea fragment casts one analytic ray toward the sun against this bounded set of
// nearby proxy spheres. With the toggle off this block has no visual effect.
const int RAY_SPHERE_MAX = 16;
uniform int uRayTracingEnabled;
uniform int uRaySphereCount;
uniform vec4 uRaySpheres[RAY_SPHERE_MAX];       // xyz = centre, w = radius

bool rayIntersectsSphere(vec3 rayOrigin, vec3 rayDir,
                         vec3 centre, float radius)
{
    vec3 oc = rayOrigin - centre;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0)
        return false;

    float root = sqrt(h);
    float nearT = -b - root;
    float farT = -b + root;
    return farT > 0.08 && (nearT > 0.08 || c < 0.0);
}

bool isSunBlocked(vec3 origin, vec3 direction)
{
    for (int i = 0; i < RAY_SPHERE_MAX; ++i) {
        if (i >= uRaySphereCount)
            break;
        if (rayIntersectsSphere(origin, direction,
                                uRaySpheres[i].xyz, uRaySpheres[i].w))
            return true;
    }
    return false;
}

// A cheap caustic pattern: the bright network of lines light makes after bending through a rippled surface. Two crossed sine fields folded into ridges.
float causticPattern(vec2 p, float t)
{
    float a = sin(p.x * 1.7 + t * 0.9 + sin(p.y * 1.3 + t * 0.6));
    float b = sin(p.y * 1.9 - t * 0.8 + sin(p.x * 1.1 - t * 0.5));
    float c = sin((p.x + p.y) * 1.2 + t * 0.7);
    float v = 1.0 - abs(a + b + 0.5 * c) * 0.45;
    return pow(clamp(v, 0.0, 1.0), 7.0);
}

// The final red, green, blue, and alpha values written to the screen.
out vec4 FragColor;

void main()
{
    // Phase 45: hand the interpolated vertex colour to computeLighting() (see basic.vert).
    gVertexColor = vColor;

    // ---- FLAT shading (Phase 31) ------------------------------------------
    //
    // A flat-shaded face uses ONE normal for the whole triangle - the triangle's own
    // geometric normal. The mesh does not store that anywhere, so it has to be
    // recovered here.
    //
    // dFdx and dFdy are screen-space derivatives: they report how a value changes
    // between this pixel and the one next to it, horizontally and vertically. The GPU
    // can do this because it shades pixels in 2x2 blocks. Applied to the world
    // position they give two vectors lying IN the surface - and the cross product of
    // two vectors in a plane is perpendicular to that plane.
    //
    // So this is the triangle's true facet normal, constant across the whole triangle
    // because the surface is planar between its three corners.
    vec3 faceNormal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));

    // The cross product's SIGN depends on screen-space handedness, so it can come out
    // pointing into the surface instead of out of it. Comparing against the
    // interpolated normal - which is known to point outward - and flipping if they
    // disagree fixes that without caring about winding order or which way the
    // derivatives happened to run.
    if (dot(faceNormal, vNormal) < 0.0)
        faceNormal = -faceNormal;

    // Phase 26/27: both shaders normalize, for two different reasons. The vertex
    // shader does it because the normal matrix changes a normal's length. This does it
    // because INTERPOLATING between two unit vectors gives something shorter than 1 -
    // the straight line between two points on a circle passes inside it. On a flat
    // face whose corners share one normal it costs nothing; on the sphere it is the
    // difference between correct and slightly too dark.
    vec3 N = normalize(vNormal);

    // The deliberately limited ray-traced part of the renderer. The ray starts
    // just above the moving water to avoid self-intersection and asks whether a
    // nearby analytic proxy blocks the directional sun.
    float rayVisibility = 1.0;
    if (uSeaMode != 0 && uRayTracingEnabled != 0 && uViewPos.y >= vWorldPos.y) {
        rayVisibility = isSunBlocked(vWorldPos + N * 0.05,
                                     -normalize(uSunDirection)) ? 0.0 : 1.0;
    }
    gSunVisibility = rayVisibility;

    if (uDebugNormals == 1) {
        // A normal's components run from -1 to +1, a colour channel from 0 to 1.
        // Halving and shifting maps one onto the other, so a face pointing along +X
        // comes out (1.0, 0.5, 0.5).
        //
        // No lighting and no material here, deliberately: the only job of this view is
        // to be read accurately.
        //
        // In Flat mode it shows the FACET normal, which is itself worth seeing - the
        // sphere stops being a smooth colour sweep and becomes a patchwork.
        vec3 shown = (uShadingMode == 0) ? faceNormal : N;
        FragColor = vec4(shown * 0.5 + 0.5, 1.0);
        return;
    }

    // Phase 31: ONE branch, in ONE program, choosing between three shading models.
    //
    // All three use the SAME computeLighting() from src/Lighting.h - Gouraud simply
    // called it earlier, in the vertex shader. The models differ only in WHAT normal
    // is used and WHERE the function runs:
    //
    //   Flat    - the facet normal, constant per triangle, so every pixel of a
    //             triangle comes out the same: visible faceting, and Mach banding
    //             where neighbouring facets meet (L9 s27).
    //   Gouraud - per VERTEX, then the COLOUR is interpolated. Cheap, but a highlight
    //             falling between vertices is never computed at all (L9 s28).
    //   Phong   - the NORMAL is interpolated, then lit per fragment. Correct, and the
    //             most expensive: the whole model is re-evaluated for every pixel
    //             (L9 s36).
    vec3 lit;
    if (uShadingMode == 0) {
        lit = computeLighting(faceNormal, vWorldPos);
    } else if (uShadingMode == 1) {
        // A per-fragment ray cannot be evaluated in the already-lit vertices.
        // Re-evaluate only the sea while RT is on; off is the original Gouraud path.
        lit = (uSeaMode != 0 && uRayTracingEnabled != 0)
            ? computeLighting(N, vWorldPos) : vLitColor;
    } else {
        lit = computeLighting(N, vWorldPos);
    }

    // Environment build: THE SEA'S SURFACE. Two things the lit colour does not have yet.
    //
    //   FOAM   the crests are whiter, in patches (vSeaFoam comes from the vertex shader), as bright as the light on them allows.
    //   MIRROR the sea reflects what is above it, more the lower the angle you look at it - the Fresnel effect. Schlick's approximation:
    //              F = F0 + (1 - F0) (1 - N.V)^5,   F0 = 0.03 for water.
    //          Looking straight down F is 0.03 (you see into the water); at a grazing angle it climbs to 1 (you see the sky and the ship).
    //          What is reflected is already in the framebuffer: renderScene drew the whole scene upside down below the sea first (the planar
    //          reflection), and this blends over it: result = lit * (1 - w) + what is behind * w, with w the mirror weight. The blend
    //          function (ONE, SRC_ALPHA) is set up for exactly that, so the alpha written here is w itself. With uReflect = 0, w = 0 and this
    //          is the plain opaque sea of before.
    if (uSeaMode != 0) {
        vec3 V = normalize(uViewPos - vWorldPos);

        // THE SURFACE SEEN FROM BELOW (the eye is lower than this point of the sea). Looking nearly straight up you see out through a circular window
        // onto the sky (light bends at the surface, so the whole sky is squeezed into a cone of 97 degrees: Snell's window); outside it the surface is a
        // mirror turned to the water (total internal reflection) and shows only dark water. A glint of the sun or moon shows in the window, and the
        // distance haze fades the far surface into the water's colour.
        if (uViewPos.y < vWorldPos.y) {
            float cosAngle = clamp(dot(-N, V), 0.0, 1.0);                       // 1 straight up, small at a glancing angle
            float window = smoothstep(0.55, 0.72, cosAngle);
            vec3 skyThrough = uUnderSky * (0.65 + 0.35 * pow(cosAngle, 3.0));
            vec3 mirror = uUnderWater * (0.30 + 0.20 * cosAngle);
            vec3 under = mix(mirror, skyThrough, window);
            vec3 toSun = -normalize(uSunDirection);
            under += uSunColor * 0.55 * pow(max(dot(-V, toSun), 0.0), 48.0) * window;
            float d = length(uViewPos - vWorldPos);
            float f = 1.0 - exp(-pow(uHazeDensity * d, 2.0));
            FragColor = vec4(mix(under, uHazeColor, f), 1.0);
            return;
        }
        float fresnel = 0.03 + 0.97 * pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 5.0);
        float foam = clamp(vSeaFoam, 0.0, 1.0);
        vec3 foamTint = clamp(uGlobalAmbient * 1.3 + uSunColor * 0.55, 0.0, 1.0);
        lit = mix(lit, foamTint, foam * 0.85);
        float shallow = 0.0;
        if (uShoreCount > 0) {
            float shoreD = 1.0e9;
            for (int i = 0; i < uShoreCount; ++i)
                shoreD = min(shoreD, length(vWorldPos.xz - uShore[i].xy) - uShore[i].z);
            shallow = 1.0 - smoothstep(0.0, 12.0, shoreD);
            vec3 light = clamp(uGlobalAmbient * 1.15 + uSunColor * 0.50, 0.0, 1.0);
            vec3 shelf = light * vec3(0.30, 0.78, 0.72);                                    // clear green-blue water over sand
            vec3 sand = light * vec3(0.74, 0.66, 0.46);                                      // the bottom showing at the very edge
            lit = mix(lit, shelf, shallow * 0.55);
            lit = mix(lit, sand, (1.0 - smoothstep(0.0, 2.6, shoreD)) * 0.50);
            float surf = smoothstep(0.0, 0.4, shoreD) * (1.0 - smoothstep(0.8, 2.0, shoreD));
            surf *= 0.55 + 0.45 * sin(shoreD * 4.5 - uWaveTime * 1.7);
            lit = mix(lit, foamTint, clamp(surf, 0.0, 1.0) * 0.8);
        }
        float w = clamp(fresnel * 1.35 * uReflect, 0.0, 0.8) * (1.0 - 0.7 * foam);
        w *= 1.0 - 0.55 * shallow;                                                          // a shallow bottom shows through: less of a mirror
        FragColor = vec4(lit * (1.0 - w), w);
        return;
    }

    // Underwater: light patterns play over everything below the surface, weaker with depth and strongest on surfaces that face up.
    if (uCaustic > 0.0 && vWorldPos.y < uWaterY) {
        float depthFade = exp(-(uWaterY - vWorldPos.y) * 0.10);
        float facing = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
        float seen = exp(-pow(uHazeDensity * length(uViewPos - vWorldPos), 2.0));          // the pattern fades into the fog with distance, like everything else
        depthFade *= seen;
        lit *= 1.0 + uCaustic * causticPattern(vWorldPos.xz * 0.9, uWaveTime) * depthFade * (0.35 + 0.65 * facing);
    }

    FragColor = vec4(lit, uAlpha);
}
