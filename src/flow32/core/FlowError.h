#pragma once

#include <stdint.h>

/** Deterministic failures for bounds that cannot be represented as a return. */
enum class FlowError : uint8_t {
  UiArenaExhausted,
  UiTreeLimitExceeded,
  PageFramebufferAllocationFailed,
  InvalidInputConfiguration,
};

/**
 * A fatal handler must not return. On embedded targets it may log and restart;
 * host tests commonly throw. If it does return, Flow32 calls abort().
 */
using FlowFatalHandler = void (*)(FlowError error);

void setFlowFatalHandler(FlowFatalHandler handler);
FlowFatalHandler flowFatalHandler();
void flowFatal(FlowError error);

