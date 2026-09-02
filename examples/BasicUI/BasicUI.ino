#include <Flow32.h>

namespace {

DisplayPanel makePanel() {
  DisplayPanel panel;
  panel.id = "basic-ui";
  panel.width = 240;
  panel.height = 320;
  return panel;
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

struct BasicState {};

class BasicApp final : public App<BasicState> {
public:
  explicit BasicApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Basic", "layout-dashboard");
    addPage("Flow32");
  }

protected:
  void build(Page &page, uint8_t) override {
    const Theme::ThemeTokens &theme = Theme::active();
    page.add(page.div()
                 .style(Style()
                            .setWidth(Length::Pct(100))
                            .setPadding(20)
                            .setGap(16))
                 .add(page.text("Retained UI").style(
                     Style().setFont(FontRole::Title).setColor(theme.baseContent)))
                 .add(page.text("Layout, focus, scrolling, and rendering are owned by Flow32.")
                          .style(Style().setColor(theme.baseContent)))
                 .add(page.button()
                          .add(page.text("Select me"))
                          .style(Style().setWidth(Length::Pct(100)))));
  }
};

const DisplayPanel panel = makePanel();
St77xxTransport transport(makeTransport());
BasicApp app(Rect(0, 0, panel.width, panel.height));
SerialInput serialInput;
Flow32 flow(panel, transport);

} // namespace

void setup() {
  Serial.begin(115200);
  flow.apps({&app}).input(serialInput).begin();
}

void loop() { flow.tick(); }
