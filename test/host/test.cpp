#include <cassert>
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <Flow32.h>

namespace {

class CaptureTransport final : public DisplayTransport {
public:
  bool begin(const DisplayPanel &) override {
    began = beginResult;
    return beginResult;
  }
  void end() override {
    began = false;
    ++endCount;
  }
  void setBacklight(bool enabled) override {
    backlight = enabled;
    ++backlightRequests;
  }
  void present(const uint16_t *pixels, int16_t, int16_t, int16_t width,
               int16_t height, int16_t stride) override {
    ++writes;
    firstPixel = pixels ? pixels[0] : 0;
    lastStride = stride;
    lastWidth = width;
    lastHeight = height;
  }

  bool beginResult = true;
  bool began = false;
  bool backlight = false;
  int backlightRequests = 0;
  int endCount = 0;
  int writes = 0;
  uint16_t firstPixel = 0;
  int16_t lastStride = 0;
  int16_t lastWidth = 0;
  int16_t lastHeight = 0;
};

class MemoryAssetStore final : public AssetStore {
public:
  bool ready() const override { return true; }
  bool exists(const char *path) const override {
    return path && files.find(path) != files.end();
  }
  File open(const char *path, const char * = FILE_READ) const override {
    const auto found = path ? files.find(path) : files.end();
    return found == files.end() ? File() : File(found->second);
  }
  bool resolve(const char *relative, char *out, size_t outLen) const override {
    if (!relative || !out || outLen == 0) return false;
    const int n = std::snprintf(out, outLen, "%s", relative);
    return n >= 0 && static_cast<size_t>(n) < outLen;
  }

  std::map<std::string, std::vector<uint8_t>> files;
};

struct TestState {
  bool on = false;
};

class TestApp final : public App<TestState> {
public:
  explicit TestApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Test", nullptr);
    addPage("Home");
    addPage("Detail");
  }

protected:
  void build(Page &page, uint8_t) override {
    page.add(page.div()
                 .style(Style().setWidth(Length::Pct(100)).setPadding(4))
                 .add(page.text("hello"))
                 .add(page.toggle().checked(state().on)));
  }
};

DisplayPanel smallPanel() {
  DisplayPanel panel;
  panel.width = 32;
  panel.height = 24;
  panel.cornerRadius = 2;
  panel.preferPsram = false;
  return panel;
}

void testDisplayAndUi() {
  CaptureTransport transport;
  const DisplayPanel panel = smallPanel();
  {
    Display display(panel, transport);
    assert(display.begin());
    assert(transport.began);
    assert(transport.backlight);

    Canvas canvas(display);
    Page page(Rect(0, 0, panel.width, panel.height));
    page.beginUI();
    page.add(page.text("Flow32"));
    page.layoutUI(canvas);
    page.syncFocus();
    assert(page.drawUI(canvas));
    assert(transport.writes == 1);
    assert(transport.lastWidth == panel.width);

    display.panelBuffer()[3 * panel.width + 2] = 0x1234;
    display.present(2, 3, 4, 5);
    assert(transport.firstPixel == 0x1234);
    assert(transport.lastStride == panel.width);
    assert(transport.lastWidth == 4);
    assert(transport.lastHeight == 5);

    display.end();
    assert(transport.endCount == 1);
    assert(!transport.backlight);
  }
  assert(transport.endCount == 1);
}

