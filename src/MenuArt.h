#pragma once

// Pirate-ship OPTIONS SCREEN, drawn with nothing but coloured triangles. No texture, no font file, no GUI library: the weathered wood is horizontal planks (each a vertical
// colour gradient of its own tone, with a seam and a few grain lines), the frame is dark iron with a bronze inner line, rivets and a rope strip, a button is an iron plate with a
// bevel that warms and slides when the cursor is on it, and every icon is a few triangles, discs and strokes on a 16 x 16 grid scaled to the button.
//
// ANIMATION IS EASED STATE, NOT A TABLE. main.cpp keeps one number per button for "how hovered" and one for "how selected", each moved towards 0 or 1 by an exponential approach
// every frame (so a button warms and cools smoothly at any frame rate), and one number for how far the screen has slid open. This file only reads them.
//
// WHERE THE LIVE PREVIEW IS. The world is not hidden behind the menu any more: the picture is only darkened at its two edges (a vignette) and the panels stand at the sides, clear
// of the ship in the middle. A choice is applied at once (src/main.cpp), and the sky, the light, the land, the sea, the rain and the lightning in the middle of the screen glide to
// it over a couple of seconds, so the screen itself is the preview.

#include "Overlay.h"

namespace MenuArt {

using glm::vec3;
using glm::vec4;

// ---- the palette ----------------------------------------------------------------------------------------------------------------------
const vec4 WOOD_DARK(0.060f, 0.038f, 0.024f, 0.96f);
const vec4 WOOD_MID(0.115f, 0.072f, 0.042f, 0.96f);
const vec4 WOOD_LIGHT(0.170f, 0.108f, 0.060f, 0.96f);
const vec4 IRON_DARK(0.055f, 0.058f, 0.066f, 0.98f);
const vec4 IRON_MID(0.135f, 0.140f, 0.155f, 0.98f);
const vec4 IRON_LIGHT(0.260f, 0.270f, 0.295f, 1.0f);
const vec4 BRONZE(0.60f, 0.40f, 0.15f, 1.0f);
const vec4 BRONZE_DARK(0.30f, 0.19f, 0.07f, 1.0f);
const vec4 GOLD(0.95f, 0.76f, 0.34f, 1.0f);
const vec4 GOLD_BRIGHT(1.0f, 0.90f, 0.55f, 1.0f);
const vec4 CREAM(0.93f, 0.90f, 0.80f, 1.0f);
const vec4 INK(0.06f, 0.045f, 0.03f, 1.0f);

inline vec4 mix4(const vec4& a, const vec4& b, float t) { return a + (b - a) * std::clamp(t, 0.0f, 1.0f); }
inline vec4 withAlpha(const vec4& c, float a) { return vec4(c.r, c.g, c.b, a); }
inline vec4 scaled(const vec4& c, float k) { return vec4(c.r * k, c.g * k, c.b * k, c.a); }
inline float smooth(float t) { t = std::clamp(t, 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t); }

inline unsigned int hash(unsigned int n)
{
    n ^= n >> 16; n *= 0x7feb352dU; n ^= n >> 15; n *= 0x846ca68bU; n ^= n >> 16;
    return n;
}
inline float hash01(unsigned int n) { return static_cast<float>(hash(n) & 0xFFFFu) / 65535.0f; }

// ---- surfaces ------------------------------------------------------------------------------------------------------------------------

// A soft drop shadow: a few rectangles, each larger and fainter than the last, offset down and to the right.
inline void shadow(UiBatch& ui, float x, float y, float w, float h, float s, float strength)
{
    for (int i = 0; i < 5; ++i) {
        const float grow = (1.0f + 1.4f * static_cast<float>(i)) * s;
        ui.rect(x - grow + 2.0f * s, y - grow + 3.0f * s, w + 2.0f * grow, h + 2.0f * grow, vec4(0.0f, 0.0f, 0.0f, strength * (0.20f - 0.035f * static_cast<float>(i))));
    }
}

// A rivet: a dark pit, a bronze dome, a spark of light.
inline void rivet(UiBatch& ui, float cx, float cy, float s)
{
    ui.disc(cx + 0.4f * s, cy + 0.6f * s, 1.9f * s, vec4(0.0f, 0.0f, 0.0f, 0.55f), vec4(0.0f, 0.0f, 0.0f, 0.0f), 8);
    ui.disc(cx, cy, 1.5f * s, vec4(0.85f, 0.62f, 0.28f, 1.0f), vec4(0.34f, 0.21f, 0.08f, 1.0f), 8);
    ui.rect(cx - 0.6f * s, cy - 0.8f * s, 0.7f * s, 0.6f * s, vec4(1.0f, 0.95f, 0.75f, 0.85f));
}

// Weathered planks: each plank a vertical gradient of its own tone, a dark seam under it, and a couple of grain lines.
inline void planks(UiBatch& ui, float x, float y, float w, float h, float s, unsigned int seed)
{
    const float plank = 9.0f * s;
    int i = 0;
    for (float py = y; py < y + h - 0.5f; py += plank, ++i) {
        const float ph = std::min(plank, y + h - py);
        const float tone = 0.78f + 0.40f * hash01(seed + static_cast<unsigned int>(i) * 7919u);
        ui.rectV(x, py, w, ph, scaled(WOOD_LIGHT, tone * 0.92f), scaled(WOOD_MID, tone * 0.85f));
        ui.rect(x, py + ph - s, w, s, vec4(0.0f, 0.0f, 0.0f, 0.55f));                           // the seam
        ui.rect(x, py + 0.6f * s, w, 0.5f * s, vec4(1.0f, 0.85f, 0.6f, 0.045f));                  // the lit top edge
        for (int g = 0; g < 2; ++g) {                                                             // grain
            const float gx = x + w * hash01(seed + static_cast<unsigned int>(i * 5 + g) * 104729u) * 0.7f;
            const float gl = w * (0.18f + 0.3f * hash01(seed + static_cast<unsigned int>(i * 3 + g) * 15485863u));
            ui.rect(gx, py + ph * (0.3f + 0.35f * static_cast<float>(g)), gl, 0.6f * s, vec4(0.0f, 0.0f, 0.0f, 0.16f));
        }
        const float knot = hash01(seed + static_cast<unsigned int>(i) * 31337u);                    // now and then a knot
        if (knot > 0.86f)
            ui.disc(x + w * (0.15f + 0.7f * knot), py + 0.5f * ph, 1.6f * s, vec4(0.02f, 0.012f, 0.008f, 0.55f), vec4(0.02f, 0.012f, 0.008f, 0.0f), 8);
    }
}

// A twist of rope along an edge: alternating light and dark slanted strands.
inline void rope(UiBatch& ui, float x, float y, float w, float s)
{
    const float step = 3.0f * s;
    ui.rect(x, y, w, 2.6f * s, vec4(0.05f, 0.04f, 0.02f, 0.9f));
    int i = 0;
    for (float px = x; px < x + w - step; px += step, ++i) {
        const vec4 c = (i & 1) ? vec4(0.62f, 0.52f, 0.32f, 1.0f) : vec4(0.42f, 0.34f, 0.19f, 1.0f);
        ui.tri(px, y + 2.4f * s, px + 1.8f * s, y + 0.2f * s, px + step, y + 2.4f * s, c);
        ui.tri(px + 1.8f * s, y + 0.2f * s, px + step + 1.8f * s, y + 0.2f * s, px + step, y + 2.4f * s, scaled(c, 0.8f));
    }
}

// A wooden panel in an iron frame: shadow, planks, iron border with a bronze inner line, corner plates with rivets, and a rope along the top.
inline void woodPanel(UiBatch& ui, float x, float y, float w, float h, float s, unsigned int seed)
{
    shadow(ui, x, y, w, h, s, 1.0f);
    planks(ui, x, y, w, h, s, seed);
    ui.rectV(x, y, w, 7.0f * s, vec4(0.0f, 0.0f, 0.0f, 0.38f), vec4(0.0f, 0.0f, 0.0f, 0.0f));  // the frame casts a shade inside
    ui.rectV(x, y + h - 9.0f * s, w, 9.0f * s, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.42f));
    // the iron frame: outer dark band, a lighter band, a bronze line
    const float f = 2.2f * s;
    ui.rectV(x - f, y - f, w + 2.0f * f, f, IRON_LIGHT, IRON_MID);
    ui.rectV(x - f, y + h, w + 2.0f * f, f, IRON_MID, IRON_DARK);
    ui.rectH(x - f, y, f, h, IRON_LIGHT, IRON_MID);
    ui.rectH(x + w, y, f, h, IRON_MID, IRON_DARK);
    ui.rect(x + s, y + s, w - 2.0f * s, 0.8f * s, withAlpha(BRONZE, 0.8f));
    ui.rect(x + s, y + h - 1.8f * s, w - 2.0f * s, 0.8f * s, withAlpha(BRONZE_DARK, 0.9f));
    ui.rect(x + s, y + s, 0.8f * s, h - 2.0f * s, withAlpha(BRONZE, 0.55f));
    ui.rect(x + w - 1.8f * s, y + s, 0.8f * s, h - 2.0f * s, withAlpha(BRONZE_DARK, 0.7f));
    // corner plates
    const float cp = 7.0f * s;
    const float cx[2] = { x - f, x + w - cp + f }, cy[2] = { y - f, y + h - cp + f };
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j) {
            ui.rectV(cx[i], cy[j], cp, cp, BRONZE, BRONZE_DARK);
            ui.rect(cx[i], cy[j], cp, 0.7f * s, vec4(1.0f, 0.88f, 0.55f, 0.5f));
            rivet(ui, cx[i] + 0.5f * cp, cy[j] + 0.5f * cp, s);
        }
}

