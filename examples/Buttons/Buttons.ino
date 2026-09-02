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

ButtonInputConfig makeButtons() {
  ButtonInputConfig config;
  config.up = 32;
  config.down = 33;
  config.select = 27;
  config.back = 26;
  config.activeLow = true;
  config.debounceMs = 20;
  return config;
}

struct ButtonState {};

class ButtonApp final : public App<ButtonState> {
public:
  explicit ButtonApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Buttons", "gamepad-2");
    addPage("Debounced buttons");
  }

protected:
  void build(Page &page, uint8_t) override {
    page.add(page.div()
                 .style(Style().setPadding(18).setGap(14))
                 .add(page.text("Up/Down move focus. Select activates."))
                 .add(page.button().add(page.text("First")))
                 .add(page.button().add(page.text("Second")))
                 .add(page.button().add(page.text("Third"))));
  }
};

const DisplayPanel panel = makePanel();
St77xxTransport transport(makeTransport());
ButtonInput buttons(makeButtons());
ButtonApp app(Rect(0, 0, panel.width, panel.height));
Flow32 flow(panel, transport);

} // namespace

void setup() { flow.apps({&app}).input(buttons).begin(); }
void loop() { flow.tick(); }
