#pragma once

// Environment build: the on-screen control panel and HUD, drawn with OpenGL and nothing else.
//
// NO FONT FILE, NO TEXTURE, NO GUI LIBRARY. Each letter is a 5 x 7 grid of on/off pixels stored as seven small integers, and a lit
// pixel is drawn as a coloured rectangle. A word is therefore a few dozen rectangles, all collected into ONE vertex buffer and drawn in
// ONE call per frame (src/Overlay.h builds it, main.cpp draws it with the effects program, shaders/effects.vert, mode 0).
//
// The layout and the hit-testing are plain arithmetic on rectangles, kept apart from the OpenGL buffer so they can be checked on the
// CPU: where is each button, and which one is under the cursor?
//
// Coordinates are WINDOW pixels, origin at the top-left, y growing downward - the same as the cursor position GLFW reports, so "is the
// mouse on this button" is a plain comparison with no conversion.

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "Environment.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

// ---- the vertex and the buffer ---------------------------------------------------------------------------------------------

struct EffectVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec4 extra = glm::vec4(0.0f);      // particles (effects mode 4): u, v inside the quad, size, kind. Unused (0) by everything else
};

// A VAO and a VBO for EffectVertex data. The panel's is refilled every frame (GL_STREAM_DRAW); the rain's and the stars' are filled
// once (GL_STATIC_DRAW). Like Mesh and ShaderProgram it owns its GPU objects and frees them in destroy(), which main() calls while the
// context still exists.
class EffectBuffer {
public:
    EffectBuffer() = default;
    ~EffectBuffer() { destroy(); }
    EffectBuffer(const EffectBuffer&) = delete;
    EffectBuffer& operator=(const EffectBuffer&) = delete;

    bool create()
    {
        destroy();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EffectVertex), reinterpret_cast<void*>(offsetof(EffectVertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(EffectVertex), reinterpret_cast<void*>(offsetof(EffectVertex, color)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(EffectVertex), reinterpret_cast<void*>(offsetof(EffectVertex, extra)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        return m_vao != 0 && m_vbo != 0;
    }

    void upload(const std::vector<EffectVertex>& vertices, GLenum usage)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        // Re-specifying the whole store ("orphaning") lets the driver hand back fresh memory instead of waiting for the last frame's draw.
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(EffectVertex)),
                     vertices.empty() ? nullptr : vertices.data(), usage);
        m_count = static_cast<int>(vertices.size());
    }

    void draw(GLenum mode, int count) const
    {
        if (count <= 0 || m_vao == 0)
            return;
        glBindVertexArray(m_vao);
        glDrawArrays(mode, 0, std::min(count, m_count));
        glBindVertexArray(0);
    }

    int count() const { return m_count; }

    void destroy()
    {
        if (m_vbo != 0) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
        if (m_vao != 0) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
        m_count = 0;
    }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_count = 0;
};

// ---- the font ----------------------------------------------------------------------------------------------------------------
//
// ASCII 32 (space) to 95 (underscore), seven rows of five bits each, the leftmost pixel being bit 4. Capitals only; lower case is
// drawn as capitals. Characters outside the table are drawn as a question mark.
namespace UiFont {

constexpr int WIDTH = 5;
constexpr int HEIGHT = 7;
constexpr int ADVANCE = 6;      // a pixel of space between letters
constexpr int FIRST = 32;
constexpr int LAST = 95;

const std::uint8_t GLYPHS[LAST - FIRST + 1][HEIGHT] = {
    { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 },   // space
    { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 },   // !
    { 0b01010, 0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000 },   // "
    { 0b01010, 0b01010, 0b11111, 0b01010, 0b11111, 0b01010, 0b01010 },   // #
    { 0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100 },   // $
    { 0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011 },   // %
    { 0b01100, 0b10010, 0b10100, 0b01000, 0b10101, 0b10010, 0b01101 },   // &
    { 0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000 },   // '
    { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 },   // (
    { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 },   // )
    { 0b00000, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00000 },   // *
    { 0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000 },   // +
    { 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b00100, 0b01000 },   // ,
    { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 },   // -
    { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100 },   // .
    { 0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000 },   // /
    { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 },   // 0
    { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 },   // 1
    { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 },   // 2
    { 0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110 },   // 3
    { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 },   // 4
    { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 },   // 5
    { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 },   // 6
    { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 },   // 7
    { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 },   // 8
    { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 },   // 9
    { 0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000 },   // :
    { 0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b00100, 0b01000 },   // ;
    { 0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010 },   // <
    { 0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000 },   // =
    { 0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000 },   // >
    { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100 },   // ?
    { 0b01110, 0b10001, 0b10111, 0b10101, 0b10110, 0b10000, 0b01110 },   // @
    { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 },   // A
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 },   // B
    { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 },   // C
    { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 },   // D
    { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 },   // E
    { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 },   // F
    { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 },   // G
    { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 },   // H
    { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 },   // I
    { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 },   // J
    { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 },   // K
    { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 },   // L
    { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 },   // M
    { 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001 },   // N
    { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 },   // O
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 },   // P
    { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 },   // Q
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 },   // R
    { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 },   // S
    { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 },   // T
    { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 },   // U
    { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 },   // V
    { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 },   // W
    { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 },   // X
    { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 },   // Y
    { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 },   // Z
    { 0b01110, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01110 },   // [
    { 0b10000, 0b01000, 0b01000, 0b00100, 0b00010, 0b00010, 0b00001 },   // backslash
    { 0b01110, 0b00010, 0b00010, 0b00010, 0b00010, 0b00010, 0b01110 },   // ]
    { 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000, 0b00000 },   // ^
    { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111 },   // _
};

