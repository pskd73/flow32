# Architecture

Flow32 has one retained-mode UI architecture:

```text
Application
    │
    ├── concrete DisplayTransport
    ├── concrete InputSource(s)
    ├── concrete AssetStore
    └── FontPack
           │
           ▼
        Flow32 runtime
           │
          App
           │
          Page
           │
       retained UI tree
           │
         layout
           │
         Canvas
           │
        Display
           │
    DisplayTransport
           │
        hardware
```

Dependencies point downward. UI controls know Canvas and semantic styles, but
never SPI or filesystem mounting. Generic Display owns a framebuffer and knows
only the transport contract. Concrete adapters are leaf components configured
and owned by the application.

## Lifecycle

1. The application initializes peripherals and mounted storage.
2. It constructs transports, inputs, fonts, assets, apps, and `Flow32`.
3. `Flow32::begin()` starts explicitly supplied components, allocates the panel
   framebuffer, opens the first app, and renders the first frame.
4. `Flow32::tick()` polls inputs at call frequency and renders at the configured
   frame interval.
5. Shell owns root Back behavior; App owns its page stack; Page owns focus and
   scroll behavior.

Initialization failures are returned where the API can express them. Bounds
that occur behind reference-returning UI builders use the configurable fatal
handler and must not continue.

## Retained tree and arena

Apps rebuild a bounded UI tree when application state becomes dirty. Nodes live
in a fixed bump arena and are valid until the next rebuild. The arena never
wraps. Child, root, and focus limits are checked before writes.

Layout computes node boxes. Rasterization writes the whole content into a page
cache. Scroll animation copies only the visible rows into the panel framebuffer;
it does not rebuild text and layout on every animation frame.

## Framebuffers

Display owns one panel-sized RGB565 framebuffer. Each active App owns one Page,
and its Page grows a content framebuffer to the largest rasterized content
height observed. Both allocations prefer PSRAM and may fall back to internal
RAM. See [configuration](CONFIGURATION.md) for formulas and limits.

The retained-page strategy is intentionally unchanged. Dirty rectangles,
partial buffers, and DMA presentation are future work.

## Shell invalidation

Shell hashes the current title and status line and tracks the active app,
fullscreen state, panel geometry, nav height, and theme generation. It rebuilds
and presents the navigation region only when one of those values changes.

## State and persistence

`AppStore<State>` always owns an in-memory state value and UI dirty flag. A null
or empty namespace keeps it RAM-only. A namespace opts into Preferences/NVS and
debounced writes. UI invalidation and persistence dirtiness are separate.

## External assets

Asset storage is borrowed from the application. Atlas readers decode explicit
little-endian fields and verify file size, section arithmetic, per-record
offsets, glyph dimensions, name termination, and payload ranges before marking
an atlas ready. Corrupt assets are rejected with a safe fallback.
