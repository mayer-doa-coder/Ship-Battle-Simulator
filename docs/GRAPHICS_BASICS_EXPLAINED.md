# The Basics Behind This Project

This is written for someone with **no prior knowledge** of computer
graphics. Every topic is explained twice: first **in plain words**, with
an everyday comparison and no jargon at all, and then **more precisely**,
using the real words this field uses and real code from this project. Read
the plain-words part first. Only move to the precise part once the plain
idea makes sense - the precise part will make a lot more sense once it
does.

This document has two parts:

- **Part One** (below) explains the four ideas underneath everything in
  this project - triangles, the camera, moving things in space, and
  vertices and indices - without tying them to any one specific file.
- **Part Two** walks through **every single file in this project, one at a
  time** - what it is for, and what every function and every piece of data
  inside it does - so nothing is left unexplained. It leans on Part One
  throughout, so reading Part One first will make Part Two much faster to
  follow.

---

# Part One: The Underlying Ideas

## Before anything else: what is actually happening on your screen?

### In plain words

Your screen is a huge grid of tiny lights (pixels) that can each be any
colour. A picture - any picture, a photo, a cartoon, this game - is just
"which colour is each of these thousands of tiny lights right now."

A drawing program does not know what a "ship" or a "person" is. It only
knows one trick, over and over, extremely fast: **fill in a triangle with
a colour.** Everything you see in this project - the ship, the crew, the
mountains, the sun - is secretly hundreds of tiny triangles, each filled in
with a colour, sitting next to each other so closely that your eye reads
them as one solid shape.

A useful comparison: think of a stained-glass window. From a distance it
looks like a picture. Up close, it is actually hundreds of separate pieces
of coloured glass, each one a flat, straight-edged shape, held together by
strips of lead. Nobody bent the glass into a curve - the curve is an
*illusion* made of enough small flat pieces. That is exactly what a curved,
round-looking object like the ship's wheel or the crew's head is, in this
project: not actually smooth, just made of enough small flat triangles that
it reads as round from a normal distance.

### More precisely

The computer's graphics chip (the **GPU**) can only draw a small number of
basic things, and a **triangle** is the main one it is built to fill in
with colour, incredibly fast, thousands of times a frame. Every object in
this project - built in [src/Mesh.h](../src/Mesh.h), [src/Ship.h](../src/Ship.h),
[src/Crew.h](../src/Crew.h), and the other shape files - is, underneath,
nothing but a list of triangles.

Describing an object means answering two separate questions, over and
over, for every triangle:

1. **Where are its three corners, and what colour is each one?** (Section 1
   and Section 4 below.)
2. **Given where the object and the camera both are, where does each
   corner end up on the flat screen?** (Section 2 and Section 3 below.)

Everything else in this document is really just filling in the details of
those two questions.

---

## 1. Triangles and basic shapes

### In plain words

Take three fingers and touch their tips together on a table so they make a
triangle shape. That is all a triangle is: three points, connected by three
straight lines, with the space between them filled in.

Now here is the useful trick: **you can build any shape at all out of
enough triangles.** A cube is 12 triangles (2 for each of its 6 flat
faces). A ball is dozens of small triangles arranged in a sphere - it is
never actually round, it just has so many small flat faces that it looks
round, the same stained-glass idea from above. A person's head in this
project is the exact same trick, at the exact same scale as a ball, just
tinted a skin colour.

The three corners of a triangle do not have to be the same colour. If you
touch one fingertip with a red pen, one with a green pen, and one with a
blue pen, and then imagine the colour smoothly blending across your hand
between them, that is exactly what the graphics chip does automatically -
it blends the three corner colours smoothly across the whole triangle.

### More precisely

Every shape in this project is a list of corners (called **vertices** -
more on those in Section 4) plus a list telling the computer which three
vertices to connect to form each triangle. [src/Mesh.h](../src/Mesh.h)'s
`makeCube()` is a good first example to open and read: it lists 24 corners
and groups them, six at a time, into the cube's six flat, square faces
(each square being two triangles).

Two shapes this project reuses constantly, both built the exact same way -
a list of corners plus a list of triangles - are the **cylinder**
(`makeCylinder()`) and the **sphere** (`makeSphere()`). A cylinder is a
ring of corners repeated at the top and the bottom, with triangles filling
the gap between them - that is a mast, a cannon barrel, or a wheel spoke in
this project, just scaled to a different size and thickness each time. A
sphere is rings of corners stacked from one pole to the other - that is a
head, a cannonball, a porthole, or the sun. Nothing about the CODE differs
between a giant sun and a tiny cannonball; only the SIZE they are drawn at
differs, which is Section 3's subject.

---

## 2. The camera, and how "looking at something" actually works

### In plain words

There is no actual camera anywhere in this program - no lens, no sensor.
"The camera" is really just one question, asked fresh for every single
object, every single frame: **"if a person were standing at this exact
spot, facing this exact direction, where would this object appear in front
of them?"**

Think about closing one eye and holding your thumb up at arm's length to
"cover" a distant building. Your thumb has not moved and the building has
not moved - but from YOUR particular standing spot, looking in YOUR
particular direction, they happen to line up. Moving your head two steps to
the left completely changes that lineup, even though neither your thumb nor
the building moved an inch. That is the entire idea of a camera in a
computer graphics program: nothing about the scene changes when the camera
moves - only the answer to "where does this appear from here" changes.

This project's camera is specifically an **orbit camera**: instead of
walking around freely, it always stays pointed at one fixed spot (the
ship, or whichever single object you are currently inspecting) and only
lets you circle around that spot and step closer or farther away - the
same way you might slowly walk in a circle around a car in a showroom,
always facing it, to see every side.

### More precisely

An orbit camera is described with just three numbers, the same three
numbers you would use to describe a satellite circling a planet:

- **radius** - the distance from the thing you're looking at;
- **yaw** - the angle turned left or right around it;
- **pitch** - the angle tilted up or down toward it.

[src/Camera.h](../src/Camera.h) turns those three numbers into an actual
`(x, y, z)` position with this exact formula:

```cpp
x = radius * cos(pitch) * sin(yaw)
y = radius * sin(pitch)
z = radius * cos(pitch) * cos(yaw)
```

This is the same "angle to position" idea a clock's hour hand uses to trace
a circle, just extended from a flat circle to a sphere (one angle for
"around," one angle for "up and down").

Two more matrices finish the job of "where does this object end up on the
flat screen":

- The **view** matrix re-measures the whole world as if the camera's
  position were the centre of everything, the same way the thumb-and-
  building example above only makes sense measured from YOUR eye.
- The **projection** matrix does the "far away things look smaller" trick a
  real eye or a real camera does automatically, called **perspective**. It
  also decides how wide a field of view the camera has, and cuts off
  anything closer than its near limit or farther than its far limit.

