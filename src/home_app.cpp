#include "home_app.h"

#include <string.h>

HomeApp *HomeApp::self_ = nullptr;

void HomeApp::iconText(char *out, size_t cap, const char *iconName,
                       const char *label) {
  if (!out || cap == 0) return;
  out[0] = '\0';
  size_t n = 0;
  if (icons_ && icons_->ready()) {
    n = icons_->utf8(iconName, out, cap);
  }
  if (n + 1 < cap && label && *label) {
    out[n++] = ' ';
    out[n] = '\0';
    strncpy(out + n, label, cap - n - 1);
    out[cap - 1] = '\0';
  } else if (n < cap) {
    out[n] = '\0';
  }
}

void HomeApp::onBtnPress(UIButton &btn) {
  Serial.printf("Select → button color=%d variant=%d\n", (int)btn.color(),
                (int)btn.variant());
}

bool HomeApp::onBodyEvent(UINode & /*self*/, UIEvent &e) {
  if (!self_) return false;
  if (e.key == UIKey::Back && e.phase == UIKeyPhase::Down) {
    if (self_->pageIndex() != Main) {
      self_->goTo(Main);
      return true;
    }
    Serial.println("Back bubbled to body");
    return true;
  }
  return false;
}

void HomeApp::onWifiToggle(UIToggle &t) {
  if (!self_) return;
  self_->setWifi(t.checked());
  Serial.printf("wifiOn=%d\n", (int)self_->state().wifiOn);
}

void HomeApp::onNotifyToggle(UIToggle &t) {
  if (!self_) return;
  self_->setNotify(t.checked());
  Serial.printf("notifyOn=%d\n", (int)self_->state().notifyOn);
}

void HomeApp::onVolumeChange(UIRange &r) {
  if (!self_) return;
  self_->setVolume(r.value());
  Serial.printf("volume=%d\n", (int)self_->state().volume);
}

void HomeApp::onThemeSelect(UISelect &s) {
  if (!self_) return;
  self_->setTheme(s.selected());
  const UISelectOption *opt =
      s.optionAt(static_cast<uint8_t>(self_->state().theme));
  Serial.printf("theme=%d title=%s\n", (int)self_->state().theme,
                opt && opt->title() ? opt->title() : "?");
}

void HomeApp::onOpenTheme(UIButton & /*btn*/) {
  if (!self_) return;
  self_->goTo(Theme);
}

void HomeApp::onBackMain(UIButton & /*btn*/) {
  if (!self_) return;
  self_->goTo(Main);
}

UIButton &HomeApp::makeBtn(Page &p, const char *label, ButtonColor color,
                           ButtonVariant variant, bool isDisabled) {
  const Theme::ThemeTokens &th = Theme::active();
  return p.button()
      .color(color)
      .variant(variant)
      .onPress(onBtnPress)
      .disabled(isDisabled)
      .disabledBackdrop(th.base100)
      .style(Style()
                 .setWidth(Length::Pct(100))
                 .setPadding(Edges(8, 12))
                 .setRadius(th.radiusField))
      .add(p.text(label).style(
          Style().setFont(FontRole::Small).setWidth(Length::Pct(100))));
}

UIDiv &HomeApp::toggleRow(Page &p, const char *label, bool state,
                          ButtonColor color, UIToggle::ChangeFn onChange) {
  const Theme::ThemeTokens &th = Theme::active();
  return p.div()
      .style(Style()
                 .setWidth(Length::Pct(100))
                 .setColumns(2)
                 .setGap(8)
                 .setAlignV(Align::Center))
      .add(p.text(label).style(Style()
                                   .setFont(FontRole::Small)
                                   .setColor(th.baseContent)
                                   .setWidth(Length::Pct(100))))
      .add(p.toggle().color(color).checked(state).onChange(onChange));
}

