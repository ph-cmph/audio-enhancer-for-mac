#!/bin/bash
set -euo pipefail
FXMAC_ROOT="$(cd "$(dirname "$0")" && pwd)"
FXMAC_CTEST="$(command -v ctest || true)"
if test -z "$FXMAC_CTEST"; then
    if test -x "$FXMAC_ROOT/../fxsound-mac-research/toolchain/bin/ctest"; then
        FXMAC_CTEST="$FXMAC_ROOT/../fxsound-mac-research/toolchain/bin/ctest"
    else
        FXMAC_CTEST="$FXMAC_ROOT/.build-tools/bin/ctest"
    fi
fi
"$FXMAC_CTEST" --test-dir "$FXMAC_ROOT/build" --output-on-failure
