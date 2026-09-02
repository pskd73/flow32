#include "RamManager.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <string.h>

namespace RamManager {

namespace {

static constexpr uint32_t kInternalCaps =
    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
static constexpr uint32_t kBlockMagic = 0x52414D42u; // 'RAMB'
static constexpr uint8_t kMaxEntries = 40;
static constexpr uint8_t kMaxDrainPasses = 12;

struct BlockHeader {
  uint32_t magic;
  uint16_t handle;
  uint16_t pad;
  size_t userSize;
};

struct Entry {
  bool active = false;
  bool ownsPtr = false;
  uint16_t handle = 0;
  uint8_t drainEpoch = 0;
  const char* owner = "";
  size_t bytes = 0;
  Priority priority = Priority::App;
  DrainFn drain = nullptr;
  void* ctx = nullptr;
  void* ptr = nullptr;
};

Entry gEntries[kMaxEntries];
uint16_t gNextHandle = 1;
uint8_t gDrainEpoch = 0;
uint8_t gEnsureDepth = 0;
char gForeground[24] = "";
char gPendingRequester[24] = "";
DrainedNotifyFn gDrainedNotify = nullptr;
void* gDrainedCtx = nullptr;
bool gInDrain = false;
bool gPendingNotify = false;

bool ownerIs(const char* owner, const char* name) {
  if (!owner || !name || !name[0]) return false;
  return strcmp(owner, name) == 0;
}

Snapshot measure() {
  Snapshot s;
  s.internalFree = heap_caps_get_free_size(kInternalCaps);
  s.internalLargest = heap_caps_get_largest_free_block(kInternalCaps);
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    if (!gEntries[i].active) continue;
    s.registeredBytes += (uint32_t)gEntries[i].bytes;
    s.holderCount++;
  }
  return s;
}

bool meets(size_t minContig, size_t minFree) {
  const Snapshot s = measure();
  if (minFree && s.internalFree < minFree) return false;
  if (minContig && s.internalLargest < minContig) return false;
  return true;
}

Entry* findByHandle(uint16_t handle) {
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    if (gEntries[i].active && gEntries[i].handle == handle) {
      return &gEntries[i];
    }
  }
  return nullptr;
}

void clearEntry(Entry& e) {
  e.active = false;
  e.ownsPtr = false;
  e.handle = 0;
  e.drainEpoch = 0;
  e.owner = "";
  e.bytes = 0;
  e.priority = Priority::App;
  e.drain = nullptr;
  e.ctx = nullptr;
  e.ptr = nullptr;
}

void queueNotify(const char* requester) {
  gPendingNotify = true;
  if (!requester) requester = "?";
  strncpy(gPendingRequester, requester, sizeof(gPendingRequester) - 1);
  gPendingRequester[sizeof(gPendingRequester) - 1] = '\0';
}

void flushNotify() {
  if (gEnsureDepth > 0 || !gPendingNotify) return;
  gPendingNotify = false;
  if (gDrainedNotify) {
    gDrainedNotify(gPendingRequester[0] ? gPendingRequester : "?", gDrainedCtx);
  }
}

void runDrain(Entry& e, const char* requester, bool notify) {
  if (e.drain) {
    gInDrain = true;
    e.drain(e.ctx);
    gInDrain = false;
  }
  e.drainEpoch = gDrainEpoch;
  if (e.ownsPtr && e.ptr) {
    heap_caps_free(e.ptr);
    clearEntry(e);
  } else {
    e.bytes = 0;
    e.ptr = nullptr;
  }
  if (notify) queueNotify(requester);
}

bool drainOne(Priority upTo, const char* requester) {
  Entry* best = nullptr;
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    Entry& e = gEntries[i];
    if (!e.active) continue;
    if (!e.drain && !(e.ownsPtr && e.ptr)) continue;
    if (e.priority > upTo) continue;
    if (gDrainEpoch && e.drainEpoch == gDrainEpoch) continue;
    if (!e.ownsPtr && e.bytes == 0) continue;
    if (e.priority == Priority::App || e.priority == Priority::Session) {
      if (ownerIs(e.owner, requester) || ownerIs(e.owner, gForeground)) {
        continue;
      }
    }
    if (!best || e.priority < best->priority) {
      best = &e;
    }
  }
  if (!best) return false;
  runDrain(*best, requester, /*notify=*/true);
  return true;
}

uint16_t allocHandle() {
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    if (!gEntries[i].active) {
      const uint16_t h = gNextHandle++;
      if (gNextHandle == 0) gNextHandle = 1;
      gEntries[i].active = true;
      gEntries[i].handle = h;
      return h;
    }
  }
  return 0;
}

} // namespace

void init() {
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    clearEntry(gEntries[i]);
  }
  gNextHandle = 1;
  gDrainEpoch = 0;
  gEnsureDepth = 0;
  gForeground[0] = '\0';
  gPendingRequester[0] = '\0';
  gDrainedNotify = nullptr;
  gDrainedCtx = nullptr;
  gInDrain = false;
  gPendingNotify = false;
}

Snapshot snapshot() { return measure(); }

Profile profileFor(Need need) {
  Profile p;
  switch (need) {
  case Need::SslHandshake:
    p.minInternalContig = 40000;
    p.minInternalFree = 52000;
    break;
  case Need::HttpClient:
    p.minInternalContig = 32000;
    p.minInternalFree = 45000;
    break;
  default:
    break;
  }
  return p;
}

void setForeground(const char* appName) {
  if (!appName) appName = "";
  strncpy(gForeground, appName, sizeof(gForeground) - 1);
  gForeground[sizeof(gForeground) - 1] = '\0';
}

