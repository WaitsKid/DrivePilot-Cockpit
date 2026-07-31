#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPOSITORY_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
SOURCE_DIR="$REPOSITORY_ROOT/hmi-client"
QT_ROOT="${QT_ROOT:-$HOME/Qt/6.9.1/gcc_64}"
BUILD_DIR="${BUILD_DIR:-$REPOSITORY_ROOT/out/hmi-linux-debug}"

if [[ ! -d "$QT_ROOT/lib/cmake/Qt6" ]]; then
    printf 'Qt was not found: %s\n' "$QT_ROOT" >&2
    printf 'Set QT_ROOT to the Qt 6.9.1 gcc_64 directory.\n' >&2
    exit 1
fi

cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$QT_ROOT"

cmake --build "$BUILD_DIR" --parallel
printf '\nBuild completed: %s/appDrivePilot\n' "$BUILD_DIR"