UIDiv &HomeApp::rangeRow(Page &p, const char *label, int16_t value,
                         const char *valueLabel, ButtonColor color,
                         UIRange::ChangeFn onChange) {
  const Theme::ThemeTokens &th = Theme::active();
  return p.div()
      .style(Style()
                 .setWidth(Length::Pct(100))
                 .setColumns(1)
                 .setGap(4))
      .add(p.div()
               .style(Style()
                          .setWidth(Length::Pct(100))
                          .setColumns(2)
                          .setGap(8)
                          .setAlignV(Align::Center))
               .add(p.text(label).style(Style()
                                            .setFont(FontRole::Small)
                                            .setColor(th.baseContent)
                                            .setWidth(Length::Pct(100))))
               .add(p.text(valueLabel).style(
                   Style()
                       .setFont(FontRole::Small)
                       .setColor(Theme::lerp(th.baseContent, th.base100, 0.35f))
                       .setAlign(Align::End)
                       .setWidth(Length::Pct(100)))))
      .add(p.range()
               .color(color)
               .min(0)
               .max(100)
               .step(5)
               .value(value)
               .onChange(onChange));
}

UIDiv &HomeApp::statCard(Page &p, const char *line, uint16_t fg,
                         uint16_t cardBg) {
  return p.div()
      .style(Style()
                 .setWidth(Length::Pct(100))
                 .setPadding(Edges(8, 6))
                 .setGap(2)
                 .setColumns(1)
                 .setAlignH(Align::Center)
                 .setBackground(cardBg)
                 .setRadius(12))
      .add(p.text(line).style(Style()
                                  .setFont(FontRole::Body)
                                  .setColor(fg)
                                  .setIconSize(22)
                                  .setAlign(Align::Center)
                                  .setWidth(Length::Pct(100))));
}

void HomeApp::build(Page &page, uint8_t pageIndex) {
  if (pageIndex == Theme) {
    buildTheme(page);
  } else {
    buildMain(page);
  }
}

void HomeApp::buildMain(Page &p) {
  const HomeState &st = state();
  const Theme::ThemeTokens &th = Theme::active();
  const uint16_t fg = th.baseContent;
  const uint16_t muted = Theme::lerp(th.baseContent, th.base100, 0.4f);
  const uint16_t card = th.base200;

  iconText(titleLine_, sizeof(titleLine_), "house", "Home");
  iconText(onlineLine_, sizeof(onlineLine_), "radio", "Online");
  iconText(heatLine_, sizeof(heatLine_), "flame", "Heat");
  iconText(humidLine_, sizeof(humidLine_), "droplet", "Humid");
  iconText(powerLine_, sizeof(powerLine_), "zap", "Power");

  auto &header =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(2)
                     .setGap(8)
                     .setAlignV(Align::Center))
          .add(p.div()
                   .style(Style()
                              .setWidth(Length::Pct(100))
                              .setColumns(1)
                              .setGap(2))
                   .add(p.text(titleLine_).style(
                       Style()
                           .setFont(FontRole::BodyLarge)
                           .setColor(fg)
                           .setIconSize(28)
                           .setWidth(Length::Pct(100))))
                   .add(p.text("nested grid · stacks")
                            .style(Style()
                                       .setFont(FontRole::Small)
                                       .setColor(muted)
                                       .setWidth(Length::Pct(100)))))
          .add(p.div()
                   .style(Style()
                              .setWidth(Length::Pct(100))
                              .setPadding(Edges(6, 8))
                              .setBackground(th.base300)
                              .setRadius(th.radiusField)
                              .setColumns(1)
                              .setAlignH(Align::Center)
                              .setAlignV(Align::Center))
                   .add(p.text(onlineLine_).style(
                       Style()
                           .setFont(FontRole::Small)
                           .setColor(fg)
                           .setIconSize(16)
                           .setAlign(Align::Center)
                           .setWidth(Length::Pct(100)))));

  auto &stats =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(3)
                     .setGap(6)
                     .setAlignV(Align::Start))
          .add(statCard(p, heatLine_, Theme::brand(ButtonColor::Primary), card))
          .add(statCard(p, humidLine_, Theme::brand(ButtonColor::Accent), card))
          .add(statCard(p, powerLine_, Theme::brand(ButtonColor::Secondary),
                        card));

  auto &actions =
      p.div()
          .style(Style().setWidth(Length::Pct(100)).setColumns(1).setGap(6))
          .add(toggleRow(p, "Wi-Fi", st.wifiOn, ButtonColor::Primary,
                         onWifiToggle))
          .add(toggleRow(p, "Alerts", st.notifyOn, ButtonColor::Accent,
                         onNotifyToggle))
          .add(rangeRow(p, "Volume", st.volume, st.volumeLabel,
                        ButtonColor::Secondary, onVolumeChange))
          .add(makeBtn(p, "Primary", ButtonColor::Primary, ButtonVariant::Solid))
          .add(makeBtn(p, "Outline", ButtonColor::Primary,
                       ButtonVariant::Outline))
          .add(makeBtn(p, "Disabled", ButtonColor::Primary, ButtonVariant::Solid,
                       true));

  auto &tip =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setPadding(Edges(10, 8))
                     .setGap(6)
                     .setColumns(1)
                     .setAlignV(Align::Center)
                     .setBackground(card)
                     .setRadius(14))
          .add(p.text("Tip 💡").style(Style()
                                          .setFont(FontRole::Body)
                                          .setColor(fg)
                                          .setEmojiSize(20)
                                          .setWidth(Length::Pct(100))))
          .add(p.text("HomeApp owns Main + Theme pages — Back returns here.")
                   .style(Style()
                              .setFont(FontRole::Small)
                              .setColor(muted)
                              .setWidth(Length::Pct(100))));

  auto &split =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(2)
                     .setGap(8)
                     .setAlignV(Align::Center))
          .add(actions)
          .add(tip);

  auto &footer =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(2)
                     .setGap(8)
                     .setAlignH(Align::Center))
          .add(makeBtn(p, "Soft", ButtonColor::Secondary, ButtonVariant::Soft))
          .add(p.button()
                   .color(ButtonColor::Accent)
                   .variant(ButtonVariant::Solid)
                   .icon("arrow-right", "right")
                   .onPress(onOpenTheme)
                   .style(Style()
                              .setWidth(Length::Pct(100))
                              .setPadding(Edges(8, 12))
                              .setRadius(th.radiusField))
                   .add(p.text("Theme").style(
                       Style()
                           .setFont(FontRole::Small)
                           .setWidth(Length::Pct(100)))));

  auto &body =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setPadding(Edges(16, 10))
                     .setGap(12)
                     .setColumns(1))
          .onEvent(onBodyEvent)
          .add(header)
          .add(stats)
          .add(split)
          .add(footer);

  p.add(body);
}

