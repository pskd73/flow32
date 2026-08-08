#include "settings_app.h"

SettingsApp *SettingsApp::self_ = nullptr;

void SettingsApp::onThemeSelect(UISelect &s) {
  if (!self_) return;
  self_->setTheme(s.selected());
  Serial.printf("Settings: theme=%d (%s)\n", (int)self_->state().theme,
                Theme::active().name ? Theme::active().name : "?");
}

void SettingsApp::build(Page &page, uint8_t /*pageId*/) {
  const SettingsState &st = state();

  auto &chooser =
      page.select()
          .selected(st.theme)
          .onChange(onThemeSelect)
          .style(Style().setWidth(Length::Pct(100)).setGap(6))
          .add(page.selectOption()
                   .icon("sun")
                   .title("Flow")
                   .description("Warm dark surfaces")
                   .value(0))
          .add(page.selectOption()
                   .icon("snowflake")
                   .title("Winter")
                   .description("Cool light surfaces")
                   .value(1));

  page.add(page.div()
               .style(Style()
                          .setWidth(Length::Pct(100))
                          .setPadding(Edges(16, 12))
                          .setGap(14)
                          .setColumns(1))
               .add(chooser));
}
