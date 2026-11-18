#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
IDF_ACTIVATE_SCRIPT="${IDF_ACTIVATE_SCRIPT:-$HOME/.espressif/tools/activate_idf_v6.1.sh}"

if [[ ! -f "$IDF_ACTIVATE_SCRIPT" ]]; then
    printf 'ESP-IDF activation script not found: %s\n' "$IDF_ACTIVATE_SCRIPT" >&2
    printf 'Set IDF_ACTIVATE_SCRIPT to your ESP-IDF activation script.\n' >&2
    exit 1
fi

# EIM's activation script can export its environment without being sourced.
while IFS='=' read -r name value; do
    case "$name" in
        PATH)
            export PATH="$value:$PATH"
            ;;
        ESP_IDF_VERSION|IDF_VERSION|IDF_TOOLS_PATH|IDF_PATH|IDF_PYTHON_ENV_PATH|ESP_ROM_ELF_DIR)
            export "$name=$value"
            ;;
    esac
done < <("$IDF_ACTIVATE_SCRIPT" -e)

exec idf.py -C "$PROJECT_DIR/src/esp32" "$@"
