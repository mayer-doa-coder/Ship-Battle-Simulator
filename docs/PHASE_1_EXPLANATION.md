# Phase 1 - Render Loop and Frame Time

## Status

Phase 1 is complete and verified. Debug and Release builds succeeded. A live Release run stayed responsive, reported approximately `0.0083` seconds per frame at `120 FPS` with V-sync, and closed normally.

No object, shader, animation, ship, or gameplay feature is part of this phase.

## What changed

The program is now divided into small jobs:

| Function | Job |
|---|---|
| `updateClock()` | Measures the current time and the duration of the last frame |
| `processInput()` | Reads keyboard input; currently only `ESC` |
| `updateScene()` | Will calculate movement and other scene changes later |
| `renderScene()` | Clears and draws the current frame |
| `reportFrame()` | Prints average frame time and FPS once per second |

The main loop is now easy to read:

```text
measure time
read input
update scene
render scene
show frame
read events
report timing
```

## Important values

The changeable values remain together in `AppConfig` near the top of `src/main.cpp`.

| Value | Current value | What happens if it changes |
|---|---:|---|
| `WINDOW_WIDTH` | `1280` | Changes the starting window width |
| `WINDOW_HEIGHT` | `720` | Changes the starting window height |
| `CLEAR_COLOR` | `(0.08, 0.16, 0.24)` | Changes the red, green, and blue background values |
| `MAX_DELTA_TIME` | `0.10` seconds | Sets the largest movement step a future update may receive |
| `REPORT_INTERVAL` | `1.0` second | Changes how often timing information is printed |
| `VSYNC_INTERVAL` | `1` | `1` enables V-sync; `0` allows uncapped rendering |

## The three time values

`FrameClock` keeps three related values:

- `now`: seconds since GLFW started;
- `lastFrameTime`: the previous frame's time;
- `deltaTime`: `now - lastFrameTime`, measured in seconds.

Future movement uses seconds so it does not depend on FPS:

```text
distance moved = speed per second x deltaTime
```

For example, a speed of `5` units per second with `deltaTime = 0.016` moves approximately `0.08` units during that frame.

## Why delta time is limited

Dragging or pausing a window can create one very slow frame. Without a limit, a future ship could jump a large distance when the program continues.

`MAX_DELTA_TIME = 0.10` means a single update can simulate at most one tenth of a second. This affects future movement safety; it does not change the real `now` clock.

## Why update and render are separate

`updateScene()` changes data. `renderScene()` displays the current data. Keeping them separate makes later code easier to explain:

- steering belongs in update;
- wave movement belongs in update;
- OpenGL drawing belongs in render;
- keyboard reading belongs in input.

## Likely teacher questions

### What is a render loop?

It is the loop that repeatedly reads input, updates the scene, draws a frame, and displays it until the window closes.

### What is `deltaTime`?

It is the number of seconds between the current frame and the previous frame. It makes movement speed independent of FPS.

### Why use `glfwGetTime()`?

It provides a steadily increasing time value. We use it to measure frame duration and later to calculate animation.

### Why call `glfwSwapBuffers()`?

Drawing happens in a hidden back buffer. Swapping places the finished frame on the screen, which prevents half-drawn images from appearing.

### Why call `glfwPollEvents()`?

It asks the operating system for keyboard, mouse, resize, and close events. Without it, the window can stop responding.

### Why clear the depth buffer when there is no object?

It has no visible effect yet, but the same correct render structure will be ready when 3D objects are added.

### What does V-sync do?

It normally limits frame presentation to the monitor's refresh rate, reducing tearing and unnecessary work.

### Why are update and render separate functions?

They have different jobs. Update calculates state; render displays state. This makes changes easier and prevents input, movement, and drawing code from becoming mixed together.

### Why is `updateScene()` empty?

There is no scene object yet. The function creates the correct place for future movement code without adding a feature before its phase.

### Why use `static_cast<void>(now)` in the empty update?

It tells the compiler that the parameter is intentionally unused in this phase. The parameter remains ready for future animation.

### What does `FrameClock& clock` mean?

The `&` passes the original clock to the function, so `updateClock()` can change its values. Without it, the function would change only a copy.

### Why does `reportFrame()` receive `const FrameClock&`?

It reads the original clock without copying it. `const` prevents the reporting function from changing the clock accidentally.

## Simple viva modifications

If the teacher asks for a change:

- Change the background: edit `CLEAR_COLOR`.
- Change the window size: edit `WINDOW_WIDTH` and `WINDOW_HEIGHT`.
- Print timing twice per second: set `REPORT_INTERVAL` to `0.5f`.
- Disable V-sync for a test: set `VSYNC_INTERVAL` to `0`.
- Change the maximum update step: edit `MAX_DELTA_TIME`.

## Checkpoint

Phase 1 passes when:

- Debug and Release builds succeed;
- the clear-colour window opens and stays responsive;
- the console prints one timing line approximately every second;
- `dt` and average `dt` are sensible positive values;
- `ESC` and the window close button end the program normally.
