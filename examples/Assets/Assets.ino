#include <Flow32.h>
#include <LittleFS.h>

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

struct AssetState {};

class AssetApp final : public App<AssetState> {
public:
  explicit AssetApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Assets", "image");
    addPage("Application-owned assets");
  }

protected:
  void build(Page &page, uint8_t) override {
    page.add(page.div()
                 .style(Style().setPadding(18).setGap(16))
                 .add(page.text("LittleFS is mounted by this sketch, not Flow32."))
                 .add(page.button().icon("wifi").add(page.text("Streamed icon"))));
  }
};

const DisplayPanel panel = makePanel();
St77xxTransport transport(makeTransport());
FsAssetStore assetStore(LittleFS, "/");
StreamedIconAtlas icons;
AssetApp app(Rect(0, 0, panel.width, panel.height));
Flow32 flow(panel, transport);

} // namespace

void setup() {
  if (!LittleFS.begin(false)) return;
  icons.begin(assetStore, "flow32/icons.atlas");
  flow.apps({&app}).assets(&icons).begin();
}

void loop() { flow.tick(); }
