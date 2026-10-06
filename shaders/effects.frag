#version 330 core
// See effects.vert. The colour is already worked out per vertex; blending (set by the C++ side) does the rest.

in vec4 vColor;
in vec4 vExtra;

uniform int uEffectMode;
uniform vec3 uGlowWarm;    // mode 3: the colour of a lamp
uniform vec3 uGlowCool;    // mode 3: the colour of the sun or moon, which the C++ side sets each frame
uniform vec3 uGlowBolt;    // mode 3: the cold white-blue of a lightning strike (a glow whose "coolness" is 2)

out vec4 FragColor;

void main()
{
    vec4 c = vColor;
    if (uEffectMode == 4) {
        // The SHAPE of a particle, from where in its square this fragment is. kind: 0 soft puff, 1 ring, 2 spark, 3 foam, 4 light ray, 5 bubble.
        vec2 q = vExtra.xy * 2.0 - 1.0;
        float r = length(q);
        int kind = int(vExtra.w + 0.5);
        float a;
        if (kind == 0) { a = 1.0 - smoothstep(0.0, 1.0, r); a *= a; }
        else if (kind == 1) { a = smoothstep(0.16, 0.0, abs(r - 0.82)) * (1.0 - smoothstep(0.8, 1.0, r)); }
        else if (kind == 2) { a = 1.0 - smoothstep(0.35, 1.0, r); }
        else if (kind == 3) { float n = 0.5 + 0.5 * sin(q.x * 9.0 + q.y * 7.0) * sin(q.y * 8.0 - q.x * 5.0); a = (1.0 - smoothstep(0.45, 1.0, r)) * (0.55 + 0.45 * n); }
        else if (kind == 4) { float s = sin(vExtra.x * 3.14159); a = s * s * (1.0 - vExtra.y) * smoothstep(0.0, 0.08, vExtra.y); }
        else { a = (1.0 - smoothstep(0.82, 1.0, r)) * (0.12 + 0.88 * smoothstep(0.55, 0.85, r)) + 0.9 * (1.0 - smoothstep(0.0, 0.25, length(q - vec2(-0.35, 0.35)))); }
        a *= c.a;
        if (a < 0.003) discard;
        FragColor = vec4(c.rgb, a);
        return;
    }
    if (uEffectMode == 3) {
        // A halo: brightest in the middle, fading smoothly to nothing at the edge of the square. The falloff is a power of (1 - r), which
        // has no visible rim - the hard-edged discs a translucent sphere gives.
        float r = length(c.xy * 2.0 - 1.0);
        float falloff = pow(clamp(1.0 - r, 0.0, 1.0), 2.4);
        FragColor = vec4((c.z > 1.5) ? uGlowBolt : mix(uGlowWarm, uGlowCool, c.z), falloff * c.w);
        return;
    }
    if (uEffectMode == 2) {
        // A point is drawn as a square; fade it into a round dot.
        float d = length(gl_PointCoord - vec2(0.5));
        c.a *= smoothstep(0.5, 0.12, d);
    }
    FragColor = c;
}
