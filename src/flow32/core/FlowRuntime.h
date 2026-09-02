#pragma once

#include "flow32/core/App.h"
#include "flow32/graphics/Canvas.h"
#include "flow32/assets/StreamedEmojiAtlas.h"
#include "flow32/graphics/Display.h"
#include "flow32/graphics/DisplayPanel.h"
#include "flow32/graphics/FontPack.h"
#include "Flow32Config.h"
#include "flow32/assets/StreamedIconAtlas.h"
#include "flow32/graphics/Rect.h"
#include "flow32/core/Shell.h"
#include "flow32/input/InputHub.h"
#include "flow32/input/InputSource.h"
#include "flow32/ui/Theme.h"
#include "flow32/ui/UIDebug.h"

#include <initializer_list>
#include <string.h>

/**
 * Optional bootstrap settings for Flow32::config().
 *
 *   FlowConfig{}
 *     .theme(Theme::DarkTheme())
 *     .fonts(myFonts)
 *     .debugBorders(false)
 */
class FlowConfig {
public:
  FlowConfig &theme(const Theme::ThemeTokens &t) {
    theme_ = t;
    hasTheme_ = true;
    return *this;
  }
  FlowConfig &fonts(const FontPack &pack) {
    fonts_ = pack;
    hasFonts_ = true;
    return *this;
  }
  FlowConfig &debugBorders(bool v) {
    debugBorders_ = v;
    return *this;
  }

  const Theme::ThemeTokens *theme() const { return hasTheme_ ? &theme_ : nullptr; }
  const FontPack *fonts() const { return hasFonts_ ? &fonts_ : nullptr; }
  bool debugBorders() const { return debugBorders_; }

private:
  friend class Flow32;
  Theme::ThemeTokens theme_{};
  FontPack fonts_{};
  bool hasTheme_ = false;
  bool hasFonts_ = false;
  bool debugBorders_ = false;
};

/**
 * Fluent runtime — display, shell, apps, input, and frame loop.
 *
 *   static St77xxTransport transport(makeTransport());
 *   static Flow32 flow(makePanel(), transport);
 *   void setup() {
 *     flow.apps({&home})
 *         .config(FlowConfig{}
 *                   .theme(Theme::DarkTheme()))
 *         .input(buttons)
 *         .begin();
 *   }
 *   void loop() { flow.tick(); }
 */
class Flow32 : public AppHost {
public:
  static constexpr uint8_t kMaxApps = FLOW32_MAX_APPS;
  static_assert(FLOW32_MAX_APPS > 0 && FLOW32_MAX_APPS <= 255,
                "FLOW32_MAX_APPS must be 1..255");

  Flow32(const DisplayPanel &panel, DisplayTransport &transport)
      : panel_(panel),
        display_(panel_, transport),
        canvas_(display_),
        shell_(Rect(0, 0, panel_.width, panel_.height)) {}

  ~Flow32() {
    if (begun_) {
      if (AppBase *a = activeApp()) a->close();
    }
  }

  Flow32(const Flow32 &) = delete;
  Flow32 &operator=(const Flow32 &) = delete;

  /** Register apps; first entry becomes active. */
  Flow32 &apps(std::initializer_list<AppBase *> list) {
    appCount_ = 0;
    active_ = 0;
    home_ = 0;
    for (AppBase *a : list) {
      if (!a || appCount_ >= kMaxApps) continue;
      apps_[appCount_++] = a;
    }
    refreshAppMeta();
    shell_.setHost(this);
    for (uint8_t i = 0; i < appCount_; i++) {
      if (apps_[i]) apps_[i]->onAttach(*this);
    }
    if (appCount_ > 0) shell_.setApp(apps_[0]);
    return *this;
  }

  Flow32 &config(const FlowConfig &c) {
    config_ = c;
    return *this;
  }

  /** Select the app opened by root-level Back. Defaults to index zero. */
  Flow32 &home(uint8_t index) {
    if (index < appCount_) home_ = index;
    return *this;
  }

  /** Register an input source; ownership remains with the application. */
  Flow32 &input(InputSource &source) {
    input_.add(source);
    return *this;
  }

