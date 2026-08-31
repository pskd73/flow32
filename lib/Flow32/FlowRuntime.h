#pragma once

#include "App.h"
#include "Canvas.h"
#include "ColorEmojiSd.h"
#include "Display.h"
#include "DisplayPanel.h"
#include "IconSd.h"
#include "Rect.h"
#include "Shell.h"
#include "Storage.h"
#include "StorageConfig.h"
#include "input/InputHub.h"
#include "input/InputSource.h"
#include "input/SerialInput.h"
#include "ui/Theme.h"
#include "ui/UIDebug.h"

#include <initializer_list>
#include <new>
#include <string.h>

/**
 * Optional bootstrap settings for Flow32::config().
 *
 *   FlowConfig{}
 *     .theme(Theme::FlowTheme())
 *     .storage(SdDefault())
 *     .debugBorders(false)
 *
 * Hardware input addons (e.g. JoystickInput) are not config — register them
 * with Flow32::input() and pass pins at construction.
 */
class FlowConfig {
public:
  FlowConfig &theme(const Theme::ThemeTokens &t) {
    theme_ = &t;
    return *this;
  }
  FlowConfig &storage(const StorageConfig &c) {
    storage_ = c;
    hasStorage_ = true;
    return *this;
  }
  FlowConfig &debugBorders(bool v) {
    debugBorders_ = v;
    return *this;
  }

  const Theme::ThemeTokens *theme() const { return theme_; }
  bool hasStorage() const { return hasStorage_; }
  const StorageConfig &storage() const { return storage_; }
  bool debugBorders() const { return debugBorders_; }

private:
  friend class Flow32;
  const Theme::ThemeTokens *theme_ = nullptr;
  StorageConfig storage_{};
  bool hasStorage_ = false;
  bool debugBorders_ = false;
};

/**
 * Fluent runtime — display, shell, apps, storage, frame loop.
 *
 *   static JoystickInput joy(/* VRx */ 1, /* VRy */ 2, /* SW */ 3);
 *   static Flow32 flow(Panel183());
 *   void setup() {
 *     flow.apps({&home})
 *         .config(FlowConfig{}
 *                   .theme(Theme::FlowTheme())
 *                   .storage(SdDefault()))
 *         .input(joy)
 *         .begin();
 *   }
 *   void loop() { flow.tick(); }
 *
 * Core only sees UIKey events. Hardware sources (joystick, …) are addons
 * registered via input() with pins at construction.
 */
class Flow32 : public AppHost {
public:
  static constexpr uint8_t kMaxApps = 12;
  static constexpr uint8_t kMaxExtraInputs = 3;

  explicit Flow32(const DisplayPanel &panel)
      : panel_(panel),
        display_(panel_),
        canvas_(display_),
        shell_(Rect(0, 0, panel_.width, panel_.height)) {}

  ~Flow32() {
    if (begun_) {
      if (AppBase *a = activeApp()) a->close();
    }
    if (iconsReady_) {
      icons_.end();
      iconsReady_ = false;
    }
    if (emojiReady_) {
      emoji_.end();
      emojiReady_ = false;
    }
    destroyStorage();
  }

  Flow32(const Flow32 &) = delete;
  Flow32 &operator=(const Flow32 &) = delete;

