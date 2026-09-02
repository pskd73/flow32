#pragma once

#include <Arduino.h>
#include <SPI.h>

/** How the microSD slot is wired on this board. */
enum class SdBus : uint8_t { Spi, Sdmmc };

/**
 * Board wiring for optional SD storage. Applications can instead use
 * FsAssetStore with any filesystem they mount themselves.
 */
struct StorageConfig {
  const char *id = "Sd";
  SdBus bus = SdBus::Sdmmc;

  /** Mount point used by Arduino FS (e.g. "/sdcard"). */
  const char *mountPoint = "/sdcard";

  // --- SPI mode ---
  int pinCs = -1;
  int pinMosi = -1;
  int pinMiso = -1;
  int pinSclk = -1;
  uint32_t spiHz = 20000000;
  SPIClass *spi = nullptr;

  // --- SDMMC mode (ESP32 / ESP32-S3) ---
  int pinClk = -1;
  int pinCmd = -1;
  int pinD0 = -1;
  int pinD1 = -1;
  int pinD2 = -1;
  int pinD3 = -1;
  /** Prefer 1-bit SDMMC (common on CAM boards with only D0 wired). */
  bool sdmmc1bit = true;
};
