#include <Arduino.h>

#include <Flow32.h>
#include "panels.h"
#include "storage.h"
#include "home_app.h"

// Active panel — switch for your glass.
// static const DisplayPanel kPanel = Panel18();
static const DisplayPanel kPanel = Panel183();

static constexpr bool kUIDebugBorders = false;

Display display(kPanel);
Canvas canvas(display);

static InputHub input;
static SerialInput serialInput;

static Storage storage(SdDefault());
static ColorEmojiSd emojiSd;
static IconSd iconSd;

static const Rect kPanelRect(0, 0, kPanel.width, kPanel.height);
static HomeApp home(kPanelRect);
static Shell shell(kPanelRect);

void setup() {
  Serial.begin(115200);
  delay(200);

  Theme::setActive(Theme::FlowTheme());

  Serial.printf("Flow32 demo | theme=%s | Panel %s %dx%d\n", Theme::active().name,
                kPanel.id, kPanel.width, kPanel.height);

  if (storage.begin()) {
    storage.printInfo();
    if (emojiSd.begin(storage)) {
      canvas.setEmojiSd(&emojiSd);
    } else {
      Serial.println(
          "Emoji atlas missing — copy sd/flow32/emoji.atlas onto the card");
    }
    if (iconSd.begin(storage)) {
      canvas.setIconSd(&iconSd);
      home.setIcons(&iconSd);
    } else {
      Serial.println(
          "Icon atlas missing — copy sd/flow32/icons.atlas onto the card");
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

  shell.setApp(&home);
  if (!home.open()) {
    Serial.println("HomeApp: NVS open failed — using defaults");
  }

  shell.frame(canvas, input, 0.04f);
}

void loop() {
  display.setBacklight(true);

  const uint32_t now = millis();
  input.poll(now);

  static uint32_t last = 0;
  if (now - last < 16) return;
  const float dt = (now - last) / 1000.0f;
  last = now;

  shell.frame(canvas, input, dt);
}