void testPageFramebufferClippingAndCacheLifecycle() {
  CaptureTransport transport;
  const DisplayPanel panel = smallPanel();
  Display display(panel, transport);
  assert(display.begin());
  Canvas canvas(display);

  const size_t panelPixels =
      static_cast<size_t>(panel.width) * static_cast<size_t>(panel.height);
  std::fill(display.panelBuffer(), display.panelBuffer() + panelPixels, 0x1111);

  Page right(Rect(static_cast<int16_t>(panel.width - 2), 0, 4, 2));
  right.setContentBackground(0xBEEF);
  right.setContentHeight(2);
  assert(right.drawUI(canvas));
  for (int16_t y = 0; y < panel.height; ++y) {
    for (int16_t x = 0; x < panel.width; ++x) {
      const bool covered = y < 2 && x >= panel.width - 2;
      assert(display.panelBuffer()[y * panel.width + x] ==
             (covered ? 0xBEEF : 0x1111));
    }
  }

  std::fill(display.panelBuffer(), display.panelBuffer() + panelPixels, 0x2222);
  Page left(Rect(-2, 2, 4, 2));
  left.setContentBackground(0xCAFE);
  left.setContentHeight(2);
  assert(left.drawUI(canvas));
  for (int16_t y = 0; y < panel.height; ++y) {
    for (int16_t x = 0; x < panel.width; ++x) {
      const bool covered = y >= 2 && y < 4 && x < 2;
      assert(display.panelBuffer()[y * panel.width + x] ==
             (covered ? 0xCAFE : 0x2222));
    }
  }

  Page cache(Rect(0, 0, 10, 10));
  cache.setContentHeight(30);
  assert(cache.drawUI(canvas));
  assert(cache.contentCacheBytes() == 10U * 30U * sizeof(uint16_t));
  cache.setContentHeight(10);
  cache.invalidateContent();
  assert(cache.drawUI(canvas));
  assert(cache.contentCacheBytes() == 10U * 10U * sizeof(uint16_t));
  cache.releaseContentCache();
  assert(cache.contentCacheBytes() == 0);

  InputHub input;
  TestApp app(Rect(0, 0, panel.width, panel.height));
  assert(app.open());
  app.frame(canvas, input, 0.0f);
  assert(app.page().contentCacheBytes() > 0);
  app.goTo(1);
  assert(app.page().contentCacheBytes() == 0);
  app.frame(canvas, input, 0.0f);
  assert(app.page().contentCacheBytes() > 0);
  app.close();
  assert(app.page().contentCacheBytes() == 0);
}

void testSt77xxStridedPartialPresentation() {
  St77xxConfig config;
  config.spi = &SPI;
  config.pinCs = 5;
  config.pinDc = 4;
  config.pinMosi = 18;
  config.pinSclk = 2;
  config.gramWidth = 32;
  config.gramHeight = 32;
  config.panelYOffset = 7;

  St77xxTransport transport(config);
  const DisplayPanel panel = smallPanel();
  assert(transport.begin(panel));

  uint16_t pixels[15] = {};
  for (uint16_t i = 0; i < 15; ++i) pixels[i] = i;
  st77xx_stub::reset();
  transport.present(pixels, 3, 2, 2, 3, 5);

  assert(st77xx_stub::startWrites == 1);
  assert(st77xx_stub::endWrites == 1);
  assert(st77xx_stub::windows.size() == 1);
  assert(st77xx_stub::windows[0].x == 3);
  assert(st77xx_stub::windows[0].y == 9);
  assert(st77xx_stub::windows[0].width == 2);
  assert(st77xx_stub::windows[0].height == 3);
  assert(st77xx_stub::pixelCounts.size() == 3);
  assert(st77xx_stub::firstPixels.size() == 3);
  for (size_t row = 0; row < 3; ++row) {
    assert(st77xx_stub::pixelCounts[row] == 2);
    assert(st77xx_stub::firstPixels[row] == row * 5);
  }
}

std::vector<UIEvent> drain(InputHub &hub) {
  std::vector<UIEvent> events;
  UIEvent event;
  while (hub.pop(event)) events.push_back(event);
  return events;
}

void pollButton(InputHub &hub, int level, uint32_t at) {
  arduino_stub::digitalValues[7] = level;
  arduino_stub::nowMs = at;
  hub.poll(at);
}

