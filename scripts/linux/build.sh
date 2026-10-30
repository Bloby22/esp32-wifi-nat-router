#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

BAUD_RATE=115200

ACTION="$(printf '%s' "${1:-build}" | tr '[:upper:]' '[:lower:]')"
BOARD="${2:-}"

# Without an environment name, PlatformIO uses what platformio.ini defines.
ENV_ARGS=()
if [ -n "$BOARD" ]; then
    ENV_ARGS=(-e "$BOARD")
fi
LABEL="${BOARD:-the default environment}"

usage() {
    echo "Usage: ./build.sh [build|flash|monitor|clean] [environment]"
}

case "$ACTION" in
    build|flash|monitor|clean) ;;
    help|-h|--help)
        usage
        exit 0
        ;;
    *)
        echo "[ERROR] Unknown action \"$ACTION\"." >&2
        usage >&2
        exit 1
        ;;
esac

# Find PlatformIO; fall back to its default install location.
if ! command -v pio >/dev/null 2>&1; then
    export PATH="$PATH:$HOME/.platformio/penv/bin"
fi
if ! command -v pio >/dev/null 2>&1; then
    echo "[ERROR] PlatformIO executable \"pio\" was not found." >&2
    echo "Install PlatformIO or add ~/.platformio/penv/bin to PATH." >&2
    exit 1
fi

case "$ACTION" in
    build)
        echo "=== Building firmware for $LABEL ==="
        pio run ${ENV_ARGS[@]+"${ENV_ARGS[@]}"}
        echo "=== Building LittleFS image for $LABEL ==="
        pio run ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} --target buildfs
        ;;
    flash)
        echo "=== Uploading web files to $LABEL ==="
        pio run ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} --target uploadfs
        echo "=== Uploading firmware to $LABEL ==="
        pio run ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} --target upload
        echo "=== Opening serial monitor at $BAUD_RATE baud ==="
        pio device monitor -b "$BAUD_RATE"
        ;;
    monitor)
        pio device monitor -b "$BAUD_RATE"
        ;;
    clean)
        echo "=== Cleaning $LABEL ==="
        pio run ${ENV_ARGS[@]+"${ENV_ARGS[@]}"} --target clean
        ;;
esac

echo "Done."