  /** Register apps; first entry becomes active. */
  Flow32 &apps(std::initializer_list<AppBase *> list) {
    appCount_ = 0;
    active_ = 0;
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

  /**
   * Register an optional InputSource addon (call before begin()).
   * Serial is always present; sources emit UIKey Left/Right/Up/Down/Select/Back.
   *
   *   static JoystickInput joy(pinX, pinY, pinSw);
   *   flow.input(joy);
   */
  Flow32 &input(InputSource &source) {
    if (extraInputCount_ >= kMaxExtraInputs) return *this;
    extraInputs_[extraInputCount_++] = &source;
    return *this;
  }

  /** Shorthand for config storage (same as FlowConfig::storage). */
  Flow32 &storage(const StorageConfig &c) {
    config_.storage(c);
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
  IconSd *icons() { return iconsReady_ ? &icons_ : nullptr; }
  ColorEmojiSd *emoji() { return emojiReady_ ? &emoji_ : nullptr; }

  uint8_t appCount() const { return appCount_; }
  AppBase *activeApp() const {
    return (active_ < appCount_) ? apps_[active_] : nullptr;
  }

  /** AppHost: copy launcher metadata only (name, icon, index). */
  uint8_t getApps(AppInfo *out, uint8_t maxOut) const override {
    if (!out || maxOut == 0) return 0;
    uint8_t n = appCount_;
    if (n > maxOut) n = maxOut;
    for (uint8_t i = 0; i < n; i++) out[i] = meta_[i];
    return n;
  }

  bool openApp(uint8_t index) override { return setActiveApp(index); }

  uint8_t activeAppIndex() const override { return active_; }

  bool openLauncher() override {
    if (active_ == 0) return false;
    return setActiveApp(0);
  }

  Storage *storage() override { return storage_; }

  bool setActiveApp(uint8_t index) {
    if (index >= appCount_) return false;
    if (begun_ && active_ < appCount_ && apps_[active_]) {
      apps_[active_]->close();
    }
    active_ = index;
    shell_.setApp(apps_[active_]);
    if (begun_ && apps_[active_]) {
      apps_[active_]->open();
    }
    return true;
  }

  bool begin() {
    if (begun_) return true;

    Serial.begin(115200);
    delay(200);

    if (config_.theme()) {
      Theme::setActive(*config_.theme());
    } else {
      Theme::setActive(Theme::FlowTheme());
    }

    Serial.printf("Flow32 | theme=%s | Panel %s %dx%d\n", Theme::active().name,
                  panel_.id, panel_.width, panel_.height);
    Serial.printf("Heap before assets: %u\n", (unsigned)ESP.getFreeHeap());

    pinMode(panel_.pinBl, OUTPUT);
    digitalWrite(panel_.pinBl, HIGH);

    // no_psram: panel FB is ~134KB. Load icon index before the FB, but defer
    // emoji until after display — both indexes + FB do not fit together.
    if (config_.hasStorage()) {
      destroyStorage();
      storage_ = new (storageMem_) Storage(config_.storage());
      if (storage_->begin()) {
        storage_->printInfo();
#if !defined(FLOW32_SKIP_ICONS)
        if (icons_.begin(*storage_)) {
          canvas_.setIconSd(&icons_);
          iconsReady_ = true;
        } else {
          Serial.println(
              "Icon atlas missing — copy sd/flow32/icons.atlas onto the card");
        }
#else
        Serial.println("Icons skipped (FLOW32_SKIP_ICONS)");
#endif
      } else {
        Serial.println("SD mount failed — check StorageConfig pins");
        destroyStorage();
      }
    }

    Serial.printf("Heap after icons: %u\n", (unsigned)ESP.getFreeHeap());

    if (!display_.begin()) {
      Serial.printf("Display begin failed (heap=%u, need ~%u for FB)\n",
                    (unsigned)ESP.getFreeHeap(),
                    (unsigned)display_.bufferBytes());
      return false;
    }
    display_.setBacklight(true);
    Serial.printf("Display begin ok (heap=%u)\n",
                  (unsigned)ESP.getFreeHeap());

#if !defined(FLOW32_SKIP_EMOJI)
    if (storage_ && storage_->ready()) {
      if (emoji_.begin(*storage_)) {
        canvas_.setEmojiSd(&emoji_);
        emojiReady_ = true;
      } else {
        Serial.println(
            "Emoji atlas missing — copy sd/flow32/emoji.atlas onto the card");
      }
      Serial.printf("Heap after emoji: %u\n", (unsigned)ESP.getFreeHeap());
    }
#endif

    for (uint8_t i = 0; i < appCount_; i++) {
      if (apps_[i]) {
        apps_[i]->onAssets(iconsReady_ ? &icons_ : nullptr,
                           emojiReady_ ? &emoji_ : nullptr);
      }
    }

    input_.add(serial_);
    for (uint8_t i = 0; i < extraInputCount_; i++) {
      if (extraInputs_[i]) input_.add(*extraInputs_[i]);
    }
    input_.begin();

    UIDebug::borders = config_.debugBorders();

    shell_.setPanel(panelRect());
    if (AppBase *a = activeApp()) {
      shell_.setApp(a);
      if (!a->open()) {
        Serial.println("App: open failed — using defaults");
      }
    }

    begun_ = true;
    lastMs_ = millis();
    shell_.frame(canvas_, input_, 0.04f);
    return true;
  }

  void tick() {
    if (!begun_) return;
    display_.setBacklight(true);

    const uint32_t now = millis();
    input_.poll(now);

    if (now - lastMs_ < 16) return;
    const float dt = (now - lastMs_) / 1000.0f;
    lastMs_ = now;

    shell_.frame(canvas_, input_, dt);
  }

private:
  void destroyStorage() {
    if (!storage_) return;
    storage_->~Storage();
    storage_ = nullptr;
  }

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
  SerialInput serial_{};
  InputSource *extraInputs_[kMaxExtraInputs] = {};
  uint8_t extraInputCount_ = 0;
  FlowConfig config_{};

  AppBase *apps_[kMaxApps] = {};
  AppInfo meta_[kMaxApps] = {};
  uint8_t appCount_ = 0;
  uint8_t active_ = 0;

  alignas(Storage) uint8_t storageMem_[sizeof(Storage)] = {};
  Storage *storage_ = nullptr;
  ColorEmojiSd emoji_{};
  IconSd icons_{};
  bool emojiReady_ = false;
  bool iconsReady_ = false;
  bool begun_ = false;
  uint32_t lastMs_ = 0;
};
