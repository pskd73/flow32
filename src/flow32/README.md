# Source map

| Directory | Responsibility |
| --- | --- |
| `core/` | errors, app lifecycle, state, Shell, and runtime |
| `graphics/` | framebuffer, Canvas, fonts, transport boundary, ST77xx adapter |
| `ui/` | retained nodes, layout, controls, pages, themes, and focus |
| `input/` | event queue, debounce, repeat, joystick, buttons, and serial adapter |
| `assets/` | storage abstraction and bounds-checked icon/emoji readers |

Dependency direction is application → runtime → app → page → retained tree →
Canvas → Display → DisplayTransport. Hardware-specific includes belong only in
concrete adapter files.

The public entry point is `Flow32.h`. Internal headers remain usable for
advanced minimal builds, but they do not define alternate UI models.
