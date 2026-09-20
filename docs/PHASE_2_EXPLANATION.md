# Phase 2 - Shader Test and First Triangle

## Status

Phase 2 is complete and verified. Debug and Release builds succeeded, both shaders compiled, program `3` linked successfully, and a live Release run displayed the red/green/blue blended triangle without OpenGL errors. The loop remained near `120 FPS` with V-sync and closed normally.

The triangle is a temporary pipeline test. It is not one of the final project objects.

## What changed

This phase adds:

- `src/Shader.h`: reads, compiles, links, uses, and deletes a shader program;
- `shaders/basic.vert`: processes each vertex;
- `shaders/basic.frag`: chooses the final pixel colour;
- one VAO and one VBO for three temporary test vertices;
- one `glDrawArrays()` call in `renderScene()`;
- a CMake step that copies the shader folder beside the executable.

## Simple pipeline

```text
Triangle numbers in C++
        ↓
VBO stores the numbers on the GPU
        ↓
VAO explains the position/colour layout
        ↓
Vertex shader processes each corner
        ↓
OpenGL fills the triangle
        ↓
Fragment shader colours each visible pixel
```

## Vertex data

Each vertex contains six floating-point values:

```text
x, y, z, red, green, blue
```

The triangle therefore uses `3 vertices x 6 floats = 18 floats`.

The first three values go to shader location `0`. The last three values go to shader location `1`.

## Important changeable values

### Triangle position and colour

Edit `TriangleConfig::VERTICES` in `src/main.cpp`.

```cpp
-0.60f, -0.50f, 0.0f,   1.0f, 0.1f, 0.1f
```

The first three numbers are position. The next three are RGB colour values from `0.0` to `1.0`.

### Shader paths

`VERTEX_SHADER_PATH` and `FRAGMENT_SHADER_PATH` are in `AppConfig`. Normally they should remain:

```text
shaders/basic.vert
shaders/basic.frag
```

### Background colour

`AppConfig::CLEAR_COLOR` still controls the window background.

## Why the shader loader matters

Shader source is compiled by the graphics driver while the program runs. A spelling mistake can compile the C++ program successfully but break the GLSL shader.

The loader checks:

1. Could each file be opened?
2. Did the vertex shader compile?
3. Did the fragment shader compile?
4. Did both stages link into one program?

If a step fails, the driver's message is printed instead of continuing with a silent black screen.

## Likely teacher questions

### What is a vertex shader?

It runs once for each vertex. In this phase it places the vertex using `gl_Position` and passes its colour forward.

### What is a fragment shader?

It runs for visible fragments, which usually become pixels. It writes the final RGBA colour to `FragColor`.

### Why is `#version 330 core` used?

The project requests OpenGL 3.3 Core, so the shaders use the matching GLSL 330 Core language.

### What are VBO and VAO?

- VBO means Vertex Buffer Object. It stores vertex numbers on the GPU.
- VAO means Vertex Array Object. It remembers how those numbers are divided into attributes.

### What does `layout(location = 0)` mean?

It connects shader attribute `0` to the position layout configured by `glVertexAttribPointer(0, ...)` in C++.

### What coordinate system does the triangle use?

There are no transformation matrices yet, so the C++ position goes directly to `gl_Position`. Visible x and y values are normally between `-1` and `+1`; `(-1, -1)` is near the lower-left and `(1, 1)` is near the upper-right.

### Why is the stride six floats?

OpenGL must move past three position floats and three colour floats to reach the same field in the next vertex.

### Why does the colour blend across the triangle?

Each corner has a different colour. OpenGL interpolates `vColor` between the three vertices before the fragment shader receives it.

### What is shader linking?

Linking combines the compiled vertex and fragment stages into one program and checks that their inputs and outputs agree.

### Why delete the shader objects after linking?

The linked program already contains their compiled result. Keeping the separate stage objects would waste GPU resources.

### Why delete the VAO, VBO, and program before closing the window?

They are OpenGL resources and require a valid OpenGL context to be deleted safely.

### Why use `GL_STATIC_DRAW`?

The triangle data is uploaded once and does not change every frame.

### Why use `w = 1.0` in `vec4(aPosition, 1.0)`?

It marks the value as a position. Later, matrix translation will affect positions with `w = 1`.

## Simple viva modifications

- Move a corner: edit its `x` or `y` value in `VERTICES`.
- Change a corner colour: edit its RGB values.
- Make the triangle wider: decrease the left `x` and increase the right `x`.
- Turn the triangle upside down: move the top vertex below the other two.
- Change the background: edit `CLEAR_COLOR`.
- Demonstrate error handling: make a temporary GLSL spelling error, build/run, read the compile log, then undo the spelling error.

## Checkpoint

Phase 2 passes when:

- Debug and Release builds succeed;
- both shader files compile and link;
- the console prints the successful shader-program message;
- one red/green/blue blended triangle is visible;
- the window remains responsive and reports frame timing;
- closing the window releases all GPU resources cleanly.