// ---- icons: drawn on a 16 x 16 grid centred on (cx, cy), `u` pixels to a grid unit ---------------------------------------------------

struct IconCtx {
    UiBatch& ui;
    float cx, cy, u;
    float alpha;
    vec4 bg;                                         // the colour behind the icon (the crescent moon is a disc with a bite of the background taken out)
    vec4 c(float r, float g, float b) const { return vec4(r, g, b, alpha); }
    float X(float gx) const { return cx + gx * u; }
    float Y(float gy) const { return cy + gy * u; }
    void tri(float x0, float y0, float x1, float y1, float x2, float y2, const vec4& col) const { ui.tri(X(x0), Y(y0), X(x1), Y(y1), X(x2), Y(y2), col); }
    void line(float x0, float y0, float x1, float y1, float t, const vec4& col) const { ui.line(X(x0), Y(y0), X(x1), Y(y1), std::max(1.0f, t * u), col); }
    void disc(float x, float y, float r, const vec4& col) const { ui.disc(X(x), Y(y), r * u, col, col, 12); }
    void rect(float x, float y, float w, float h, const vec4& col) const { ui.rect(X(x), Y(y), w * u, h * u, col); }
};

inline void sunShape(const IconCtx& I, float x, float y, float r, const vec4& col, bool rays)
{
    I.ui.disc(I.X(x), I.Y(y), r * I.u, mix4(col, vec4(1.0f, 1.0f, 0.85f, col.a), 0.5f), col, 14);
    if (rays)
        for (int k = 0; k < 8; ++k) {
            const float a = 0.7854f * static_cast<float>(k);
            I.line(x + (r + 1.4f) * std::cos(a), y + (r + 1.4f) * std::sin(a), x + (r + 3.4f) * std::cos(a), y + (r + 3.4f) * std::sin(a), 1.3f, col);
        }
}

