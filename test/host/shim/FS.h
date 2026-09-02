#pragma once

#include "Arduino.h"

#include <algorithm>
#include <memory>
#include <vector>

#define FILE_READ "r"

namespace fs {
class FS;
}

class File {
public:
  File() = default;
  explicit File(const std::vector<uint8_t> &bytes)
      : data_(new std::vector<uint8_t>(bytes)), valid_(true) {}
  explicit operator bool() const { return valid_ && data_; }
  size_t read(uint8_t *out, size_t size) {
    if (!valid_ || !data_ || !out) return 0;
    const size_t remaining = data_->size() - position_;
    const size_t amount = std::min(size, remaining);
    if (amount) std::memcpy(out, &(*data_)[position_], amount);
    position_ += amount;
    return amount;
  }
  bool seek(uint32_t offset) {
    if (!valid_ || !data_ || offset > data_->size()) return false;
    position_ = offset;
    return true;
  }
  void close() {
    valid_ = false;
    data_.reset();
    position_ = 0;
  }
  bool isDirectory() const { return false; }
  File openNextFile() { return File(false); }
  const char *name() const { return ""; }
  size_t size() const { return data_ ? data_->size() : 0; }

private:
  explicit File(bool valid) : valid_(valid) {}
  std::shared_ptr<std::vector<uint8_t>> data_;
  size_t position_ = 0;
  bool valid_ = false;
  friend class fs::FS;
};

namespace fs {
class FS {
public:
  virtual ~FS() = default;
  virtual bool exists(const char *) { return false; }
  virtual File open(const char *, const char * = FILE_READ) { return File(); }
};
} // namespace fs
