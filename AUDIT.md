# Mac port audit — 28 October 2026

## Starting points

The upstream Windows app and two community Mac implementations were inspected. The simpler okku007 development port was chosen for its modular processing wrapper and focused tests. The alternative daemon-based port remains useful reference material, but adds two login services and defaults to a 48 kHz path; its native process-tap backend is documented as experimental. Those are design trade-offs and unverified hardware coverage, not claims of reproduced defects.

Pinned revisions and licences are listed in THIRD_PARTY_NOTICES.md. Neither upstream installer nor community setup scripts were run.

## Confirmed issues and changes

| Finding | Evidence | Change |
|---|---|---|
| Windows-sized parameter read on LP64 macOS | AddressSanitizer reported a stack-buffer-overflow in `comRealWrite`: reading 8 bytes from a 4-byte float | Explicit 32-bit word copying with `memcpy`, including the software parameter bridge and legacy bit conversion |
| Uninitialised DSP ownership/state members | Handles, logging pointer and section fields had no initial values | Explicit null/zero member initialisers; repeated construction/destruction covered by tests |
| UI and audio thread can mutate the same DSP | Controller setters/load/save and audio processing used the same adapter without synchronisation | One DSP gate; audio only uses try-lock and bounded dry pass-through during control edits |
| Callback memory allocation and possible DSP reset | Audio callback grew scratch buffers; adapter called `prepare` for oversized blocks | Allocate at device preparation; process oversized callbacks in fixed prepared chunks without resetting filter history |
| DSP initialisation result ignored | Controller marked prepared regardless of adapter return | Propagate failure; never route system audio before the complete engine is ready |
| Invalid/virtual playback route and implicit microphone fallback | Backend accepted capture as output and enabled fallback device selection | Exact BlackHole 2ch capture, physical output filtering and no fallback to microphone |
| Stale status after stop/unplug | Empty `audioDeviceStopped` and unchanged running state | Atomic device lifecycle flags, message-thread status updates and explicit stopped/disconnected recovery |
| Mac output can remain routed into an inactive virtual device | Base required manual routing/restoration | Scoped output lease, durable recovery journal, restore on stop/failure, preserve later user changes, speaker fallback if headphones disappear |
| Custom Unicode presets fail | New round-trip test failed saving `音楽` | Locale-independent UTF-8 path/label bridge and direct path composition |
| Preset tests did not compile | `scan` now requires two directories; tests still supplied one | Repaired tests, unique writable test directories and real factory/custom preset processing tests |
| Initial preset label did not load the displayed preset | UI selected a combo item without loading its settings | Explicit default factory preset load and session restoration |
| Control/device preferences lost at relaunch | Base persisted only close-button behaviour | Atomic local session save, restore controls/output, refresh device list |
| Fragile UI lifetime callbacks and permanent preset deletion | Raw `this` async status callback; alert had two owners; deletion used unlink | SafePointer status/save callback, single modal owner, custom preset deletion to Trash |

## Verification completed

- Baseline build failure reproduced before fixing outdated preset tests.
- Baseline expanded test crash reproduced.
- AddressSanitizer reproduced the 32-bit/64-bit parameter stack read.
- Expanded memory-checked tests pass, including concurrent edits, oversized chunks, nonfinite samples, original factory preset audio, Unicode custom preset save/load and recovery scenarios.
- Recovery policy tests use a fake audio backend and never change the Mac's actual default output.
- Release build and both CTest suites pass; the installed app's strict code-signature verification passes.
- BlackHole 2ch 0.7.1 was installed with user approval using the official signed, notarised package after verifying its published SHA-256. The user restarted macOS before live testing.
- Live stereo processing at 48 kHz through BlackHole 2ch to Explorer² USB DAC was verified. The user confirmed audible playback and a sound change when toggling processing power.
- Read-only CoreAudio snapshots confirmed BlackHole as default during processing and Explorer² USB DAC as default after both Stop and Quit. Repeated Start succeeded.
- The first start after installation briefly reported a device change; a subsequent live run and repeated Start/Stop worked. This initial transition needs further investigation if it recurs.

## 0.2.1 volume and equaliser correction

