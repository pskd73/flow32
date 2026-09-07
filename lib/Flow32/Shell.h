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
 *   shell.setHost(&flow);
 *   shell.setApp(home);
 *   shell.frame(canvas, input, dt);
 *
 * Owns the Back key: in-app goBack → clear focus → openLauncher.
 * Nav: padded div, 2 columns — title | status icons (right-aligned, packed).
 * Fullscreen pages hide the bar and use the full panel.
 */
class Shell {
public:
  static constexpr int16_t kNavHeight = 36;
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

  void setHost(AppHost *host) { host_ = host; }
  AppHost *host() const { return host_; }

  void setApp(AppBase *app) {
    if (app_ != app) clearToast();
    app_ = app;
  }
  AppBase *app() const { return app_; }

  void setNavHeight(int16_t h) {
    if (h < 0) h = 0;
    navH_ = h;
    navPage_.setViewport(Rect(panel_.x, panel_.y, panel_.w, navH_));
  }
  int16_t navHeight() const { return navH_; }

  /**
   * Bottom toast: slides up, holds `durationMs`, slides down. Message copied.
   * Replaces any active toast. durationMs clamped to at least 500.
   */
  void showToast(const char *message, ToastKind kind = ToastKind::Info,
                 uint16_t durationMs = 2000) {
    if (!message) message = "";
    size_t n = 0;
    while (message[n] && n + 1 < sizeof(toastMsg_)) {
      toastMsg_[n] = message[n];
      n++;
    }
    toastMsg_[n] = '\0';
    if (!toastMsg_[0]) {
      clearToast();
      return;
    }
    toastKind_ = kind;
    if (durationMs < 500) durationMs = 500;
    toastHoldMs_ = durationMs;
    toastPhase_ = ToastPhase::In;
    toastT_ = 0.f;
    // Refresh content cache once. Do not invalidate every animation frame —
    // that SPI-presented the page without the toast and caused flicker.
    if (app_) app_->invalidateContent();
  }

  void clearToast() {
    if (toastPhase_ == ToastPhase::Idle) return;
    toastPhase_ = ToastPhase::Idle;
    toastT_ = 0.f;
    toastMsg_[0] = '\0';
    if (app_) app_->invalidateContent();
  }

  bool toastVisible() const {
    return toastPhase_ != ToastPhase::Idle && toastMsg_[0];
  }

  void frame(Canvas &canvas, InputHub &input, float dt) {
    if (!app_) return;

    // May switch apps (root Back → launcher); apply layout to the active app after.
    handleBack(input);
    if (!app_) return;

    tickToast(dt);

    app_->setPanel(panel_);
    const bool fullscreen = app_->shellFullscreen();
    const int16_t chrome = fullscreen ? 0 : navH_;
    const Rect content(panel_.x, static_cast<int16_t>(panel_.y + chrome),
                       panel_.w, static_cast<int16_t>(panel_.h - chrome));
    app_->setContentViewport(content);

    app_->frame(canvas, input, dt);

    const bool suppressPresent =
        host_ && (host_->toastVisible() || host_->overlaySuppressesPresent());

    if (!fullscreen) {
      canvas.setOrigin(0, 0);
      canvas.clearClip();
      drawNav(canvas, /*allowPresent=*/!suppressPresent);
    }

    if (toastVisible()) {
      // Same model as scroll: blit cache → composite overlay → one SPI present.
      // Always present while visible so a theme/content change mid-hold still
      // lands on the glass (app present is suppressed for the toast lifetime).
      app_->blitContentToPanel(canvas.display());
      drawToast(canvas, content, /*doPresent=*/true);
    }
  }

private:
  Rect panel_{};
  AppHost *host_ = nullptr;
  AppBase *app_ = nullptr;
  int16_t navH_ = kNavHeight;
  Page navPage_;
  /** Lucide "circle" → solid disc; other names → UIIcon via lucide name. */
  struct StatusSlot {
    const char *name = nullptr;
    uint16_t color = 0;
    bool solid = false;
  };
  StatusSlot statusSlots_[kMaxStatus] = {};
  uint8_t statusSlotCount_ = 0;

