#include "flow32/ui/Theme.h"

namespace Theme {

namespace {

/** Named-field builder — avoids brittle positional brace lists. */
ThemeTokens makeDark() {
  ThemeTokens t{};
  t.name = "dark";
  t.base100 = rgb(48, 32, 28);
  t.base200 = rgb(60, 42, 36);
  t.base300 = rgb(72, 52, 44);
  t.baseContent = rgb(255, 248, 235);
  t.primary = rgb(255, 160, 90);
  t.primaryContent = rgb(40, 24, 20);
  t.secondary = rgb(120, 140, 160);
  t.secondaryContent = rgb(245, 248, 252);
  t.accent = rgb(90, 200, 170);
  t.accentContent = rgb(20, 36, 32);
  t.neutral = rgb(36, 28, 26);
  t.neutralContent = rgb(210, 195, 185);
  t.info = rgb(120, 180, 220);
  t.infoContent = rgb(16, 32, 48);
  t.success = rgb(90, 180, 140);
  t.successContent = rgb(12, 36, 28);
  t.warning = rgb(254, 236, 65);
  t.warningContent = rgb(40, 36, 12);
  t.error = rgb(220, 90, 80);
  t.errorContent = rgb(40, 16, 16);
  t.focusRing = rgb(255, 248, 235);
  t.radiusSelector = 16;
  t.radiusField = 16;
  t.radiusBox = 16;
  return t;
}

ThemeTokens makeLight() {
  ThemeTokens t{};
  t.name = "light";
  t.base100 = rgb(255, 255, 255);
  t.base200 = rgb(242, 244, 249);
  t.base300 = rgb(229, 232, 241);
  t.baseContent = rgb(70, 84, 120);
  t.primary = hex(0x760031);
  t.primaryContent = rgb(255, 248, 242);
  t.secondary = hex(0xD51C39);
  t.secondaryContent = rgb(255, 245, 246);
  t.accent = hex(0xFF6060);
  t.accentContent = rgb(28, 12, 20);
  t.neutral = rgb(28, 36, 56);
  t.neutralContent = rgb(200, 206, 220);
  t.info = rgb(170, 220, 235);
  t.infoContent = rgb(20, 40, 48);
  t.success = rgb(150, 210, 200);
  t.successContent = rgb(16, 40, 36);
  t.warning = hex(0xFEEC41);
  t.warningContent = rgb(40, 36, 12);
  t.error = rgb(220, 140, 140);
  t.errorContent = rgb(40, 16, 16);
  t.focusRing = rgb(70, 84, 120);
  t.radiusSelector = 16;
  t.radiusField = 16;
  t.radiusBox = 16;
  return t;
}

const ThemeTokens kDark = makeDark();
const ThemeTokens kLight = makeLight();
const ThemeTokens *g_active = &kDark;
uint32_t g_generation = 1;

} // namespace

const ThemeTokens &DarkTheme() { return kDark; }
const ThemeTokens &LightTheme() { return kLight; }

const ThemeTokens &active() { return *g_active; }

void setActive(const ThemeTokens &t) {
  g_active = &t;
  ++g_generation;
  if (g_generation == 0) g_generation = 1;
}

uint32_t generation() { return g_generation; }

} // namespace Theme
