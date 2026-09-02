#include "flow32/core/FlowError.h"

#include <stdlib.h>

namespace {

void defaultFatalHandler(FlowError) { abort(); }

FlowFatalHandler g_handler = defaultFatalHandler;

} // namespace

void setFlowFatalHandler(FlowFatalHandler handler) {
  g_handler = handler ? handler : defaultFatalHandler;
}

FlowFatalHandler flowFatalHandler() { return g_handler; }

void flowFatal(FlowError error) {
  g_handler(error);
  abort();
}

