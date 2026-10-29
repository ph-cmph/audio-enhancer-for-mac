# Publication review — 28 October 2026

This records the preparation of an independent open-source derivative. It is not a legal clearance or a guarantee against complaints.

## Verified source licences

- FxSound: AGPL-3.0, with source headers permitting version 3 or later. Original copyrights and the full root licence are retained. https://github.com/fxsound2/fxsound-app/blob/main/LICENSE
- Primary macOS base: okku007/fxsound-mac at `3fd150984c1660e2f1f885c55c74ea256266d96e`, which identifies AGPL-3.0. https://github.com/okku007/fxsound-mac
- Additional factory preset payloads: harshitethic/fxsound-linux-macos at `b3fb11d8f628234baa0b8694df34f469f03dc725`, which identifies AGPL-3.0. https://github.com/harshitethic/fxsound-linux-macos
- JUCE is pinned to 7.0.12, commit `4f43011b96eb0636104cb3e433894cda98243626`. Its module headers offer GPL v3 as an alternative to commercial licensing; permissive component notices remain intact. We use that open-source route, retain the complete dependency source, and provide the full GPL text. https://github.com/juce-framework/JUCE/blob/7.0.12/LICENSE.md
- BlackHole is a separately installed dependency. Its installer, driver and branding assets are not redistributed here. https://github.com/ExistentialAudio/BlackHole

## Distribution changes

- Exclude unused Gilroy font files: no separate redistribution permission was supplied with those files.
- Exclude original FxSound logo/icon artwork and the unused Windows launcher resources; replace application artwork with original geometric artwork.
- Retain all upstream copyright notices and licence files. Mark modified source and document changes and their date.
- Supply complete corresponding source, dependencies and build scripts with binary releases under their respective licences. Do not replace the copyleft licence with MIT or a proprietary licence.
- Clearly identify this as an independent derivative, with no claim of endorsement by FxSound LLC, Apple, JUCE or Existential Audio.
- State the preview's actual platform/test limitations and local signing status. Include no-warranty notices.
- Exclude local preferences, recovery journals, credentials, test recordings, compiled working folders and machine-specific logs.

## Branding decision

The maintainer chose the neutral name **Audio Enhancer for Mac** with original artwork. FxSound is retained for factual attribution to the upstream project, not as this product's brand. No upstream branding permission is claimed. The public repository is intended to be `ph-cmph/audio-enhancer-for-mac`.

Only a qualified legal professional can provide a legal opinion on all applicable copyright, trademark, patent and jurisdiction-specific issues. This review is a practical compliance record, not such an opinion.

## Legacy upstream notices

75 inherited source files contain old Power Technology confidentiality wording. At official FxSound commit `4383180e02ff435d191f71220e8da41f9fa06951`, 73 are byte-identical to the publicly released AGPL repository. The other two (`audiopassthru/src/sndDevices/u_sndDevices.h` and `dsp/ptutil/include/File.h`) retain those same public headers with inherited function-naming and macOS compatibility changes. Original notices have not been removed. See `UPSTREAM_NOTICE_PROVENANCE.json` for exact public paths. Public availability and the repository licence are evidence of provenance, not a guarantee resolving every historical notice.

## 0.4.0 update — 29 October 2026

This update changes interface appearance, control readouts and preset editing. It introduces no new third-party dependencies or branding assets. Existing licences, attribution and inherited source notices remain intact. Release packages continue to pair the application with complete corresponding source, including pinned JUCE source and build scripts. Local settings, recordings, recovery journals, credentials and build working directories are excluded. The release remains an Apple Silicon preview, locally ad-hoc signed without Developer ID signing or notarisation.
