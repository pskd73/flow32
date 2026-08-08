#pragma once

/**
 * Flow32 — ESP32 SPI TFT UI toolkit.
 *
 * Display / Canvas / Page + composable UI + input hub + App/AppStore.
 * Pass a DisplayPanel from the app (see src/panels.h in this repo).
 */

#include "Display.h"
#include "DisplayPanel.h"
#include "Canvas.h"
#include "Page.h"
#include "Rect.h"
#include "AAFont.h"
#include "ColorEmoji.h"
#include "ColorEmojiSd.h"
#include "Icon.h"
#include "IconSd.h"
#include "Storage.h"
#include "StorageConfig.h"

#include "ui/Style.h"
#include "ui/Theme.h"
#include "ui/UIEvent.h"
#include "ui/UINode.h"
#include "ui/UIDiv.h"
#include "ui/UIText.h"
#include "ui/UIImage.h"
#include "ui/UIButton.h"
#include "ui/UIToggle.h"
#include "ui/UIRange.h"
#include "ui/UISelect.h"
#include "ui/UIDebug.h"
#include "ui/UIArena.h"

#include "input/InputHub.h"
#include "input/InputSource.h"
#include "input/KeyTracker.h"
#include "input/SerialInput.h"
#include "input/JoystickInput.h"

#include "AppStore.h"
#include "App.h"
