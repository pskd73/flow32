#include <Flow32.h>

class LoggingTransport final : public DisplayTransport {
public:
  bool begin(const DisplayPanel &panel) override {
    Serial.printf("transport: %dx%d\n", panel.width, panel.height);
    return true;
  }

  void present(const uint16_t *, int16_t x, int16_t y, int16_t width,
               int16_t height, int16_t stride) override {
    Serial.printf("present: x=%d y=%d w=%d h=%d stride=%d\n", x, y, width,
                  height, stride);
  }
};

DisplayPanel makePanel() {
  DisplayPanel panel;
  panel.width = 64;
  panel.height = 48;
  panel.preferPsram = false;
  return panel;
}

const DisplayPanel panel = makePanel();
LoggingTransport transport;
Display display(panel, transport);

void setup() {
  Serial.begin(115200);
  if (!display.begin()) return;
  display.clear(Display::color565(20, 24, 32));
  display.present();
}

void loop() {}