// The vertical bar, outside the table above (its ASCII code is 124): used to separate the status line's three parts.
const std::uint8_t PIPE[HEIGHT] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };

inline const std::uint8_t* glyph(char c)
{
    if (c == '|')
        return PIPE;
    int code = static_cast<unsigned char>(c);
    if (code >= 'a' && code <= 'z')
        code -= 'a' - 'A';
    if (code < FIRST || code > LAST)
        code = '?';
    return GLYPHS[code - FIRST];
}

} // namespace UiFont

// ---- drawing into a batch -------------------------------------------------------------------------------------------------------

// Collects rectangles - the panel's backgrounds, its buttons, and every lit pixel of its text - as triangles in window pixels.
class UiBatch {
public:
    std::vector<EffectVertex> vertices;

    void clear() { vertices.clear(); }

    void rect(float x, float y, float w, float h, const glm::vec4& color)
    {
        const glm::vec3 a(x, y, 0.0f), b(x + w, y, 0.0f), c(x + w, y + h, 0.0f), d(x, y + h, 0.0f);
        vertices.push_back({ a, color }); vertices.push_back({ b, color }); vertices.push_back({ c, color });
        vertices.push_back({ a, color }); vertices.push_back({ c, color }); vertices.push_back({ d, color });
    }

    // A rectangle with a colour at each corner (top-left, top-right, bottom-right, bottom-left): the rasteriser blends them, which is how the menu gets its
    // gradients (wood that darkens towards the bottom, a bevel that catches the light) without a texture.
    void rectGradient(float x, float y, float w, float h, const glm::vec4& tl, const glm::vec4& tr, const glm::vec4& br, const glm::vec4& bl)
    {
        const glm::vec3 a(x, y, 0.0f), b(x + w, y, 0.0f), c(x + w, y + h, 0.0f), d(x, y + h, 0.0f);
        vertices.push_back({ a, tl }); vertices.push_back({ b, tr }); vertices.push_back({ c, br });
        vertices.push_back({ a, tl }); vertices.push_back({ c, br }); vertices.push_back({ d, bl });
    }
    void rectV(float x, float y, float w, float h, const glm::vec4& top, const glm::vec4& bottom) { rectGradient(x, y, w, h, top, top, bottom, bottom); }
    void rectH(float x, float y, float w, float h, const glm::vec4& left, const glm::vec4& right) { rectGradient(x, y, w, h, left, right, right, left); }

    void tri(float x0, float y0, float x1, float y1, float x2, float y2, const glm::vec4& color)
    {
        vertices.push_back({ glm::vec3(x0, y0, 0.0f), color }); vertices.push_back({ glm::vec3(x1, y1, 0.0f), color }); vertices.push_back({ glm::vec3(x2, y2, 0.0f), color });
    }

    // A filled disc (a fan of `segments` triangles), the middle colour blending to the rim colour.
    void disc(float cx, float cy, float r, const glm::vec4& centre, const glm::vec4& rim, int segments = 16)
    {
        for (int i = 0; i < segments; ++i) {
            const float a0 = 6.2831853f * static_cast<float>(i) / static_cast<float>(segments), a1 = 6.2831853f * static_cast<float>(i + 1) / static_cast<float>(segments);
            vertices.push_back({ glm::vec3(cx, cy, 0.0f), centre });
            vertices.push_back({ glm::vec3(cx + r * std::cos(a0), cy + r * std::sin(a0), 0.0f), rim });
            vertices.push_back({ glm::vec3(cx + r * std::cos(a1), cy + r * std::sin(a1), 0.0f), rim });
        }
    }

    // A straight stroke of a given thickness between two points.
    void line(float x0, float y0, float x1, float y1, float thickness, const glm::vec4& color)
    {
        const float dx = x1 - x0, dy = y1 - y0, len = std::max(std::sqrt(dx * dx + dy * dy), 1e-4f);
        const float nx = -dy / len * 0.5f * thickness, ny = dx / len * 0.5f * thickness;
        const glm::vec3 a(x0 + nx, y0 + ny, 0.0f), b(x1 + nx, y1 + ny, 0.0f), c(x1 - nx, y1 - ny, 0.0f), d(x0 - nx, y0 - ny, 0.0f);
        vertices.push_back({ a, color }); vertices.push_back({ b, color }); vertices.push_back({ c, color });
        vertices.push_back({ a, color }); vertices.push_back({ c, color }); vertices.push_back({ d, color });
    }