inline void cloudShape(const IconCtx& I, float dx, float dy, float scale, const vec4& col)
{
    const vec4 dark = scaled(col, 0.72f);
    I.disc(dx - 3.6f * scale, dy + 2.0f * scale, 3.3f * scale, dark);
    I.disc(dx + 0.6f * scale, dy - 0.4f * scale, 4.4f * scale, col);
    I.disc(dx + 4.6f * scale, dy + 2.2f * scale, 3.1f * scale, dark);
    I.rect(dx - 5.4f * scale, dy + 1.8f * scale, 12.0f * scale, 3.6f * scale, dark);
    I.rect(dx - 2.0f * scale, dy + 1.0f * scale, 8.0f * scale, 1.2f * scale, col);
}

inline void drawIcon(const IconCtx& I, int group, int index)
{
    if (group == UI_GROUP_LOCATION) {
        switch (index) {
        case 0: {                                                              // open ocean: three rows of waves
            for (int r = 0; r < 3; ++r) {
                const float y = -3.5f + 5.0f * static_cast<float>(r), k = 1.0f - 0.18f * static_cast<float>(r);
                const vec4 col = I.c(0.35f * k, 0.78f * k, 0.92f * k);
                for (int seg = 0; seg < 4; ++seg)
                    I.line(-7.0f + 3.5f * static_cast<float>(seg), y + ((seg & 1) ? -2.2f : 0.0f), -3.5f + 3.5f * static_cast<float>(seg), y + ((seg & 1) ? 0.0f : -2.2f), 1.5f, col);
            }
            break;
        }
        case 1: {                                                              // jungle island: a palm on a mound of sand
            I.disc(0.0f, 8.0f, 7.0f, I.c(0.85f, 0.72f, 0.45f));
            I.line(0.5f, 6.5f, 1.5f, -2.5f, 1.8f, I.c(0.62f, 0.42f, 0.22f));
            const vec4 leaf = I.c(0.32f, 0.78f, 0.34f);
            I.tri(1.5f, -3.0f, -7.0f, 0.5f, -3.5f, -5.5f, leaf);
            I.tri(1.5f, -3.0f, 8.0f, 0.0f, 4.5f, -6.0f, scaled(leaf, 0.85f));
            I.tri(1.5f, -3.0f, -2.5f, -8.5f, 3.5f, -8.0f, scaled(leaf, 1.1f));
            I.disc(1.0f, -2.2f, 1.0f, I.c(0.45f, 0.28f, 0.1f));
            break;
        }
        case 2: {                                                              // mountain coast: two peaks with snow
            I.tri(-8.0f, 7.0f, -2.5f, -6.5f, 4.0f, 7.0f, I.c(0.52f, 0.58f, 0.68f));
            I.tri(0.0f, 7.0f, 5.0f, -3.0f, 9.0f, 7.0f, I.c(0.38f, 0.44f, 0.54f));
            I.tri(-2.5f, -6.5f, -4.8f, -2.0f, -0.1f, -2.0f, I.c(0.96f, 0.97f, 1.0f));
            I.tri(5.0f, -3.0f, 3.4f, 0.0f, 6.6f, 0.0f, I.c(0.9f, 0.92f, 0.98f));
            break;
        }
        case 3: {                                                              // rocky islands: jagged stacks in the sea
            I.tri(-8.0f, 6.0f, -5.0f, -1.0f, -1.0f, 6.0f, I.c(0.5f, 0.46f, 0.42f));
            I.tri(-3.0f, 6.0f, 1.0f, -6.0f, 6.0f, 6.0f, I.c(0.42f, 0.38f, 0.35f));
            I.tri(3.0f, 6.0f, 6.0f, 0.5f, 8.5f, 6.0f, I.c(0.55f, 0.5f, 0.45f));
            I.rect(-8.0f, 5.0f, 16.5f, 3.2f, I.c(0.3f, 0.62f, 0.8f));
            break;
        }
        default: {                                                             // foggy coast: a headland drowned in bars of mist
            I.tri(-8.0f, 7.0f, -3.0f, -1.0f, 3.0f, 7.0f, I.c(0.34f, 0.38f, 0.42f));
            I.rect(-3.0f, -6.0f, 1.6f, 6.0f, I.c(0.8f, 0.8f, 0.78f));
            for (int k = 0; k < 3; ++k)
                I.rect(-7.5f + 2.2f * static_cast<float>(k & 1), -3.5f + 4.2f * static_cast<float>(k), 13.0f, 2.2f, I.c(0.86f, 0.9f, 0.95f) * vec4(1.0f, 1.0f, 1.0f, 0.62f));
            break;
        }
        }
    } else if (group == UI_GROUP_WEATHER) {
        switch (index) {
        case 0: sunShape(I, 0.0f, 0.0f, 3.6f, I.c(1.0f, 0.82f, 0.25f), true); break;
        case 1: cloudShape(I, 0.0f, 0.0f, 1.0f, I.c(0.86f, 0.9f, 0.95f)); break;
        case 2: {
            cloudShape(I, 0.0f, -2.6f, 0.9f, I.c(0.62f, 0.7f, 0.8f));
            for (int k = 0; k < 3; ++k)
                I.line(-4.0f + 4.0f * static_cast<float>(k), 3.0f, -5.4f + 4.0f * static_cast<float>(k), 7.4f, 1.3f, I.c(0.35f, 0.65f, 1.0f));
            break;
        }
        case 3: {
            for (int k = 0; k < 4; ++k)
                I.rect(-7.0f + 2.0f * static_cast<float>(k & 1) - 1.0f, -6.0f + 4.0f * static_cast<float>(k), 12.0f - 2.0f * static_cast<float>(k & 1), 2.0f, I.c(0.8f, 0.86f, 0.9f) * vec4(1.0f, 1.0f, 1.0f, 0.85f - 0.12f * static_cast<float>(k)));
            break;
        }
        case 4: {
            cloudShape(I, 0.0f, -3.0f, 0.95f, I.c(0.38f, 0.42f, 0.5f));
            const vec4 bolt = I.c(1.0f, 0.9f, 0.25f);
            I.tri(1.8f, 1.0f, -2.6f, 5.2f, 0.7f, 5.2f, bolt);
            I.tri(0.7f, 5.2f, -2.2f, 9.5f, -0.4f, 5.6f, bolt);
            I.tri(1.8f, 1.0f, 0.7f, 5.2f, 3.6f, 1.0f, bolt);
            break;
        }
        default: {                                                             // dynamic: two arrows chasing each other round a ring
            const vec4 col = I.c(0.45f, 0.9f, 0.8f);
            for (int arc = 0; arc < 2; ++arc) {
                const float a0 = 0.35f + 3.1416f * static_cast<float>(arc), a1 = a0 + 2.3f;
                for (int k = 0; k < 6; ++k) {
                    const float t0 = a0 + (a1 - a0) * static_cast<float>(k) / 6.0f, t1 = a0 + (a1 - a0) * static_cast<float>(k + 1) / 6.0f;
                    I.line(5.6f * std::cos(t0), 5.6f * std::sin(t0), 5.6f * std::cos(t1), 5.6f * std::sin(t1), 1.6f, col);
                }
                const float ex = 5.6f * std::cos(a1), ey = 5.6f * std::sin(a1);
                I.tri(ex + 3.0f * std::cos(a1 + 1.5708f + 0.35f), ey + 3.0f * std::sin(a1 + 1.5708f + 0.35f), ex + 3.0f * std::cos(a1 - 1.5708f + 0.2f), ey + 3.0f * std::sin(a1 - 1.5708f + 0.2f),
                      ex + 3.6f * std::cos(a1 + 1.5708f - 0.6f), ey + 3.6f * std::sin(a1 + 1.5708f - 0.6f), col);
            }
            break;
        }
        }
    } else if (group == UI_GROUP_TIME) {
        switch (index) {
        case 0: sunShape(I, 0.0f, 0.0f, 3.8f, I.c(1.0f, 0.88f, 0.3f), true); break;
        case 1: {                                                              // sunset: half a sun on the horizon, its glitter on the water
            const vec4 col = I.c(1.0f, 0.55f, 0.2f);
            for (int k = 0; k < 12; ++k) {
                const float a0 = 3.1416f + 3.1416f * static_cast<float>(k) / 12.0f, a1 = 3.1416f + 3.1416f * static_cast<float>(k + 1) / 12.0f;
                I.ui.tri(I.X(0.0f), I.Y(1.5f), I.X(5.0f * std::cos(a0)), I.Y(1.5f + 5.0f * std::sin(a0)), I.X(5.0f * std::cos(a1)), I.Y(1.5f + 5.0f * std::sin(a1)), col);
            }
            for (int k = 0; k < 5; ++k) {
                const float a = 3.1416f + 0.5f + 0.54f * static_cast<float>(k);
                I.line(6.8f * std::cos(a), 1.5f + 6.8f * std::sin(a), 8.2f * std::cos(a), 1.5f + 8.2f * std::sin(a), 1.2f, I.c(1.0f, 0.7f, 0.3f));
            }
            I.rect(-8.0f, 1.5f, 16.0f, 1.2f, I.c(1.0f, 0.8f, 0.5f));
            I.rect(-5.0f, 4.0f, 10.0f, 1.0f, I.c(0.95f, 0.5f, 0.2f) * vec4(1.0f, 1.0f, 1.0f, 0.8f));
            I.rect(-3.0f, 6.2f, 6.0f, 1.0f, I.c(0.95f, 0.5f, 0.2f) * vec4(1.0f, 1.0f, 1.0f, 0.55f));
            break;
        }
        case 2: {                                                              // night: a crescent and two stars
            I.disc(-0.5f, 0.0f, 5.6f, I.c(0.9f, 0.93f, 1.0f));
            I.ui.disc(I.X(2.2f), I.Y(-1.6f), 4.9f * I.u, I.bg, I.bg, 14);
            I.rect(4.0f, -6.0f, 1.4f, 1.4f, I.c(1.0f, 1.0f, 0.8f));
            I.rect(6.2f, 2.2f, 1.2f, 1.2f, I.c(1.0f, 1.0f, 0.8f));
            break;
        }
        default: {                                                             // dynamic: a clock face
            const vec4 col = I.c(0.45f, 0.9f, 0.8f);
            I.disc(0.0f, 0.0f, 6.6f, col);
            I.ui.disc(I.X(0.0f), I.Y(0.0f), 4.8f * I.u, I.bg, I.bg, 14);
            I.line(0.0f, 0.0f, 0.0f, -3.6f, 1.3f, col);
            I.line(0.0f, 0.0f, 2.8f, 1.2f, 1.3f, col);
            break;
        }
        }
    } else if (group == UI_GROUP_CAMERA) {
        const vec4 col = I.c(0.82f, 0.84f, 0.9f);
        switch (index) {
        case 0: I.tri(-7.0f, 1.0f, 7.0f, 1.0f, 4.0f, 6.0f, I.c(0.6f, 0.4f, 0.2f)); I.line(0.0f, 1.0f, 0.0f, -7.0f, 1.2f, col); I.tri(0.6f, -6.5f, 6.0f, -0.5f, 0.6f, -0.5f, col); break;     // chase: a ship
        case 1: I.line(-7.0f, 0.0f, 7.0f, 0.0f, 1.4f, col); I.line(0.0f, -7.0f, 0.0f, 7.0f, 1.4f, col); I.tri(7.5f, 0.0f, 4.5f, -2.0f, 4.5f, 2.0f, col); I.tri(-7.5f, 0.0f, -4.5f, -2.0f, -4.5f, 2.0f, col); I.tri(0.0f, -7.5f, -2.0f, -4.5f, 2.0f, -4.5f, col); I.tri(0.0f, 7.5f, -2.0f, 4.5f, 2.0f, 4.5f, col); break;  // free: four arrows
        case 2: I.disc(0.0f, 0.0f, 6.5f, col); I.disc(0.0f, 0.0f, 3.4f, I.c(0.2f, 0.45f, 0.8f)); I.disc(0.0f, 0.0f, 1.5f, I.c(0.02f, 0.02f, 0.05f)); break;                                   // first person: an eye
        case 3: I.rect(-7.0f, -2.0f, 14.0f, 9.0f, col); for (int k = 0; k < 4; ++k) I.tri(-7.0f + 3.6f * static_cast<float>(k), -2.0f, -4.0f + 3.6f * static_cast<float>(k), -2.0f, -6.0f + 3.6f * static_cast<float>(k), -6.5f, (k & 1) ? col : I.c(0.1f, 0.1f, 0.12f)); break;  // film: a clapper
        case 4: I.tri(-8.0f, 3.0f, 0.0f, -6.5f, 8.0f, 3.0f, I.c(0.1f, 0.1f, 0.14f)); I.rect(-8.0f, 3.0f, 16.0f, 2.0f, I.c(0.1f, 0.1f, 0.14f)); I.rect(-1.0f, -1.0f, 2.0f, 2.0f, GOLD); break;         // captain: a hat
        default: I.line(-4.0f, -7.0f, -4.0f, 7.0f, 1.5f, col); I.line(4.0f, -7.0f, 4.0f, 7.0f, 1.5f, col); for (int k = 0; k < 4; ++k) I.line(-4.0f, -4.5f + 3.0f * static_cast<float>(k), 4.0f, -4.5f + 3.0f * static_cast<float>(k), 1.2f, col); break;    // below deck: a ladder
        }
    } else {
        if (index == UI_ACTION_RESUME)
            I.tri(-4.0f, -6.0f, -4.0f, 6.0f, 6.5f, 0.0f, I.c(0.5f, 1.0f, 0.6f));
        else {
            I.line(-5.0f, -5.0f, 5.0f, 5.0f, 2.0f, I.c(1.0f, 0.5f, 0.45f));
            I.line(5.0f, -5.0f, -5.0f, 5.0f, 2.0f, I.c(1.0f, 0.5f, 0.45f));
        }
    }
}

