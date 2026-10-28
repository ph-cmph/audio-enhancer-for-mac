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
