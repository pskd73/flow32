#include "SplashPng.h"

#include "Storage.h"

#include <Arduino.h>
#include <PNGdec.h>
#include <esp_heap_caps.h>
#include <string.h>

namespace {

Storage *sd_ = nullptr;
char path_[96] = {};
File file_;
PNG *png_ = nullptr;
uint16_t *pixels_ = nullptr;
uint16_t *line_ = nullptr;
int16_t w_ = 0;
int16_t h_ = 0;

void *psramOrRam(size_t n) {
  void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = malloc(n);
  return p;
}

void *fileOpen(const char *filename, int32_t *size) {
  (void)filename;
  if (!sd_ || !path_[0]) return nullptr;
  file_ = sd_->open(path_, FILE_READ);
  if (!file_) return nullptr;
  *size = (int32_t)file_.size();
  return &file_;
}

void fileClose(void *handle) {
  (void)handle;
  if (file_) file_.close();
}

int32_t pngRead(PNGFILE *file, uint8_t *buf, int32_t len) {
  (void)file;
  if (!file_) return 0;
  return (int32_t)file_.read(buf, len);
}

int32_t pngSeek(PNGFILE *file, int32_t pos) {
  (void)file;
  if (!file_) return 0;
  return file_.seek(pos) ? pos : 0;
}

int pngDraw(PNGDRAW *draw) {
  if (!pixels_ || !png_ || !draw || !line_ || !draw->pPixels) return 0;
  if (draw->y < 0 || draw->y >= h_) return 1;
  png_->getLineAsRGB565(draw, line_, PNG_RGB565_LITTLE_ENDIAN, 0x00000000u);
  memcpy(pixels_ + (size_t)draw->y * (size_t)w_, line_,
         (size_t)w_ * sizeof(uint16_t));
  return 1;
}

void freeScratch() {
  if (line_) {
    heap_caps_free(line_);
    line_ = nullptr;
  }
  if (png_) {
    heap_caps_free(png_);
    png_ = nullptr;
  }
}

} // namespace

void splashFreePng(uint16_t *pixels) {
  if (pixels) heap_caps_free(pixels);
}

bool splashLoadPng(Storage *sd, const char *path, uint16_t **pixels, int16_t *w,
                   int16_t *h) {
  if (pixels) *pixels = nullptr;
  if (w) *w = 0;
  if (h) *h = 0;
  if (!sd || !sd->ready() || !path || !path[0] || !pixels || !w || !h) {
    return false;
  }

  char abs[96];
  if (!sd->absPath(path, abs, sizeof(abs))) return false;
  if (!sd->exists(abs)) {
    Serial.printf("Splash: not found %s\n", abs);
    return false;
  }

  sd_ = sd;
  strncpy(path_, abs, sizeof(path_) - 1);
  path_[sizeof(path_) - 1] = '\0';
  pixels_ = nullptr;
  w_ = h_ = 0;

  png_ = (PNG *)psramOrRam(sizeof(PNG));
  if (!png_) {
    sd_ = nullptr;
    path_[0] = '\0';
    return false;
  }
  memset(png_, 0, sizeof(PNG));

  if (png_->open(path_, fileOpen, fileClose, pngRead, pngSeek, pngDraw) !=
      PNG_SUCCESS) {
    Serial.printf("Splash: png open failed %s err=%d\n", path_,
                  png_->getLastError());
    freeScratch();
    sd_ = nullptr;
    path_[0] = '\0';
    return false;
  }

  const int srcW = png_->getWidth();
  const int srcH = png_->getHeight();
  if (srcW < 1 || srcH < 1 || srcW > 1024 || srcH > 1024 ||
      png_->isInterlaced()) {
    Serial.printf("Splash: png %dx%d unsupported\n", srcW, srcH);
    png_->close();
    freeScratch();
    sd_ = nullptr;
    path_[0] = '\0';
    return false;
  }

  w_ = static_cast<int16_t>(srcW);
  h_ = static_cast<int16_t>(srcH);
  const size_t bytes = (size_t)w_ * (size_t)h_ * sizeof(uint16_t);
  pixels_ = (uint16_t *)psramOrRam(bytes);
  line_ = (uint16_t *)psramOrRam((size_t)w_ * sizeof(uint16_t));
  if (!pixels_ || !line_) {
    Serial.println("Splash: OOM");
    png_->close();
    if (pixels_) heap_caps_free(pixels_);
    pixels_ = nullptr;
    freeScratch();
    sd_ = nullptr;
    path_[0] = '\0';
    return false;
  }
  memset(pixels_, 0, bytes);

  const int rc = png_->decode(nullptr, 0);
  const int err = png_->getLastError();
  png_->close();
  freeScratch();
  sd_ = nullptr;
  path_[0] = '\0';

  if (rc != PNG_SUCCESS) {
    Serial.printf("Splash: png decode failed err=%d\n", err);
    heap_caps_free(pixels_);
    pixels_ = nullptr;
    w_ = h_ = 0;
    return false;
  }

  *pixels = pixels_;
  *w = w_;
  *h = h_;
  pixels_ = nullptr;
  w_ = h_ = 0;
  return true;
}