    // The width of a string, in window pixels, at a given pixel scale (the last letter's trailing gap is not counted).
    static float textWidth(const char* s, int scale)
    {
        int n = 0;
        for (const char* p = s; *p != '\0'; ++p) ++n;
        return n == 0 ? 0.0f : static_cast<float>((n * UiFont::ADVANCE - 1) * scale);
    }

    // Draws a string with its top-left corner at (x, y). A run of lit pixels along one row of a letter becomes ONE rectangle, so the
    // letter "E" is not 17 squares but 7 bars.
    void text(float x, float y, const char* s, int scale, const glm::vec4& color)
    {
        float penX = x;
        for (const char* p = s; *p != '\0'; ++p) {
            const std::uint8_t* g = UiFont::glyph(*p);
            for (int row = 0; row < UiFont::HEIGHT; ++row) {
                int col = 0;
                while (col < UiFont::WIDTH) {
                    if ((g[row] >> (UiFont::WIDTH - 1 - col)) & 1) {
                        int end = col;
                        while (end + 1 < UiFont::WIDTH && ((g[row] >> (UiFont::WIDTH - 2 - end)) & 1)) ++end;
                        rect(penX + static_cast<float>(col * scale), y + static_cast<float>(row * scale),
                             static_cast<float>((end - col + 1) * scale), static_cast<float>(scale), color);
                        col = end + 1;
                    } else {
                        ++col;
                    }
                }
            }
            penX += static_cast<float>(UiFont::ADVANCE * scale);
        }
    }

    // The same, centred on x.
    void textCentred(float cx, float y, const char* s, int scale, const glm::vec4& color)
    {
        text(cx - 0.5f * textWidth(s, scale), y, s, scale, color);
    }
};

// ---- the panel: where everything is -----------------------------------------------------------------------------------------------

namespace UiConfig {

// The colours. Named so the look of the panel is a few edits, not a hunt.
const glm::vec4 PANEL_BACK(0.03f, 0.05f, 0.08f, 0.74f);
const glm::vec4 PANEL_EDGE(0.86f, 0.66f, 0.26f, 0.95f);
const glm::vec4 BUTTON_IDLE(0.12f, 0.16f, 0.22f, 0.88f);
const glm::vec4 BUTTON_HOVER(0.22f, 0.30f, 0.40f, 0.95f);
const glm::vec4 BUTTON_SELECTED(0.86f, 0.62f, 0.20f, 0.97f);
const glm::vec4 TEXT(0.92f, 0.93f, 0.90f, 1.0f);
const glm::vec4 TEXT_DARK(0.07f, 0.07f, 0.08f, 1.0f);
const glm::vec4 TEXT_GOLD(0.95f, 0.76f, 0.34f, 1.0f);
const glm::vec4 TEXT_DIM(0.62f, 0.66f, 0.70f, 1.0f);

// How big a font pixel is, from the window's height: a pixel font stays crisp only at whole-number scales.
inline int scaleFor(int windowHeight)
{
    return windowHeight < 560 ? 1 : (windowHeight < 1000 ? 2 : 3);
}

// Which panels are showing. While you PLAY nothing is: MODE_HIDDEN. ESC pauses the game and shows the menu and the controls list
// (MODE_MENU_AND_HELP); ESC again, or the RESUME button, hides them. (MODE_MENU, the menu without the list, is kept for the options.)
constexpr int MODE_HIDDEN = 0;
constexpr int MODE_MENU = 1;
constexpr int MODE_MENU_AND_HELP = 2;
constexpr int MODE_COUNT = 3;

} // namespace UiConfig

// The three groups of buttons, in the order they are listed.
constexpr int UI_GROUP_LOCATION = 0;
constexpr int UI_GROUP_WEATHER = 1;
constexpr int UI_GROUP_TIME = 2;
constexpr int UI_GROUP_CAMERA = 4;       // the five views, in a panel under the controls list
constexpr int UI_GROUP_ACTION = 3;       // RESUME and QUIT: not choices of environment
constexpr int UI_ACTION_RESUME = 0;
constexpr int UI_ACTION_QUIT = 1;

struct UiButton {
    float x, y, w, h;
    int group;
    int index;
    const char* label;
};