// ---- one button ------------------------------------------------------------------------------------------------------------------------

// `hover` and `sel` are the eased 0..1 amounts; `focus` is the keyboard (or controller) cursor; `time` is for the pulse.
inline void drawButton(UiBatch& ui, const UiButton& b, float hover, float sel, bool focus, float time, int scale)
{
    const float s = static_cast<float>(scale);
    const float charH = 7.0f * s;
    const float lift = 1.6f * s * hover;                                     // a hovered button slides to the right
    const float x = b.x + lift, y = b.y, w = b.w - lift, h = b.h;
    const float warm = std::max(hover, focus ? 0.8f : 0.0f);
    const bool isAction = b.group == UI_GROUP_ACTION;

    // the plate
    ui.rect(x + s, y + 1.4f * s, w, h, vec4(0.0f, 0.0f, 0.0f, 0.45f));
    vec4 top = mix4(IRON_MID, vec4(0.31f, 0.21f, 0.11f, 0.98f), warm), bottom = mix4(IRON_DARK, vec4(0.16f, 0.105f, 0.055f, 0.98f), warm);
    if (isAction) {
        const vec4 tint = (b.index == UI_ACTION_RESUME) ? vec4(0.10f, 0.30f, 0.16f, 0.98f) : vec4(0.36f, 0.10f, 0.09f, 0.98f);
        top = mix4(tint, scaled(tint, 1.7f), warm);
        bottom = scaled(tint, 0.55f + 0.5f * warm);
    }
    top = mix4(top, vec4(0.96f, 0.74f, 0.30f, 0.99f), sel);
    bottom = mix4(bottom, vec4(0.62f, 0.38f, 0.11f, 0.99f), sel);
    ui.rectV(x, y, w, h, top, bottom);
    ui.rect(x, y, w, s, vec4(1.0f, 0.95f, 0.8f, 0.14f + 0.32f * warm + 0.2f * sel));                // the lit top edge
    ui.rect(x, y + h - s, w, s, vec4(0.0f, 0.0f, 0.0f, 0.55f));                                    // the shaded bottom edge
    if (sel > 0.01f)                                                                                 // a sheen that sweeps across the selected plate
        ui.rectH(x, y + s, w, 0.45f * h, vec4(1.0f, 1.0f, 0.85f, 0.10f * sel), vec4(1.0f, 1.0f, 0.85f, 0.0f));

    // the outline: iron, warming to gold; a keyboard cursor adds a pulsing gold ring
    const float edge = std::max(std::max(hover, sel), focus ? 1.0f : 0.0f);
    const vec4 outline = mix4(withAlpha(IRON_LIGHT, 0.55f), GOLD, edge);
    ui.rect(x, y, w, 0.8f * s, outline); ui.rect(x, y + h - 0.8f * s, w, 0.8f * s, outline);
    ui.rect(x, y, 0.8f * s, h, outline); ui.rect(x + w - 0.8f * s, y, 0.8f * s, h, outline);
    if (focus) {
        const float pulse = 0.65f + 0.35f * std::sin(time * 7.0f);
        const vec4 ring = withAlpha(GOLD_BRIGHT, pulse);
        const float o = 1.6f * s;
        ui.rect(x - o, y - o, w + 2.0f * o, 0.9f * s, ring); ui.rect(x - o, y + h + o - 0.9f * s, w + 2.0f * o, 0.9f * s, ring);
        ui.rect(x - o, y - o, 0.9f * s, h + 2.0f * o, ring); ui.rect(x + w + o - 0.9f * s, y - o, 0.9f * s, h + 2.0f * o, ring);
        const float my = y + 0.5f * h;                                                                // and a pointer on the left
        ui.tri(x - 6.5f * s, my - 2.6f * s, x - 6.5f * s, my + 2.6f * s, x - 2.5f * s, my, ring);
    }

    // the icon (in a little round socket) and the label
    const float iconSize = 9.0f * s;
    const float icx = x + 8.5f * s, icy = y + 0.5f * h;
    ui.disc(icx, icy, 0.62f * iconSize, vec4(0.0f, 0.0f, 0.0f, 0.38f), vec4(0.0f, 0.0f, 0.0f, 0.22f), 14);
    IconCtx ctx{ ui, icx, icy, iconSize / 16.0f, 1.0f, mix4(mix4(top, bottom, 0.5f), vec4(0.0f, 0.0f, 0.0f, 0.38f), 0.35f) };
    ctx.bg.a = 1.0f;
    drawIcon(ctx, b.group, b.index);

    const vec4 labelCol = mix4(mix4(vec4(0.78f, 0.78f, 0.76f, 1.0f), CREAM, hover), INK, sel);
    const float tx = x + 16.5f * s, ty = y + 0.5f * (h - charH);
    if (sel < 0.5f)
        ui.text(tx + s, ty + s, b.label, scale, vec4(0.0f, 0.0f, 0.0f, 0.65f));                    // letters cut into the plate: a shadow, then the face
    else
        ui.text(tx + 0.5f * s, ty + 0.7f * s, b.label, scale, vec4(1.0f, 0.9f, 0.6f, 0.35f));
    ui.text(tx, ty, b.label, scale, labelCol);

    // the lamp on the right: dark until the option is chosen, then lit gold
    const float lx = x + w - 5.0f * s;
    ui.disc(lx, y + 0.5f * h, 2.2f * s, vec4(0.0f, 0.0f, 0.0f, 0.55f), vec4(0.0f, 0.0f, 0.0f, 0.55f), 10);
    if (sel > 0.02f) {
        ui.disc(lx, y + 0.5f * h, 3.6f * s * sel, vec4(1.0f, 0.85f, 0.4f, 0.35f * sel), vec4(1.0f, 0.7f, 0.2f, 0.0f), 12);
        ui.disc(lx, y + 0.5f * h, 1.7f * s, mix4(vec4(0.3f, 0.2f, 0.08f, 1.0f), vec4(1.0f, 0.93f, 0.55f, 1.0f), sel), mix4(vec4(0.3f, 0.2f, 0.08f, 1.0f), vec4(0.9f, 0.62f, 0.15f, 1.0f), sel), 10);
    }
}

