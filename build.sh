#!/bin/bash
set -euo pipefail
FXMAC_ROOT="$(cd "$(dirname "$0")" && pwd)"
FXMAC_TOOLS="$FXMAC_ROOT/../fxsound-mac-research/toolchain/bin"
if command -v cmake >/dev/null && command -v ninja >/dev/null; then
    FXMAC_CMAKE="$(command -v cmake)"
elif test -x "$FXMAC_TOOLS/cmake"; then
    FXMAC_CMAKE="$FXMAC_TOOLS/cmake"
    export PATH="$FXMAC_TOOLS:$PATH"
else
    python3 -m venv "$FXMAC_ROOT/.build-tools"
    "$FXMAC_ROOT/.build-tools/bin/python" -m pip install cmake==3.31.10 ninja==1.13.2
    export PATH="$FXMAC_ROOT/.build-tools/bin:$PATH"
    FXMAC_CMAKE="$FXMAC_ROOT/.build-tools/bin/cmake"
fi
FXMAC_JUCE="$FXMAC_ROOT/third_party/JUCE"
if ! test -f "$FXMAC_JUCE/CMakeLists.txt"; then
    mkdir -p "$FXMAC_ROOT/third_party"
    git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git "$FXMAC_JUCE"
    test "$(git -C "$FXMAC_JUCE" rev-parse HEAD)" = 4f43011b96eb0636104cb3e433894cda98243626
fi
python3 "$FXMAC_ROOT/FxSoundMac/patches/patch_juce_windowing.py" "$FXMAC_JUCE/modules/juce_gui_basics/native/juce_Windowing_mac.mm"
"$FXMAC_CMAKE" -S "$FXMAC_ROOT/FxSoundMac" -B "$FXMAC_ROOT/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="${FXMAC_ARCHS:-$(uname -m)}" \
    -DFETCHCONTENT_SOURCE_DIR_JUCE="$FXMAC_JUCE"
"$FXMAC_CMAKE" --build "$FXMAC_ROOT/build" --parallel 4
FXMAC_APP="$FXMAC_ROOT/build/FxSoundMac_artefacts/Release/Audio Enhancer for Mac.app"
cp "$FXMAC_ROOT/LICENSE" "$FXMAC_APP/Contents/Resources/LICENSE.txt"
cp "$FXMAC_ROOT/THIRD_PARTY_NOTICES.md" "$FXMAC_APP/Contents/Resources/THIRD_PARTY_NOTICES.md"
mkdir -p "$FXMAC_APP/Contents/Resources/Licences"
cp "$FXMAC_ROOT/LICENSES/GPL-3.0.txt" "$FXMAC_APP/Contents/Resources/Licences/GPL-3.0.txt"
cp "$FXMAC_ROOT/LICENSES/JUCE-7-LICENSE.md" "$FXMAC_APP/Contents/Resources/Licences/JUCE-7-LICENSE.md"
codesign --force --sign - "$FXMAC_APP"
codesign --verify --deep --strict "$FXMAC_APP"
printf 'Built: %s\n' "$FXMAC_APP"
