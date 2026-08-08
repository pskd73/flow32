#pragma once

#include <Flow32.h>

/**
 * App-side storage profiles — change pins here when the board differs.
 * Library Storage code stays board-agnostic.
 */

/**
 * OceanLabz ESP32-S3 WROOM N16R8 CAM (onboard microSD, SDMMC 1-bit).
 * Typical clone wiring: CLK=39, CMD=38, D0=40.
 * Amazon / OceanLabz CAM boards with a rear TF slot.
 */
inline StorageConfig SdOceanLabzCam() {
  StorageConfig c;
  c.id = "OceanLabz-N16R8-CAM";
  c.bus = SdBus::Sdmmc;
  c.mountPoint = "/sdcard";
  c.pinClk = 39;
  c.pinCmd = 38;
  c.pinD0 = 40;
  c.sdmmc1bit = true;
  return c;
}

/** Alias — default board for this repo's demo. */
inline StorageConfig SdDefault() { return SdOceanLabzCam(); }
