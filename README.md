# Audio Enhancer for Mac — 0.3.0 preview

**An independent, unofficial macOS audio enhancer derived from FxSound and community ports.** Not affiliated with or endorsed by FxSound LLC, Apple, JUCE or Existential Audio. FxSound's original DSP and upstream contributors are credited in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This is not an official FxSound release. Apple Silicon build; macOS 13 or later. Stereo playback is the supported starting point.

## Use

1. Open **Audio Enhancer for Mac.app**. Its power button starts grey/off, and launching the app does not change your sound output.
2. Install the separately supplied **BlackHole 2ch** driver from Existential Audio if needed. The official installer requires administrator authorisation and a Mac restart; save your work first.
3. Choose your actual speakers or headphones, then click the **power button**. It illuminates only after the audio engine starts successfully. Allow audio-input access if macOS asks. The app captures only the BlackHole virtual input, not a hardware microphone.
4. Choose a preset or adjust the effects, equaliser and output gain. Your settings are kept locally.
5. Click the **power button** again to turn processing off and restore your previous output. Quitting also restores it. If you manually select a different output while processing, that choice is respected.

If a device disconnects, processing stops and restores a connected output where possible. Choose an available device and restart processing. If a previous run was interrupted, the recovery journal is checked next time the app opens.

For outputs with adjustable volume, the Mac's system volume controls are linked to your selected playback device during processing. The app corrects BlackHole's separate input attenuation so the signal is not turned down twice before playback. Your playback volume stays at your chosen level; BlackHole's previous volume is restored on Stop or Quit. Fixed-level outputs keep their existing virtual volume path. The in-app dB slider is an additional output trim; start at 0 dB and use Dynamic Boost for the original FxSound loudness effect.

The power button is the only processing control. Setup and the separate Start/Stop controls have been removed, along with the routine bottom status paragraph. Actionable failures appear in a focused dialog and leave power off. No audio is uploaded; this build has no telemetry, automatic downloads, website-launch buttons or login services.

## Audio activity

The equaliser includes a quiet blue spectrum behind the controls, driven by the actual stereo output of the audio engine. The EQ knobs and adjustment line remain fixed. The label distinguishes **Processing**, **Waiting for audio**, and **Stopped**. Silence releases the spectrum naturally; turning power off clears it. The display is an activity guide, not a calibrated loudness meter.

Settings → **Show audio visualiser** turns animation off while keeping the activity label. macOS **Reduce Motion** is also respected. The spectrum uses a bounded queue; its analysis and drawing happen on the interface thread, and a hidden window skips analysis. It does not record audio, save samples or add a delay to playback.

Live animation, effects-off and Stop/Start behaviour were verified with MacBook Air Speakers at 48 kHz. Stop restored normal speaker playback and preserved the user's volume.

## What changed

Version 0.3.0 is named **Audio Enhancer for Mac**. It uses a single power control, a more compact window, rounded neutral cards, system fonts, quieter sliders and colours that follow macOS Light/Dark appearance. Independent-project attribution remains in About. The original application identity and existing `FxSound Community` support folder are retained internally to preserve preferences, presets, permissions and route recovery; there is no automatic start after reopening.

See [AUDIT.md](AUDIT.md) for reproduced issues, fixes and verification limits. Main changes include memory safety fixes for macOS integer widths, guarded control changes, bounded audio-buffer processing, safer routing and restoration, useful stopped/error states, saved sessions, working Unicode presets and a restrained blue interface using system fonts.

This preview has offline processing and regression coverage, plus confirmed live stereo playback at 48 kHz to Explorer² USB DAC. Stop and Quit both restored the normal output, and the user confirmed audible effects. Live hardware coverage is recorded separately in the audit; Bluetooth, HDMI and sleep/wake remain unverified.

## Public distribution

The public project uses an original geometric icon and system fonts; no original FxSound logo artwork or Gilroy font binaries are included. The complete corresponding source, pinned JUCE dependency and reproducible build scripts accompany the app. See [PUBLICATION_REVIEW.md](PUBLICATION_REVIEW.md) and [MODIFICATIONS.md](MODIFICATIONS.md). This remains copyleft open-source software, with no warranty.

## Preview packages

The app ZIP contains the Apple Silicon application. Copy it to Applications, install BlackHole 2ch separately from its official source, and follow the steps above. The app is locally ad-hoc signed; it is not Developer ID signed or notarised for public distribution. The accompanying Source ZIP contains the complete modified application, DSP, JUCE dependency, licences and build scripts. Keep both download links together if sharing this preview.

## Build

Clone with `git clone --recurse-submodules https://github.com/ph-cmph/audio-enhancer-for-mac.git`. JUCE is pinned to commit `4f43011b96eb0636104cb3e433894cda98243626`; the helper applies the included SDK patch reproducibly. The full Source release archive includes the complete JUCE dependency as well as this repository's code and build scripts.

Requires Xcode Command Line Tools, Python 3 and CMake 3.22+ (Ninja is used by the helper). `build.sh` can create an isolated local build-tool environment if CMake is absent. JUCE 7.0.12 source is bundled in the source package; when omitted, the helper downloads the pinned official revision. It never installs an audio driver or changes system sound settings.

```sh
bash build.sh
bash test.sh
```

The app is generated under `build/FxSoundMac_artefacts/Release/`. The helper copies licence notices and applies a local ad-hoc signature after all resources are added. Sharing a polished public release additionally needs Developer ID signing and notarisation.

Local preferences and the output/virtual-volume recovery journals live under `~/Library/Application Support/FxSound Community/`. Custom presets are in its `Presets` subfolder. Factory presets cannot be deleted in the app; custom preset deletion moves the file to the Trash.

## Licence and attribution

AGPL-3.0-or-later; full licence in LICENSE. Corresponding modified source accompanies this preview. See THIRD_PARTY_NOTICES.md for upstream authors, exact revisions and JUCE licence details.
