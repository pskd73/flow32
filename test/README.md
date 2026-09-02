# Host tests

Run from the Flow32 directory:

```bash
./test/host/run.sh
```

The suite compiles with strict warnings under C++11 and C++17. It covers the
public headers, framebuffer transport, retained UI, fatal arena exhaustion,
input queue, switch debounce, repeat behavior, joystick hysteresis, runtime
initialization, Shell invalidation, RAM-only and persistent state, malformed
asset bounds, optional package export, and every example.

Temporary output uses `${TMPDIR:-/tmp}` so the script works on macOS and Linux.
