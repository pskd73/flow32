#pragma once

#include "RamManager.h"

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

/** Transient Shell toast (theme status colours). */
enum class ToastKind : uint8_t { Info, Success, Warning, Error };

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

  /**
   * Bottom toast over the active app (auto-dismiss). Safe from any app via
   * host(); message is copied.
   */
  virtual void showToast(const char *message, ToastKind kind = ToastKind::Info,
                         uint16_t durationMs = 2000) = 0;

  /**
   * True while a Shell toast is on screen. Apps should blit into the panel
   * buffer but skip SPI present — Shell composites the toast and presents once.
   */
  virtual bool toastVisible() const { return false; }

  /**
   * True while a full-panel overlay (splash, idle eyes) owns the glass. Apps
   * and Shell chrome must not SPI-present — the overlay presents the panel.
   */
  virtual bool overlaySuppressesPresent() const { return false; }

  /** Mounted microSD, or nullptr if storage is unavailable. */
  virtual class Storage *storage() { return nullptr; }

  /** Drain Cache plus background App/Session holders until RAM meets `profile`. */
  virtual bool ramEnsureProfile(RamManager::Profile profile,
                                const char *requester = nullptr,
                                RamManager::Priority drainUpTo =
                                    RamManager::Priority::Session) = 0;

  virtual bool ramEnsureNeed(RamManager::Need need,
                             const char *requester = nullptr,
                             RamManager::Priority drainUpTo =
                                 RamManager::Priority::Session) = 0;

  virtual RamManager::Snapshot ramSnapshot() const = 0;
  virtual void ramLog(const char *tag) const = 0;
};
