#!/usr/bin/env bash
set -euo pipefail

# Author: watsonryan
# Purpose: Build profile preset and run HWM14 throughput benchmark.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cmake --preset macos-profile
cmake --build --preset macos-profile -j

HWM14_PERF_SAMPLES="${HWM14_PERF_SAMPLES:-40}" \
HWM14_PERF_ITERATIONS="${HWM14_PERF_ITERATIONS:-10000}" \
  ./build/macos-profile/tests/hwm14_perf_benchmark