Every object drawn in [src/main.cpp](../src/main.cpp)'s `renderScene()`
gets multiplied by both of these same two matrices before it reaches the
screen, which is exactly why moving the ONE camera changes how ALL the
objects in the scene look, all at once.

---

## 3. Moving things in space: translation, rotation, and scaling

### In plain words

Imagine you are holding a small toy boat in your hand. There are exactly
three different things you can do to it without changing its actual
shape:

- **slide it** across the table (translation - moving it);
- **turn it** on the spot, like a steering wheel (rotation - spinning it);
- **grow or shrink it**, like a photo you are resizing (scaling).

Here is the part that surprises almost everyone the first time: **the
order you do these in changes the result.** Picture the toy boat sitting
off to one side of the table, not in the middle.

- If you **spin it first, then slide it** sideways, it spins on the spot
  exactly where it started, and only afterward slides over - it ends up
  facing wherever it happened to be facing when the spin finished, sitting
  in a new spot.
- If you **slide it first, then spin it**, you have moved it away from the
  centre BEFORE spinning - so the spin now swings the whole boat around
  the table's centre in a wide circle, like a compass needle, instead of
  turning it on the spot.

Both are "the same two actions," done in a different order, with visibly
different results. This project's own Phase 6 demonstrated exactly this,
on purpose, with the very first triangle - pressing a key rebuilt the same
three moves in the opposite order and made the triangle visibly smear
through a wide loop instead of calmly spinning and pulsing in place.

### More precisely

Each of the three actions above - translate, rotate, scale - is written as
a small grid of numbers called a **matrix**. Combining several of them into
one matrix is done by multiplying them together, and multiplication is
read from **right to left**: the rightmost matrix is applied to the object
FIRST.

The correct order for "resize it, then turn it, then move it to its final
spot" is `Translate * Rotate * Scale`, often written **T \* R \* S**. Here
is a real example from this project, building the ship's hull, from
[src/Ship.h](../src/Ship.h):

```cpp
const glm::mat4 hullFrame =
    glm::translate(shipRoot, glm::vec3(0.0f, HULL_HEIGHT * 0.5f, 0.0f));
drawTintedPart(
    shader, hullShape,
    glm::scale(hullFrame, glm::vec3(HULL_WIDTH, HULL_HEIGHT, HULL_LENGTH)),
    palette.hull);
```

Reading `glm::scale(hullFrame, ...)` right to left: the hull shape is
scaled to its real size FIRST (while still centred on the origin, where
its corners are measured from), and the already-correctly-sized hull is
moved into position SECOND, by `hullFrame`. Scaling AFTER moving would
instead stretch the hull away from the ship's centre in the wrong
direction - the exact "spin around the table" mistake from the plain-words
example above, just with resizing instead of spinning.

Rotation shows up the same way, for example turning the cannon's barrel to
point sideways instead of standing straight up:

```cpp
const glm::mat4 barrelFrame = yokeFrame
    * glm::translate(glm::mat4(1.0f), glm::vec3(BARREL_LENGTH * 0.5f, 0.0f, 0.0f))
    * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

Right to left: **rotate** the barrel on the spot first (so it points
sideways instead of up), **then translate** the now-sideways barrel
outward from the yoke by half its own length. Rotating AFTER moving would
instead swing the barrel's far end around in an arc, the same mistake
again.

One more detail worth knowing, since it trips people up the first time
they see it: every position in this project is stored as **four** numbers,
not three - `(x, y, z, w)` - and `w` is always either exactly `1` (for a
position - a specific point in space) or exactly `0` (for a direction -
"which way," with no particular location). Moving only makes sense for a
position, never for a direction, and this `w` value is precisely what
switches that behaviour on or off inside the matrix multiplication.

---

## 4. Vertices and indices

### In plain words

Picture a children's dot-to-dot puzzle book: a page full of numbered dots,
and instructions saying "draw a line from dot 1 to dot 2, then dot 2 to
dot 3," and so on, until a picture appears. Two separate ingredients make
that puzzle work:

- **the dots themselves** - where each one is on the page, and (in a
  colouring version) what colour crayon is sitting next to it;
- **the numbered order** - which dots to actually connect, and in what
  sequence.

A 3D shape in this project is built exactly the same way. The "dots" are
called **vertices**, and the "connect-the-dots order" is called
**indices**.

The genuinely useful part of this system: a single dot can be reused
Many times over, in many different lines, without ever needing to be drawn
twice. A square table's four corners belong to two different triangles (if
you cut the square along a diagonal), but each corner still only needs to
be marked on the page ONCE - the instructions just mention that same dot
twice.

### More precisely

[src/Mesh.h](../src/Mesh.h) defines exactly what one vertex is:

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};
```

`position` is the dot's location in 3D space (three numbers: how far along
X, Y, and Z). `color` is the crayon sitting next to it (three numbers: how
much red, green, and blue). Every shape in this project - the cube, the
cylinder, the sphere, and everything built from them - is nothing more
than a `std::vector<Vertex>` (a list of these dots) plus a
`std::vector<unsigned int>` (a list of plain numbers - the connect-the-dots
order, three numbers at a time, one triangle per group of three).

A flat square is the simplest real example. It only needs 4 corners, not
6, even though it is made of 2 triangles (2 triangles x 3 corners each
would be 6, if every corner had to be listed separately for every triangle
that uses it):

```cpp
const std::vector<Vertex> vertices = {
    { {-0.4f,  0.4f, 0.0f}, {1.0f, 0.0f, 0.0f} },   // 0: top-left,     red
    { { 0.4f,  0.4f, 0.0f}, {0.0f, 1.0f, 0.0f} },   // 1: top-right,    green
    { { 0.4f, -0.4f, 0.0f}, {0.0f, 0.0f, 1.0f} },   // 2: bottom-right, blue
    { {-0.4f, -0.4f, 0.0f}, {1.0f, 1.0f, 0.0f} },   // 3: bottom-left,  yellow
};

const std::vector<unsigned int> indices = {
    0, 3, 2,   // first triangle:  top-left, bottom-left, bottom-right
    2, 1, 0,   // second triangle: bottom-right, top-right, top-left
};
```

(This exact quad is where Phase 9 of this project's own step-by-step build,
[docs/PHASE_9_EXPLANATION.md](PHASE_9_EXPLANATION.md), first introduced
indexed drawing - it originally used a flat array of numbers rather than
the `Vertex` struct shown here, since `Vertex` did not exist yet at that
point in the build, but the four corners and the six connect-the-dots
numbers are the same.)

Corner 0 (top-left) and corner 2 (bottom-right) each get used by BOTH
triangles - they are only written out once, in `vertices`, and simply
mentioned twice, by number, in `indices`. That reuse is the entire reason
this system exists: a real shape in this project (a cylinder with 32
segments, say) shares far more corners between neighbouring triangles than
a small square does, and writing each shared corner out only once instead
of three or four times keeps the amount of data the GPU has to store far
smaller.

