#pragma once

#include "flow32/core/App.h"
#include "flow32/graphics/Canvas.h"
#include "flow32/assets/StreamedIconAtlas.h"
#include "flow32/ui/Page.h"
#include "flow32/graphics/Rect.h"
#include "flow32/input/InputHub.h"
#include "flow32/ui/Style.h"
#include "flow32/ui/Theme.h"

#include <string.h>

/**
 * Flow32 shell: nav bar + content viewport around an AppBase.
 *
 *   Shell shell(Rect(0, 0, w, h));
 *   shell.setHost(&flow);
 *   shell.setApp(home);
 *   shell.frame(canvas, input, dt);
 *
 * Owns the Back key: in-app goBack → clear focus → openHome.
 * Nav: padded div, 2 columns — title | status icons (right-aligned, packed).
 * Fullscreen pages hide the bar and use the full panel.
 */
class Shell {
public:
  static constexpr int16_t kNavHeight = 36;
  /** Extra inset for curved panel edges. */
  static constexpr int16_t kNavPadLeft = 28;
  static constexpr int16_t kNavPadRight = 28;
  static constexpr int16_t kNavPadY = 6;
  static constexpr int16_t kStatusIcon = 14;
  static constexpr uint8_t kMaxStatus = 4;

  explicit Shell(const Rect &panel)
      : panel_(panel),
        navPage_(Rect(panel.x, panel.y, panel.w, kNavHeight)) {}

  void setPanel(const Rect &panel) {
    if (panel.x == panel_.x && panel.y == panel_.y && panel.w == panel_.w &&
        panel.h == panel_.h) {
      return;
    }
    panel_ = panel;
    navPage_.setViewport(Rect(panel_.x, panel_.y, panel_.w, navH_));
    navDirty_ = true;
  }
  Rect panel() const { return panel_; }

  void setHost(AppHost *host) { host_ = host; }
  AppHost *host() const { return host_; }

  void setApp(AppBase *app) {
    if (app_ == app) return;
    app_ = app;
    navDirty_ = true;
  }
  AppBase *app() const { return app_; }

  void setNavHeight(int16_t h) {
    if (h < 0) h = 0;
    if (navH_ == h) return;
    navH_ = h;
    navPage_.setViewport(Rect(panel_.x, panel_.y, panel_.w, navH_));
    navDirty_ = true;
  }
  int16_t navHeight() const { return navH_; }

  void frame(Canvas &canvas, InputHub &input, float dt) {
    if (!app_) return;

    // Root Back may switch apps; apply layout to the active app afterwards.
    handleBack(input);
    if (!app_) return;

    app_->setPanel(panel_);
    const bool fullscreen = app_->shellFullscreen();
    const int16_t chrome = fullscreen ? 0 : navH_;
    const Rect content(panel_.x, static_cast<int16_t>(panel_.y + chrome),
                       panel_.w, static_cast<int16_t>(panel_.h - chrome));
    app_->setContentViewport(content);

    app_->frame(canvas, input, dt);

    if (fullscreen != lastFullscreen_) {
      navDirty_ = true;
      lastFullscreen_ = fullscreen;
    }

    if (!fullscreen) {
      canvas.setOrigin(0, 0);
      canvas.clearClip();
      buildStatusLine(canvas.streamedIcons());
      const char *title = app_->shellTitle();
      if (!title) title = "";
      const uint32_t titleHash = hashText(title);
      const uint32_t statusHash = hashText(statusLine_);
      const uint32_t themeGeneration = Theme::generation();
      if (drawnApp_ != app_ || titleHash != lastTitleHash_ ||
          statusHash != lastStatusHash_ ||
          themeGeneration != lastThemeGeneration_) {
        navDirty_ = true;
      }
      if (navDirty_) {
        drawNav(canvas, title);
        drawnApp_ = app_;
        lastTitleHash_ = titleHash;
        lastStatusHash_ = statusHash;
        lastThemeGeneration_ = themeGeneration;
        navDirty_ = false;
      }
    }
  }

private:
  Rect panel_{};
  AppHost *host_ = nullptr;
  AppBase *app_ = nullptr;
  int16_t navH_ = kNavHeight;
  Page navPage_;
  char statusLine_[48] = {};
  AppBase *drawnApp_ = nullptr;
  uint32_t lastTitleHash_ = 0;
  uint32_t lastStatusHash_ = 0;
  uint32_t lastThemeGeneration_ = 0;
  bool navDirty_ = true;
  bool lastFullscreen_ = false;

