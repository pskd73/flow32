# Contributing

Flow32 favors straightforward embedded C++ over broad abstraction.

Before opening a change:

1. Keep the retained App → Page → UI tree model as the only UI path.
2. Put controller, bus, GPIO, and filesystem policy behind an application-owned
   adapter rather than a conditional in generic code.
3. Keep hot paths allocation-free or deterministically bounded.
4. Validate all offsets, lengths, counts, and arithmetic from external files.
5. Preserve C++11 unless a compelling, documented reason requires otherwise.
6. Add no dependency without explaining the memory, flash, and ownership cost.
7. Update an example and host regression test for user-visible behavior.

Run:

```bash
./test/host/run.sh
python3 tools/flow32_configure.py --check
```

An ESP32 PlatformIO build of at least one real application is required for
changes involving Arduino libraries, transports, storage, PSRAM, or GPIO.

## Adding a transport

Implement `DisplayTransport` in a small adapter. Keep its vendor includes and
hardware configuration out of `Display`. Pins must default to unset. The
application remains responsible for constructing buses and choosing wiring.

## Adding an input source

Emit normalized UI events through `InputSource`. Mechanical inputs must
stabilize before `KeyTracker`; analog inputs need explicit threshold behavior.
Cover noisy samples and time wrap-safe subtraction in host tests.

## Adding an asset format

Treat the file as hostile. Decode fixed-width little-endian values explicitly,
check multiplication/addition before use, cap plausible index sizes, validate
every record against its section, and return a safe failure without partial
ready state.

## Adding a widget

Use the existing retained node lifecycle: bounded arena allocation, layout,
paint, focus/event behavior, and state callbacks. Do not create a parallel
representation. Document any fixed bound introduced and test overflow behavior.

## Optional module registry

Every header and implementation under `src/` belongs to exactly one module in
`modules.json`. The registry exists only for advanced reduced-source exports.
Normal users continue to include `Flow32.h`.
