#include <Arduino.h>

#include <Flow32.h>
#include "panels.h"
#include "storage.h"

// Active panel — switch for your glass.
// static const DisplayPanel kPanel = Panel18();
static const DisplayPanel kPanel = Panel183();

static constexpr bool kUIDebugBorders = false;

Display display(kPanel);
Canvas canvas(display);

static Page page(Rect(0, 0, kPanel.width, kPanel.height));

static InputHub input;
static SerialInput serialInput;

static Storage storage(SdDefault());
static ColorEmojiSd emojiSd;

static bool gWifiOn = true;
static bool gNotifyOn = false;
static int16_t gVolume = 40;

static void onBtnPress(UIButton &btn) {
  Serial.printf("Select → button color=%d variant=%d\n", (int)btn.color(),
                (int)btn.variant());
}

static void onToggleChange(UIToggle &t) {
  Serial.printf("Toggle → checked=%d color=%d\n", (int)t.checked(),
                (int)t.color());
}

static bool onBodyEvent(UINode & /*self*/, UIEvent &e) {
  if (e.key == UIKey::Back && e.phase == UIKeyPhase::Down) {
    Serial.println("Back bubbled to body");
    return true;
  }
  return false;
}

static UIButton &makeBtn(Page &p, const char *label, ButtonColor color,
                         ButtonVariant variant, bool isDisabled = false) {
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

static void onWifiToggle(UIToggle &t) {
  gWifiOn = t.checked();
  onToggleChange(t);
}

static void onNotifyToggle(UIToggle &t) {
  gNotifyOn = t.checked();
  onToggleChange(t);
}

static void onVolumeChange(UIRange &r) {
  gVolume = r.value();
  Serial.printf("Range → volume=%d\n", (int)gVolume);
}

static UIDiv &toggleRow(Page &p, const char *label, bool state, ButtonColor color,
                        UIToggle::ChangeFn onChange) {
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

static UIDiv &rangeRow(Page &p, const char *label, int16_t value,
                       ButtonColor color, UIRange::ChangeFn onChange) {
  const Theme::ThemeTokens &th = Theme::active();
  char valueBuf[8];
  snprintf(valueBuf, sizeof(valueBuf), "%d", (int)value);
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
               .add(p.text(valueBuf).style(
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

/** Small nested card for grid cells. */
static UIDiv &statCard(Page &p, const char *emoji, const char *label,
                       uint16_t fg, uint16_t cardBg) {
  return p.div()
      .style(Style()
                 .setWidth(Length::Pct(100))
                 .setPadding(Edges(8, 6))
                 .setGap(2)
                 .setColumns(1)
                 .setAlignH(Align::Center)
                 .setBackground(cardBg)
                 .setRadius(12))
      .add(p.text(emoji).style(Style()
                                   .setFont(FontRole::Body)
                                   .setColor(fg)
                                   .setEmojiSize(22)
                                   .setAlign(Align::Center)
                                   .setWidth(Length::Pct(100))))
      .add(p.text(label).style(Style()
                                   .setFont(FontRole::Small)
                                   .setColor(fg)
                                   .setAlign(Align::Center)
                                   .setWidth(Length::Pct(100))));
}

static void drawFrame(float dt) {
  const Theme::ThemeTokens &th = Theme::active();
  const uint16_t bg = th.base100;
  const uint16_t fg = th.baseContent;
  const uint16_t muted = Theme::lerp(th.baseContent, th.base100, 0.4f);
  const uint16_t card = th.base200;

  page.setContentBackground(bg);

  const bool animOnly = page.uiAnimating() && input.empty();
  if (!animOnly) {
    page.beginUI();

    // Header: title left, chip-like status right (2-col row).
    auto &header =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setColumns(2)
                       .setGap(8)
                       .setAlignV(Align::Center))
            .add(page.div()
                     .style(Style()
                                .setWidth(Length::Pct(100))
                                .setColumns(1)
                                .setGap(2))
                     .add(page.text("Home 🏠")
                              .style(Style()
                                         .setFont(FontRole::BodyLarge)
                                         .setColor(fg)
                                         .setEmojiSize(28)
                                         .setWidth(Length::Pct(100))))
                     .add(page.text("nested grid · stacks")
                              .style(Style()
                                         .setFont(FontRole::Small)
                                         .setColor(muted)
                                         .setWidth(Length::Pct(100)))))
            .add(page.div()
                     .style(Style()
                                .setWidth(Length::Pct(100))
                                .setPadding(Edges(6, 8))
                                .setBackground(th.base300)
                                .setRadius(th.radiusField)
                                .setColumns(1)
                                .setAlignH(Align::Center)
                                .setAlignV(Align::Center))
                     .add(page.text("Online 😎")
                              .style(Style()
                                         .setFont(FontRole::Small)
                                         .setColor(fg)
                                         .setEmojiSize(16)
                                         .setAlign(Align::Center)
                                         .setWidth(Length::Pct(100)))));

    // Stats strip: 3 equal columns.
    auto &stats =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setColumns(3)
                       .setGap(6)
                       .setAlignV(Align::Start))
            .add(statCard(page, "🔥", "Heat", fg, card))
            .add(statCard(page, "💧", "Humid", fg, card))
            .add(statCard(page, "⚡", "Power", fg, card));

    // Body: 2 columns — actions | tip card.
    auto &actions =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setColumns(1)
                       .setGap(6))
            .add(toggleRow(page, "Wi-Fi", gWifiOn, ButtonColor::Primary,
                           onWifiToggle))
            .add(toggleRow(page, "Alerts", gNotifyOn, ButtonColor::Accent,
                           onNotifyToggle))
            .add(rangeRow(page, "Volume", gVolume, ButtonColor::Secondary,
                          onVolumeChange))
            .add(makeBtn(page, "Primary", ButtonColor::Primary,
                         ButtonVariant::Solid))
            .add(makeBtn(page, "Outline", ButtonColor::Primary,
                         ButtonVariant::Outline))
            .add(makeBtn(page, "Disabled", ButtonColor::Primary,
                         ButtonVariant::Solid, true));

    auto &tip =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setPadding(Edges(10, 8))
                       .setGap(6)
                       .setColumns(1)
                       .setAlignV(Align::Center)
                       .setBackground(card)
                       .setRadius(14))
            .add(page.text("Tip 💡")
                     .style(Style()
                                .setFont(FontRole::Body)
                                .setColor(fg)
                                .setEmojiSize(20)
                                .setWidth(Length::Pct(100))))
            .add(page.text("Nest divs with columns 1–3 and alignH / alignV.")
                     .style(Style()
                                .setFont(FontRole::Small)
                                .setColor(muted)
                                .setLineHeight(16)
                                .setWidth(Length::Pct(100))));

    auto &split =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setColumns(2)
                       .setGap(8)
                       .setAlignV(Align::Center))
            .add(actions)
            .add(tip);

    // Footer actions: 2-col, centered cells.
    auto &footer =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setColumns(2)
                       .setGap(8)
                       .setAlignH(Align::Center))
            .add(makeBtn(page, "Soft", ButtonColor::Secondary,
                         ButtonVariant::Soft))
            .add(makeBtn(page, "Accent", ButtonColor::Accent,
                         ButtonVariant::Solid));

    auto &body =
        page.div()
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

    page.add(body);
    page.layoutUI(canvas);
    page.syncFocus();
  }
  input.dispatchTo(page);

  page.tick(dt);
  page.drawUI(canvas);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Theme::setActive(Theme::WinterTheme());
  Theme::setActive(Theme::FlowTheme());

  Serial.printf("Flow32 demo | theme=%s | Panel %s %dx%d\n", Theme::active().name,
                kPanel.id, kPanel.width, kPanel.height);

  // microSD first — emoji atlas lives on the card (not in flash).
  if (storage.begin()) {
    storage.printInfo();
    if (emojiSd.begin(storage)) {
      canvas.setEmojiSd(&emojiSd);
    } else {
      Serial.println(
          "Emoji atlas missing — copy sd/flow32/emoji.atlas onto the card");
    }
  } else {
    Serial.println("SD mount failed — check card + pins in src/storage.h");
  }

  pinMode(kPanel.pinBl, OUTPUT);
  digitalWrite(kPanel.pinBl, HIGH);

  if (!display.begin()) {
    Serial.println("Display begin failed");
    return;
  }
  display.setBacklight(true);
  Serial.println("Display begin ok");

  input.add(serialInput);
  input.begin();

  UIDebug::borders = kUIDebugBorders;

  drawFrame(0.04f);
}

void loop() {
  display.setBacklight(true);

  const uint32_t now = millis();
  input.poll(now);

  static uint32_t last = 0;
  if (now - last < 16) return;
  const float dt = (now - last) / 1000.0f;
  last = now;

  drawFrame(dt);
}
