# Phase 3 - Uniforms: Sending Values Into the Shader

## Status

Phase 3 is complete and verified. Debug and Release builds succeeded, both
shaders compiled, program `3` linked, and a live Release run showed the
tinted triangle with no `uniform not found` warning. The loop stayed near
`120 FPS` with V-sync and closed normally.

The triangle is still the temporary Phase 2 test object. This phase changes how
its colour is decided, not what shape it is.

## What changed

| File | Change |
|---|---|
| `src/Shader.h` | Four uniform setters, plus a location lookup that warns once when a name is missing |
| `shaders/basic.frag` | A new `uniform vec3 uTint` that multiplies the interpolated colour |
| `src/main.cpp` | A named `AppConfig::TINT` value, sent every frame after `shader.use()` |

No new geometry, no new buffer, no new draw call.

## The one idea in this phase

There are two ways to get numbers into a shader. Phase 2 used the first one.
This phase adds the second.

| | Vertex attribute (Phase 2) | Uniform (Phase 3) |
|---|---|---|
| Set up by | `glVertexAttribPointer` | `glUniform*` |
| Stored in | The VBO, on the GPU | The shader program |
| Value | **Different for every vertex** | **The same for the whole draw call** |
| Changed by | Re-uploading the buffer | One function call, any frame |
| Example here | `aColor`, different at each corner | `uTint`, identical everywhere |

The triangle's three corner colours are attributes: each corner carries its own.
The tint is a uniform: one value for the entire triangle.

This matters because almost everything later in this project is a uniform. The
model, view, and projection matrices, the camera position, the light colours,
the material values, and the shading mode are all single values that apply to a
whole object and change every frame. Attributes could never do that job without
re-uploading the mesh each time.

## How the value travels

```text
AppConfig::TINT            a glm::vec3 in C++
        |
shader.use()               make this program the current one
        |
shader.setVec3("uTint", ...)
        |
glGetUniformLocation       ask the driver: where does "uTint" live?
        |
glUniform3fv               write 3 floats into that slot
        |
uniform vec3 uTint         the fragment shader reads it
        |
vColor * uTint             every pixel is multiplied
```

## Why `use()` must come first

OpenGL does not ask which program you mean. It writes the uniform into
**whichever program is currently in use**.

```cpp
shader.use();                              // correct order
shader.setVec3("uTint", AppConfig::TINT);
```

Reversing those two lines sends the value to whatever program happened to be
bound before, which is usually program `0`, meaning nowhere. Nothing crashes and
nothing is printed. The triangle simply keeps its old colour. This is worth
trying once on purpose so the symptom is familiar.

## Why a missing uniform is dangerous

`glGetUniformLocation` returns `-1` when it cannot find the name, and OpenGL
then **silently ignores** the write. There is no error and no crash.

Two things cause it:

1. The name is spelled differently in the shader than in the C++ string.
2. The uniform is declared in the shader but never actually used, so the GLSL
   compiler removed it as dead code.

The second one surprises people. Declaring `uniform vec3 uTint;` is not enough.
If no line reads it, it will not exist in the linked program.

Because the setter runs every frame, printing a warning every time would fill
the console about 120 times per second. `Shader.h` therefore remembers which
names it already complained about:

```cpp
std::vector<std::string> m_missingUniforms;
```

## The four setters

| Setter | OpenGL call | First used |
|---|---|---|
| `setInt` | `glUniform1i` | Later, for mode switches such as `uShadingMode` |
| `setFloat` | `glUniform1f` | Later, for single values such as shininess |
| `setVec3` | `glUniform3fv` | **This phase**, for `uTint` |
| `setMat4` | `glUniformMatrix4fv` | **Phase 4**, for the model matrix |

All four are added now because they are the same idea four times, and writing
them together means `Shader.h` does not need reopening in Phase 4.

`glm::value_ptr(value)` hands OpenGL the address of the first float in the glm
object. A `glm::vec3` is three floats in a row and a `glm::mat4` is sixteen, so
OpenGL can read them directly with no conversion.

The `GL_FALSE` in `setMat4` is the transpose flag. glm already stores matrices
in the column order OpenGL expects, so no transpose is needed.

## Why `renderScene` lost its `const`

```cpp
static void renderScene(ShaderProgram& shader, const TriangleGpu& triangle)
```

It used to take `const ShaderProgram&`. Setting a uniform changes the shader
program and records any missing-name warnings, so the function can no longer
promise to leave the shader unchanged. Removing `const` is the honest
description of what it does.

## Important changeable values

### The tint

`AppConfig::TINT` in `src/main.cpp`:

```cpp
const glm::vec3 TINT(3.0f, 2.4f, 0.6f);
```

Each channel multiplies the matching colour channel:

| Tint value | Effect on that channel |
|---|---|
| `1.0` | No change |
| Below `1.0` | Dimmer |
| `0.0` | Removed completely |
| Above `1.0` | Brighter, up to the display limit of `1.0` |