- The user's low-volume report was investigated separately from DSP gain. A read-only CoreAudio snapshot showed BlackHole at **-24 dB** and Explorer² USB DAC at **-34 dB**, introducing two attenuation stages. The [official BlackHole source](https://github.com/ExistentialAudio/BlackHole/blob/master/BlackHole/BlackHole.c) applies its master volume to captured samples.
- A continuous quiet 997 Hz tone at 48 kHz measured **-0.30 dB** with Dynamic Boost 0, **+9.70 dB** at 5, and **+11.30 dB** at 10. These are offline measurements for that signal, not Windows loudness-parity claims. Regression assertions cover those gains.
- The new volume bridge mirrors system-volume/mute edits to the selected physical output, corrects BlackHole attenuation before DSP, and follows direct hardware-volume edits back to the system control. It never raises physical volume when starting. CoreAudio reads/writes and recovery-file I/O remain on the message thread. Fixed-level outputs keep the existing single-volume path.
- A separate ownership journal restores BlackHole's original volume on Stop/Quit or the next launch after interruption. The user's chosen playback volume remains in effect. Failed restoration retains the journal; an unrelated later virtual-volume edit wins during recovery.
- Equaliser preset/session refresh now initialises slider values before first paint and repaints the complete parent curve after silent child updates. Previously only narrow slider areas were invalidated, leaving old flat segments between controls.
- Restoring a saved effects-off session now updates the power button too. Previously the button could display On while the effect panels and DSP were disabled. Preset changes also synchronise the button with DSP power.
- New tests cover volume/mute mirroring, attenuation cancellation through the actual DSP, Stop/crash recovery, failed restoration, and fixed-volume outputs. Both normal test suites pass.
- Live 0.2.1 check: system and DAC volume controls were observed in sync at 0.25 scalar. Quit restored Explorer² USB DAC as default and BlackHole's original 0.6875 scalar while retaining the user's DAC volume at 0.25. The final app restarted processing successfully with extra gain 0 dB and effects enabled.
- The equaliser was checked visually with nonzero test values and after closing/reopening through the session-restore path. The curve remained connected; test EQ edits were then returned to the user's previous flat settings. A screenshot is saved as `equaliser-fixed-0.2.1.png` alongside the project. Matching perceived Windows loudness still requires the user's listening comparison on the same material.
- After the final update, the user confirmed: "Volume and boost are better." Exact cross-platform loudness parity is not claimed.

## 0.2.2 audio activity display

- Added a passive stereo tap after processing. A fixed-size single-writer/single-reader queue drops visualisation frames when full; playback never waits for the interface. Samples are not modified, saved or uploaded.
- The interface performs a 2048-point FFT for each stereo channel and combines magnitudes into 48 logarithmic frequency bands. Separate stereo analysis prevents opposite-phase channels from falsely cancelling. Smooth attack/release and a short signal-presence hold avoid flicker; the display is an activity guide, not a calibrated measurement instrument.
- The spectrum sits behind the EQ controls. The EQ adjustment line and knobs remain stationary. The panel explicitly distinguishes Processing, Waiting for audio, Effects off and Stopped. Stop/bypass clears the spectrum; silence decays naturally. A hidden/minimised window skips spectrum analysis.
- Settings includes Show audio visualiser. Disabling it or enabling macOS Reduce Motion leaves the status label available with no spectrum animation.
- Tests passed for real tone frequency response at 8/44.1/48/96/192 kHz, silence/release, right-only and opposite-phase stereo audio, nonfinite input, reduced-motion behaviour, unchanged playback samples and a full visualisation queue. Both CTest suites passed; final build and installed code-signature verification passed.
- The installed Stopped display was checked visually. After the user chose and unmuted MacBook Air Speakers, live animation was verified at 48 kHz: successive screenshots showed changing spectrum bars from actual playback, while the EQ controls stayed fixed. Switching power off cleared the bars and displayed Effects off; Stop cleared the display and showed Stopped. A read-only CoreAudio snapshot confirmed Stop restored MacBook Air Speakers as default, retained the user's 0.75 speaker volume and restored BlackHole's previous 0.6875 volume. Processing was restarted successfully and left on. The live screenshot is saved alongside the project as `audio-activity-live-0.2.2.png`.

## 0.3.0 single power control and Mac appearance