This is also exactly what `Mesh`'s constructor
([src/Mesh.h](../src/Mesh.h)) takes as its two arguments - a list of
vertices and a list of indices - because those two lists are a complete,
sufficient description of any shape at all, from a flat quad to a whole
ship's hull.

---

## How the four ideas fit together

Every single object drawn in this project, from the tiniest porthole to
the whole ship, is the result of the same four ideas working together:

1. A list of **vertices and indices** describes its raw shape - flat
   corners, connected into triangles, in its own unmoved, unsized,
   default position (Sections 1 and 4).
2. A **translate/rotate/scale** matrix, built in the right order, decides
   where that shape actually sits in the world, which way it is turned,
   and how big it really is (Section 3).
3. The **camera's** view and projection matrices decide how that
   positioned, real-sized shape looks from wherever the camera currently
   is (Section 2).
4. A colour (sent as `uTint`) decides what shade it is drawn in - the same
   plain white cube, cylinder, or sphere becomes a brown hull, a grey
   cannon, or a red flag, depending only on this one value at the moment
   it is drawn.

Every object in [src/Ship.h](../src/Ship.h), [src/Crew.h](../src/Crew.h),
[src/Props.h](../src/Props.h), and [src/Scenery.h](../src/Scenery.h) is
just these four ideas, repeated once per part.

---

# Part Two: A Guided Tour of Every File

Part One explained the ideas. This part walks through **every file in
[src/](../src/) and [shaders/](../shaders/), plus the build file**, one at
a time, saying what it is for and what every function or block of data
inside it does. Nothing is skipped - if a file exists in this project, it
is explained here.

## The map, before the tour

| File | Its one job |
|---|---|
| [CMakeLists.txt](../CMakeLists.txt) | The recipe for turning all the `.cpp`/`.c` files into one runnable program |
| `src/glad.c` | A vendored (ready-made, not hand-written) file that finds and connects every OpenGL function the graphics driver offers |
| [shaders/basic.vert](../shaders/basic.vert) | Runs on the GPU, once per corner: decides where that corner ends up on screen |
| [shaders/basic.frag](../shaders/basic.frag) | Runs on the GPU, once per pixel: decides that pixel's final colour |
| [src/Shader.h](../src/Shader.h) | Loads the two files above onto the GPU, and sends values into them while the program runs |
| [src/Mesh.h](../src/Mesh.h) | Defines what a "corner" is, owns a shape's GPU data, and builds every basic shape (cube, cylinder, sphere, and two special ship-related shapes) |
| [src/Camera.h](../src/Camera.h) | Turns mouse drags and scrolls into a camera position and hands that position to whoever asks for it |
| [src/Ship.h](../src/Ship.h) | Builds one entire ship - hull, deck, masts, cannon, wheel, and more - out of Mesh.h's basic shapes |
| [src/Crew.h](../src/Crew.h) | Builds one crew member's body out of Mesh.h's basic shapes |
| [src/Props.h](../src/Props.h) | Builds a barrel, a crate, and a cannonball |
| [src/Scenery.h](../src/Scenery.h) | Builds a mountain, a cloud, and a rock |
| [src/main.cpp](../src/main.cpp) | The conductor: opens the window, decides where everything in the scene sits, reads the keyboard and mouse, and runs the loop that draws every frame |
| `src/output/` | Empty. A leftover folder with nothing in it - not part of the program. |

---

## `CMakeLists.txt` - the build recipe

### In plain words

Before you can run a C++ program, all its separate `.cpp` files have to be
turned into one program a computer can actually double-click and run - a
step called **building**. `CMakeLists.txt` is not part of the program
itself; it never runs. It is a recipe card telling a separate tool
(**CMake**) which files to combine, which outside libraries to attach, and
where to put the result - the same way a recipe card is not the cake, it
is the instructions for making one.

### More precisely

```cmake
add_executable(ship_battle_simulator
    src/main.cpp
    src/glad.c
)
```

This says: build one program, called `ship_battle_simulator`, out of these
two files. (Every `.h` file in `src/` is included FROM `main.cpp` with
`#include`, so it does not need to be listed separately here - `#include`
is explained in the `main.cpp` section below.)

The rest of the file:

- `set(CMAKE_CXX_STANDARD 17)` - use the 2017 version of the C++ language
  rules for the whole project.
- `find_package(OpenGL REQUIRED)` and the `GLFW_LIBRARY`/`glfw3` lines -
  find and attach two outside libraries this project depends on: **OpenGL**
  itself (the graphics system) and **GLFW** (which opens the window and
  reads the mouse and keyboard - explained more in the `main.cpp` section).
  On Windows, GLFW's already-built files are bundled directly into this
  project ("vendored") so nothing extra has to be installed; on Linux, it
  expects the system to already have GLFW installed.
- The final `add_custom_command(...)` block copies the `shaders/` folder
  next to the finished program every time it is built, because the shader
  files ([shaders/basic.vert](../shaders/basic.vert) and
  [shaders/basic.frag](../shaders/basic.frag)) are read from disk while the
  program is running, not baked into it while building.

---

## `src/glad.c` - the phrasebook for talking to the graphics card

### In plain words

Different computers have different graphics cards, from different
manufacturers, each running its own driver software. Not every driver
supports every single OpenGL feature, and even the ones that do do not
all keep those features in the same predictable spot in memory. **GLAD**
is a tool that was run ONCE, ahead of time (not by you, and not while this
program runs), to generate a big file that knows how to go and find each
OpenGL function on whatever computer the program actually runs on, and
make it available under one plain, reliable name.

This is exactly like carrying a phrasebook someone else already wrote:
you do not need to know Finnish to ask for directions in Finland if you
have a phrasebook that translates your question and reads back the
answer - you just trust the phrasebook and use the plain words you already
know.

### More precisely

`src/glad.c` is machine-generated, not hand-written, and it is never
opened or edited in this project. `main.cpp` calls exactly one function
from it, near the very start of the program:

```cpp
if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
    std::fprintf(stderr, "Failed to load OpenGL functions with GLAD.\n");
    ...
}
```

This one call is what makes every other `gl...` function used throughout
this project (`glClear`, `glDrawElements`, `glGenBuffers`, and dozens more)
actually work. Calling any of them before this line would fail, because
their real addresses have not been looked up yet.

---

## `shaders/basic.vert` - deciding where a corner ends up

### In plain words

This file's job happens once for every single corner (vertex) of every
shape, every frame, and it runs **on the graphics card itself**, not on
the computer's main processor. It answers one question: "given this
corner's position in the 3D world, and where the camera currently is,
where does this corner actually land on the flat screen?" This is exactly
Section 2 and Section 3 of Part One, written out as real, running code.

### More precisely

```glsl
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
```