void HomeApp::buildTheme(Page &p) {
  const HomeState &st = state();
  const Theme::ThemeTokens &th = Theme::active();
  const uint16_t fg = th.baseContent;
  const uint16_t muted = Theme::lerp(th.baseContent, th.base100, 0.4f);

  iconText(titleLine_, sizeof(titleLine_), "palette", "Theme");

  auto &header =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(1)
                     .setGap(2))
          .add(p.text(titleLine_).style(Style()
                                            .setFont(FontRole::BodyLarge)
                                            .setColor(fg)
                                            .setIconSize(28)
                                            .setWidth(Length::Pct(100))))
          .add(p.text("Pick a look — saved with HomeApp state.")
                   .style(Style()
                              .setFont(FontRole::Small)
                              .setColor(muted)
                              .setWidth(Length::Pct(100))));

  auto &chooser =
      p.div()
          .style(Style().setWidth(Length::Pct(100)).setColumns(1).setGap(6))
          .add(p.select()
                   .selected(st.theme)
                   .onChange(onThemeSelect)
                   .style(Style().setWidth(Length::Pct(100)).setGap(6))
                   .add(p.selectOption()
                            .icon("sun")
                            .title("Flow")
                            .description("Default blue accents")
                            .value(0))
                   .add(p.selectOption()
                            .icon("snowflake")
                            .title("Winter")
                            .description("Cool light surfaces")
                            .value(1))
                   .add(p.selectOption()
                            .icon("sparkles")
                            .title("Accent")
                            .description("Emphasize brand color")
                            .value(2)));

  auto &nav =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setColumns(1)
                     .setGap(8))
          .add(p.button()
                   .color(ButtonColor::Secondary)
                   .variant(ButtonVariant::Outline)
                   .icon("arrow-left", "left")
                   .onPress(onBackMain)
                   .style(Style()
                              .setWidth(Length::Pct(100))
                              .setPadding(Edges(8, 12))
                              .setRadius(th.radiusField))
                   .add(p.text("Back").style(
                       Style()
                           .setFont(FontRole::Small)
                           .setWidth(Length::Pct(100)))));

  auto &body =
      p.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setPadding(Edges(16, 10))
                     .setGap(12)
                     .setColumns(1))
          .onEvent(onBodyEvent)
          .add(header)
          .add(chooser)
          .add(nav);

  p.add(body);
}