// An engraved section heading with a rule running out of it and a small icon at its end.
inline void heading(UiBatch& ui, float x, float y, float w, const char* text, int scale, const vec4& colour)
{
    const float s = static_cast<float>(scale);
    ui.text(x + s, y + s, text, scale, vec4(0.0f, 0.0f, 0.0f, 0.7f));
    ui.text(x, y, text, scale, colour);
    const float tw = UiBatch::textWidth(text, scale), ry = y + 3.0f * s;
    ui.rectH(x + tw + 3.0f * s, ry, w - tw - 3.0f * s, s, withAlpha(GOLD, 0.65f), withAlpha(GOLD, 0.0f));
    ui.rectH(x + tw + 3.0f * s, ry + s, w - tw - 3.0f * s, 0.7f * s, vec4(0.0f, 0.0f, 0.0f, 0.5f), vec4(0.0f, 0.0f, 0.0f, 0.0f));
    ui.tri(x + tw + 1.0f * s, ry + 0.5f * s, x + tw + 3.5f * s, ry - 1.8f * s, x + tw + 3.5f * s, ry + 2.8f * s, GOLD);
}

// A bronze plaque with engraved lettering.
inline void plaque(UiBatch& ui, float x, float y, float w, float h, const char* text, int scale)
{
    const float s = static_cast<float>(scale);
    ui.rect(x + s, y + 1.5f * s, w, h, vec4(0.0f, 0.0f, 0.0f, 0.5f));
    ui.rectV(x, y, w, h, vec4(0.80f, 0.58f, 0.24f, 1.0f), vec4(0.42f, 0.27f, 0.09f, 1.0f));
    ui.rect(x, y, w, s, vec4(1.0f, 0.92f, 0.62f, 0.7f));
    ui.rect(x, y + h - s, w, s, vec4(0.0f, 0.0f, 0.0f, 0.5f));
    rivet(ui, x + 3.0f * s, y + 0.5f * h, s * 0.8f);
    rivet(ui, x + w - 3.0f * s, y + 0.5f * h, s * 0.8f);
    const float ty = y + 0.5f * (h - 7.0f * s);
    ui.textCentred(x + 0.5f * w + 0.6f * s, ty + 0.8f * s, text, scale, vec4(1.0f, 0.92f, 0.65f, 0.5f));
    ui.textCentred(x + 0.5f * w, ty, text, scale, INK);
}

