#pragma once

#include "FS.h"

class LittleFSClass : public fs::FS {
public:
  bool begin(bool = false) { return true; }
};

extern LittleFSClass LittleFS;