`aPosition` and `aColor` are this corner's own two pieces of data - exactly
`Vertex::position` and `Vertex::color` from [src/Mesh.h](../src/Mesh.h),
handed to this file one corner at a time. The `location` numbers are what
`Mesh::create()` refers to when it calls `glVertexAttribPointer(0, ...)`
and `glVertexAttribPointer(1, ...)` - `0` is "this is `aPosition`," `1` is
"this is `aColor`."

```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
```

These are the three matrices from Part One, Sections 2 and 3 - "where is
this object, turned which way, and how big" (`uModel`), "re-measured from
the camera" (`uView`), and "flattened with perspective" (`uProjection`).
A `uniform` is a value set once from the C++ side (by
[src/Shader.h](../src/Shader.h)'s `setMat4()`) that stays exactly the same
for every corner in one draw call - unlike `aPosition`, which is different
for every corner.

```glsl
void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vColor = aColor;
}
```

This is the entire job: combine the three matrices with this corner's own
position (read right to left, exactly as Part One, Section 3 explained -
`uModel` first, then `uView`, then `uProjection`), and hand the result to
`gl_Position`, a special name the GPU itself watches for. `vec4(aPosition, 1.0)`
is where `w = 1` from Part One's Section 3 shows up in real code - `aPosition`
is only three numbers, and a `1.0` is joined on as the fourth, marking it
as a position rather than a direction. `vColor = aColor` just passes this
corner's colour onward to the next file, unchanged.

---

## `shaders/basic.frag` - deciding one pixel's final colour

### In plain words

If the vertex shader's job is "where do the corners go," the fragment
shader's job is "now that the shape has been filled in, what colour is
THIS particular pixel, right here in the middle of it?" This runs once for
every pixel the shape covers on screen - far more often than the vertex
shader, which only runs once per corner.

### More precisely

```glsl
in vec3 vColor;
uniform vec3 uTint;

void main()
{
    FragColor = vec4(vColor * uTint, 1.0);
}
```

`vColor` arrives already smoothly blended between the triangle's three
corners' colours - the GPU does that blending automatically between the
vertex shader and this one, which is exactly the "blend the colour across
your hand" example from Part One, Section 1. `uTint` is the colour filter
from Part One's closing summary - the same value every `drawTintedPart()`
call in [src/Ship.h](../src/Ship.h) and every other draw call in this
project sets before drawing. Multiplying the two together is what turns a
plain white mesh into a brown hull, a red flag, or a grey cannon, chosen
fresh every time something is drawn, without ever touching this file.

---

## `src/Shader.h` - the `ShaderProgram` class

### In plain words

This file is a small, careful librarian for the two files above. Before a
shader can be used, its text has to be read from disk, handed to the
graphics driver to be checked and compiled (the same idea as compiling any
program - turning readable source code into something the machine can
run), and the two halves (vertex and fragment) have to be linked together
into one finished, usable program. If any step goes wrong - a typo, a
missing file - `ShaderProgram` prints exactly what the graphics driver said
was wrong, instead of just showing a blank black screen and leaving you to
guess.

### More precisely

`loadFromFiles(vertexPath, fragmentPath)` does the whole job in order:

1. `readTextFile()` (private) opens each file and reads its whole text
   into a `std::string`, failing with a clear message if the file is
   missing or empty.
2. `compileShader()` (private, called once per file) hands that text to
   `glCreateShader` + `glShaderSource` + `glCompileShader`, then checks
   `GL_COMPILE_STATUS` and calls `printShaderLog()` to print the driver's
   own error text if anything went wrong.
3. `glCreateProgram()`, `glAttachShader()` (twice), and `glLinkProgram()`
   combine the two compiled shaders into one program the GPU can actually
   run, and `printProgramLog()` prints any linking error the same way.
4. The two individual shader objects are deleted right after linking
   (`glDeleteShader`) - once linked into the program, they are no longer
   needed on their own.

`use()` tells OpenGL "every draw call from now on should use THIS
program" - it must be called before any of the uniform setters below, or
they would be writing into whichever program happened to be active
already.

The four uniform setters - `setInt`, `setFloat`, `setVec3`, `setMat4` -
each do the same one thing for a different type of value: look up where a
named uniform (like `"uModel"` or `"uTint"`) lives inside the compiled
program, and hand OpenGL a new value for it. The private helper they all
share, `uniformLocation()`, is worth reading closely:

```cpp
GLint uniformLocation(const char* name)
{
    const GLint location = glGetUniformLocation(m_id, name);

    if (location == -1) {
        const bool alreadyWarned = ...;
        if (!alreadyWarned) {
            m_missingUniforms.push_back(name);
            std::fprintf(stderr, "[shader] uniform '%s' not found or unused...\n", name);
        }
    }
    return location;
}
```

A misspelled uniform name fails **silently** in OpenGL by default - the
value just goes nowhere, with no error at all, which is one of the most
confusing bugs in graphics programming for a beginner. This function
prints a warning the FIRST time a given name comes up missing, and
remembers it in `m_missingUniforms` so the exact same warning does not
print again on every single frame (which would be about 120 times a
second).

`valid()` reports whether a program was successfully created at all.
`destroy()` deletes the GPU program and clears the missing-uniform list (a
freshly loaded program has fresh uniform locations, so old warnings no
longer apply). The destructor calls `destroy()` automatically, and the
copy constructor and copy-assignment operator are both `= delete`d for the
same reason `Mesh` (below) deletes them: copying would let two C++ objects
both believe they own, and both try to delete, the same GPU program.

---

## `src/Mesh.h` - the shape of everything

### In plain words

This is the file that answers Part One's Section 4 for real: "what is a
corner, and how does a whole list of them become an actual shape sitting
on the graphics card, ready to draw?" It also contains every "basic shape
recipe" this project uses - the cube, the cylinder, the sphere, and two
more specialised shapes invented for this project's ship and mountains.

### `struct Vertex` - one corner

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};
```

Exactly Part One's Section 4: one dot, with a location and a crayon colour
attached. Nothing more complicated than that.

### `class Mesh` - one shape, owned safely

**In plain words:** think of `Mesh` as a small safe-deposit box for one
shape's data. Once you put a shape's corners and connect-the-dots list
into it, it locks the box, hands the data to the graphics card, and keeps
track of the claim ticket needed to get it back out later (to draw it, or
to throw it away when it is no longer needed). The box refuses to be
copied - only one claim ticket can exist per shape, or two boxes might
both try to throw the same data away and the program would crash.

**More precisely**, `Mesh` owns three GPU handles:

- `m_vao` (Vertex Array Object) - remembers HOW to read the vertex data
  (which numbers are `position`, which are `color`, and how far apart they
  sit in memory).
- `m_vbo` (Vertex Buffer Object) - the actual list of vertices, sitting in
  the graphics card's own memory.
- `m_ebo` (Element Buffer Object) - the actual list of indices, also on the
  graphics card.

The constructor `Mesh(vertices, indices)` calls the private `create()`
function, which does, in order:

1. `glGenVertexArrays` / `glGenBuffers` (twice) - ask the driver for three
   empty handles.
2. Bind the VAO, then upload `vertices` into the VBO with `glBufferData`,
   and `indices` into the EBO the same way.
3. Two calls to `glVertexAttribPointer` + `glEnableVertexAttribArray` tell
   the VAO "position data starts here, is 3 floats, and repeats every
   `sizeof(Vertex)` bytes" (and the same for colour, starting at a
   different offset inside each `Vertex`). `offsetof(Vertex, position)`
   and `offsetof(Vertex, color)` ask the compiler for those exact byte
   offsets rather than hand-counting them.

`draw()` is deliberately tiny:

```cpp
void draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
```

Bind this shape's own remembered layout, tell the GPU to draw
`m_indexCount` indices' worth of triangles using it, then unbind. Every
single object in this whole project - the ship, a crew member's eyeball, a
cloud - eventually calls exactly this function.

`valid()` reports whether all three handles were actually created.
`destroy()` deletes all three (each guarded by `if (handle != 0)`, so
calling it twice, or on a `Mesh` that was never filled in, is always
safe) and resets them to `0`.

The destructor, copy constructor, copy-assignment, move constructor, and
move-assignment together handle **who owns the GPU data and when it gets
freed** - this is the same idea Part One's `ShaderProgram` section
explained: copying is `= delete`d so two `Mesh` objects can never both
believe they own (and both try to delete) the same handles, while
`Mesh(Mesh&& other)` (a **move**) is allowed, and instead **transfers**
ownership - which is exactly what makes `Mesh cube = makeCube();` work:
`makeCube()` builds a `Mesh` locally and returns it, and the move
constructor lets `cube` become the new sole owner of that same shape's
data without ever copying it.

### The shape-generator functions

Each of these builds one specific shape by filling a `std::vector<Vertex>`
and a `std::vector<unsigned int>` and handing them to `Mesh`'s constructor.
All of them are also described from the "why does this shape look the way
it does" angle in Part One's Section 1 - here is what each one's code
actually does.

**`makeCube()`** - a rainbow-coloured cube, 24 vertices (4 per face, so
each of the 6 faces can have its own single colour - a corner SHARED
between three faces could only ever be one colour, which is why this is
24 corners and not 8), 36 indices (6 faces x 2 triangles x 3 corners).
This exists purely so Phases 10-11 of this project's own step-by-step
build (see [docs/PHASE_10_EXPLANATION.md](PHASE_10_EXPLANATION.md) and
[PHASE_11_EXPLANATION.md](PHASE_11_EXPLANATION.md)) could prove winding
order and face culling with six visibly different faces.

**`makeUnitCube()`** - the exact same 24-corner, 36-index shape as
`makeCube()`, but every corner is plain white instead of rainbow. This is
the shape actually used to build the ship's deck, the crew's torso, a
crate, and dozens of other parts - each one gets its real colour later,
from `uTint`, as explained above.

**`makeUnitTaperedBox(bottomScale)`** - the same box again, except its
four BOTTOM corners (`y = -0.5`) are multiplied by `bottomScale` before
being placed, pulling them in toward the centre, while the four TOP
corners stay full size. `bottomScale = 1.0` gives back a plain box;
anything smaller makes it wider at the top than the bottom. This shape is
reused for two completely different things in this project: with a modest
taper it approximates the mountains in
[src/Scenery.h](../src/Scenery.h) (see that section below for how the SAME
shape becomes a mountain just by flipping it upside down).

**`makeUnitShipHull(bottomScale)`** - the most involved generator in this
file. Instead of one width for the whole shape, it defines five
`Station`s along its length, each with its own `halfWidth`:

```cpp
const Station stations[stationCount] = {
    { -0.50f, 0.00f },   // bow - a point
    { -0.25f, 0.42f },
    {  0.05f, 0.50f },   // widest point
    {  0.30f, 0.44f },
    {  0.50f, 0.34f },   // stern - a flat transom, not a point
};
```

For each station it adds four corners (top-left, top-right, bottom-right,
bottom-left - the bottom pair pulled in by `bottomScale`, same idea as
`makeUnitTaperedBox()`), then a loop connects each station to the next
with four quads (top, bottom, left, right side walls). At the bow, where
`halfWidth` is `0`, the left and right corners of that station land on
the exact same point - so the quads touching it automatically become
triangles that meet at a point, with no special-case code needed. The
stern, which is NOT a point, gets one extra, explicit face at the very
end to close it off - a flat rectangle, since nothing else would fill in
that end.

**`makeCylinder(segments)`** - builds one ring of corners at the bottom
and a matching ring at the top (`segments` corners each, evenly spaced
around a circle using `cos`/`sin`, the same "clock face" idea Part One's
camera section used), connects each pair of neighbouring corners into a
wall (a quad, i.e. two triangles, for every one of the `segments` steps
around), and then adds one extra centre corner at the very top and very
bottom, fanning triangles out from each centre to its own ring to close
the two flat ends. This is the shape behind every mast, the cannon barrel,
the wheel's spokes, and a barrel prop's body.

**`makeSphere(stacks, slices)`** - builds rings of corners from the top
pole to the bottom pole (`stacks` rings, this is the "latitude" idea a
world globe uses), each ring itself split into `slices` points going all
the way around (the "longitude" idea), using the same two-angle,
`cos`/`sin` approach as the cylinder, just with a SECOND angle controlling
how wide each ring is (a ring near a pole is a small circle; a ring at the
equator is the widest one). Every group of four neighbouring corners (two
from one ring, two from the next) becomes one quad, split into two
triangles. This is the shape behind every head, every eye, every
porthole, the sun, the cannonball, and every cloud and rock puff.

---

## `src/Camera.h` - turning a mouse into a viewpoint

### In plain words

This file is the one place in the whole project responsible for "which
way is the player currently looking?" It does two things: it remembers
three numbers (how far away, and two angles - explained fully in Part
One, Section 2), and it updates those three numbers whenever the mouse
drags or scrolls.

### More precisely

`struct OrbitCamera` holds the three numbers themselves (`radius`, `yaw`,
`pitch`) plus `lastCursorX`/`lastCursorY` - the mouse's position the last
time it moved, needed because GLFW only ever reports an ABSOLUTE cursor
position, never "how far did it move since last time," so something has
to remember the previous position to work that difference out.

`namespace OrbitCameraConfig` groups every tuning number this camera
uses: `MOUSE_SENSITIVITY` (how many radians one pixel of mouse movement
turns), `ZOOM_SPEED`, `MIN_RADIUS`/`MAX_RADIUS` (how close or far the
camera may get), and `MAX_PITCH_DEGREES` (clamped to 89, one degree short
of a full quarter-turn, so the camera can never flip upside-down by going
directly overhead).

`orbitCameraPosition(camera)` is the exact formula from Part One, Section
2, turning the three stored numbers into a real `(x, y, z)` position -
this function itself does not know or care WHERE that position is measured
from; [src/main.cpp](../src/main.cpp) is the one that decides that, by
adding this function's result onto whatever point the camera is currently
supposed to be looking at.

`handleOrbitCameraCursorMove()` and `handleOrbitCameraScroll()` are
**callbacks** - functions this project never calls directly. Instead,
[src/main.cpp](../src/main.cpp) hands their names to GLFW once
(`glfwSetCursorPosCallback`, `glfwSetScrollCallback`), and GLFW calls them
back, automatically, exactly when the mouse actually moves or the wheel
actually scrolls. Because GLFW is a C library, these callbacks cannot
"capture" a C++ variable the way a lambda could - there is no ordinary way
to hand them "the camera" as a parameter. The fix,
`glfwSetWindowUserPointer`/`glfwGetWindowUserPointer`, is a single free
"sticky note" GLFW lets a program attach to its own window: `main()`
writes a pointer to the camera onto the window once, and both callbacks
read that same note back out whenever GLFW calls them.

`handleOrbitCameraCursorMove()` also only actually changes `yaw`/`pitch`
while the LEFT mouse button is held (`glfwGetMouseButton(...) == GLFW_PRESS`)
- this is what makes it "click and drag" rather than "the camera follows
the mouse everywhere on screen." It still updates `lastCursorX`/`lastCursorY`
even when the button is not held, so that the very FIRST movement of a new
drag is measured from an accurate, recent position rather than from
wherever the cursor happened to be during the previous drag.

`handleOrbitCameraScroll()` adjusts `radius` and immediately clamps it
between `MIN_RADIUS` and `MAX_RADIUS` with `std::clamp`, so no amount of
scrolling can zoom the camera through an object or out to nothing.

---

## `src/Ship.h` - building a whole pirate ship

### In plain words

If [src/Mesh.h](../src/Mesh.h) is a box of building blocks (cubes,
cylinders, spheres), this file is the instruction booklet for arranging a
specific number of them into "a ship." Nothing here invents a new kind of
block - every part of the ship is one of Mesh.h's existing shapes, moved,
resized, and coloured into place.

### `namespace ShipShape` - every measurement and colour, named

This is a long list of `constexpr float` values (`HULL_WIDTH`,
`MAST_HEIGHT`, `WHEEL_RADIUS`, and many more) and two `ShipPalette`
values. Nothing here does any work by itself - it exists purely so every
size and colour the ship uses has one name and one place to change it,
instead of being a "magic number" typed directly into the drawing code
below (and possibly typed slightly differently in two different places by
mistake).

```cpp
struct ShipPalette {
    glm::vec3 hull;
    glm::vec3 deck;
    glm::vec3 mast;
    glm::vec3 sail;
    glm::vec3 flag;
    glm::vec3 cannonMetal;
    glm::vec3 barrel;
    glm::vec3 wheel;
};
```

A `ShipPalette` is just eight colours, bundled together as one value. This
project defines exactly two: `PLAYER_PALETTE` (warm wood tones, a red
flag) and `ENEMY_PALETTE` (near-black tones). Because every part below is
drawn using `palette.hull`, `palette.sail`, and so on - never a colour
typed directly - the SAME drawing code, given a different `ShipPalette`,
produces a completely different-looking ship. This is exactly why
[src/main.cpp](../src/main.cpp) can draw an "enemy ship" without a single
line of separate ship-building code: it calls the exact same functions
below, just handing them `ENEMY_PALETTE` instead of the default.

### `drawTintedPart()` - the one-line pattern repeated everywhere

```cpp
inline void drawTintedPart(
    ShaderProgram& shader, const Mesh& mesh, const glm::mat4& model, const glm::vec3& color)
{
    shader.setMat4("uModel", model);
    shader.setVec3("uTint", color);
    mesh.draw();
}
```

This is Part One's closing summary, as one small reusable function: set
where a shape is and how big it is (`uModel`), set what colour it is
(`uTint`), then draw it. Every single part of the ship - and, following
the same pattern, every part of the crew, the props, and the scenery -
eventually comes down to one call like this.

### `drawHullAndRigging()` - the hull, deck, masts, bowsprit, and portholes

This function places, in order:

- **The hull**, using `hullShape` (`makeUnitShipHull()`), scaled to
  `HULL_WIDTH` x `HULL_HEIGHT` x `HULL_LENGTH` and moved up so its bottom
  sits at `y = 0`.
- **The deck**, a plain box sitting right on top of the hull.
- **The quarterdeck**, a second, shorter box raised `QUARTERDECK_HEIGHT`
  above the main deck near the stern, where the wheel later stands.
- **Three masts and their sails**, in a loop over
  `{ FORE_MAST_Z, MAIN_MAST_Z, MIZZEN_MAST_Z }` - the loop body is written
  once and simply runs three times, at three different positions, which
  is why adding a third mast to this project only meant adding one more
  number to that list.
- **The bowsprit**, the one part of this file that needs a rotation NOT
  lined up with a plain axis - see the boxed explanation below.
- **The portholes**, a nested loop over each side of the ship
  (`{ -1.0f, 1.0f }`) and each `PORTHOLE_ZS` position, placing a small
  sphere with its centre exactly on the hull's outer surface so half of it
  reads as a round window.

**The bowsprit's rotation, explained:** a cylinder's own "up" direction is
always its local Y axis. To make it point up and out from the bow instead,
this code first works out what direction that actually needs to be in the
real world:

```cpp
const glm::vec3 bowspritDir(0.0f, std::cos(bowspritPitch), std::sin(bowspritPitch));
```

then uses that same direction TWICE - once to `rotate` the cylinder onto
it, and once to `translate` the already-rotated cylinder outward along it
by half its own length, so its base stays attached at the bow and only its
far end swings out and up. This is the exact same "translate along the
target direction, then rotate the mesh to match" pairing Part One's
Section 3 showed for the cannon barrel, just with an angled direction
instead of a straight sideways one.

### `drawCannon()` - mount, yoke, and barrel

Three parts, each a **child** of the one drawn just before it - the yoke's
position is written relative to `mountFrame`, and the barrel's position is
written relative to `yokeFrame`, rather than all three being measured
independently from the ship's own root. The barrel's rotate-then-translate
composition is the exact example already quoted in Part One, Section 3.

### `drawFlag()` - one box, on top of the mizzen mast

The simplest of these functions: one box, positioned at the top of
whichever mast `MIZZEN_MAST_Z` names (the back-most one), at a height
that clears the mast's own top.

### `drawWheel()` - a post, a dark disc, a hub, and eight spokes

Explained in full in Part One's spirit already, but to recap the actual
code: `wheelCenter * facing` is a shared rotation (90 degrees about X)
that every part of the wheel uses, turning a cylinder's naturally
UPRIGHT disc shape to instead face ALONG the ship, the direction a
helmsman standing behind it would actually look. The dark backing disc
uses that shared rotation directly; the hub and every spoke use a version
nudged slightly forward (`raised`) so they are not drawn at the exact same
depth as the backing disc, and are tinted a LIGHTER colour than it - with
no lighting in this project yet, that colour difference is the only thing
that makes the spokes readable as separate lines rather than disappearing
into one plain-coloured disc. Each spoke's own placement -
`rotate(angle, Z) * translate(0, WHEEL_RADIUS * 0.5, 0)` inside a loop
counting to `spokeCount` - is the exact "translate outward, then let a
following rotation sweep it around a circle" trick explained in Part One.

### `drawShip()` - all of the above, once

```cpp
inline void drawShip(...)
{
    drawHullAndRigging(shader, unitCube, unitCylinder, unitSphere, hullShape, shipRoot, palette);
    drawCannon(shader, unitCube, unitCylinder, shipRoot, palette);
    drawFlag(shader, unitCube, shipRoot, palette);
    drawWheel(shader, unitCube, unitCylinder, shipRoot, palette);
}
```

Four calls, nothing more. This function exists so that anywhere in
[src/main.cpp](../src/main.cpp) that wants "the whole ship" can say so in
one line, while anywhere that wants to show just the cannon, or just the
flag, on its own can call that ONE function directly instead.

---

## `src/Crew.h` - building one person

### In plain words

The exact same idea as `Ship.h`, at a much smaller scale: a list of named
sizes and colours, and one function that places a handful of boxes and
spheres relative to each other until they read as a standing person.

### More precisely

`namespace CrewShape` names every measurement: two separate legs (with a
small `LEG_GAP` between them, rather than one wide box standing in for
both), a torso, two arms, two hands, a head, and the face - eyes, ears, a
nose, and lips.

`drawCrewMember(shader, unitCube, unitSphere, crewRoot, shirtColor)`
places each part relative to `crewRoot`, whose own origin is defined as
"the ground the feet stand on" - every other part's height is measured
upward from there: legs from `y = 0`, the torso on top of the legs, the
head on top of the torso, and the face features relative to the head's own
centre point (`headCenter`), using `+Z` as "the front of the face" the
same way every other direction in this project is just a plain axis, not
something special to faces.

The one detail worth re-reading closely:

```cpp
const glm::vec3 NOSE_COLOR(0.90f, 0.64f, 0.52f);
const glm::vec3 EAR_COLOR(0.78f, 0.60f, 0.46f);
```

The nose and ears are NOT given the exact same colour as the rest of the
head, even though they are meant to be "the same skin." With no lighting
in this project yet, a same-coloured bump on a same-coloured sphere has no
shading to reveal its shape at all - it would be geometrically there but
completely invisible. A slightly different shade is a cheap stand-in for
the shading a later phase of this project will eventually add.

`shirtColor` is the one part of a crew member NOT fixed inside this file -
it is a parameter, defaulting to `DEFAULT_SHIRT_COLOR` if the caller does
not specify one, which is exactly what lets
[src/main.cpp](../src/main.cpp) give its five crew members five different
shirt colours by calling this SAME function five times with a different
colour each time.

---

## `src/Props.h` - a barrel, a crate, and a cannonball

### In plain words

Three small, self-contained objects, each just a few calls to Mesh.h's
existing shapes, each following the exact same "place it relative to its
own base" idea as the crew member's feet.

### More precisely

`drawBarrel(shader, unitCylinder, root)` draws one cylinder for the
barrel's body, then two MORE cylinders - thin and slightly wider than the
body - near its top and bottom, tinted a darker colour, as the two
"hoops" that make it read as a barrel instead of a plain cylinder.

`drawCrate(shader, unitCube, root)` draws one cube for the crate itself,
then two thin, flat boxes crossing over its lid, one running along each
direction, tinted darker, as the crate's "straps."

`drawCannonball(shader, unitSphere, root)` is the simplest of the three:
one sphere, positioned so it rests ON TOP of `root`'s surface (its centre
is raised by exactly its own radius) rather than being centred AT `root`,
which would leave half of it buried underground.

All three take `root` as "wherever this object's base should sit" -
[src/main.cpp](../src/main.cpp) decides the actual position by building
that `root` matrix and handing it in, and these functions never need to
know or care where in the world that turns out to be.

---

## `src/Scenery.h` - mountains, clouds, and rocks

### In plain words

Three more small object-builders, following the same pattern as
`Props.h`, for the objects that make the scene feel like a real place
instead of a ship floating in empty space.

### More precisely

`drawMountain(shader, mountainShape, unitSphere, root, width, height, depth)`
is the most interesting reuse in this project. `mountainShape` is
`makeUnitTaperedBox()` from [src/Mesh.h](../src/Mesh.h) - the SAME
generator an earlier version of this project used for the ship's hull -
but called with a much more extreme taper, and then drawn like this:

```cpp
shader.setMat4("uModel", glm::scale(frame, glm::vec3(width, -height, depth)));
```

Notice the **negative** `height`. `makeUnitTaperedBox()` always makes its
TOP full width and its BOTTOM narrow. A negative scale on the Y axis
flips the shape upside down for free - no new mesh, no rotation matrix -
because scaling by `-height` still produces a shape spanning the same
`[-0.5, +0.5] * height` range, it just swaps which end lands where. The
end result: a shape that is WIDE at the bottom and NARROW at the top -
exactly a mountain's silhouette - built from a generator whose doc comment
in Mesh.h describes the opposite shape. A small lighter sphere near the
peak (`capFrame`) is added afterward as a simple "snow cap."

`drawCloud()` and `drawRock()` share one idea: a small, fixed list of
spheres (`Puff`/`Chunk` - each just an offset and a radius) at slightly
different sizes and positions, all drawn relative to the same `root`.
Several overlapping spheres read as "one fluffy or lumpy object" far
better than one perfectly round sphere would - the same "several simple
shapes make a more convincing whole" idea `Ship.h` and `Crew.h` both
already rely on, just applied to clouds and rocks instead of a ship or a
person.

---

## `src/main.cpp` - the conductor

### In plain words

Every file above knows how to build or draw ONE thing. Nothing decides
WHERE any of those things actually go in the world, WHEN a frame gets
drawn, or WHAT happens when a key is pressed, until this file. If the
other files are musicians who each know how to play one instrument,
`main.cpp` is the conductor - it does not play a note itself, but it
decides the order everyone plays in and cues each one at the right time.

### More precisely, section by section

**The `#include` lines** at the top are what actually bring every other
file's functions and structs into this one - `#include "Ship.h"` is what
makes `drawShip()` available here, and the same for every other header.
This is also why [CMakeLists.txt](../CMakeLists.txt) never needed to list
`Ship.h`, `Crew.h`, and the rest separately: they are not compiled on
their own, they are copied in, textually, wherever they are `#include`d.