const char* foregroundApp() { return gForeground; }

void setDrainedNotify(DrainedNotifyFn fn, void* ctx) {
  gDrainedNotify = fn;
  gDrainedCtx = ctx;
}

bool ensure(size_t minContig, size_t minFree, const char* requester,
            Priority drainUpTo) {
  if (meets(minContig, minFree)) return true;
  if (gInDrain || gEnsureDepth > 0) return false;

  gEnsureDepth++;
  gDrainEpoch++;
  if (gDrainEpoch == 0) gDrainEpoch = 1;

  const char* who = requester ? requester : "?";
  for (uint8_t pass = 0; pass < kMaxDrainPasses; pass++) {
    if (meets(minContig, minFree)) break;
    if (!drainOne(drainUpTo, who)) break;
  }
  const bool ok = meets(minContig, minFree);
  gEnsureDepth--;
  flushNotify();
  return ok;
}

bool ensureProfile(Profile profile, const char* requester, Priority drainUpTo) {
  return ensure(profile.minInternalContig, profile.minInternalFree, requester,
                drainUpTo);
}

bool ensureNeed(Need need, const char* requester, Priority drainUpTo) {
  return ensureProfile(profileFor(need), requester, drainUpTo);
}

void log(const char* tag) {
  const Snapshot s = measure();
  Serial.printf("[ram %s] free=%u contig=%u held=%u entries=%u fg=%s\n",
                tag ? tag : "?", (unsigned)s.internalFree,
                (unsigned)s.internalLargest, (unsigned)s.registeredBytes,
                (unsigned)s.holderCount, gForeground[0] ? gForeground : "-");
}

void dumpHolders() {
  Serial.println("[ram holders]");
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    const Entry& e = gEntries[i];
    if (!e.active) continue;
    Serial.printf("  #%u %s %u B pri=%u ptr=%s drain=%s\n",
                  (unsigned)e.handle, e.owner ? e.owner : "?",
                  (unsigned)e.bytes, (unsigned)e.priority,
                  e.ptr ? "yes" : "no", e.drain ? "yes" : "no");
  }
}

uint16_t registerHolder(const char* owner, size_t bytes, Priority priority,
                        DrainFn drain, void* ctx, void* ptr) {
  const uint16_t h = allocHandle();
  if (!h) {
    Serial.printf("[ram] holder table full (%s)\n", owner ? owner : "?");
    return 0;
  }
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    Entry& e = gEntries[i];
    if (e.handle != h) continue;
    e.owner = owner ? owner : "";
    e.bytes = bytes;
    e.priority = priority;
    e.drain = drain;
    e.ctx = ctx;
    e.ptr = ptr;
    e.ownsPtr = false;
    e.drainEpoch = 0;
    return h;
  }
  return 0;
}

uint16_t registerDrainer(const char* owner, Priority priority, DrainFn drain,
                         void* ctx) {
  return registerHolder(owner, 0, priority, drain, ctx, nullptr);
}

void setHolderBytes(uint16_t handle, size_t bytes) {
  Entry* e = findByHandle(handle);
  if (!e) return;
  e->bytes = bytes;
}

void unregisterHolder(uint16_t handle) {
  Entry* e = findByHandle(handle);
  if (!e) return;
  if (e->ownsPtr && e->ptr) {
    heap_caps_free(e->ptr);
  }
  clearEntry(*e);
}

void releaseAllForOwner(const char* owner) {
  if (!owner) return;
  for (uint8_t i = 0; i < kMaxEntries; i++) {
    Entry& e = gEntries[i];
    if (!e.active || !e.owner) continue;
    if (strcmp(e.owner, owner) != 0) continue;
    runDrain(e, owner, /*notify=*/false);
    if (e.active) clearEntry(e);
  }
}

void* alloc(size_t bytes, const char* owner, Priority priority, DrainFn drain,
            void* ctx) {
  if (bytes == 0) return nullptr;
  if (gInDrain) return nullptr;

  const size_t total = sizeof(BlockHeader) + bytes;
  void* raw = heap_caps_malloc(total, kInternalCaps);
  if (!raw) {
    ensure(bytes + 4096, 0, owner ? owner : "?", priority);
    raw = heap_caps_malloc(total, kInternalCaps);
  }
  if (!raw) return nullptr;

  const uint16_t h = allocHandle();
  if (!h) {
    Serial.printf("[ram] holder table full (%s)\n", owner ? owner : "?");
    heap_caps_free(raw);
    return nullptr;
  }

  Entry* e = findByHandle(h);
  if (!e) {
    heap_caps_free(raw);
    return nullptr;
  }

  auto* hdr = reinterpret_cast<BlockHeader*>(raw);
  hdr->magic = kBlockMagic;
  hdr->handle = h;
  hdr->pad = 0;
  hdr->userSize = bytes;

  e->owner = owner ? owner : "";
  e->bytes = bytes;
  e->priority = priority;
  e->drain = drain;
  e->ctx = ctx;
  e->ptr = raw;
  e->ownsPtr = true;
  e->drainEpoch = 0;

  return reinterpret_cast<uint8_t*>(raw) + sizeof(BlockHeader);
}

void free(void* userPtr) {
  if (!userPtr) return;
  auto* raw = reinterpret_cast<uint8_t*>(userPtr) - sizeof(BlockHeader);
  auto* hdr = reinterpret_cast<BlockHeader*>(raw);
  if (hdr->magic != kBlockMagic) return;
  unregisterHolder(hdr->handle);
}

} // namespace RamManager