  enum class ToastPhase : uint8_t { Idle, In, Hold, Out };
  static constexpr size_t kToastMsgLen = 48;
  static constexpr float kToastInSec = 0.28f;
  static constexpr float kToastOutSec = 0.24f;
  char toastMsg_[kToastMsgLen] = {};
  ToastKind toastKind_ = ToastKind::Info;
  ToastPhase toastPhase_ = ToastPhase::Idle;
  float toastT_ = 0.f; // 0 = off-screen below, 1 = rested
  uint16_t toastHoldMs_ = 2000;
  uint32_t toastHoldUntilMs_ = 0;

  static float easeOutCubic(float t) {
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    const float u = 1.f - t;
    return 1.f - u * u * u;
  }

  void tickToast(float dt) {
    if (toastPhase_ == ToastPhase::Idle) return;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.05f) dt = 0.05f;

    if (toastPhase_ == ToastPhase::In) {
      toastT_ += dt / kToastInSec;
      if (toastT_ >= 1.f) {
        toastT_ = 1.f;
        toastPhase_ = ToastPhase::Hold;
        toastHoldUntilMs_ = millis() + toastHoldMs_;
      }
    } else if (toastPhase_ == ToastPhase::Hold) {
      if (static_cast<int32_t>(millis() - toastHoldUntilMs_) >= 0) {
        toastPhase_ = ToastPhase::Out;
      }
    } else if (toastPhase_ == ToastPhase::Out) {
      toastT_ -= dt / kToastOutSec;
      if (toastT_ <= 0.f) {
        toastT_ = 0.f;
        toastPhase_ = ToastPhase::Idle;
        toastMsg_[0] = '\0';
        // One clean repaint without the toast.
        if (app_) app_->invalidateContent();
      }
    }
  }

  /**
   * Consume Back+Down from the queue (apps never see it).
   * Policy: goBack → clear focus → launcher.
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
    if (host_) host_->openLauncher();
  }

  void buildStatusSlots(IconSd *icons) {
    statusSlotCount_ = 0;
    if (!app_) return;

    uint8_t n = app_->shellStatusCount();
    if (n > kMaxStatus) n = kMaxStatus;

    for (uint8_t i = 0; i < n; i++) {
      const char *name = app_->shellStatusIcon(i);
      if (!name || !name[0]) continue;
      StatusSlot &slot = statusSlots_[statusSlotCount_];
      slot = StatusSlot{};
      uint16_t c = app_->shellStatusColor(i);
      slot.color = c ? c : Theme::active().baseContent;
      // "circle" = solid health/status disc (outline Lucide circle looks hollow).
      if (!strcmp(name, "circle")) {
        slot.solid = true;
        statusSlotCount_++;
        continue;
      }
      if (!icons || !icons->ready()) continue;
      if (!icons->findByName(name)) continue;
      slot.name = name;
      statusSlotCount_++;
    }
  }

  void drawNav(Canvas &canvas, bool allowPresent = true) {
    const Theme::ThemeTokens &th = Theme::active();
    const Rect nav(panel_.x, panel_.y, panel_.w, navH_);
    navPage_.setViewport(nav);
    navPage_.setContentBackground(th.base200);

    buildStatusSlots(canvas.iconSd());

    const char *title = app_->shellTitle();
    if (!title) title = "";

    navPage_.beginUI();

    auto &statusArea =
        navPage_.div().style(Style()
                                 .setWidth(Length::Pct(100))
                                 .setHeight(Length::Px(navH_)));
    {
      constexpr int16_t kGap = 4;
      constexpr int16_t kDot = 10;
      int16_t right = 0;
      // Pack trailing-first so index 0 is leftmost in the cluster.
      for (int8_t i = static_cast<int8_t>(statusSlotCount_) - 1; i >= 0; i--) {
        const StatusSlot &slot = statusSlots_[i];
        if (slot.solid) {
          statusArea.add(
              navPage_.div().style(Style()
                                       .setPosition(Position::Absolute)
                                       .setRight(Length::Px(right))
                                       .setWidth(Length::Px(kDot))
                                       .setHeight(Length::Px(kDot))
                                       .setRadius(static_cast<uint8_t>(kDot / 2))
                                       .setBackground(slot.color)));
          right = static_cast<int16_t>(right + kDot + kGap);
        } else if (slot.name && slot.name[0]) {
          statusArea.add(navPage_.icon(slot.name)
                             .style(Style()
                                        .setPosition(Position::Absolute)
                                        .setRight(Length::Px(right))
                                        .setWidth(Length::Px(kStatusIcon))
                                        .setHeight(Length::Px(kStatusIcon))
                                        .setIconSize(
                                            static_cast<uint8_t>(kStatusIcon))
                                        .setColor(slot.color)
                                        .setAlignH(Align::Center)
                                        .setAlignV(Align::Center)));
          right = static_cast<int16_t>(right + kStatusIcon + kGap);
        }
      }
    }

    // [ title | status→ ] with H padding for curved edges.
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
                    .setFont(FontRole::Title)
                    .setColor(th.baseContent)
                    .setWidth(Length::Pct(100))))
            .add(statusArea);

    navPage_.add(row);
    navPage_.layoutUI(canvas);
    navPage_.invalidateContent();
    navPage_.drawUI(canvas, allowPresent);

    canvas.setOrigin(0, 0);
    canvas.clearClip();
    canvas.fillRect(
        Rect(nav.x, static_cast<int16_t>(nav.y + nav.h - 1), nav.w, 1),
        th.base300);
    if (allowPresent) {
      canvas.present(
          Rect(nav.x, static_cast<int16_t>(nav.y + nav.h - 1), nav.w, 1));
    }
  }

  void drawToast(Canvas &canvas, const Rect &content, bool doPresent) {
    if (toastPhase_ == ToastPhase::Idle || !toastMsg_[0]) return;

    const Theme::ThemeTokens &th = Theme::active();
    uint16_t bg = th.neutral;
    uint16_t fg = th.neutralContent;
    switch (toastKind_) {
    case ToastKind::Success:
      bg = th.success;
      fg = th.successContent;
      break;
    case ToastKind::Warning:
      bg = th.warning;
      fg = th.warningContent;
      break;
    case ToastKind::Error:
      bg = th.error;
      fg = th.errorContent;
      break;
    case ToastKind::Info:
    default:
      break;
    }

    TextStyle ts;
    ts.font = FontRole::Small;
    ts.color = fg;
    ts.align = Align::Center;
    // Small AA face baseline is 18 / yAdvance ~25. Without lineHeight, drawText
    // requires the full yAdvance to fit the box and paints nothing in 14px.
    constexpr int16_t kLineH = 18;
    ts.lineHeight = static_cast<uint8_t>(kLineH);

    constexpr int16_t kPadX = 10;
    constexpr int16_t kPadY = 6;
    constexpr int16_t kMarginX = 20;
    constexpr int16_t kMarginBottom = 12;

    const int16_t maxInner =
        static_cast<int16_t>(panel_.w - 2 * kMarginX - 2 * kPadX);
    int16_t tw = canvas.measureTextWidth(toastMsg_, ts);
    if (tw > maxInner) tw = maxInner;
    if (tw < 1) tw = 1;

    const int16_t boxW = static_cast<int16_t>(tw + 2 * kPadX);
    const int16_t boxH = static_cast<int16_t>(kLineH + 2 * kPadY);
    const int16_t restY =
        static_cast<int16_t>(panel_.y + panel_.h - boxH - kMarginBottom);
    // Travel: fully below the panel bottom → rested inset.
    const int16_t travel = static_cast<int16_t>(boxH + kMarginBottom + 4);
    const float visible = easeOutCubic(toastT_);
    const int16_t y = static_cast<int16_t>(
        restY + static_cast<int16_t>((1.f - visible) * travel + 0.5f));
    const int16_t x =
        static_cast<int16_t>(panel_.x + (panel_.w - boxW) / 2);
    const Rect box(x, y, boxW, boxH);

    // Clip to panel so the enter/exit doesn't paint past the glass.
    canvas.setOrigin(0, 0);
    canvas.setClip(panel_);
    canvas.fillRoundRect(box, th.radiusBox ? th.radiusBox : 8, bg);
    // drawText baselines at penY+fontBaseline (18). Optically center capital
    // ink (~12px) in the toast pill — same trick as UISelect titles.
    constexpr int16_t kSmallCapH = 12;
    const int16_t textY = static_cast<int16_t>(
        y + boxH / 2 - kLineH + kSmallCapH / 2);
    canvas.drawText(Rect(static_cast<int16_t>(x + kPadX), textY, tw,
                         static_cast<int16_t>(kLineH + 4)),
                    toastMsg_, ts, false);
    canvas.clearClip();

    if (!doPresent) return;

    // Full content width — vacated toast pixels were restored by the blit.
    if (content.w > 0 && content.h > 0) {
      canvas.present(content);
    }
  }
};
