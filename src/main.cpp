#include <Arduino.h>

#include "Canvas.h"
#include "Display.h"
#include "DisplayPanel.h"
#include "Page.h"
#include "input/InputHub.h"
#include "input/SerialInput.h"
#include "ui/UIButton.h"
#include "ui/UIDebug.h"
#include "ui/UIDiv.h"
#include "ui/UIEvent.h"
#include "ui/UINode.h"
#include "ui/UIText.h"

// Active panel — 1.8" ST7735 landscape module.
static const DisplayPanel kPanel = Panel18();
// static const DisplayPanel kPanel = Panel183();

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
                         ButtonVariant variant) {
  return p.button()
      .color(color)
      .variant(variant)
      .onPress(onBtnPress)
      .style(Style().setWidth(Length::Pct(100)).setPadding(Edges(10, 12)).setRadius(14))
      .add(p.text(label).style(
          Style().setFont(FontRole::Body).setWidth(Length::Pct(100))));
}

static void drawFrame(float dt) {
  const uint16_t bg = Display::color565(48, 32, 28);
  const uint16_t fg = Display::color565(255, 248, 235);
  const uint16_t muted = Display::color565(210, 195, 185);

  canvas.clear(bg);

  page.beginUI();

  auto &body =
      page.div()
          .style(Style()
                     .setWidth(Length::Pct(100))
                     .setPadding(Edges(12, 14))
                     .setGap(10))
          .onEvent(onBodyEvent)
          .add(page.text("Buttons")
                   .style(Style()
                              .setFont(FontRole::BodyLarge)
                              .setColor(fg)
                              .setWidth(Length::Pct(100))))
          .add(page.text("d/u scroll · e focus · e again activate")
                   .style(Style()
                              .setFont(FontRole::Small)
                              .setColor(muted)
                              .setWidth(Length::Pct(100))
                              .setLineGap(3)))
          .add(makeBtn(page, "Primary solid", ButtonColor::Primary,
                       ButtonVariant::Solid))
          .add(makeBtn(page, "Primary outline", ButtonColor::Primary,
                       ButtonVariant::Outline))
          .add(makeBtn(page, "Secondary soft", ButtonColor::Secondary,
                       ButtonVariant::Soft))
          .add(makeBtn(page, "Accent ghost", ButtonColor::Accent,
                       ButtonVariant::Ghost))
          .add(makeBtn(page, "Accent solid", ButtonColor::Accent,
                       ButtonVariant::Solid));

  page.add(body);
  page.layoutUI(canvas);
  page.syncFocus();
  input.dispatchTo(page);

  page.tick(dt);
  page.drawUI(canvas);

  canvas.present();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("Panel %s design %dx%d native %dx%d rot=%u chip=%d BL=GPIO%d\n",
                kPanel.id, kPanel.width, kPanel.height, kPanel.nativeWidth,
                kPanel.nativeHeight, (unsigned)kPanel.rotation, (int)kPanel.chip,
                kPanel.pinBl);

  // Force backlight pin HIGH before anything else (LED → GPIO13).
  pinMode(kPanel.pinBl, OUTPUT);
  digitalWrite(kPanel.pinBl, HIGH);

  if (!display.begin()) {
    Serial.println("Display begin failed");
    return;
  }
  display.setBacklight(true);
  Serial.println("Display begin ok, backlight HIGH");

  input.add(serialInput);
  // Later: input.add(joystickInput);
  input.begin();

  UIDebug::borders = kUIDebugBorders;

  drawFrame(0.04f);
}

void loop() {
  // Keep LED asserted every frame (cheap insurance).
  display.setBacklight(true);

  const uint32_t now = millis();
  input.poll(now);

  static uint32_t last = 0;
  if (now - last < 40) return;
  const float dt = (now - last) / 1000.0f;
  last = now;

  drawFrame(dt);
}