  static uint32_t hashText(const char *text) {
    uint32_t hash = 2166136261UL;
    if (!text) return hash;
    while (*text) {
      hash ^= static_cast<uint8_t>(*text++);
      hash *= 16777619UL;
    }
    return hash;
  }

  /**
   * Consume Back+Down from the queue (apps never see it).
   * Policy: goBack → clear focus → home app.
   */
  void handleBack(InputHub &input) {
    UIEvent kept[InputHub::kMaxQueue];
    uint8_t n = 0;
    bool backDown = false;
    UIEvent e;
    while (input.pop(e)) {
      if (e.key == UIKey::Back && e.phase == UIKeyPhase::Down) {
        backDown = true;
      } else if (e.key != UIKey::Back) {
        if (n < InputHub::kMaxQueue) kept[n++] = e;
      }
      // Back Up / other phases dropped — shell owns the key.
    }
    for (uint8_t i = 0; i < n; i++) input.push(kept[i]);
    if (!backDown || !app_) return;

    if (app_->canGoBack()) {
      app_->goBack();
      return;
    }
    if (app_->contentHasFocus()) {
      app_->clearContentFocus();
      return;
    }
    if (host_) host_->openHome();
  }

  void buildStatusLine(StreamedIconAtlas *icons) {
    statusLine_[0] = '\0';
    if (!app_ || !icons || !icons->ready()) return;

    uint8_t n = app_->shellStatusCount();
    if (n > kMaxStatus) n = kMaxStatus;

    size_t pos = 0;
    for (uint8_t i = 0; i < n; i++) {
      const char *name = app_->shellStatusIcon(i);
      if (!name || !name[0]) continue;
      char tmp[8];
      const size_t got = icons->utf8(name, tmp, sizeof(tmp));
      if (!got) continue;
      if (pos > 0 && pos + 1 < sizeof(statusLine_)) {
        statusLine_[pos++] = ' '; // tight gap between icons
        statusLine_[pos] = '\0';
      }
      if (pos + got >= sizeof(statusLine_)) break;
      memcpy(statusLine_ + pos, tmp, got);
      pos += got;
      statusLine_[pos] = '\0';
    }
  }

  void drawNav(Canvas &canvas, const char *title) {
    const Theme::ThemeTokens &th = Theme::active();
    const Rect nav(panel_.x, panel_.y, panel_.w, navH_);
    navPage_.setViewport(nav);
    navPage_.setContentBackground(th.base200);

    navPage_.beginUI();

    // [ title | icons→ ] with H padding for curved edges
    auto &row =
        navPage_.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setHeight(Length::Px(navH_))
                       .setPadding(Edges(kNavPadY, kNavPadRight, kNavPadY,
                                         kNavPadLeft))
                       .setColumns(2)
                       .setGap(8)
                       .setAlignV(Align::Center)
                       .setBackground(th.base200))
            .add(navPage_.text(title).style(
                Style()
                    .setFont(FontRole::Small)
                    .setColor(th.baseContent)
                    .setWidth(Length::Pct(100))))
            .add(navPage_.text(statusLine_).style(
                Style()
                    .setIconSize(static_cast<uint8_t>(kStatusIcon))
                    .setColor(th.baseContent)
                    .setAlign(Align::End)
                    .setWidth(Length::Pct(100))));

    navPage_.add(row);
    navPage_.layoutUI(canvas);
    navPage_.invalidateContent();
    navPage_.drawUI(canvas);

    canvas.setOrigin(0, 0);
    canvas.clearClip();
    canvas.fillRect(
        Rect(nav.x, static_cast<int16_t>(nav.y + nav.h - 1), nav.w, 1),
        th.base300);
    canvas.present(
        Rect(nav.x, static_cast<int16_t>(nav.y + nav.h - 1), nav.w, 1));
  }
};
