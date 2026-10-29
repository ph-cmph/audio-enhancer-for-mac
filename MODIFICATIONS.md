# Independent macOS port modifications

Modified 28 October 2026. Original copyright and licence notices are retained.

The original FxSound engine and community macOS base are credited in THIRD_PARTY_NOTICES.md. This release adds reliability fixes, single-button routing, volume synchronisation, Unicode presets, session persistence, system appearance and a passive audio visualiser. Public-distribution preparation removes fonts without supplied redistribution terms and replaces upstream product artwork with original geometric assets.

Files added or changed relative to the pinned primary macOS base:

- `FxSoundMac/Source/App/FxSoundMacApp.h`
- `FxSoundMac/Source/Audio/AudioActivity.cpp`
- `FxSoundMac/Source/Audio/AudioActivity.h`
- `FxSoundMac/Source/Audio/AudioEngineStatus.h`
- `FxSoundMac/Source/Audio/AudioRouteLease.h`
- `FxSoundMac/Source/Audio/DeviceValidation.cpp`
- `FxSoundMac/Source/Audio/DeviceValidation.h`
- `FxSoundMac/Source/Audio/MacAudioEngine.cpp`
- `FxSoundMac/Source/Audio/MacAudioEngine.h`
- `FxSoundMac/Source/Audio/SystemAudioRoute.cpp`
- `FxSoundMac/Source/Audio/SystemVolumeBridge.h`
- `FxSoundMac/Source/DSP/LegacyDspAdapter.cpp`
- `FxSoundMac/Source/DSP/LegacyDspAdapter.h`
- `FxSoundMac/Source/DSP/Utf8Paths.h`
- `FxSoundMac/Source/DSP/file_mac_stub.cpp`
- `FxSoundMac/Source/DSP/pstrWide_mac.cpp`
- `FxSoundMac/Source/DSP/pt_mac_compat.h`
- `FxSoundMac/Source/Support/AppSettings.h`
- `FxSoundMac/Source/Support/FxController.cpp`
- `FxSoundMac/Source/Support/FxController.h`
- `FxSoundMac/Source/UI/FxEffects.cpp`
- `FxSoundMac/Source/UI/FxEffects.h`
- `FxSoundMac/Source/UI/FxEqualizer.cpp`
- `FxSoundMac/Source/UI/FxEqualizer.h`
- `FxSoundMac/Source/UI/FxPowerButton.cpp`
- `FxSoundMac/Source/UI/FxPowerButton.h`
- `FxSoundMac/Source/UI/FxTheme.cpp`
- `FxSoundMac/Source/UI/FxTheme.h`
- `FxSoundMac/Source/UI/MainComponent.cpp`
- `FxSoundMac/Source/UI/MainComponent.h`
- `FxSoundMac/Source/UI/NativeMotion.h`
- `FxSoundMac/Source/UI/NativeMotion.mm`
- `FxSoundMac/Source/UI/SetupStatusPanel.cpp`
- `FxSoundMac/Tests/AudioActivityTest.cpp`
- `FxSoundMac/Tests/CoreAudioSnapshot.cpp`
- `FxSoundMac/Tests/LegacyDspAdapterTest.cpp`
- `FxSoundMac/Tests/PresetLibraryTest.cpp`
- `FxSoundMac/Tests/ReliabilityTests.cpp`
- `FxSoundMac/Tests/SystemVolumeBridgeTest.cpp`
- `dsp/ptComSftDfx/Comsftwr.c`
- `dsp/ptutil/COM/Comwrite.cpp`
- `dsp/ptutil/VALS/Valsfile.cpp`
- `dsp/u_DfxDsp.h`

JUCE 7.0.12 retains its original licensing. Its SDK compatibility change is reproducibly applied by `FxSoundMac/patches/patch_juce_windowing.py`.

## 0.3.1 — 29 October 2026

Corrected opaque light/dark palette colours and native-window refresh during appearance changes. Added `FxSoundMac/Source/UI/FxPalette.h` and `FxSoundMac/Tests/ThemePaletteTest.cpp`; updated FxTheme, MainComponent, MainWindow and the build version. Original notices remain intact.

## 0.3.2 — 29 October 2026

Rebalanced MainComponent header/control rows, aligned labels, and kept selector positions stable. Updated the build version. No DSP or routing logic changed.

## 2026-10-29 — numeric readouts and preset edit protection (0.4.0)

Added fixed enhancement and EQ readouts with quiet original-value references, and a compact toolbar. Added PresetSession tracking, guarded preset switching, explicit save-copy naming without overwrites, stable file-based selection after rescans, session baseline recovery, and safer import/delete selection handling. Save failures retain the working settings. Replaced problematic non-ASCII action labels with English ASCII text and gave dialog alternatives a quieter appearance.

Validation: preset edit/copy tests exercise original-file preservation, custom-name collisions, saved-value round trips, restored edits, failed loads/saves and independent master-volume/power state. Existing audio, routing, palette and bundle checks also run. The installed private build passed its test suite and strict signature verification. Live dark-mode UI showed fixed readouts and original references, the edited indicator, the switch prompt, and active audio visualisation. The neutral public 0.4.0 build also passes both CTest suites and strict local signature verification. It retains the public name, original artwork, attribution and licence notices.