The current triangle has very dark corner colours of `0.1`. Multiplying a dark
colour can only make it darker, so the tint is set above `1.0` to brighten it
instead. That is why the values are `3.0, 2.4, 0.6` rather than something near
`1.0`.

With the current vertex colours the result is:

| Corner | Vertex colour | After tint |
|---|---|---|
| Two top corners | `(0.1, 0.1, 0.1)` | `(0.30, 0.24, 0.06)` warm brown |
| Bottom point | `(0.1, 0.1, 1.0)` | `(0.30, 0.24, 0.60)` violet |

### Everything from earlier phases

`CLEAR_COLOR`, `WINDOW_WIDTH`, `WINDOW_HEIGHT`, `REPORT_INTERVAL`,
`VSYNC_INTERVAL`, `MAX_DELTA_TIME`, and `TriangleConfig::VERTICES` all still
work exactly as before.

## Likely teacher questions

### What is a uniform?

A value set from C++ that stays the same for every vertex and every pixel in a
draw call. It is how the program talks to a shader while it is running.

### How is a uniform different from a vertex attribute?

An attribute is stored per vertex inside the VBO and is different at each
corner. A uniform is stored in the shader program and is one value for the whole
draw call.

### Why not just edit the shader file instead?

The shader is compiled by the graphics driver when the program starts. Changing
a value in the shader file means restarting the program. A uniform can change
every frame, which is what animation, camera movement, and lighting all need.

### Why must `glUseProgram` be called before `glUniform`?

Because `glUniform` writes into the currently bound program. With no program
bound the value goes nowhere, silently.

### What does `glGetUniformLocation` return, and what does `-1` mean?

It returns the slot number of that uniform inside the linked program. `-1` means
the name was not found, either because it is misspelled or because the shader
never uses it and the compiler removed it.

### Why does the code remember which uniforms were missing?

The setter is called every frame. Without that list the warning would print
around 120 times per second and hide everything else in the console.

### What does `glm::value_ptr` do?

It returns a pointer to the first float inside the glm object so OpenGL can read
the values directly, with no copying or conversion.

### Why is the `3fv` version used instead of `glUniform3f`?

`glUniform3f` takes three separate floats. `glUniform3fv` takes a pointer to
three floats in a row, which is exactly what a `glm::vec3` already is.

### Why is the tint multiplied rather than added?

Multiplying behaves like a filter: it scales each channel. A tint of `1.0`
leaves the colour untouched, which makes it easy to reason about. Adding would
brighten black areas that should stay black.

### Why is the tint bigger than 1.0?

The triangle's vertex colours are very dark. Multiplication can only reduce a
value when the multiplier is below `1.0`, so brightening a dark colour requires
a multiplier above `1.0`.

### What happens if a colour goes above 1.0?

The display cannot show more than full brightness, so OpenGL clamps it to `1.0`.
This becomes important in the lighting phases, where several light
contributions are added together and can easily exceed `1.0`.

### Why does the fragment shader do the multiplication and not the vertex shader?

Either would work here, because the tint is constant. It is in the fragment
shader because that is where the final colour is decided. In Phase 31 the choice
of which stage does the lighting maths becomes the whole Gouraud versus Phong
comparison.

### Is `setMat4` unused right now?

Yes. It exists so that Phase 4 can send the model matrix without reopening
`Shader.h`. It is three lines and it is the same idea as the other three
setters.

## Simple viva modifications

- Remove the tint: set `TINT` to `(1.0f, 1.0f, 1.0f)` and the triangle returns
  to its dark vertex colours.
- Make it cool instead of warm: set `TINT` to `(0.6f, 2.4f, 3.0f)`.
- Remove one channel: set the red component to `0.0f`.
- Demonstrate the silent failure: change `"uTint"` in `main.cpp` to `"uTints"`,
  rebuild, and read the single warning line while the triangle keeps its old
  colour.
- Demonstrate the dead-code removal: keep the name correct but comment out the
  line in `basic.frag` that uses `uTint`, rebuild, and get the same warning.
- Demonstrate the ordering rule: move `shader.setVec3(...)` above `shader.use()`
  and watch the tint stop working with no error message.

## Checkpoint

Phase 3 passes when:

- Debug and Release builds succeed;
- both shaders still compile and link;
- the triangle is visibly tinted rather than near black;
- changing `AppConfig::TINT` and rebuilding changes the colour, with no edit to
  `shaders/basic.frag`;
- no `uniform 'uTint' not found` warning appears during a normal run;
- misspelling the uniform name produces exactly one warning line, not one per
  frame;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 3

No matrices, no camera, no 3D, no meshes, and no movement. The triangle still
goes straight to `gl_Position` in clip space exactly as it did in Phase 2.

Phase 4 uses `setMat4` for the first time and gives the triangle a model matrix
so it can be moved.
