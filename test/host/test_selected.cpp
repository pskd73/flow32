#include <cassert>

#include <Flow32Selected.h>

namespace {

class SelectedTransport final : public DisplayTransport {
public:
  bool begin(const DisplayPanel &) override { return true; }
  void present(const uint16_t *, int16_t, int16_t, int16_t, int16_t,
               int16_t) override {
    writes++;
  }
  int writes = 0;
};

} // namespace

int main() {
  static_assert(FLOW32_MODULE_RETAINED_UI == 1,
                "selected profile needs retained UI");
  static_assert(FLOW32_MODULE_STORAGE == 0,
                "selected profile excludes concrete storage");
  static_assert(FLOW32_MODULE_RUNTIME == 0,
                "selected profile must exclude runtime");
  static_assert(FLOW32_UI_ARENA_BYTES == 6144,
                "generated limits must be visible");

  DisplayPanel panel;
  panel.width = 16;
  panel.height = 16;
  panel.preferPsram = false;
  SelectedTransport transport;
  Display display(panel, transport);
  assert(display.begin());
  display.clear(0x1234);
  display.present();
  assert(transport.writes == 1);

  Canvas canvas(display);
  Page page(Rect(0, 0, 16, 16));
  page.beginUI();
  page.add(page.text("ok"));
  page.layoutUI(canvas);
  page.syncFocus();
  assert(page.drawUI(canvas));
  return 0;
}