struct UiLayout {
    int scale = 2;
    int mode = UiConfig::MODE_MENU_AND_HELP;
    float panelX = 0.0f, panelY = 0.0f, panelW = 0.0f, panelH = 0.0f;       // the menu on the right
    float helpX = 0.0f, helpY = 0.0f, helpW = 0.0f, helpH = 0.0f;           // the controls list on the left
    float camX = 0.0f, camY = 0.0f, camW = 0.0f, camH = 0.0f;               // the view buttons under it
    float camTitleY = 0.0f;
    std::vector<UiButton> buttons;
    float groupTitleY[3] = { 0.0f, 0.0f, 0.0f };
    float headerY = 0.0f;
};

// The key list. Each row is a label and the keys that do it; an empty label continues the row above.
struct HelpRow { const char* label; const char* keys; };
const HelpRow HELP_ROWS[] = {
    { "SAIL",    "W A S D / ARROWS" },
    { "AIM",     "MOVE THE MOUSE" },
    { "FIRE",    "SPACE / RIGHT CLICK" },
    { "CAMERA",  "DRAG, SCROLL, Q E" },
    { "",        ", . PGUP PGDN" },
    { "VIEWS",   "F1-F5   R: CHASE" },
    { "MODES",   "F6 FREE  F7 1ST" },
    { "",        "F8 FILM  F9 CAPT" },
    { "FREE",    "WASD FLY E UP Q DN" },
    { "BELOW",   "TAB  WASD WALK F USE" },
    { "SHIP",    "Z SAILS   J ANCHOR" },
    { "",        "SHIFT RUN" },
    { "PLACE",   "I  (SHIFT: BACK)" },
    { "WEATHER", "C  (SHIFT: BACK)" },
    { "TIME",    "T  (SHIFT: BACK)" },
    { "PAUSE",   "ESC  (THIS MENU)" },
    { "SCREEN",  "F11 FULL  F12 SHOT" },
};
constexpr int HELP_ROW_COUNT = static_cast<int>(sizeof(HELP_ROWS) / sizeof(HELP_ROWS[0]));

// Works out where every rectangle goes for a window of this size. Called when the panel is drawn and when the mouse is tested, so the
// two can never disagree about where a button is.
inline UiLayout buildUiLayout(int windowWidth, int windowHeight, int mode)
{
    UiLayout L;
    L.scale = UiConfig::scaleFor(windowHeight);
    L.mode = mode;
    const float s = static_cast<float>(L.scale);
    const float charH = 7.0f * s;
    const float margin = 4.0f * s;
    const float buttonH = charH + 4.0f * s;
    const float buttonGap = 2.0f * s;

    // ---- the menu, down the right-hand side
    const float buttonW = (14.0f * UiFont::ADVANCE + 10.0f + 16.0f) * s;      // + room for the icon on the left           // "MOUNTAIN COAST" is the longest label
    L.panelW = buttonW + 2.0f * margin;
    L.panelX = static_cast<float>(windowWidth) - L.panelW - 2.0f * margin;
    L.panelY = 2.0f * margin;

    float y = L.panelY + margin;
    L.headerY = y;
    y += charH + 4.0f * margin;                                      // the OPTIONS plaque

    for (int group = 0; group < 3; ++group) {
        L.groupTitleY[group] = y;
        y += charH + margin;
        const int count = (group == UI_GROUP_LOCATION) ? LOCATION_COUNT : (group == UI_GROUP_WEATHER ? WEATHER_OPTIONS : TIME_OPTIONS);
        for (int i = 0; i < count; ++i) {
            const char* label = (group == UI_GROUP_LOCATION) ? locationName(i) : (group == UI_GROUP_WEATHER ? weatherName(i) : timeName(i));
            L.buttons.push_back({ L.panelX + margin, y, buttonW, buttonH, group, i, label });
            y += buttonH + buttonGap;
        }
        y += margin;
    }

    // RESUME and QUIT, below the three lists.
    static const char* const ACTIONS[2] = { "RESUME", "QUIT" };
    for (int i = 0; i < 2; ++i) {
        L.buttons.push_back({ L.panelX + margin, y, buttonW, buttonH, UI_GROUP_ACTION, i, ACTIONS[i] });
        y += buttonH + buttonGap;
    }
    y += margin;
    L.panelH = y - L.panelY;

    // ---- the controls list, top-left
    float widest = 0.0f;
    for (int i = 0; i < HELP_ROW_COUNT; ++i)
        widest = std::max(widest, 9.0f * UiFont::ADVANCE * s + UiBatch::textWidth(HELP_ROWS[i].keys, L.scale));
    L.helpW = widest + 2.0f * margin;
    L.helpX = 2.0f * margin;
    L.helpY = 2.0f * margin;
    L.helpH = margin + charH + margin + static_cast<float>(HELP_ROW_COUNT) * (charH + 2.0f * s) + margin;

    // ---- the view buttons, under the controls list
    L.camX = L.helpX;
    L.camW = L.helpW;
    L.camY = L.helpY + L.helpH + 2.0f * margin;
    float cy = L.camY + margin;
    L.camTitleY = cy;
    cy += charH + margin;
    for (int i = 0; i < CAMERA_MODE_COUNT; ++i) {
        L.buttons.push_back({ L.camX + margin, cy, L.camW - 2.0f * margin, buttonH, UI_GROUP_CAMERA, i, cameraModeName(i) });
        cy += buttonH + buttonGap;
    }
    L.camH = cy - L.camY + margin;
    return L;
}

