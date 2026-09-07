#pragma once

#include "App.h"
#include "Canvas.h"
#include "ColorEmojiSd.h"
#include "Display.h"
#include "DisplayPanel.h"
#include "IconSd.h"
#include "IdleEyes.h"
#include "Rect.h"
#include "Shell.h"
#include "Storage.h"
#include "StorageConfig.h"
#include "SplashPng.h"
#include "input/InputHub.h"
#include "input/InputSource.h"
#include "input/SerialInput.h"
#include "ui/Theme.h"
#include "ui/Style.h"
#include "ui/UIDebug.h"
#include "RamManager.h"

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
 *     .idleEyes(30)
 *     .splash(splashPage, 2, "/logo/mark.png")
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
  /**
   * Paint the idle eyes after this many seconds with no input. 0 (default)
   * disables it, so existing callers are unaffected.
   */
  FlowConfig &idleEyes(uint32_t seconds) {
    idleEyesSec_ = seconds;
    return *this;
  }
  /**
   * Show `page` full-screen for `seconds` after the display comes up, before
   * the first app opens. 0 seconds skips. Optional `sdImage` is a card-rooted
   * PNG (e.g. "/logo/mark.png") loaded onto the page after storage mounts.
   */
  FlowConfig &splash(Page &page, uint32_t seconds,
                     const char *sdImage = nullptr) {
    splashPage_ = &page;
    splashSec_ = seconds;
    splashImage_ = sdImage;
    return *this;
  }

  const Theme::ThemeTokens *theme() const { return theme_; }
  bool hasStorage() const { return hasStorage_; }
  const StorageConfig &storage() const { return storage_; }
  bool debugBorders() const { return debugBorders_; }
  uint32_t idleEyesSec() const { return idleEyesSec_; }
  Page *splashPage() const { return splashPage_; }
  uint32_t splashSec() const { return splashSec_; }
  const char *splashImage() const { return splashImage_; }

private:
  friend class Flow32;
  const Theme::ThemeTokens *theme_ = nullptr;
  StorageConfig storage_{};
  bool hasStorage_ = false;
  bool debugBorders_ = false;
  uint32_t idleEyesSec_ = 0;
  Page *splashPage_ = nullptr;
  uint32_t splashSec_ = 0;
  const char *splashImage_ = nullptr;
};

