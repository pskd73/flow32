# Optional minimal-package exporter

The normal developer experience is the complete Arduino library:

```cpp
#include <Flow32.h>
```

No module selection is required. `modules.json`, `flow32.config.json`, and
`tools/flow32_configure.py` are advanced maintainer tooling for projects that
must vendor a physically reduced source package. Link-time dead stripping often
makes final firmware sizes similar; the exporter mainly reduces vendored code
and compile surface.

Available profiles:

| Profile | Contents |
| --- | --- |
| `display-only` | generic framebuffer, fonts, and transport interface |
| `st77xx-display` | display plus the Adafruit ST77xx adapter |
| `retained-ui` | hardware-neutral retained pages, controls, layout, and assets |
| `full` | the normal complete library |

Check and inspect a configuration:

```bash
python3 tools/flow32_configure.py --list
python3 tools/flow32_configure.py --check
python3 tools/flow32_configure.py --dry-run
```

Export it outside this source tree:

```bash
python3 tools/flow32_configure.py \
  --config flow32.config.json \
  --output /path/to/project/lib/Flow32
```

The generated package contains only the dependency closure, a
`Flow32Selected.h` umbrella, generated compile-time overrides, and a lock file
with source/output hashes. `--force` replaces only directories bearing the
generator's ownership marker.

Example configuration:

```json
{
  "schema": 1,
  "profile": "retained-ui",
  "modules": [],
  "defines": {
    "FLOW32_UI_ARENA_BYTES": 6144,
    "FLOW32_INPUT_QUEUE_SIZE": 12
  }
}
```

Profiles are not a second runtime architecture. They only select source files
for the same retained UI and display stack.
