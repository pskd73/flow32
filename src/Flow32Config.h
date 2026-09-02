#pragma once

#include "Flow32BuildConfig.h"

/* Override any value with a PlatformIO build flag before including Flow32. */
#ifndef FLOW32_MAX_APPS
#define FLOW32_MAX_APPS 8
#endif

#ifndef FLOW32_UI_ARENA_BYTES
#define FLOW32_UI_ARENA_BYTES 8192
#endif

#ifndef FLOW32_MAX_INPUT_SOURCES
#define FLOW32_MAX_INPUT_SOURCES 4
#endif

#ifndef FLOW32_INPUT_QUEUE_SIZE
#define FLOW32_INPUT_QUEUE_SIZE 24
#endif

#ifndef FLOW32_ICON_CACHE_SLOTS
#define FLOW32_ICON_CACHE_SLOTS 16
#endif

#ifndef FLOW32_EMOJI_CACHE_SLOTS
#define FLOW32_EMOJI_CACHE_SLOTS 8
#endif

#ifndef FLOW32_FRAME_INTERVAL_MS
#define FLOW32_FRAME_INTERVAL_MS 16
#endif

#ifndef FLOW32_BUTTON_DEBOUNCE_MS
#define FLOW32_BUTTON_DEBOUNCE_MS 20
#endif
