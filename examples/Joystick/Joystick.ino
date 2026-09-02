#include <Flow32.h>

namespace {

DisplayPanel makePanel() {
  DisplayPanel value;
  value.width = 240;
  value.height = 320;
  return value;
}

St77xxConfig makeTransport() {
  St77xxConfig config;
  config.spi = &SPI;
  config.pinCs = 5;
  config.pinDc = 4;
  config.pinMosi = 18;
  config.pinSclk = 2;
  config.pinRst = 15;
  config.gramWidth = 240;
  config.gramHeight = 320;
  return config;
}

JoystickConfig makeJoystick() {
  JoystickConfig config;
  config.pinX = 34;
  config.pinY = 35;
  config.pinSelect = 27;
  config.centerX = 2048;
  config.centerY = 2048;
  config.deadZoneEnter = 700;
  config.deadZoneExit = 500;
  return config;
}

struct JoystickState {};

class JoystickApp final : public App<JoystickState> {
public:
  explicit JoystickApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Joystick", "circle-dot");
    addPage("Hysteretic joystick");
  }

protected:
  void build(Page &page, uint8_t) override {
    page.add(page.div()
                 .style(Style().setPadding(18).setGap(14))
                 .add(page.text("The exit threshold is closer to center, so noisy ADC values do not flicker focus."))
                 .add(page.button().add(page.text("One")))
                 .add(page.button().add(page.text("Two")))
                 .add(page.range().min(0).max(100).value(50)));
  }
};

const DisplayPanel panel = makePanel();
St77xxTransport transport(makeTransport());
JoystickInput joystick(makeJoystick());
JoystickApp app(Rect(0, 0, panel.width, panel.height));
Flow32 flow(panel, transport);

} // namespace

void setup() { flow.apps({&app}).input(joystick).begin(); }
void loop() { flow.tick(); }