void testButtonDebounceAndRepeat() {
  arduino_stub::nowMs = 0;
  arduino_stub::digitalValues[7] = LOW;
  InputHub hub;
  ButtonInputConfig config;
  config.select = 7;
  config.activeLow = false;
  config.debounceMs = 20;
  ButtonInput buttons(config);
  buttons.tracker().holdDelayMs = 40;
  buttons.tracker().holdRepeatMs = 20;
  assert(hub.add(buttons));
  assert(!hub.add(buttons));
  hub.begin();

  pollButton(hub, HIGH, 5);
  pollButton(hub, LOW, 9);
  pollButton(hub, HIGH, 12);
  pollButton(hub, LOW, 16);
  pollButton(hub, HIGH, 20);
  pollButton(hub, HIGH, 39);
  assert(drain(hub).empty());
  pollButton(hub, HIGH, 40);
  std::vector<UIEvent> events = drain(hub);
  assert(events.size() == 1);
  assert(events[0].key == UIKey::Select);
  assert(events[0].phase == UIKeyPhase::Down);

  pollButton(hub, HIGH, 79);
  assert(drain(hub).empty());
  pollButton(hub, HIGH, 80);
  events = drain(hub);
  assert(events.size() == 1 && events[0].phase == UIKeyPhase::Hold);
  pollButton(hub, HIGH, 100);
  events = drain(hub);
  assert(events.size() == 1 && events[0].phase == UIKeyPhase::Hold);

  buttons.tracker().holdDelayMs = 1000;
  pollButton(hub, LOW, 110);
  pollButton(hub, HIGH, 114);
  pollButton(hub, LOW, 118);
  pollButton(hub, HIGH, 122);
  pollButton(hub, LOW, 126);
  pollButton(hub, LOW, 145);
  assert(drain(hub).empty());
  pollButton(hub, LOW, 146);
  events = drain(hub);
  assert(events.size() == 1 && events[0].phase == UIKeyPhase::Up);
  pollButton(hub, LOW, 200);
  assert(drain(hub).empty());

  // A clean edge follows the same stable interval.
  pollButton(hub, HIGH, 220);
  pollButton(hub, HIGH, 239);
  assert(drain(hub).empty());
  pollButton(hub, HIGH, 240);
  events = drain(hub);
  assert(events.size() == 1 && events[0].phase == UIKeyPhase::Down);
}

void expectEvent(InputHub &hub, UIKey key, UIKeyPhase phase) {
  const std::vector<UIEvent> events = drain(hub);
  assert(events.size() == 1);
  assert(events[0].key == key);
  assert(events[0].phase == phase);
}

void testJoystickHysteresis() {
  arduino_stub::analogValues[8] = 2048;
  arduino_stub::analogValues[9] = 2048;
  InputHub hub;
  JoystickConfig config;
  config.pinX = 8;
  config.pinY = 9;
  config.deadZoneEnter = 700;
  config.deadZoneExit = 500;
  JoystickInput joystick(config);
  assert(joystick.validConfig());
  assert(hub.add(joystick));
  hub.begin();

  const int rightNoise[] = {2740, 2748, 2735, 2747};
  for (uint8_t i = 0; i < 4; ++i) {
    arduino_stub::analogValues[8] = rightNoise[i];
    hub.poll(++arduino_stub::nowMs);
  }
  assert(drain(hub).empty());

  arduino_stub::analogValues[8] = 2749;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Right, UIKeyPhase::Down);
  const int rightHeld[] = {2700, 2600, 2550, 2548};
  for (uint8_t i = 0; i < 4; ++i) {
    arduino_stub::analogValues[8] = rightHeld[i];
    hub.poll(++arduino_stub::nowMs);
  }
  assert(drain(hub).empty());
  arduino_stub::analogValues[8] = 2547;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Right, UIKeyPhase::Up);

  arduino_stub::analogValues[8] = 1347;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Left, UIKeyPhase::Down);
  arduino_stub::analogValues[8] = 1549;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Left, UIKeyPhase::Up);

  arduino_stub::analogValues[8] = 2048;
  arduino_stub::analogValues[9] = 1347;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Up, UIKeyPhase::Down);
  arduino_stub::analogValues[9] = 1549;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Up, UIKeyPhase::Up);

  arduino_stub::analogValues[9] = 2749;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Down, UIKeyPhase::Down);
  arduino_stub::analogValues[9] = 2547;
  hub.poll(++arduino_stub::nowMs);
  expectEvent(hub, UIKey::Down, UIKeyPhase::Up);

  JoystickConfig invalid;
  invalid.deadZoneEnter = 100;
  invalid.deadZoneExit = 100;
  JoystickInput invalidJoystick(invalid);
  assert(!invalidJoystick.validConfig());
}