**`namespace AppConfig`** - window size, the window's title text, the two
shader file paths, timing limits, how many samples MSAA uses (explained in
[docs/OBJECTS_BUILD.md](OBJECTS_BUILD.md)), and the sky colour.

**`namespace CameraConfig`** - which way is "up" for the camera, plus the
field of view and near/far plane numbers Part One's Section 2 explained.

**`namespace SceneConfig`** - genuinely the biggest block of named numbers
in the whole project: how detailed the round shapes are, the sea's size
and colour, the sun's position and two colours, both ships' positions, all
five crew members' positions and shirt colours, the three props'
positions, and every number controlling the mountain ring, the clouds, and
the rocks. Nothing here does any WORK - exactly like `ShipShape` in
`Ship.h`, it exists so every tunable number has one name and one home.

**`enum class ViewMode`** lists every object that can be shown, either
together (`All`) or alone - one named value per number key (`0`
through `9`) plus `EnemyShip` for the `E` key.

**`struct ViewPreset` and `viewPresetFor(mode)`** answer "if the player
just switched to looking at THIS one object, where should the camera look,
how far back should it start, and how far should it tilt?" - returning a
different answer for a cannonball than for the whole ship, since a single
fixed camera distance could never suit both.

**`viewModeName(mode)`** turns a `ViewMode` back into readable text, used
only for the `[view] ...` message printed to the console when the object
being shown changes.

