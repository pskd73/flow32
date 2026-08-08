#pragma once

#include <stdint.h>

/**
 * Launcher-facing metadata for an installed app (no AppBase* exposed).
 * `name` / `icon` must outlive the host (use string literals).
 */
struct AppInfo {
  uint8_t index = 0;
  const char *name = "";
  /** Lucide icon name (e.g. "house"), or nullptr. */
  const char *icon = nullptr;
};

/**
 * Narrow runtime view for apps (launcher, settings, …).
 * Flow32 implements this; apps receive it via AppBase::onAttach.
 */
class AppHost {
public:
  virtual ~AppHost() = default;

  /**
   * Copy installed-app metadata into `out` (up to `maxOut` entries).
   * Returns the number of apps written.
   */
  virtual uint8_t getApps(AppInfo *out, uint8_t maxOut) const = 0;

  /** Switch to the app at `index` (from AppInfo::index). */
  virtual bool openApp(uint8_t index) = 0;

  /** Index of the currently open app. */
  virtual uint8_t activeAppIndex() const = 0;

  /**
   * Return to the launcher (first registered app). No-op if already there.
   * Shell uses this for root Back.
   */
  virtual bool openLauncher() = 0;
};
