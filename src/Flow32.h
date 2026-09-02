#pragma once

/**
 * Flow32 — modular ESP32 display and UI toolkit.
 *
 * Display / Canvas / Page + composable UI + input hub + App/Shell/Flow32 runtime.
 * Hardware profiles and policies are supplied by the application.
 */

#include "flow32/graphics/Display.h"
#include "flow32/graphics/DisplayPanel.h"
#include "flow32/graphics/DisplayTransport.h"
#include "flow32/graphics/St77xxTransport.h"
#include "flow32/graphics/Canvas.h"
#include "flow32/ui/Page.h"
#include "flow32/graphics/Rect.h"
#include "flow32/graphics/AAFont.h"
#include "flow32/graphics/FontPack.h"
#include "flow32/assets/ColorEmoji.h"
#include "flow32/assets/StreamedEmojiAtlas.h"
#include "flow32/assets/Icon.h"
#include "flow32/assets/StreamedIconAtlas.h"
#include "flow32/assets/AssetStore.h"
#include "flow32/assets/Storage.h"
#include "flow32/assets/StorageConfig.h"

#include "flow32/ui/Style.h"
#include "flow32/ui/Theme.h"
#include "flow32/ui/UIEvent.h"
#include "flow32/ui/UINode.h"
#include "flow32/ui/UIDiv.h"
#include "flow32/ui/UIText.h"
#include "flow32/ui/UIImage.h"
#include "flow32/ui/UIButton.h"
#include "flow32/ui/UIToggle.h"
#include "flow32/ui/UIRange.h"
#include "flow32/ui/UISelect.h"
#include "flow32/ui/UIDebug.h"
#include "flow32/ui/UIArena.h"

#include "flow32/input/InputHub.h"
#include "flow32/input/InputSource.h"
#include "flow32/input/KeyTracker.h"
#include "flow32/input/SerialInput.h"
#include "flow32/input/JoystickInput.h"
#include "flow32/input/ButtonInput.h"

#include "flow32/core/AppStore.h"
#include "flow32/core/AppHost.h"
#include "flow32/core/App.h"
#include "flow32/core/FlowError.h"
#include "flow32/core/Shell.h"
#include "flow32/core/FlowRuntime.h"

#include "flow32/Aliases.h"
