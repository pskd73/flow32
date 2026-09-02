#!/usr/bin/env bash
set -euo pipefail

flow_root="$(cd "$(dirname "$0")/../.." && pwd)"
flow_tmp_root="${TMPDIR:-/tmp}"
flow_build_dir="$(mktemp -d "$flow_tmp_root/flow32-host.XXXXXX")"
cleanup() {
  rm -rf -- "$flow_build_dir"
}
trap cleanup EXIT

sources=(
  "$flow_root"/src/flow32/assets/*.cpp
  "$flow_root"/src/flow32/core/*.cpp
  "$flow_root"/src/flow32/graphics/*.cpp
  "$flow_root"/src/flow32/input/*.cpp
  "$flow_root"/src/flow32/ui/*.cpp
  "$flow_root"/test/host/shim/*.cpp
  "$flow_root"/test/host/test.cpp
)

PYTHONPATH="$flow_root/tools" python3 "$flow_root/test/host/test_configure.py"
python3 "$flow_root/tools/flow32_configure.py" \
  --config "$flow_root/flow32.config.json" \
  --output "$flow_build_dir/Flow32Selected"

for standard in c++11 c++17; do
  binary="$flow_build_dir/flow32_host_test_${standard}"
  c++ -std="$standard" -Wall -Wextra -Werror -pedantic \
    -I"$flow_root/test/host/shim" \
    -I"$flow_root/src" \
    "${sources[@]}" \
    -o "$binary"
  "$binary"

  selected_binary="$flow_build_dir/flow32_selected_${standard}"
  c++ -std="$standard" -Wall -Wextra -Werror -pedantic \
    -I"$flow_root/test/host/shim" \
    -I"$flow_build_dir/Flow32Selected/src" \
    "$flow_build_dir"/Flow32Selected/src/flow32/assets/*.cpp \
    "$flow_build_dir"/Flow32Selected/src/flow32/core/*.cpp \
    "$flow_build_dir"/Flow32Selected/src/flow32/graphics/*.cpp \
    "$flow_build_dir"/Flow32Selected/src/flow32/input/*.cpp \
    "$flow_build_dir"/Flow32Selected/src/flow32/ui/*.cpp \
    "$flow_root"/test/host/shim/*.cpp \
    "$flow_root/test/host/test_selected.cpp" \
    -o "$selected_binary"
  "$selected_binary"

  for sketch in "$flow_root"/examples/*/*.ino; do
    c++ -x c++ -std="$standard" -Wall -Wextra -Werror -pedantic \
      -I"$flow_root/test/host/shim" \
      -I"$flow_root/src" \
      -fsyntax-only "$sketch"
  done
done

printf 'flow32 host tests and examples passed (C++11 and C++17)\n'