/**
 * Fluent runtime — display, shell, apps, storage, frame loop.
 *
 *   static JoystickInput joy(1, 2, 3);  // VRx, VRy, SW
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
    splashFreePng(splashPixels_);
    splashPixels_ = nullptr;
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

  /** Bind product AA faces to Title / TitleLarge. Call before begin(). */
  Flow32 &titleFonts(const AAFont *px16, const AAFont *px22,
                     const AAFont *px34) {
    canvas_.setTitleFonts(px16, px22, px34);
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

  void showToast(const char *message, ToastKind kind,
                 uint16_t durationMs) override {
    shell_.showToast(message, kind, durationMs);
  }

  bool toastVisible() const override { return shell_.toastVisible(); }

  bool overlaySuppressesPresent() const override {
    return idleShowing_ || splashShowing_;
  }

  Storage *storage() override { return storage_; }

  bool ramEnsureProfile(RamManager::Profile profile, const char *requester,
                        RamManager::Priority drainUpTo) override {
    if (!requester) {
      AppBase *a = activeApp();
      requester = (a && a->appName()[0]) ? a->appName() : "flow";
    }
    return RamManager::ensureProfile(profile, requester, drainUpTo);
  }

  bool ramEnsureNeed(RamManager::Need need, const char *requester,
                     RamManager::Priority drainUpTo) override {
    if (!requester) {
      AppBase *a = activeApp();
      requester = (a && a->appName()[0]) ? a->appName() : "flow";
    }
    return RamManager::ensureNeed(need, requester, drainUpTo);
  }

  RamManager::Snapshot ramSnapshot() const override {
    return RamManager::snapshot();
  }

  void ramLog(const char *tag) const override { RamManager::log(tag); }

  bool setActiveApp(uint8_t index) {
    if (index >= appCount_) return false;
    if (begun_ && active_ < appCount_ && apps_[active_]) {
      apps_[active_]->close();
    }
    active_ = index;
    AppBase *next = apps_[active_];
    shell_.setApp(next);
    if (next && next->appName()[0]) {
      RamManager::setForeground(next->appName());
    } else {
      RamManager::setForeground("");
    }
    // App switch (wake word → Ask, Back → launcher, …) always dismisses the
    // idle overlay. Reset the idle timer so we do not bounce straight back.
    if (idleShowing_) {
      idleShowing_ = false;
      lastInputMs_ = millis();
    }
    if (begun_ && next) {
      if (!next->open()) {
        Serial.printf("App: open failed (%s)\n",
                      next->appName()[0] ? next->appName() : "?");
      }
    }
    return true;
  }

  bool begin() {
    if (begun_) return true;

    RamManager::init();
    RamManager::setDrainedNotify(&Flow32::ramDrainedNotify, this);

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
    digitalWrite(panel_.pinBl, panel_.blActiveHigh ? LOW : HIGH);

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

    begun_ = true;
    lastMs_ = millis();
    lastInputMs_ = lastMs_;

    if (config_.splashPage() && config_.splashSec() > 0) {
      prepareSplash();
      paintSplash();
      splashShowing_ = true;
      splashBlOnMs_ = lastMs_ + kSplashBacklightDelayMs;
      splashUntilMs_ = splashBlOnMs_ + config_.splashSec() * 1000u;
      return true;
    }

    openFirstApp();
    shell_.frame(canvas_, input_, 0.04f);
    backlightOn_ = true;
    display_.setBacklight(true);
    return true;
  }

  void tick() {
    if (!begun_) return;
    if (backlightOn_) display_.setBacklight(true);

    if (splashShowing_) {
      const uint32_t now = millis();
      if (!backlightOn_ && now >= splashBlOnMs_) {
        backlightOn_ = true;
        display_.setBacklight(true);
      }
      if (now < splashUntilMs_) {
        if (now - lastMs_ < 16) return;
        lastMs_ = now;
        return;
      }
      finishSplash();
      return;
    }

    const uint32_t now = millis();
    // Sampled around poll() so only real source events count as activity —
    // the shell re-queues events later in the frame and would look like input.
    const uint32_t eventsBefore = input_.eventCount();
    input_.poll(now);
    const bool activity = input_.eventCount() != eventsBefore;
    if (activity) lastInputMs_ = now;

    AppBase *const app = activeApp();
    const bool mayIdle = !app || app->allowsIdle();

    if (idleShowing_) {
      if (now - lastMs_ < 16) return;
      const float dt = (now - lastMs_) / 1000.0f;
      lastMs_ = now;

      // Keep the foreground app ticking under the eyes (IdleEyes contract).
      // Without this, launcher never polls wakeWordTakeDetected() until a
      // joystick event dismisses the overlay.
      shell_.frame(canvas_, input_, dt);

      // Wake→Ask calls setActiveApp, which clears idleShowing_. Joystick /
      // allowsIdle=false still go through wakeFromIdle (drop the wake press).
      if (!idleShowing_) return;

      AppBase *const appNow = activeApp();
      if (activity || (appNow && !appNow->allowsIdle())) {
        wakeFromIdle();
        shell_.frame(canvas_, input_, 0.f);
        return;
      }

      idleEyes_.frame(canvas_, panelRect(), dt);
      return;
    }

    const uint32_t idleMs = config_.idleEyesSec() * 1000u;
    if (idleMs && mayIdle && (now - lastInputMs_) >= idleMs) {
      idleShowing_ = true;
      idleEyes_.reset();
      lastMs_ = now;
      return;
    }

    if (now - lastMs_ < 16) return;
    const float dt = (now - lastMs_) / 1000.0f;
    lastMs_ = now;

    shell_.frame(canvas_, input_, dt);
  }

private:
  void openFirstApp() {
    if (AppBase *a = activeApp()) {
      shell_.setApp(a);
      if (a->appName()[0]) {
        RamManager::setForeground(a->appName());
      }
      if (!a->open()) {
        Serial.println("App: open failed — using defaults");
      }
    }
  }

  void prepareSplash() {
    Page *p = config_.splashPage();
    if (!p) return;
    p->setViewport(panelRect());
    const char *path = config_.splashImage();
    if (!path || !path[0]) return;

    p->setContentBackground(0);
    p->beginUI();
    if (storage_ && storage_->ready() &&
        splashLoadPng(storage_, path, &splashPixels_, &splashW_, &splashH_)) {
      p->add(p->image(splashPixels_, splashW_, splashH_)
                 .style(Style()
                            .setWidth(Length::Pct(100))
                            .setHeight(Length::Px(panel_.height))
                            .setFit(ImageFit::Contain)));
    } else {
      Serial.printf("Splash: failed to load %s\n", path);
    }
  }

  void paintSplash() {
    Page *p = config_.splashPage();
    if (!p) return;
    p->setViewport(panelRect());
    p->setContentBackground(0);
    canvas_.clear(0);
    p->layoutUI(canvas_);
    p->invalidateContent();
    p->drawUI(canvas_, true);
  }

  void finishSplash() {
    splashShowing_ = false;
    if (Page *p = config_.splashPage()) p->beginUI();
    splashFreePng(splashPixels_);
    splashPixels_ = nullptr;
    splashW_ = 0;
    splashH_ = 0;
    lastMs_ = millis();
    lastInputMs_ = lastMs_;
    openFirstApp();
    shell_.frame(canvas_, input_, 0.04f);
  }

  /**
   * The press that wakes the device only wakes it — dropping the queue stops
   * the app also acting on it. Content under the eyes may be stale on the
   * panel, so the active app is told to redraw.
   */
  void wakeFromIdle() {
    idleShowing_ = false;
    lastInputMs_ = millis();
    UIEvent drop;
    while (input_.pop(drop)) {
    }
    if (AppBase *a = activeApp()) a->invalidateContent();
  }

  static void ramDrainedNotify(const char *requester, void *ctx) {
    auto *self = static_cast<Flow32 *>(ctx);
    AppBase *a = self->activeApp();
    if (!a) return;
    const char *name = a->appName();
    if (requester && requester[0] && name && name[0] &&
        strcmp(name, requester) != 0) {
      return;
    }
    a->onRamDrained();
  }

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

  IdleEyes idleEyes_{};
  uint32_t lastInputMs_ = 0;
  bool idleShowing_ = false;

  static constexpr uint32_t kSplashBacklightDelayMs = 1000;
  uint16_t *splashPixels_ = nullptr;
  int16_t splashW_ = 0;
  int16_t splashH_ = 0;
  uint32_t splashUntilMs_ = 0;
  uint32_t splashBlOnMs_ = 0;
  bool splashShowing_ = false;
  bool backlightOn_ = false;
};
