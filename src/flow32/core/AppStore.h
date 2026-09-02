#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include <string.h>

/**
 * Instantiable dirty-flag store with optional NVS persistence.
 *
 *   AppStore<MyState> store;
 *   store.begin("home");          // open app → load RAM from NVS
 *   store.set(store.data().x, v); // RAM + schedule save
 *   store.tick();                 // call from loop (debounced flush)
 *   store.end();                  // leave app → flush + close
 *
 * `S` should be a POD-ish blob (fixed size). Put a magic/version at the front
 * of `S` so older layouts are ignored safely.
 */
template <typename S> class AppStore {
public:
  static constexpr const char *kBlobKey = "state";

  AppStore() = default;
  ~AppStore() { end(); }

  AppStore(const AppStore &) = delete;
  AppStore &operator=(const AppStore &) = delete;

  const S &get() const { return state_; }
  S &data() { return state_; }

  bool ready() const { return ready_; }
  bool persistent() const { return persistent_; }
  bool persistenceDirty() const { return persistDirty_; }
  const char *nvsNamespace() const { return ns_; }

  /**
   * Open this app's NVS namespace and load into RAM (or keep defaults).
   * `saveDebounceMs`: 0 = write NVS on every set(); >0 batches rapid sets
   * (e.g. range scrubbing).
   */
  bool begin(const char *nvsNamespace, uint32_t saveDebounceMs = 300) {
    end();
    if (!nvsNamespace || !nvsNamespace[0]) {
      ready_ = true;
      persistent_ = false;
      dirty_ = true;
      return true;
    }
    strncpy(nsBuf_, nvsNamespace, sizeof(nsBuf_) - 1);
    nsBuf_[sizeof(nsBuf_) - 1] = '\0';
    ns_ = nsBuf_;
    saveDebounceMs_ = saveDebounceMs;

    // RW so we can save later without reopening.
    if (!prefs_.begin(ns_, false)) {
      ns_ = nullptr;
      return false;
    }
    ready_ = true;
    persistent_ = true;

    const size_t len = prefs_.getBytesLength(kBlobKey);
    if (len == sizeof(S)) {
      S loaded{};
      if (prefs_.getBytes(kBlobKey, &loaded, sizeof(S)) == sizeof(S)) {
        state_ = loaded;
      }
    }

    dirty_ = true; // first UI paint
    persistDirty_ = false;
    return true;
  }

  /** Flush pending save and close NVS. */
  void end() {
    if (!ready_) return;
    saveNow();
    if (persistent_) prefs_.end();
    ready_ = false;
    persistent_ = false;
    ns_ = nullptr;
  }

  void markDirty() { dirty_ = true; }
  bool isDirty() const { return dirty_; }
  bool consumeDirty() {
    const bool d = dirty_;
    dirty_ = false;
    return d;
  }

  /** Assign if changed; marks UI dirty and schedules NVS save. */
  template <typename T> bool set(T &field, const T &value) {
    if (field == value) return false;
    field = value;
    dirty_ = true;
    scheduleSave();
    return true;
  }

  /** Force a save soon (e.g. after bulk edits to data()). */
  void scheduleSave() {
    if (!persistent_) return;
    persistDirty_ = true;
    lastChangeMs_ = millis();
    if (saveDebounceMs_ == 0) saveNow();
  }

  /** Call from the main loop — flushes after debounce. */
  void tick() {
    if (!ready_ || !persistDirty_) return;
    if (saveDebounceMs_ == 0) {
      saveNow();
      return;
    }
    if (millis() - lastChangeMs_ >= saveDebounceMs_) saveNow();
  }

  /** Immediate NVS write if needed. */
  bool saveNow() {
    if (!ready_) return false;
    if (!persistent_) return true;
    if (!persistDirty_) return false;
    const size_t n = prefs_.putBytes(kBlobKey, &state_, sizeof(S));
    if (n != sizeof(S)) {
      return false;
    }
    persistDirty_ = false;
    return true;
  }

private:
  S state_{};
  Preferences prefs_{};
  char nsBuf_[16] = {};
  const char *ns_ = nullptr;
  bool ready_ = false;
  bool persistent_ = false;
  bool dirty_ = true;
  bool persistDirty_ = false;
  uint32_t saveDebounceMs_ = 300;
  uint32_t lastChangeMs_ = 0;
};