**`struct FrameClock` / `struct FrameStats`** hold the handful of numbers
needed to measure time and report frames-per-second - explained more in
the "the loop" part below.

**`struct SceneState`** is everything that can change while the program is
running, gathered in one place: the camera itself, its two matrices
(`view`/`projection`), which object is currently shown, and whether
wireframe mode is on.

**`applyViewPreset(scene, mode)`** and **`switchViewMode(scene, newMode)`**
are how a `ViewPreset` actually gets used: `applyViewPreset` copies a
preset's numbers into the live camera, and `switchViewMode` calls it ONLY
if the requested object is not already the one being shown (so holding a
number key down does not repeatedly reset the camera 120 times a second).

**The GLFW callback functions** - `glfwErrorCallback` (prints any error
GLFW itself reports) and `framebufferSizeCallback` (keeps the drawing area
matching the window's actual pixel size if it is resized) - follow the
exact same "GLFW calls this, we never call it directly" pattern
[src/Camera.h](../src/Camera.h)'s two callbacks use.

**`processInput(window, scene)`** is read once every frame and checks:
`ESCAPE` (close the window), `W` (toggle wireframe, with the "was it
already down" check Part One's Section 3 area of this project relies on
throughout, so holding the key does not flip the state repeatedly), each
digit key `0`-`9` (call `switchViewMode`), and `E` (the enemy ship). The
mouse is deliberately NOT read here at all - it works entirely through the
callbacks in `Camera.h`, registered once, further down in `main()`.

**`startClock`/`updateClock`** keep track of "what time is it right now"
(`glfwGetTime()`) and "how long was the last frame," capping that second
number (`MAX_DELTA_TIME`) so one unusually slow frame (say, from the
window being dragged) cannot cause a sudden jump later, once this project
adds real motion.

**`updateScene(scene, framebufferWidth, framebufferHeight)`** runs once
per frame and rebuilds the camera's two matrices: `eye` is `scene.viewTarget`
PLUS `orbitCameraPosition(scene.camera)` (the fix described in
[docs/OBJECTS_BUILD.md](OBJECTS_BUILD.md) - the offset has to be added to
whatever point is actually being looked at, not assumed to be the world's
centre), and `scene.projection` is rebuilt fresh from the CURRENT window
size every frame, in case the window was resized.

**`drawWater`, `drawSun`, `MountainPlacement`/`mountainRingPlacement`** are
small helpers `renderScene()` uses so its own body stays readable: `drawWater`
and `drawSun` each draw one background object, and `mountainRingPlacement(index, count)`
works out where the `index`-th of `count` mountains (evenly spaced around
a full circle) should sit and how big it should be, using `sin`/`cos` of
the index itself (not a random number generator) so every mountain gets a
little size and distance variation while the exact same scene is produced
every single time the program runs.

**`renderScene(...)`** is the function that actually draws a frame:

1. Clear the screen to the sky colour, and clear the depth buffer.
2. `shader.use()`, then send the current `uView`/`uProjection` - shared by
   everything drawn this frame.
3. Set solid or wireframe polygon mode, based on `scene.wireframeEnabled`.
4. A `switch` on `scene.viewMode`: the `All` case draws the water, the
   sun, the full mountain ring, the clouds, the rocks, the player's ship,
   all five crew members, the three props, and the enemy ship, one after
   another; every OTHER case draws exactly one object, alone, at the
   world's origin, using that object's own drawing function directly.

**`reportFrame(stats, clock)`** counts frames and, once a full second has
passed, prints the `[frame] ...` line seen in the console - average frame
time and frames-per-second - then resets its counters for the next second.

**`main()`** ties everything together, in order:

1. Set up GLFW error reporting, initialise GLFW, and ask for an OpenGL 3.3
   Core context with a multisampled (`GLFW_SAMPLES`) framebuffer.
2. Create the actual window, make its OpenGL context current, and load
   OpenGL's functions with GLAD (`gladLoadGLLoader`) - the exact call the
   `glad.c` section above described.
3. Register `framebufferSizeCallback`, turn on `GL_DEPTH_TEST` and
   `GL_MULTISAMPLE`, and print the graphics driver's own version strings.
4. Build the `ShaderProgram` from the two shader files.
5. Build the five reusable `Mesh` objects (`unitCube`, `unitCylinder`,
   `unitSphere`, `hullShape`, `mountainShape`) - EVERY shape drawn this
   entire run comes from one of these five.
6. Print the on-screen help text, build the `SceneState`, and call
   `applyViewPreset(scene, ViewMode::All)` once so the very first frame
   already has a correct camera instead of a default one.
7. Register the camera on the window (`glfwSetWindowUserPointer`) and
   connect `Camera.h`'s two mouse callbacks.
8. **The main loop** - `while (glfwWindowShouldClose(window) == GLFW_FALSE)` -
   runs, every single frame, in exactly the order this whole project has
   followed since its very first phase: measure time, read input, update
   the scene, render the scene, show the result and read window/input
   events, then report timing.
9. Once the window closes, every GPU resource is destroyed EXPLICITLY,
   in the reverse of the order it was created, while the OpenGL context
   still exists - `mountainShape`, `hullShape`, `unitSphere`,
   `unitCylinder`, `unitCube`, then the shader - before finally destroying
   the window and terminating GLFW.
