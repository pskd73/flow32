#pragma once

#include "StorageConfig.h"
#include <FS.h>

/**
 * Board-agnostic microSD access.
 * Construct with a StorageConfig from the app; call begin() once.
 */
class Storage {
public:
  explicit Storage(const StorageConfig &cfg);

  /** Mount the card. Safe to call again if already mounted. */
  bool begin();
  void end();

  bool ready() const { return ready_; }
  const StorageConfig &config() const { return cfg_; }
  const char *mountPoint() const { return cfg_.mountPoint; }

  uint8_t cardType() const;
  uint64_t cardSizeBytes() const;
  uint64_t usedBytes() const;

  bool exists(const char *path) const;
  File open(const char *path, const char *mode = FILE_READ) const;

  /**
   * Build an SD/SD_MMC path for open()/exists().
   * Arduino mounts at mountPoint (e.g. "/sdcard") but open() paths are
   * card-rooted: "/flow32/emoji.atlas" — NOT "/sdcard/flow32/...".
   * `rel` may be "flow32/emoji.atlas" or "/flow32/emoji.atlas".
   */
  bool absPath(const char *rel, char *out, size_t outLen) const;

  /** Serial smoke test: mount info + optional root listing. */
  void printInfo(Stream &out = Serial) const;

private:
  StorageConfig cfg_;
  bool ready_ = false;
};
