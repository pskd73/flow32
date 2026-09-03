#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Internal-RAM registry for ESP32 — holders, drainers, and pressure-aware alloc.
 * PSRAM is handled separately (future PsramManager).
 *
 * Drain-only holders should report internal bytes via setHolderBytes; entries
 * with 0 bytes are skipped (e.g. a cache that lives entirely in PSRAM).
 */
namespace RamManager {

enum class Priority : uint8_t {
  Cache = 0,
  App = 1,
  Session = 2,
  System = 3,
};

struct Profile {
  size_t minInternalContig = 0;
  size_t minInternalFree = 0;
};

enum class Need : uint8_t {
  None,
  /** ~28 KB contiguous — TLS handshake (WebSocket / HTTPS). */
  SslHandshake,
  /** ~20 KB — short HTTPS client (signed URL, etc.). */
  HttpClient,
};

struct Snapshot {
  uint32_t internalFree = 0;
  uint32_t internalLargest = 0;
  uint32_t registeredBytes = 0;
  uint8_t holderCount = 0;
};

using DrainFn = void (*)(void* ctx);
using DrainedNotifyFn = void (*)(const char* requester, void* ctx);

void init();
Snapshot snapshot();
Profile profileFor(Need need);

void setForeground(const char* appName);
const char* foregroundApp();

void setDrainedNotify(DrainedNotifyFn fn, void* ctx);

bool ensure(size_t minContig, size_t minFree, const char* requester,
            Priority drainUpTo = Priority::Session);
bool ensureProfile(Profile profile, const char* requester,
                   Priority drainUpTo = Priority::Session);
bool ensureNeed(Need need, const char* requester,
                Priority drainUpTo = Priority::Session);

void log(const char* tag);
void dumpHolders();

/** Register memory already owned elsewhere (tracking + optional drain). */
uint16_t registerHolder(const char* owner, size_t bytes, Priority priority,
                        DrainFn drain, void* ctx, void* ptr = nullptr);
/** Register a drain-only entry (e.g. LRU cache with no fixed size). */
uint16_t registerDrainer(const char* owner, Priority priority, DrainFn drain,
                         void* ctx);
void setHolderBytes(uint16_t handle, size_t bytes);
void unregisterHolder(uint16_t handle);
void releaseAllForOwner(const char* owner);

void* alloc(size_t bytes, const char* owner, Priority priority, DrainFn drain,
            void* ctx);
void free(void* ptr);

} // namespace RamManager
