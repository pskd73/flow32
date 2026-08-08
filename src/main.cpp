#include <Arduino.h>

#include <Flow32.h>
#include "panels.h"
#include "storage.h"
#include "launcher_app.h"
#include "settings_app.h"
#include "eyes_app.h"

static const DisplayPanel kPanel = Panel183();
static const Rect kPanelRect(0, 0, kPanel.width, kPanel.height);

static LauncherApp launcher(kPanelRect);
static SettingsApp settings(kPanelRect);
static EyesApp eyes(kPanelRect);
static Flow32 flow(kPanel);

void setup() {
  flow.apps({&launcher, &settings, &eyes})
      .config(FlowConfig{}
                  .theme(Theme::FlowTheme())
                  .storage(SdDefault())
                  .debugBorders(false))
      .begin();
}

void loop() { flow.tick(); }
