#!/usr/bin/env bash
set -u

QT_ROOT="${QT_ROOT:-$HOME/Qt/6.9.1/gcc_64}"
FAILED=0

ok() { printf '[OK]   %s\n' "$1"; }
warn() { printf '[WARN] %s\n' "$1"; }
fail() { printf '[FAIL] %s\n' "$1"; FAILED=1; }

printf 'DrivePilot Cockpit - Ubuntu HMI environment check\n'
printf 'QT_ROOT=%s\n\n' "$QT_ROOT"

if [[ "$(uname -s)" == "Linux" ]]; then ok "Linux host"; else fail "This script must run on Linux"; fi
if [[ "$(uname -m)" == "x86_64" ]]; then ok "x86_64 architecture"; else warn "Current architecture: $(uname -m)"; fi

for command in cmake ninja g++ pkg-config; do
    if command -v "$command" >/dev/null 2>&1; then
        ok "$command: $(command -v "$command")"
    else
        fail "Missing command: $command"
    fi
done

if [[ -x "$QT_ROOT/bin/qtpaths" ]]; then
    ok "Qt: $($QT_ROOT/bin/qtpaths --qt-version 2>/dev/null || true)"
elif [[ -x "$QT_ROOT/bin/qtpaths6" ]]; then
    ok "Qt: $($QT_ROOT/bin/qtpaths6 --qt-version 2>/dev/null || true)"
else
    fail "Qt was not found under QT_ROOT"
fi

for module in Qt6Quick Qt6Multimedia Qt6Network Qt6WebSockets Qt6Sql Qt6Concurrent Qt6WebEngineQuick Qt6WebChannel; do
    if [[ -d "$QT_ROOT/lib/cmake/$module" ]]; then
        ok "$module"
    else
        fail "Missing Qt module: $module"
    fi
done

for library in libGL.so libnss3.so libasound.so libpulse.so libxkbcommon.so; do
    if ldconfig -p 2>/dev/null | grep -q "$library"; then
        ok "$library"
    else
        warn "$library was not found in the linker cache"
    fi
done

if [[ -n "${XDG_SESSION_TYPE:-}" ]]; then
    ok "Display session: $XDG_SESSION_TYPE"
else
    warn "XDG_SESSION_TYPE is not set"
fi

if [[ -e /dev/snd ]]; then
    ok "Audio devices detected under /dev/snd"
else
    warn "No audio device detected; the HMI can still start, but media playback may be unavailable"
fi

printf '\n'
if [[ $FAILED -eq 0 ]]; then
    printf 'PASS: the Qt HMI build environment is ready.\n'
else
    printf 'FAIL: install the required build tools or Qt modules listed above.\n'
fi
exit "$FAILED"
