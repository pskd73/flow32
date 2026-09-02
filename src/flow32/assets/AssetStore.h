#pragma once

#include <FS.h>
#include <stddef.h>
#include <stdio.h>

/** Minimal filesystem surface used by streamed icon and emoji atlases. */
class AssetStore {
public:
  virtual ~AssetStore() = default;
  virtual bool ready() const = 0;
  virtual bool exists(const char *path) const = 0;
  virtual File open(const char *path, const char *mode = FILE_READ) const = 0;
  virtual bool resolve(const char *relative, char *out, size_t outLen) const = 0;
};

/** Adapter for any filesystem already mounted by the application. */
class FsAssetStore final : public AssetStore {
public:
  explicit FsAssetStore(fs::FS &fs, const char *root = "/")
      : fs_(fs), root_(root && root[0] ? root : "/") {}

  bool ready() const override { return true; }
  bool exists(const char *path) const override { return path && fs_.exists(path); }
  File open(const char *path, const char *mode = FILE_READ) const override {
    return path ? fs_.open(path, mode) : File();
  }
  bool resolve(const char *relative, char *out, size_t outLen) const override {
    if (!out || outLen == 0) return false;
    const char *rel = relative ? relative : "";
    while (*rel == '/') rel++;
    const bool rootSlash = root_[0] == '/' && root_[1] == '\0';
    const int n = snprintf(out, outLen, rootSlash ? "/%s" : "%s/%s", root_, rel);
    return n >= 0 && static_cast<size_t>(n) < outLen;
  }

private:
  fs::FS &fs_;
  const char *root_;
};