  /** Attach already-opened optional asset atlases. */
  Flow32 &assets(StreamedIconAtlas *icons, StreamedEmojiAtlas *emoji = nullptr) {
    icons_ = icons;
    emoji_ = emoji;
    canvas_.setStreamedIcons(icons_);
    canvas_.setStreamedEmoji(emoji_);
    if (begun_) {
      for (uint8_t i = 0; i < appCount_; i++) {
        if (apps_[i]) apps_[i]->onAssets(icons_, emoji_);
      }
    }
    return *this;
  }

  /** Shorthand for config theme. */
  Flow32 &theme(const Theme::ThemeTokens &t) {
    config_.theme(t);
    return *this;
  }

  Rect panelRect() const {
    return Rect(0, 0, panel_.width, panel_.height);
  }
  const DisplayPanel &panel() const { return panel_; }
  Display &display() { return display_; }
  Canvas &canvas() { return canvas_; }
  Shell &shell() { return shell_; }
  InputHub &inputHub() { return input_; }
  StreamedIconAtlas *icons() { return icons_; }
  StreamedEmojiAtlas *emoji() { return emoji_; }

  uint8_t appCount() const { return appCount_; }
  AppBase *activeApp() const {
    return (active_ < appCount_) ? apps_[active_] : nullptr;
  }

  /** AppHost: copy app metadata only (name, icon, index). */
  uint8_t getApps(AppInfo *out, uint8_t maxOut) const override {
    if (!out || maxOut == 0) return 0;
    uint8_t n = appCount_;
    if (n > maxOut) n = maxOut;
    for (uint8_t i = 0; i < n; i++) out[i] = meta_[i];
    return n;
  }

  bool openApp(uint8_t index) override { return setActiveApp(index); }

  uint8_t activeAppIndex() const override { return active_; }

  bool openHome() override {
    if (active_ == home_) return false;
    return setActiveApp(home_);
  }

  bool setActiveApp(uint8_t index) {
    if (index >= appCount_) return false;
    const uint8_t previous = active_;
    if (begun_ && active_ < appCount_ && apps_[active_]) {
      apps_[active_]->close();
    }
    active_ = index;
    shell_.setApp(apps_[active_]);
    if (begun_ && apps_[active_]) {
      if (apps_[active_]->open()) return true;
      active_ = previous;
      shell_.setApp(apps_[active_]);
      if (apps_[active_]) apps_[active_]->open();
      return false;
    }
    return true;
  }

  bool begin() {
    if (begun_) return true;
    if (appCount_ == 0 || !activeApp()) return false;

    if (config_.theme()) {
      Theme::setActive(*config_.theme());
    } else {
      Theme::setActive(Theme::DarkTheme());
    }

    display_.setFontPack(config_.fonts());

    for (uint8_t i = 0; i < appCount_; i++) {
      if (apps_[i]) {
        apps_[i]->onAssets(icons_, emoji_);
      }
    }

    if (!display_.begin()) {
      return false;
    }
    input_.begin();

    UIDebug::borders = config_.debugBorders();

    shell_.setPanel(panelRect());
    if (AppBase *a = activeApp()) {
      shell_.setApp(a);
      if (!a->open()) {
        display_.end();
        return false;
      }
    }

    begun_ = true;
    lastMs_ = millis();
    shell_.frame(canvas_, input_, 0.04f);
    return true;
  }

  void tick() {
    if (!begun_) return;

    const uint32_t now = millis();
    input_.poll(now);

    if (now - lastMs_ < FLOW32_FRAME_INTERVAL_MS) return;
    const float dt = (now - lastMs_) / 1000.0f;
    lastMs_ = now;

    shell_.frame(canvas_, input_, dt);
  }

private:
  void refreshAppMeta() {
    for (uint8_t i = 0; i < appCount_; i++) {
      meta_[i] = AppInfo{};
      meta_[i].index = i;
      if (!apps_[i]) continue;
      const char *name = apps_[i]->appName();
      meta_[i].name = (name && name[0]) ? name : "";
      meta_[i].icon = apps_[i]->appIcon();
    }
  }

  DisplayPanel panel_;
  Display display_;
  Canvas canvas_;
  Shell shell_;
  InputHub input_{};
  FlowConfig config_{};

  AppBase *apps_[kMaxApps] = {};
  AppInfo meta_[kMaxApps] = {};
  uint8_t appCount_ = 0;
  uint8_t active_ = 0;
  uint8_t home_ = 0;

  StreamedEmojiAtlas *emoji_ = nullptr;
  StreamedIconAtlas *icons_ = nullptr;
  bool begun_ = false;
  uint32_t lastMs_ = 0;
};
