#pragma once

#include "App.h"
#include "Canvas.h"
#include "IconSd.h"
#include "Page.h"
#include "Rect.h"
#include "input/InputHub.h"
#include "ui/Style.h"
#include "ui/Theme.h"

#include <string.h>

/**
 * Flow32 shell: nav bar + content viewport around an AppBase.
 *
 *   Shell shell(Rect(0, 0, w, h));
 *   shell.setApp(home);
 *   shell.frame(canvas, input, dt);
 *
 * Nav: padded div, 2 columns — title | status icons (right-aligned, packed).
 * Fullscreen pages hide the bar and use the full panel.
 */
class Shell {
public:
  static constexpr int16_t kNavHeight = 32;
  /** Extra inset for curved panel edges. */
  static constexpr int16_t kNavPadLeft = 28;
  static constexpr int16_t kNavPadRight = 28;
  static constexpr int16_t kStatusIcon = 14;
  static constexpr uint8_t kMaxStatus = 4;

  explicit Shell(const Rect &panel)
      : panel_(panel),
        navPage_(Rect(panel.x, panel.y, panel.w, kNavHeight)) {}

  void setPanel(const Rect &panel) {
    panel_ = panel;
    navPage_.setViewport(Rect(panel_.x, panel_.y, panel_.w, navH_));
  }
  Rect panel() const { return panel_; }

  void setApp(AppBase *app) { app_ = app; }
  AppBase *app() const { return app_; }

  void setNavHeight(int16_t h) {
    if (h < 0) h = 0;
    navH_ = h;
    navPage_.setViewport(Rect(panel_.x, panel_.y, panel_.w, navH_));
  }
  int16_t navHeight() const { return navH_; }

  void frame(Canvas &canvas, InputHub &input, float dt) {
    if (!app_) return;

    app_->setPanel(panel_);
    const bool fullscreen = app_->shellFullscreen();
    const int16_t chrome = fullscreen ? 0 : navH_;
    const Rect content(panel_.x, static_cast<int16_t>(panel_.y + chrome),
                       panel_.w, static_cast<int16_t>(panel_.h - chrome));
    app_->setContentViewport(content);

    app_->frame(canvas, input, dt);

    if (!fullscreen) {
      canvas.setOrigin(0, 0);
      canvas.clearClip();
      drawNav(canvas);
    }
  }

private:
  Rect panel_{};
  AppBase *app_ = nullptr;
  int16_t navH_ = kNavHeight;
  Page navPage_;
  char statusLine_[48] = {};

  void buildStatusLine(IconSd *icons) {
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

  void drawNav(Canvas &canvas) {
    const Theme::ThemeTokens &th = Theme::active();
    const Rect nav(panel_.x, panel_.y, panel_.w, navH_);
    navPage_.setViewport(nav);
    navPage_.setContentBackground(th.base200);

    buildStatusLine(canvas.iconSd());

    const char *title = app_->shellTitle();
    if (!title) title = "";

    navPage_.beginUI();

    // [ title | icons→ ] with H padding for curved edges
    auto &row =
        navPage_.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setHeight(Length::Px(navH_))
                       .setPadding(Edges(0, kNavPadRight, 0, kNavPadLeft))
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