// ---- what main.cpp tells the menu --------------------------------------------------------------------------------------------------

constexpr int MAX_BUTTONS = 40;

struct MenuState {
    float hover[MAX_BUTTONS] = {};          // eased 0..1: the cursor (or keyboard focus) is on the button
    float selected[MAX_BUTTONS] = {};       // eased 0..1: the button is the current choice
    float open = 0.0f;                      // 0 closed .. 1 fully slid in
    float time = 0.0f;                      // real seconds, for the pulse
    int focus = -1;                         // the button the keyboard or controller is on
    bool focusVisible = false;              // draw the focus ring (keyboard / controller in use)
};

// The whole pause screen: the vignette that darkens only the edges, the controls list, the three panels, the status plaque and the hint line.
inline void buildOptionsMenu(UiBatch& ui, const UiLayout& L, const EnvironmentChoice& choice, const MenuState& m, int W, int H)
{
    using namespace UiConfig;
    if (L.mode == MODE_HIDDEN)
        return;
    const int sc = L.scale;
    const float s = static_cast<float>(sc);
    const float charH = 7.0f * s, margin = 4.0f * s;
    const float fw = static_cast<float>(W), fh = static_cast<float>(H);
    const float ease = smooth(m.open);
    const std::size_t first = ui.vertices.size();

    // ---- the vignette: dark at the left and right edges where the panels stand, a thin shade top and bottom, almost nothing across the middle of the picture (the live preview)
    ui.rect(0.0f, 0.0f, fw, fh, vec4(0.0f, 0.0f, 0.03f, 0.10f));
    ui.rectH(0.0f, 0.0f, 0.30f * fw, fh, vec4(0.0f, 0.0f, 0.02f, 0.62f), vec4(0.0f, 0.0f, 0.02f, 0.0f));
    ui.rectH(0.72f * fw, 0.0f, 0.28f * fw, fh, vec4(0.0f, 0.0f, 0.02f, 0.0f), vec4(0.0f, 0.0f, 0.02f, 0.66f));
    ui.rectV(0.0f, 0.0f, fw, 0.10f * fh, vec4(0.0f, 0.0f, 0.02f, 0.45f), vec4(0.0f, 0.0f, 0.02f, 0.0f));
    ui.rectV(0.0f, 0.86f * fh, fw, 0.14f * fh, vec4(0.0f, 0.0f, 0.02f, 0.0f), vec4(0.0f, 0.0f, 0.02f, 0.55f));

    // ---- the panels slide in from their own sides
    const float slideR = (1.0f - ease) * (L.panelW + 4.0f * margin), slideL = -(1.0f - ease) * (L.helpW + 4.0f * margin);
    const float panelX = L.panelX + slideR;

    woodPanel(ui, panelX, L.panelY, L.panelW, L.panelH, s, 11u);
    plaque(ui, panelX + margin, L.panelY + margin, L.panelW - 2.0f * margin, charH + 2.5f * margin, "OPTIONS", sc + 1);
    static const char* const TITLES[3] = { "LOCATION", "WEATHER", "TIME OF DAY" };
    for (int g = 0; g < 3; ++g)
        heading(ui, panelX + margin, L.groupTitleY[g], L.panelW - 2.0f * margin, TITLES[g], sc, GOLD);

    if (L.mode == MODE_MENU_AND_HELP) {
        woodPanel(ui, L.helpX + slideL, L.helpY, L.helpW, L.helpH, s, 29u);
        woodPanel(ui, L.camX + slideL, L.camY, L.camW, L.camH, s, 53u);
        heading(ui, L.camX + slideL + margin, L.camTitleY, L.camW - 2.0f * margin, "VIEW", sc, GOLD);
        float y = L.helpY + margin;
        heading(ui, L.helpX + slideL + margin, y, L.helpW - 2.0f * margin, "CONTROLS", sc, GOLD);
        y += charH + margin;
        for (int i = 0; i < HELP_ROW_COUNT; ++i) {
            if (HELP_ROWS[i].label[0] != '\0' && i > 0)
                ui.rectH(L.helpX + slideL + margin, y - 1.0f * s, L.helpW - 2.0f * margin, 0.7f * s, withAlpha(BRONZE, 0.25f), withAlpha(BRONZE, 0.0f));
            ui.text(L.helpX + slideL + margin + s, y + s, HELP_ROWS[i].label, sc, vec4(0.0f, 0.0f, 0.0f, 0.6f));
            ui.text(L.helpX + slideL + margin, y, HELP_ROWS[i].label, sc, GOLD);
            ui.text(L.helpX + slideL + margin + 9.0f * UiFont::ADVANCE * s, y, HELP_ROWS[i].keys, sc, CREAM);
            y += charH + 2.0f * s;
        }
    }

    // ---- the buttons (hit-tested at their final places; drawn at their sliding ones)
    for (int i = 0; i < static_cast<int>(L.buttons.size()) && i < MAX_BUTTONS; ++i) {
        UiButton b = L.buttons[i];
        if (b.group == UI_GROUP_CAMERA)
            b.x += slideL;
        else
            b.x += slideR;
        drawButton(ui, b, m.hover[i], m.selected[i], m.focusVisible && i == m.focus, m.time, sc);
    }

    // ---- the status plaque, bottom centre: the three current choices, each with its icon
    {
        const char* names[3] = { locationName(choice.location), weatherName(choice.weather), timeName(choice.time) };
        const int groups[3] = { UI_GROUP_LOCATION, UI_GROUP_WEATHER, UI_GROUP_TIME };
        const int idx[3] = { choice.location, choice.weather, choice.time };
        const float icon = 9.0f * s, gap = 6.0f * s;
        float total = 2.0f * gap;
        for (int k = 0; k < 3; ++k)
            total += icon + 2.0f * s + UiBatch::textWidth(names[k], sc) + gap;
        const float ph = icon + 3.0f * margin, py = fh - ph - 7.0f * margin, px = 0.5f * (fw - total) - margin;
        woodPanel(ui, px, py, total + 2.0f * margin, ph, s, 77u);
        float x = px + margin + 0.5f * gap;
        for (int k = 0; k < 3; ++k) {
            IconCtx ctx{ ui, x + 0.5f * icon, py + 0.5f * ph, icon / 16.0f, 1.0f, vec4(0.1f, 0.07f, 0.05f, 1.0f) };
            drawIcon(ctx, groups[k], idx[k]);
            x += icon + 2.0f * s;
            ui.text(x + s, py + 0.5f * (ph - charH) + s, names[k], sc, vec4(0.0f, 0.0f, 0.0f, 0.7f));
            ui.text(x, py + 0.5f * (ph - charH), names[k], sc, GOLD_BRIGHT);
            x += UiBatch::textWidth(names[k], sc) + gap;
            if (k < 2)
                ui.tri(x - 0.5f * gap - s, py + 0.5f * ph, x - 0.5f * gap + 1.5f * s, py + 0.5f * ph - 2.0f * s, x - 0.5f * gap + 1.5f * s, py + 0.5f * ph + 2.0f * s, withAlpha(BRONZE, 0.9f));
        }
    }

    // ---- the title and the hint line, top centre
    {
        const char* title = "GAME PAUSED";
        const float pw = UiBatch::textWidth(title, sc + 2) + 12.0f * margin, ph = 7.0f * static_cast<float>(sc + 2) + 3.0f * margin;
        plaque(ui, 0.5f * (fw - pw), 2.0f * margin - (1.0f - ease) * 3.0f * ph, pw, ph, title, sc + 2);
        const char* hint = "ARROWS MOVE   ENTER CHOOSE   ESC RESUME";
        const float hw = UiBatch::textWidth(hint, sc);
        const float hy = 2.0f * margin + ph + 2.0f * margin;
        ui.textCentred(0.5f * fw + s, hy + s, hint, sc, vec4(0.0f, 0.0f, 0.0f, 0.75f));
        ui.textCentred(0.5f * fw, hy, hint, sc, mix4(CREAM, GOLD, 0.35f));
        (void)hw;
    }

    // ---- the slide-in is also a fade: every vertex added above has its alpha scaled by how open the screen is
    for (std::size_t i = first; i < ui.vertices.size(); ++i)
        ui.vertices[i].color.a *= ease;
}

} // namespace MenuArt