void testRuntimeAndShellInvalidation() {
  CaptureTransport transport;
  const DisplayPanel panel = smallPanel();
  TestApp app(Rect(0, 0, panel.width, panel.height));
  Flow32 runtime(panel, transport);
  assert(runtime.apps({&app})
             .config(FlowConfig{}.theme(Theme::DarkTheme()))
             .begin());
  const int initialWrites = transport.writes;
  arduino_stub::nowMs += FLOW32_FRAME_INTERVAL_MS;
  runtime.tick();
  assert(runtime.activeApp() == &app);
  assert(transport.writes == initialWrites);

  Theme::setActive(Theme::LightTheme());
  arduino_stub::nowMs += FLOW32_FRAME_INTERVAL_MS;
  runtime.tick();
  assert(transport.writes > initialWrites);

  CaptureTransport failedTransport;
  failedTransport.beginResult = false;
  TestApp failedApp(Rect(0, 0, panel.width, panel.height));
  Flow32 failedRuntime(panel, failedTransport);
  assert(!failedRuntime.apps({&failedApp}).begin());
}

void testInputQueueBound() {
  InputHub hub;
  for (uint8_t i = 0; i < InputHub::kMaxQueue + 3; ++i) {
    hub.push(UIEvent(UIKey::Select, UIKeyPhase::Down));
  }
  uint8_t count = 0;
  UIEvent event;
  while (hub.pop(event)) {
    assert(event.key == UIKey::Select);
    ++count;
  }
  assert(count == InputHub::kMaxQueue - 1);
}

struct PersistedState {
  uint32_t value = 7;
};

void testAppStoreModes() {
  Preferences::clearHostStorage();
  AppStore<PersistedState> ram;
  assert(ram.begin(nullptr));
  assert(ram.ready());
  assert(!ram.persistent());
  assert(ram.set(ram.data().value, static_cast<uint32_t>(9)));
  assert(!ram.persistenceDirty());
  ram.end();

  AppStore<PersistedState> first;
  assert(first.begin("persist", 100));
  assert(first.persistent());
  assert(first.set(first.data().value, static_cast<uint32_t>(42)));
  assert(first.persistenceDirty());
  assert(first.saveNow());
  assert(!first.persistenceDirty());
  first.end();

  AppStore<PersistedState> second;
  assert(second.begin("persist"));
  assert(second.get().value == 42);
  second.end();
}

void put16(std::vector<uint8_t> &out, size_t at, uint16_t value) {
  out[at] = static_cast<uint8_t>(value);
  out[at + 1] = static_cast<uint8_t>(value >> 8);
}

void put32(std::vector<uint8_t> &out, size_t at, uint32_t value) {
  out[at] = static_cast<uint8_t>(value);
  out[at + 1] = static_cast<uint8_t>(value >> 8);
  out[at + 2] = static_cast<uint8_t>(value >> 16);
  out[at + 3] = static_cast<uint8_t>(value >> 24);
}

std::vector<uint8_t> iconAtlas() {
  std::vector<uint8_t> bytes(20 + 5 + 18 + 2, 0);
  std::memcpy(bytes.data(), "F32I", 4);
  put16(bytes, 4, 1);
  put16(bytes, 6, 16);
  put16(bytes, 8, 1);
  put32(bytes, 12, 5);
  put32(bytes, 16, 2);
  std::memcpy(bytes.data() + 20, "wifi", 5);
  const size_t glyph = 25;
  put16(bytes, glyph, 0);
  put16(bytes, glyph + 2, 0);
  put32(bytes, glyph + 4, 0);
  put16(bytes, glyph + 8, 2);
  put16(bytes, glyph + 10, 2);
  put16(bytes, glyph + 12, 2);
  bytes[43] = 0xFF;
  bytes[44] = 0xFF;
  return bytes;
}