inline bool uiPointIn(float px, float py, float x, float y, float w, float h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

// Index into layout.buttons of the button under the point, or -1.
inline int uiButtonAt(const UiLayout& L, float px, float py)
{
    if (L.mode == UiConfig::MODE_HIDDEN)
        return -1;
    for (int i = 0; i < static_cast<int>(L.buttons.size()); ++i) {
        const UiButton& b = L.buttons[i];
        if (uiPointIn(px, py, b.x, b.y, b.w, b.h))
            return i;
    }
    return -1;
}

// Is the point over any part of a visible panel? The world must not react to the mouse there: no aiming, no camera drag, no firing.
inline bool uiCaptures(const UiLayout& L, float px, float py)
{
    if (L.mode == UiConfig::MODE_HIDDEN)
        return false;
    if (uiPointIn(px, py, L.panelX, L.panelY, L.panelW, L.panelH))
        return true;
    return L.mode == UiConfig::MODE_MENU_AND_HELP && (uiPointIn(px, py, L.helpX, L.helpY, L.helpW, L.helpH) || uiPointIn(px, py, L.camX, L.camY, L.camW, L.camH));
}

// Which option a button stands for, as a currently selected one?
inline bool uiButtonSelected(const UiButton& b, const EnvironmentChoice& choice)
{
    switch (b.group) {
    case UI_GROUP_LOCATION: return b.index == choice.location;
    case UI_GROUP_WEATHER:  return b.index == choice.weather;
    case UI_GROUP_TIME:     return b.index == choice.time;
    case UI_GROUP_CAMERA:   return b.index == choice.cameraMode;
    default:                return false;               // RESUME and QUIT are actions, not choices
    }
}

// Applies a click on a button to a choice.
inline void uiApplyButton(const UiButton& b, EnvironmentChoice& choice)
{
    switch (b.group) {
    case UI_GROUP_LOCATION: choice.location = b.index; break;
    case UI_GROUP_WEATHER:  choice.weather = b.index; break;
    case UI_GROUP_TIME:     choice.time = b.index; break;
    case UI_GROUP_CAMERA:   choice.cameraMode = b.index; break;
    default:                break;                      // an action changes no choice
    }
}

// ---- what the HUD shows ---------------------------------------------------------------------------------------------------------

struct HudInfo {
    int playerHealth = 0, enemyHealth = 0, maxHealth = 1;
    float reloadLeft = 0.0f, reloadTotal = 1.0f;
    float speed = 0.0f;
    float headingDegrees = 0.0f;
    const char* sails = "FULL";
    const char* anchor = "UP";
    const char* ammo = "ROUND";
    const char* banner = nullptr;      // a big message in the middle (a ship has sunk)
    char toast[96] = "";               // a line that fades (the last thing the player changed)
    float toastAlpha = 0.0f;
    char status[128] = "";             // location | weather | time
    int shots = 0, hits = 0;
    int treasureGold = 0, treasureFound = 0, treasureTotal = 0;     // the collected treasure and the chests of this place
    float treasureFlash = 0.0f;                                      // 1 just after a chest is opened, fading: the counter flashes
    char prompt[96] = "";                                            // "F  OPEN THE CHEST" and the like, in the middle of the screen when something can be used
    float fade = 0.0f;                                               // 0..1 black over the whole picture (climbing between decks)
    bool below = false;                                              // walking about the ship
    char missionTitle[72] = "";
    char missionBrief[72] = "";
    char missionObjective[128] = "";
    char missionAction[128] = "";
    char cargo[128] = "";
    char dock[48] = "";
    int missionNumber = 1, missionCount = 10, campaignGold = 0, reputation = 0;
    float missionProgress = 0.0f, targetDistance = 0.0f, targetBearingDegrees = 0.0f, nearestPortBearingDegrees = 0.0f, nearestPortDistance = 0.0f;
    char nearestPort[32] = "";
    char worldRegion[32] = "";
    int regionRisk = 1, discoveries = 0, artifacts = 0;
    int factionRep[6] = {};
    bool mapOpen = false, treasureMapped = false, treasureExact = false, enemyKnown = false;
    bool rayTracing = false;
    float mapZoom = 1.0f, mapPanX = 0.0f, mapPanZ = 0.0f, mapHalfExtent = 850.0f;
    float playerX = 0.0f, playerZ = 0.0f, targetX = 0.0f, targetZ = 0.0f, enemyX = 0.0f, enemyZ = 0.0f, treasureX = 0.0f, treasureZ = 0.0f, treasureUncertainty = 0.0f;
    int portCount = 0, islandCount = 0, siteCount = 0;
    struct MapPoint { float x=0.0f,z=0.0f; bool known=false; char label[20]=""; } ports[16], islands[12], sites[56];
};

inline const char* compassRose(float degrees)
{
    static const char* const rose[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int i=static_cast<int>(std::floor((std::fmod(degrees,360.0f)+360.0f+22.5f)/45.0f))&7;
    return rose[i];
}

// Fills a batch with the whole overlay for this frame: the HUD, then (if showing) the menu and the controls list.
inline void buildUi(UiBatch& ui, const UiLayout& L, const EnvironmentChoice& choice, int hoverButton, const HudInfo& hud,
                    int windowWidth, int windowHeight, bool showHud)
{
    using namespace UiConfig;
    ui.clear();
    const int sc = L.scale;
    const float s = static_cast<float>(sc);
    const float charH = 7.0f * s;
    const float margin = 4.0f * s;
    const int hs = std::max(1, sc - 1);                              // gameplay HUD is deliberately smaller than menus/the chart
    const float h = static_cast<float>(hs), hChar = 7.0f*h, hm = 3.0f*h;
    const glm::vec4 HUD_BACK(PANEL_BACK.r,PANEL_BACK.g,PANEL_BACK.b,0.48f);

    if (hud.fade > 0.003f)                                    // a fade to black over the world (climbing between decks), under all the panels
        ui.rect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), glm::vec4(0.0f, 0.0f, 0.0f, std::clamp(hud.fade, 0.0f, 1.0f)));

    // (The pause screen - the dimming, the menu, the controls list - is drawn by buildOptionsMenu() in src/MenuArt.h.)

    // ---- the battle HUD: bottom left, clear of the health bars the ships carry above them
    if (showHud && !hud.mapOpen) {
        char line1[128],line2[128];
        std::snprintf(line1,sizeof(line1),"HP %d/%d  EN %d/%d  RT %s",hud.playerHealth,hud.maxHealth,hud.enemyHealth,hud.maxHealth,hud.rayTracing?"ON":"OFF");
        std::snprintf(line2,sizeof(line2),"V %.1f  H %03d  %s/%s  %s",std::fabs(hud.speed),static_cast<int>(std::fmod(std::fmod(hud.headingDegrees,360.0f)+360.0f,360.0f)),hud.sails,hud.anchor,hud.ammo);
        const float boxW=std::max(UiBatch::textWidth(line1,hs),UiBatch::textWidth(line2,hs))+4.0f*hm;
        const float boxH=2.0f*hChar+5.0f*hm+2.0f*h;
        const float bx=hm,by=static_cast<float>(windowHeight)-boxH-hm;
        ui.rect(bx,by,boxW,boxH,HUD_BACK);ui.rect(bx,by,boxW,h,PANEL_EDGE);
        ui.text(bx+2*hm,by+hm,line1,hs,TEXT);
        const float barW=boxW-4.0f*hm,barH=2.0f*h,barX=bx+2.0f*hm,barY=by+hm+hChar+hm;
        const float ready = (hud.reloadLeft <= 0.0f) ? 1.0f : 1.0f - std::clamp(hud.reloadLeft / hud.reloadTotal, 0.0f, 1.0f);
        ui.rect(barX, barY, barW, barH, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
        ui.rect(barX, barY, barW * ready, barH, ready >= 1.0f ? glm::vec4(0.45f, 0.85f, 0.35f, 1.0f) : glm::vec4(0.95f, 0.65f, 0.15f, 1.0f));
        ui.text(barX,barY+barH+hm,line2,hs,TEXT_DIM);
    }

    // Navigation compass and current campaign card.  These are part of gameplay HUD only;
    // the existing Options menu and its layout remain untouched.
    if (showHud && !hud.mapOpen && hud.missionTitle[0]) {
        char nav[128],mission[160];
        const int hdg=static_cast<int>(std::fmod(std::fmod(hud.headingDegrees,360.0f)+360.0f,360.0f));
        std::snprintf(nav,sizeof(nav),"H%03d %s OBJ %s %.0f PORT %s %.0f  %s R%d",hdg,compassRose(hud.headingDegrees),compassRose(hud.targetBearingDegrees),hud.targetDistance,compassRose(hud.nearestPortBearingDegrees),hud.nearestPortDistance,hud.worldRegion,hud.regionRisk);
        std::snprintf(mission,sizeof(mission),"M%d %s  %d%%  G%d C%d",hud.missionNumber,hud.missionBrief[0]?hud.missionBrief:hud.missionTitle,static_cast<int>(100.0f*std::clamp(hud.missionProgress,0.0f,1.0f)),hud.campaignGold,hud.treasureFound);
        const float navW=UiBatch::textWidth(nav,hs)+4*hm,missionW=UiBatch::textWidth(mission,hs)+4*hm;
        const float x=hm,y=hm;
        ui.rect(x,y,navW,hChar+2*hm,HUD_BACK);ui.rect(x,y,navW,h,PANEL_EDGE);ui.text(x+2*hm,y+hm,nav,hs,TEXT_GOLD);
        ui.rect(x,y+hChar+3*hm,missionW,hChar+2*hm,HUD_BACK);ui.rect(x,y+hChar+3*hm,missionW,h,PANEL_EDGE);ui.text(x+2*hm,y+hChar+4*hm,mission,hs,TEXT);
        ui.rect(x,y+2*hChar+5*hm,missionW*std::clamp(hud.missionProgress,0.0f,1.0f),h,glm::vec4(0.86f,0.62f,0.16f,0.9f));
    }

    // Interactive chart: procedural parchment, discovered ports, islands, mission target,
    // enemy contact and clue uncertainty. Different one-letter symbols form the legend.
    if (showHud && hud.mapOpen) {
        const float mapW=std::min(0.78f*windowWidth,900.0f),mapH=std::min(0.72f*windowHeight,620.0f);
        const float x=0.5f*(windowWidth-mapW),y=0.5f*(windowHeight-mapH);
        const glm::vec4 parchment(0.32f,0.24f,0.12f,0.94f),ink(0.92f,0.78f,0.46f,1),dim(0.55f,0.45f,0.28f,1);
        ui.rect(x-2*s,y-2*s,mapW+4*s,mapH+4*s,glm::vec4(0.08f,0.04f,0.02f,0.97f)); ui.rect(x,y,mapW,mapH,parchment);
        ui.textCentred(x+0.5f*mapW,y+2*margin,"CAPTAIN'S WORLD CHART   F10 CLOSE   ARROWS PAN   +/- ZOOM",sc,ink);
        char worldLine[192];std::snprintf(worldLine,sizeof(worldLine),"%s  RISK %d  FOUND %d  ART %d   REP P%d N%d M%d S%d C%d",hud.worldRegion,hud.regionRisk,hud.discoveries,hud.artifacts,hud.factionRep[0],hud.factionRep[1],hud.factionRep[2],hud.factionRep[3],hud.factionRep[5]);
        ui.textCentred(x+0.5f*mapW,y+charH+3*margin,worldLine,hs,ink);
        const float top=y+2*charH+6*margin,bottom=y+mapH-4*margin,left=x+4*margin,right=x+mapW-4*margin;
        for(int g=-3;g<=3;++g){const float gx=left+(g+3)*(right-left)/6.0f,gy=top+(g+3)*(bottom-top)/6.0f;ui.line(gx,top,gx,bottom,1,glm::vec4(dim.r,dim.g,dim.b,0.22f));ui.line(left,gy,right,gy,1,glm::vec4(dim.r,dim.g,dim.b,0.22f));}
        const auto point=[&](float wx,float wz){const float extent=hud.mapHalfExtent/std::max(hud.mapZoom,0.1f);return glm::vec2(0.5f*(left+right)+(wx-hud.mapPanX)/extent*0.5f*(right-left),0.5f*(top+bottom)-(wz-hud.mapPanZ)/extent*0.5f*(bottom-top));};
        for(int i=0;i<hud.islandCount;++i){glm::vec2 p=point(hud.islands[i].x,hud.islands[i].z);if(p.x<left||p.x>right||p.y<top||p.y>bottom)continue;ui.disc(p.x,p.y,4*s,glm::vec4(0.22f,0.45f,0.22f,1),glm::vec4(0.08f,0.18f,0.08f,1),10);if(hud.mapZoom>=1.45f)ui.text(p.x+5*s,p.y-3*s,"I",sc,ink);}
        const bool showMapNames=hud.mapZoom>=1.45f; // the world overview keeps its dense home waters legible; zooming reveals place names
        for(int i=0;i<hud.portCount;++i){glm::vec2 p=point(hud.ports[i].x,hud.ports[i].z);if(p.x<left||p.x>right||p.y<top||p.y>bottom)continue;ui.rect(p.x-3*s,p.y-3*s,6*s,6*s,hud.ports[i].known?glm::vec4(0.15f,0.65f,0.78f,1):glm::vec4(0.25f,0.20f,0.15f,1));if(!hud.ports[i].known||showMapNames)ui.text(p.x+5*s,p.y-3*s,hud.ports[i].known?hud.ports[i].label:"?",sc,hud.ports[i].known?TEXT:dim);}
        for(int i=0;i<hud.siteCount;++i){glm::vec2 p=point(hud.sites[i].x,hud.sites[i].z);if(p.x<left||p.x>right||p.y<top||p.y>bottom)continue;ui.disc(p.x,p.y,3*s,hud.sites[i].known?glm::vec4(0.82f,0.52f,0.12f,1):glm::vec4(0.30f,0.25f,0.18f,0.8f),glm::vec4(0.15f,0.10f,0.05f,1),8);if(!hud.sites[i].known||showMapNames)ui.text(p.x+4*s,p.y-3*s,hud.sites[i].known?hud.sites[i].label:"?",sc,hud.sites[i].known?ink:dim);}
        {glm::vec2 p=point(hud.targetX,hud.targetZ);ui.disc(p.x,p.y,6*s,glm::vec4(0.95f,0.62f,0.10f,1),glm::vec4(0.55f,0.18f,0.04f,1),12);ui.text(p.x+7*s,p.y-3*s,"D",sc,ink);}
        if(hud.treasureMapped){glm::vec2 p=point(hud.treasureX,hud.treasureZ);const float r=std::clamp(hud.treasureUncertainty*hud.mapZoom,5.0f,40.0f)*s;ui.disc(p.x,p.y,r,glm::vec4(0.70f,0.52f,0.10f,0.12f),glm::vec4(0.78f,0.55f,0.15f,0.35f),20);ui.text(p.x-2*s,p.y-3*s,hud.treasureExact?"X":"T",sc+1,ink);}
        if(hud.enemyKnown){glm::vec2 p=point(hud.enemyX,hud.enemyZ);ui.text(p.x-2*s,p.y-3*s,"E",sc+1,glm::vec4(0.95f,0.25f,0.16f,1));}
        {glm::vec2 p=point(hud.playerX,hud.playerZ);ui.tri(p.x,p.y-7*s,p.x-5*s,p.y+6*s,p.x+5*s,p.y+6*s,glm::vec4(0.96f,0.94f,0.78f,1));ui.text(p.x+7*s,p.y-3*s,"YOU",sc,ink);}
        ui.text(left,bottom+margin,"YOU  P PORT  I ISLE  S SITE  ? UNKNOWN  D GOAL  T TREASURE  E ENEMY",sc,ink);
    }

    // ---- the treasure counter, top centre: gold and chests found; it flashes when a chest is opened
    if (showHud && !hud.mapOpen && hud.treasureFlash > 0.01f) {
        char line[96];
        std::snprintf(line, sizeof(line), "+GOLD  TOTAL %d  CHEST %d/%d", hud.treasureGold, hud.treasureFound, hud.treasureTotal);
        const float w = UiBatch::textWidth(line, hs) + 4.0f * hm;
        const float x = static_cast<float>(windowWidth)-w-hm, y = hm;
        const float k = std::clamp(hud.treasureFlash, 0.0f, 1.0f);
        ui.rect(x,y,w,hChar+2*hm,glm::vec4(PANEL_BACK.r+0.20f*k,PANEL_BACK.g+0.14f*k,PANEL_BACK.b,0.55f));
        ui.text(x+2*hm,y+hm,line,hs,TEXT_GOLD);
    }
    if (showHud && !hud.mapOpen && hud.prompt[0] != 0) {
        const float cx = 0.5f * static_cast<float>(windowWidth), y = 0.62f * static_cast<float>(windowHeight);
        const float w = UiBatch::textWidth(hud.prompt, hs) + 4.0f * hm;
        ui.rect(cx - 0.5f * w, y - hm, w, hChar + 2.0f * hm, glm::vec4(PANEL_BACK.r, PANEL_BACK.g, PANEL_BACK.b, 0.42f));
        ui.textCentred(cx, y, hud.prompt, hs, TEXT);
    }

    // ---- the toast: bottom centre
    {
        const float cx = 0.5f * static_cast<float>(windowWidth);
        const float y = static_cast<float>(windowHeight) - charH - 3.0f * margin;
        if (showHud && !hud.mapOpen && hud.toastAlpha > 0.01f) {
            const glm::vec4 gold(TEXT_GOLD.r, TEXT_GOLD.g, TEXT_GOLD.b, hud.toastAlpha);
            const float tw = UiBatch::textWidth(hud.toast, hs) + 4.0f * hm;
            const float ty = y - 2.0f * hChar - 2.0f * hm;
            ui.rect(cx - 0.5f * tw, ty - hm, tw, hChar + 2.0f * hm, glm::vec4(PANEL_BACK.r, PANEL_BACK.g, PANEL_BACK.b, 0.42f * hud.toastAlpha));
            ui.textCentred(cx, ty, hud.toast, hs, gold);
        }
    }

    // ---- a banner when a ship has gone down
    if (showHud && hud.banner != nullptr && hud.banner[0] != '\0') {
        const int bs = sc + 2;
        const float bw = UiBatch::textWidth(hud.banner, bs) + 8.0f * margin;
        const float by = 0.30f * static_cast<float>(windowHeight);
        const float bh = 7.0f * static_cast<float>(bs) + 6.0f * margin;
        ui.rect(0.5f * (static_cast<float>(windowWidth) - bw), by, bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
        ui.textCentred(0.5f * static_cast<float>(windowWidth), by + 3.0f * margin, hud.banner, bs, TEXT_GOLD);
    }
}
