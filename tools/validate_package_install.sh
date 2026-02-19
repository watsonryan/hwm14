#!/usr/bin/env bash
set -euo pipefail

# Author: watsonryan
# Purpose: Validate install and downstream find_package(hwm14) consumption.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
INSTALL_DIR="${HWM14_INSTALL_PREFIX:-/tmp/hwm14-install}"
CONSUMER_SRC="$ROOT/tests/package_consumer"
CONSUMER_BUILD="$ROOT/build/package-consumer"

if [[ -n "${HWM14_RELEASE_PRESET:-}" ]]; then
  RELEASE_PRESET="$HWM14_RELEASE_PRESET"
else
  case "$(uname -s)" in
    Darwin) RELEASE_PRESET="macos-release" ;;
    Linux) RELEASE_PRESET="linux-release" ;;
    MINGW*|MSYS*|CYGWIN*) RELEASE_PRESET="windows-release" ;;
    *)
      echo "error: unsupported platform for default release preset; set HWM14_RELEASE_PRESET" >&2
      exit 1
      ;;
  esac
fi

cmake --preset "$RELEASE_PRESET"
cmake --build --preset "$RELEASE_PRESET"
cmake --install "$ROOT/build/$RELEASE_PRESET" --prefix "$INSTALL_DIR"

cmake -S "$CONSUMER_SRC" -B "$CONSUMER_BUILD" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$INSTALL_DIR"

cmake --build "$CONSUMER_BUILD"
"$CONSUMER_BUILD/hwm14_package_consumer"

echo "package install/consumer validation passed"
