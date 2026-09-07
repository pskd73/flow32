#pragma once

#include <stdint.h>

class Storage;

/** Decode a card-rooted PNG into RGB565 (PSRAM). Caller frees with splashFreePng. */
bool splashLoadPng(Storage *sd, const char *path, uint16_t **pixels, int16_t *w,
                   int16_t *h);
void splashFreePng(uint16_t *pixels);
