# Configuration and resource bounds

Normal applications include `Flow32.h` and optionally override macros with
compiler flags before compilation.

| Setting | Default | Purpose |
| --- | ---: | --- |
| `FLOW32_MAX_APPS` | 8 | registered applications |
| `FLOW32_UI_ARENA_BYTES` | 8192 | bump arena used for one retained tree |
| `FLOW32_MAX_INPUT_SOURCES` | 4 | application-owned input adapters |
| `FLOW32_INPUT_QUEUE_SIZE` | 24 | normalized event ring capacity |
| `FLOW32_ICON_CACHE_SLOTS` | 16 | streamed monochrome icon LRU slots |
| `FLOW32_EMOJI_CACHE_SLOTS` | 8 | streamed color emoji LRU slots |
| `FLOW32_FRAME_INTERVAL_MS` | 16 | minimum time between rendered frames |
| `FLOW32_BUTTON_DEBOUNCE_MS` | 20 | default stable-state switch interval |

Per-device debounce can be changed through `ButtonInputConfig::debounceMs` and
`JoystickConfig::buttonDebounceMs` without recompiling the library.

The following are documented architectural bounds rather than additional
configuration switches:

| Bound | Value |
| --- | ---: |
| roots per Page or Canvas tree | 8 |
| children per UI node | 16 |
| focusable nodes per Page | 32 |
| pages per App | 8 |
| page navigation depth | 8 |
| select options | 12 |
| Shell status icons | 4 |

Exceeding a retained-tree bound invokes the fatal handler before any array is
written. Change a bound in source only when the product has a demonstrated need
and test the resulting memory cost.

## Fatal handling

`setFlowFatalHandler()` installs an application policy for failures that cannot
be returned through a reference-building API. A handler must not return. It may
log and restart the device; host tests may throw. Passing null restores the
default `abort()` behavior.

## Framebuffer calculations

Panel memory:

```text
width × height × sizeof(uint16_t)
```

Active Page content cache:

```text
viewport width × max(content height, viewport height) × sizeof(uint16_t)
```

The allocations coexist. `FLOW32_UI_ARENA_BYTES`, input arrays, app state, and
asset cache payloads are additional. A 240 × 320 panel plus a 240 × 600 content
cache consumes 441,600 bytes solely for RGB565 pixels.

`DisplayPanel::preferPsram` defaults true. Display and Page first try PSRAM and
then internal RAM. Use PSRAM for typical 240 × 320 scrolling applications.
`Display::begin()` reports panel-buffer allocation failure with `false`; Page
cache failure invokes `FlowError::PageFramebufferAllocationFailed`.

## Joystick thresholds

`deadZoneEnter` must be positive and `deadZoneExit` must be non-negative and
strictly smaller. A direction activates outside the enter distance and remains
active until the reading crosses the closer exit distance. Invalid calibration
invokes `FlowError::InvalidInputConfiguration` during input initialization.
