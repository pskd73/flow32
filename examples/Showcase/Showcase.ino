#include <Flow32.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

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

FontPack makeFonts() {
  FontPack fonts;
  fonts.body.gfx = &FreeSans9pt7b;
  fonts.bodyBold.gfx = &FreeSansBold12pt7b;
  fonts.title.gfx = &FreeSansBold12pt7b;
  return fonts;
}

struct HomeState {};

class HomeApp final : public App<HomeState> {
public:
  explicit HomeApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Home", "house");
    addPage("Showcase");
    addPage("Scrollable page");
    self_ = this;
  }

protected:
  void build(Page &page, uint8_t pageId) override {
    if (pageId == 0) {
      page.add(page.div()
                   .style(Style().setPadding(18).setGap(16))
                   .add(page.text("Flow32").style(Style().setFont(FontRole::Title)))
                   .add(page.button().icon("list").onPress(openDetails).add(page.text("Scrolling")))
                   .add(page.button().icon("settings").onPress(openSettings).add(page.text("Settings app"))));
      return;
    }
    auto &content = page.div().style(Style().setPadding(18).setGap(14));
    content.add(page.text("Focus moves through retained controls while the content cache scrolls."));
    content.add(page.button().add(page.text("Alpha")));
    content.add(page.button().add(page.text("Bravo")));
    content.add(page.button().add(page.text("Charlie")));
    content.add(page.button().add(page.text("Delta")));
    content.add(page.button().add(page.text("Echo")));
    content.add(page.button().add(page.text("Foxtrot")));
    content.add(page.button().add(page.text("Golf")));
    page.add(content);
  }

private:
  static HomeApp *self_;
  static void openDetails(UIButton &) {
    if (self_) self_->goTo(1);
  }
  static void openSettings(UIButton &) {
    if (self_ && self_->host()) self_->host()->openApp(1);
  }
};

HomeApp *HomeApp::self_ = nullptr;

struct SettingsState {
  bool enabled = true;
  int16_t level = 60;
  int16_t theme = 0;
};

class SettingsApp final : public App<SettingsState> {
public:
  explicit SettingsApp(const Rect &viewport) : App(viewport) {
    setAppInfo("Settings", "settings");
    addPage("Persisted settings");
    self_ = this;
  }

protected:
  const char *nvsNamespace() const override { return "f32showcase"; }

  void build(Page &page, uint8_t) override {
    auto &themes = page.select().selected(state().theme).onChange(changeTheme);
    themes.add(page.selectOption().title("Dark").description("Low light"));
    themes.add(page.selectOption().title("Light").description("High contrast"));
    page.add(page.div()
                 .style(Style().setPadding(18).setGap(16))
                 .add(page.text("Enabled"))
                 .add(page.toggle().checked(state().enabled).onChange(changeEnabled))
                 .add(page.text("Level"))
                 .add(page.range().min(0).max(100).value(state().level).onChange(changeLevel))
                 .add(themes));
  }

private:
  static SettingsApp *self_;
  static void changeEnabled(UIToggle &control) {
    if (self_) self_->set(self_->data().enabled, control.checked());
  }
  static void changeLevel(UIRange &control) {
    if (self_) self_->set(self_->data().level, control.value());
  }
  static void changeTheme(UISelect &control) {
    if (!self_) return;
    self_->set(self_->data().theme, control.selected());
    Theme::setActive(control.selected() == 0 ? Theme::DarkTheme()
                                             : Theme::LightTheme());
  }
};

SettingsApp *SettingsApp::self_ = nullptr;
const DisplayPanel panel = makePanel();
St77xxTransport transport(makeTransport());
HomeApp home(Rect(0, 0, panel.width, panel.height));
SettingsApp settings(Rect(0, 0, panel.width, panel.height));
SerialInput serialInput;
Flow32 flow(panel, transport);

} // namespace

void setup() {
  Serial.begin(115200);
  flow.apps({&home, &settings})
      .home(0)
      .input(serialInput)
      .config(FlowConfig{}.fonts(makeFonts()).theme(Theme::DarkTheme()))
      .begin();
}

void loop() { flow.tick(); }
