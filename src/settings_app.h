#pragma once

#include <Flow32.h>
#include <Preferences.h>

/**
 * System settings — theme (and later more) in NVS-backed state.
 */
struct SettingsState {
  static constexpr uint32_t kMagic = 0x53455431u; // 'SET1'
  static constexpr uint16_t kVersion = 1;

  uint32_t magic = kMagic;
  uint16_t version = kVersion;
  uint16_t reserved = 0;

  /** 0 = Flow (dark), 1 = Winter (light). */
  int16_t theme = 0;
};

class SettingsApp : public App<SettingsState> {
public:
  explicit SettingsApp(const Rect &viewport, const char *name = "Settings",
                       const char *icon = "settings")
      : App(viewport) {
    setAppInfo(name, icon);
    addPage(name);
    self_ = this;
  }

  void onAssets(IconSd * /*icons*/, ColorEmojiSd * /*emoji*/) override {
    // Apply saved theme at boot (before Settings is opened).
    applyThemeFromNvs();
  }

  void setTheme(int16_t id) {
    if (id < 0) id = 0;
    if (id > 1) id = 1;
    set(data().theme, id);
    applyThemeId(id);
  }

protected:
  const char *nvsNamespace() const override { return "settings"; }

  void onOpen() override {
    self_ = this;
    if (state().magic != SettingsState::kMagic ||
        state().version != SettingsState::kVersion) {
      data() = SettingsState{};
    }
    if (state().theme < 0 || state().theme > 1) {
      data().theme = 0;
    }
    applyThemeId(state().theme);
  }

  void onClose() override {
    if (self_ == this) self_ = nullptr;
  }

  void build(Page &page, uint8_t /*pageId*/) override;

private:
  static SettingsApp *self_;

  static void applyThemeId(int16_t id) {
    if (id == 1) {
      Theme::setActive(Theme::WinterTheme());
    } else {
      Theme::setActive(Theme::FlowTheme());
    }
  }

  static void applyThemeFromNvs() {
    Preferences prefs;
    if (!prefs.begin("settings", /*readOnly=*/true)) return;
    SettingsState loaded{};
    const size_t len = prefs.getBytesLength("state");
    if (len == sizeof(SettingsState) &&
        prefs.getBytes("state", &loaded, sizeof(loaded)) == sizeof(loaded) &&
        loaded.magic == SettingsState::kMagic &&
        loaded.version == SettingsState::kVersion && loaded.theme >= 0 &&
        loaded.theme <= 1) {
      applyThemeId(loaded.theme);
    }
    prefs.end();
  }

  static void onThemeSelect(UISelect &s);
};
