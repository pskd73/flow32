#include <Arduino.h>

#include <Flow32.h>
#include "panels.h"

// Active panel — switch for your glass.
// static const DisplayPanel kPanel = Panel18();
static const DisplayPanel kPanel = Panel183();

static constexpr bool kUIDebugBorders = false;

Display display(kPanel);
Canvas canvas(display);

static Page page(Rect(0, 0, kPanel.width, kPanel.height));

static InputHub input;
static SerialInput serialInput;

static void onBtnPress(UIButton &btn) {
  Serial.printf("Select → button color=%d variant=%d\n", (int)btn.color(),
                (int)btn.variant());
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
  auto &btn = p.button()
                  .color(color)
                  .variant(variant)
                  .onPress(onBtnPress)
                  .disabled(isDisabled)
                  .disabledBackdrop(Display::color565(48, 32, 28))
                  .style(Style()
                             .setWidth(Length::Pct(100))
                             .setPadding(Edges(8, 10))
                             .setRadius(10))
                  .add(p.text(label).style(
                      Style().setFont(FontRole::Body).setWidth(Length::Pct(100))));
  return btn;
}

/** One row in the emoji size ladder (baked atlas = 64px). */
static UIText &emojiSizeRow(Page &p, const char *label, uint8_t emojiPx,
                            uint16_t color) {
  const uint8_t lineH =
      static_cast<uint8_t>(emojiPx + 6 > 255 ? 255 : emojiPx + 6);
  return p.text(label).style(Style()
                                 .setFont(FontRole::Small)
                                 .setColor(color)
                                 .setWidth(Length::Pct(100))
                                 .setEmojiSize(emojiPx)
                                 .setLineHeight(lineH));
}

static void drawFrame(float dt) {
  const uint16_t bg = Display::color565(48, 32, 28);
  const uint16_t fg = Display::color565(255, 248, 235);
  const uint16_t muted = Display::color565(210, 195, 185);

  page.setContentBackground(bg);

  const bool animOnly = page.uiAnimating() && input.empty();
  if (!animOnly) {
    page.beginUI();

    auto &body =
        page.div()
            .style(Style()
                       .setWidth(Length::Pct(100))
                       .setPadding(Edges(20, 10))
                       .setGap(8))
            .onEvent(onBodyEvent)
            .add(page.text("Emoji · 50 @ 64px")
                     .style(Style()
                                .setFont(FontRole::BodyLarge)
                                .setColor(fg)
                                .setWidth(Length::Pct(100))))
            .add(page.text("flash atlas · downscale from 64")
                     .style(Style()
                                .setFont(FontRole::Small)
                                .setColor(muted)
                                .setWidth(Length::Pct(100))
                                .setLineHeight(18)))
            .add(emojiSizeRow(page, "16  😀😂😍🥺  down", 16, fg))
            .add(emojiSizeRow(page, "22  😀😂😍🥺  down", 22, fg))
            .add(emojiSizeRow(page, "40  😀😂😍🥺  down", 40, fg))
            .add(emojiSizeRow(page, "64  😀😂😍🥺  native", 64, fg))
            .add(page.text("😀😁😂🤣😊😍🥰😎🤔😭😡🥺😷😴🤗")
                     .style(Style()
                                .setFont(FontRole::Body)
                                .setColor(fg)
                                .setWidth(Length::Pct(100))
                                .setEmojiSize(28)
                                .setLineHeight(36)))
            .add(makeBtn(page, "Primary solid", ButtonColor::Primary,
                         ButtonVariant::Solid))
            .add(makeBtn(page, "Primary outline", ButtonColor::Primary,
                         ButtonVariant::Outline))
            .add(makeBtn(page, "Disabled solid", ButtonColor::Primary,
                         ButtonVariant::Solid, /*isDisabled=*/true))
            .add(makeBtn(page, "Secondary soft", ButtonColor::Secondary,
                         ButtonVariant::Soft));

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
  canvas.setEmojiAtlas(&NotoColorEmoji());
  Serial.printf("Flow32 demo | Panel %s %dx%d rot=%u scale=%.2f\n", kPanel.id,
                kPanel.width, kPanel.height, (unsigned)kPanel.rotation,
                (double)kPanel.uiScale);

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
