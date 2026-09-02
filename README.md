# Flow32

Flow32 is a small retained-mode embedded UI framework for ESP32-class devices.
It owns apps, pages, layout, widgets, focus navigation, scrolling, rendering,
framebuffer management, theming, and state integration. The application owns
hardware policy and supplies concrete transports, inputs, fonts, and asset
storage.

The normal installation has one entry point:

```cpp
#include <Flow32.h>
```

## Hardware assumptions

Flow32 targets Arduino-compatible ESP32 builds and rasterizes into RGB565
framebuffers. It uses Adafruit GFX as its drawing surface, but generic
`Display` does not know about SPI, GPIO, or a particular controller.

The bundled `St77xxTransport` is optional and supports Adafruit's ST7735 and
ST7789 drivers. Other controllers implement the small `DisplayTransport`
interface.

## Ownership boundary

The application owns:

- pin assignments and SPI construction;
- filesystem mounting;
- `Serial.begin()` and other peripheral initialization;
- concrete `DisplayTransport` and `InputSource` objects;
- concrete fonts and asset storage;
- application data and hardware policy.

Flow32 owns:

- the panel framebuffer and retained page cache;
- application and page lifecycle;
- the retained UI tree and bounded arena;
- layout, focus, event routing, navigation, and scrolling;
- rasterization, theming, and synchronous presentation.

## Smallest retained application

```cpp
#include <Flow32.h>

DisplayPanel makePanel() {
  DisplayPanel panel;
  panel.width = 240;
  panel.height = 320;
  return panel;
}

const DisplayPanel panel = makePanel();

St77xxConfig displayConfig() {
  St77xxConfig config;
  config.spi = &SPI;
  config.pinCs = 5;
  config.pinDc = 4;
  config.pinMosi = 18;
  config.pinSclk = 2;
  config.pinRst = 15;
  config.gramWidth = 240;
  config.gramHeight = 320;
  return config;
}

struct State {};

class MainApp : public App<State> {
public:
  MainApp() : App(Rect(0, 0, panel.width, panel.height)) {
    setAppInfo("Main", "house");
    addPage("Home");
  }

  void build(Page &page, uint8_t) override {
    page.add(page.div()
                 .style(Style().setPadding(16).setGap(12))
                 .add(page.text("Hello, Flow32"))
                 .add(page.button().add(page.text("Continue"))));
  }
};

St77xxTransport transport(displayConfig());
MainApp app;
SerialInput serialInput;
Flow32 flow(panel, transport);

void setup() {
  Serial.begin(115200); // application policy
  flow.apps({&app}).input(serialInput).begin();
}

void loop() { flow.tick(); }
```

All transport configuration defaults to invalid pins and a null SPI pointer.
The example must be adapted to the board in use.

## 0.4 migration and breaking changes

- `Display` and `Flow32` now require a `DisplayTransport&` at construction.
- Controller, SPI, rotation, address offset, pin, and backlight fields moved
  from `DisplayPanel` to `St77xxConfig`.
- Custom transports implement `present(...)`; the former write callback and its
  argument order are gone.
- `JoystickConfig::deadzone` became the validated `deadZoneEnter` and
  `deadZoneExit` pair.
- `FontPack` now exposes semantic `title` and `large` roles. Older `FontRole`
  enumerator names remain cheap aliases.
- The secondary declarative execution subsystem and its build artifacts were
  removed without compatibility wrappers.
- `StorageConfig::spi` defaults to null; applications using SPI storage must
  explicitly provide the bus.

`Theme::FlowTheme()` and `Theme::WinterTheme()` remain as inexpensive aliases
for the dark and light built-ins.

## Apps, pages, and state

Derive from `App<State>`, register pages with `addPage()`, and build the active
page using `UIDiv`, `UIText`, `UIButton`, `UIToggle`, `UIRange`, `UISelect`, and
`UIImage`. `goTo()` and Back manage a bounded navigation stack.

State is RAM-only by default. Override `nvsNamespace()` with a non-empty name
to opt into debounced Preferences/NVS persistence.

## Inputs

Register application-owned sources with `flow.input(source)`. Digital buttons
use stable-state debounce before Down, Hold, and Up tracking. Analog joysticks
use separate enter and exit thresholds to prevent neutral-edge flicker.

## Display transports

`DisplayTransport` receives rectangular RGB565 regions with an explicit source
stride. `Display` is the only caller and only requests backlight changes through
that interface. A custom transport never causes Flow32 to manipulate GPIO or
SPI independently.

See [adapter boundaries](docs/ADAPTERS.md) and the
[CustomTransport example](examples/CustomTransport/CustomTransport.ino).

## Fonts and assets

`FontPack` maps semantic roles (`small`, `body`, `bodyBold`, `title`, `large`)
to application-supplied GFX or anti-aliased fonts. No product font is embedded
in the framework.

`AssetStore` abstracts an already-mounted Arduino filesystem. `FsAssetStore`
works with LittleFS, SPIFFS, SD, or another `fs::FS`. Flow32 never mounts a
filesystem implicitly. Streamed icon and emoji readers validate all section,
record, string, pixel, and alpha bounds before accepting an atlas.

## Memory model

The panel framebuffer consumes:

```text
panel width × panel height × 2 bytes
```

A scrollable `Page` additionally caches the full rasterized content:

```text
viewport width × max(content height, viewport height) × 2 bytes
```

For a 240 × 320 panel, the panel framebuffer is 153,600 bytes. A 240 × 600
page cache is another 288,000 bytes, so both together require 441,600 bytes
before UI state and asset caches. PSRAM is strongly recommended for scrollable
pages at this size.

Flow32 tries PSRAM first when requested and then internal RAM. `Display::begin`
returns `false` if its framebuffer cannot be allocated. A page-cache allocation
failure invokes the configurable fatal handler rather than drawing from invalid
memory.

Important fixed and configurable bounds are documented in
[configuration](docs/CONFIGURATION.md).

## Examples

| Example | Demonstrates |
| --- | --- |
| [BasicUI](examples/BasicUI/BasicUI.ino) | smallest retained application |
| [Buttons](examples/Buttons/Buttons.ino) | debounced digital navigation |
| [Joystick](examples/Joystick/Joystick.ino) | analog hysteresis and focus |
| [CustomTransport](examples/CustomTransport/CustomTransport.ino) | controller-independent presentation |
| [Assets](examples/Assets/Assets.ino) | application-mounted LittleFS and streamed icons |
| [Showcase](examples/Showcase/Showcase.ino) | apps, pages, scrolling, controls, themes, fonts, and persistence |

## More documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Adapters and hardware ownership](docs/ADAPTERS.md)
- [Configuration and memory bounds](docs/CONFIGURATION.md)
- [Optional minimal-package exporter](docs/BUILD_PROFILES.md)
- [Contributing](CONTRIBUTING.md)

The imported upstream project did not include a project-level license. See
[UPSTREAM.md](UPSTREAM.md) before redistributing the framework.