std::vector<uint8_t> emojiAtlas() {
  std::vector<uint8_t> bytes(20 + 17 + 8 + 2, 0);
  std::memcpy(bytes.data(), "F32E", 4);
  put16(bytes, 4, 1);
  bytes[6] = 16;
  put16(bytes, 8, 1);
  put32(bytes, 12, 8);
  put32(bytes, 16, 2);
  put32(bytes, 20, 0x1F600);
  put32(bytes, 24, 0);
  put32(bytes, 28, 0);
  bytes[32] = 2;
  bytes[33] = 2;
  bytes[34] = 2;
  bytes[45] = 0xFF;
  bytes[46] = 0xFF;
  return bytes;
}

void testAssetBounds() {
  MemoryAssetStore storage;
  storage.files["icons"] = iconAtlas();
  StreamedIconAtlas icons;
  assert(icons.begin(storage, "icons"));
  assert(icons.findByName("wifi"));
  icons.end();

  std::vector<uint8_t> badIcon = iconAtlas();
  put16(badIcon, 27, 99);
  storage.files["bad-icons"] = badIcon;
  assert(!icons.begin(storage, "bad-icons"));
  badIcon = iconAtlas();
  put32(badIcon, 29, 2);
  storage.files["bad-icons"] = badIcon;
  assert(!icons.begin(storage, "bad-icons"));

  storage.files["emoji"] = emojiAtlas();
  StreamedEmojiAtlas emoji;
  assert(emoji.begin(storage, "emoji"));
  assert(emoji.find(0x1F600));
  emoji.end();

  std::vector<uint8_t> badEmoji = emojiAtlas();
  put32(badEmoji, 24, 4);
  storage.files["bad-emoji"] = badEmoji;
  assert(!emoji.begin(storage, "bad-emoji"));
  badEmoji = emojiAtlas();
  badEmoji.pop_back();
  storage.files["bad-emoji"] = badEmoji;
  assert(!emoji.begin(storage, "bad-emoji"));
}

struct FatalSignal {
  FlowError error;
};

void throwingFatal(FlowError error) { throw FatalSignal{error}; }

void testArenaFailure() {
  struct TooLarge {
    uint8_t bytes[FLOW32_UI_ARENA_BYTES + 1];
  };
  setFlowFatalHandler(throwingFatal);
  UIArena arena;
  bool caught = false;
  try {
    arena.create<TooLarge>();
  } catch (const FatalSignal &signal) {
    caught = signal.error == FlowError::UiArenaExhausted;
  }
  setFlowFatalHandler(nullptr);
  assert(caught);
  assert(arena.used() == 0);
}

} // namespace

int main() {
  static_assert(std::is_same<flow32::Runtime, Flow32>::value,
                "namespaced runtime alias must remain available");
  static_assert(std::is_same<flow32::graphics::Display, Display>::value,
                "namespaced display alias must remain available");
  static_assert(std::is_same<IconSd, StreamedIconAtlas>::value,
                "legacy streamed icon type must remain available");
  static_assert(std::is_same<ColorEmojiSd, StreamedEmojiAtlas>::value,
                "legacy streamed emoji type must remain available");
  static_assert(FLOW32_BUTTON_DEBOUNCE_MS > 0,
                "button debounce default must be positive");
  assert(Length::Pct(50).resolve(200) == 100);
  assert(&Theme::FlowTheme() == &Theme::DarkTheme());
  assert(&Theme::WinterTheme() == &Theme::LightTheme());
  testDisplayAndUi();
  testPageFramebufferClippingAndCacheLifecycle();
  testSt77xxStridedPartialPresentation();
  testButtonDebounceAndRepeat();
  testJoystickHysteresis();
  testRuntimeAndShellInvalidation();
  testInputQueueBound();
  testAppStoreModes();
  testAssetBounds();
  testArenaFailure();
  return 0;
}
