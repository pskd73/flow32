#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

class Preferences {
public:
  bool begin(const char *name, bool = false) {
    if (!name || !name[0]) return false;
    name_ = name;
    open_ = true;
    return true;
  }
  void end() { open_ = false; }
  size_t getBytesLength(const char *key) const {
    const auto found = blobs().find(fullKey(key));
    return found == blobs().end() ? 0 : found->second.size();
  }
  size_t getBytes(const char *key, void *out, size_t size) const {
    const auto found = blobs().find(fullKey(key));
    if (found == blobs().end() || !out || size < found->second.size()) return 0;
    std::memcpy(out, found->second.data(), found->second.size());
    return found->second.size();
  }
  size_t putBytes(const char *key, const void *data, size_t size) {
    if (!open_ || !key || !data) return 0;
    const uint8_t *begin = static_cast<const uint8_t *>(data);
    blobs()[fullKey(key)] = std::vector<uint8_t>(begin, begin + size);
    return size;
  }

  static void clearHostStorage() { blobs().clear(); }

private:
  static std::map<std::string, std::vector<uint8_t>> &blobs() {
    static std::map<std::string, std::vector<uint8_t>> value;
    return value;
  }
  std::string fullKey(const char *key) const {
    return name_ + ":" + (key ? key : "");
  }
  std::string name_;
  bool open_ = false;
};
