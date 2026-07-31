#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPOSITORY_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPOSITORY_ROOT/out/hmi-linux-debug}"
APP_PATH="$BUILD_DIR/appDrivePilot"

if [[ ! -x "$APP_PATH" ]]; then
    printf 'Executable not found: %s\nRun scripts/linux/configure_hmi_debug.sh first.\n' "$APP_PATH" >&2
    exit 1
fi

if [[ "${DRIVEPILOT_FORCE_XCB:-0}" == "1" ]]; then
    export QT_QPA_PLATFORM=xcb
fi

cd "$REPOSITORY_ROOT/hmi-client"
exec "$APP_PATH"
