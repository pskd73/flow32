#pragma once

#include <Flow32.h>

#include <stdio.h>

/**
 * Home demo — state + Main / Theme pages under the Flow32 Shell.
 * Magic/version gate older NVS layouts.
 */
struct HomeState {
  static constexpr uint32_t kMagic = 0x484F4D31u; // 'HOM1'
  static constexpr uint16_t kVersion = 1;

  uint32_t magic = kMagic;
  uint16_t version = kVersion;
  uint16_t reserved = 0;

  bool wifiOn = true;
  bool notifyOn = false;
  int16_t volume = 40;
  int16_t theme = 0; // 0 Flow, 1 Winter, 2 Accent (demo index)
  char volumeLabel[8] = "40";
};

class HomeApp : public App<HomeState> {
public:
  enum PageId : uint8_t { Main = 0, Theme = 1 };

  explicit HomeApp(const Rect &viewport) : App(viewport) {
    addPage("Home");
    addPage("Theme");
    self_ = this;
  }

  void setIcons(IconSd *icons) { icons_ = icons; }

  void setWifi(bool v) { set(data().wifiOn, v); }
  void setNotify(bool v) { set(data().notifyOn, v); }

  void setVolume(int16_t v) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    if (!set(data().volume, v)) return;
    syncVolumeLabel();
  }

  void setTheme(int16_t v) { set(data().theme, v); }

  uint8_t shellStatusCount() const override { return 2; }
  const char *shellStatusIcon(uint8_t i) const override {
    if (i == 0) return state().wifiOn ? "wifi" : "wifi-off";
    if (i == 1) return state().notifyOn ? "bell" : "bell-off";
    return nullptr;
  }

protected:
  const char *nvsNamespace() const override { return "home"; }

  void onOpen() override {
    self_ = this;
    if (!valid(state())) {
      data() = HomeState{};
    }
    syncVolumeLabel();
  }

  void onClose() override {
    if (self_ == this) self_ = nullptr;
  }

  void build(Page &page, uint8_t pageIndex) override;

private:
  static HomeApp *self_;
  IconSd *icons_ = nullptr;

  // Stable UTF-8 lines for UIText (pointers must outlive paint).
  char titleLine_[24] = {};
  char onlineLine_[24] = {};
  char heatLine_[20] = {};
  char humidLine_[20] = {};
  char powerLine_[20] = {};

  static bool valid(const HomeState &s) {
    return s.magic == HomeState::kMagic && s.version == HomeState::kVersion;
  }

  void syncVolumeLabel() {
    snprintf(data().volumeLabel, sizeof(data().volumeLabel), "%d",
             (int)data().volume);
  }

  void iconText(char *out, size_t cap, const char *iconName, const char *label);

  void buildMain(Page &p);
  void buildTheme(Page &p);

  static void onBtnPress(UIButton &btn);
  static void onWifiToggle(UIToggle &t);
  static void onNotifyToggle(UIToggle &t);
  static void onVolumeChange(UIRange &r);
  static void onThemeSelect(UISelect &s);
  static void onOpenTheme(UIButton &btn);
  static void onBackMain(UIButton &btn);

  UIButton &makeBtn(Page &p, const char *label, ButtonColor color,
                    ButtonVariant variant, bool isDisabled = false);
  UIDiv &toggleRow(Page &p, const char *label, bool state, ButtonColor color,
                   UIToggle::ChangeFn onChange);
  UIDiv &rangeRow(Page &p, const char *label, int16_t value,
                  const char *valueLabel, ButtonColor color,
                  UIRange::ChangeFn onChange);
  UIDiv &statCard(Page &p, const char *line, uint16_t fg, uint16_t cardBg);
};