- Renamed the visible app, bundle filename and native window/menu title to **FxSound for Mac**. The existing bundle identifier and application-support folder were deliberately retained to preserve audio permissions, settings, presets and interrupted-route recovery. The previous installed version was retained as a backup, and its Applications bundle was renamed in place before replacing its contents.
- The power icon is now the only processing control. Its illumination follows the actual engine state, and it turns grey when processing stops or startup fails. An old saved effects-power flag cannot illuminate an idle engine after reopening. Preset selection follows the existing engine state and does not start audio by itself.
- Removed the Setup website button, separate Start/Stop button, Mac audio row and routine bottom status paragraph. Actionable failures use a deferred error dialog after route cleanup. Normal power-off produces no dialog.
- Introduced adaptive neutral Light/Dark palettes, system fonts, smaller controls, rounded cards, quiet solid EQ guides, white slider thumbs and a geometric power symbol. Appearance changes refresh the theme and component colours. The activity label remains readable and accessible even while the adjustment controls are disabled.
- Both CTest suites and strict code-signature verification passed for the renamed 0.3.0 bundle. The application version now comes from build metadata instead of a stale hardcoded string.
- Live validation on MacBook Air Speakers at 48 kHz: the app opened with power grey/off; one click started the engine, illuminated power and displayed Waiting for audio; a quiet test signal changed the label to Processing and drove the spectrum; after the signal ended, the display returned to Waiting for audio. Power-off cleared the spectrum and a read-only CoreAudio snapshot confirmed normal speaker output and previous BlackHole volume were restored. Processing was restarted and left enabled. The Light Mode interface and absence of removed controls/text were checked visually. Dark Mode adaptation is implemented but was not exercised by changing the user's system appearance.
- Screenshots of the completed interface are saved alongside the project as `fxsound-for-mac-0.3.0.png` (test audio) and `fxsound-for-mac-0.3.0-final.png` (idle signal, engine enabled).

## Remaining hardware coverage

Device hot unplug, sleep/wake, latency and long listening sessions still require hardware validation. No login services have been installed. Bluetooth, HDMI, multichannel and bit-identical Windows output are not claimed. The current app is a stereo preview; the confirmed live path is limited to the USB DAC and format above.

The wrapper uses a nonblocking DSP gate; a contended control or preset operation temporarily passes through dry audio. It is not a full hard-real-time audit of all inherited DSP internals. Final limiting is retained from the base and extreme post-gain may colour sound.

## 0.3.1 appearance correction — 29 October 2026

Reproduced from the supplied dark-mode screenshot: the main surface retained its earlier light window colour while labels and control cards used the dark palette. The RGB-only palette values had a zero alpha byte; MainComponent painted its background without correcting that alpha. FxPalette now returns opaque ARGB for every role, MainComponent declares its opaque painting contract, and MainWindow updates its own stored background on look-and-feel changes. Added regression coverage for all palette entries in both appearances, primary text contrast against the main/control surfaces, and exact window colours. The private installed build passed both CTest suites and strict signature verification. Live dark and light transitions were visually checked with processing active through Explorer² USB DAC, Classic Processing, 0 dB; the original Auto setting was restored. No DSP or audio routing logic was changed.

## 0.3.2 toolbar layout — 29 October 2026

Rebalanced the header actions and aligned preset/output/volume groups across the available width. Fixed selector geometry avoids controls shifting when a preset/device name changes. Selector changes during initial construction are ignored until the component has its size. Existing automated checks passed. These layout changes are included in 0.4.0 and were subsequently reviewed live in the local development build.

## 2026-10-29 — numeric readouts and preset edit protection (0.4.0)

Added fixed enhancement and EQ readouts with quiet original-value references, and a compact toolbar. Added PresetSession tracking, guarded preset switching, explicit save-copy naming without overwrites, stable file-based selection after rescans, session baseline recovery, and safer import/delete selection handling. Save failures retain the working settings. Replaced problematic non-ASCII action labels with English ASCII text and gave dialog alternatives a quieter appearance.

Validation: preset edit/copy tests exercise original-file preservation, custom-name collisions, saved-value round trips, restored edits, failed loads/saves and independent master-volume/power state. Existing audio, routing, palette and bundle checks also run. The installed private build passed its test suite and strict signature verification. Live dark-mode UI showed fixed readouts and original references, the edited indicator, the switch prompt, and active audio visualisation. The neutral public 0.4.0 build also passes both CTest suites and strict local signature verification. It retains the public name, original artwork, attribution and licence notices.
