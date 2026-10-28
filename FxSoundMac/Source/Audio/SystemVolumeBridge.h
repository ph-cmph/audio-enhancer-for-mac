// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

// Independent Mac reliability changes, 2026-10-28, AGPL-3.0-or-later.
struct DeviceVolume
{
    float scalar = 1.0f;
    bool muted = false;
    float decibels = 0.0f;
    bool hasDecibels = false;
};
struct VolumeBridgeBackend
{
    virtual ~VolumeBridgeBackend() = default;
    virtual bool readVolume(const juce::String&, DeviceVolume&) const = 0;
    virtual bool writeVolume(const juce::String&, const DeviceVolume&) = 0;
};

// BlackHole applies its software volume to captured samples. A physical output
// has a separate volume. Mirror the controls, undo only BlackHole's attenuation
// before DSP, and leave the physical output responsible for listening volume.
// No CoreAudio calls, file operations or locks occur on the audio thread.
class SystemVolumeBridge
{
public:
    SystemVolumeBridge(VolumeBridgeBackend& b, juce::File f) : backend(b), journal(std::move(f)) {}
    bool recover()
    {
        if (!journal.existsAsFile()) return true;
        const auto data = juce::JSON::parse(journal);
        if (!data.isObject()) return false;
        capture = data["capture"].toString();
        before = { (float)data["beforeScalar"], (bool)data["beforeMuted"] };
        lastCapture = { (float)data["ownedScalar"], (bool)data["ownedMuted"] };
        if (capture.isEmpty() || !valid(before) || !valid(lastCapture)) return false;
        DeviceVolume current;
        if (!backend.readVolume(capture, current)) return false;
        // A later edit from another app wins; never overwrite a different value.
        if (same(current, lastCapture) && !backend.writeVolume(capture, before)) return false;
        return journal.deleteFile();
    }
    bool begin(const juce::String& captureUID, const juce::String& playbackUID)
    {
        if (active || !recover()) return false;
        capture = captureUID; playback = playbackUID;
        DeviceVolume physical;
        // Fixed-level outputs have no second volume stage to synchronise.
        if (!backend.readVolume(playback, physical)) return true;
        if (!backend.readVolume(capture, before) || !before.hasDecibels || !valid(before) || !valid(physical)) return false;
        lastPhysical = physical;
        lastCapture = physical;
        if (!saveJournal()) return false;
        active = true;
        if (!backend.writeVolume(capture, physical) || !refreshCompensation()) { end(); return false; }
        return true;
    }
    bool tick()
    {
        if (!active) return true;
        DeviceVolume virtualNow, physicalNow;
        if (!backend.readVolume(capture, virtualNow) || !backend.readVolume(playback, physicalNow)
            || !valid(virtualNow) || !valid(physicalNow)) return false;
        if (!same(virtualNow, lastCapture))
        {
            // Keyboard / Control Centre edits on the default virtual output.
            if (!backend.writeVolume(playback, virtualNow)) return false;
            if (!backend.readVolume(playback, lastPhysical)) return false;
            lastCapture = virtualNow;
            if (!saveJournal()) return false;
        }
        else if (!same(physicalNow, lastPhysical))
        {
            // Hardware knobs and other direct edits stay visible in macOS.
            lastPhysical = physicalNow;
            lastCapture = physicalNow;
            if (!saveJournal() || !backend.writeVolume(capture, physicalNow)) return false;
        }
        return refreshCompensation();
    }
    bool end()
    {
        compensation = 1.0f;
        if (!active) return recover();
        DeviceVolume current;
        bool ok = backend.readVolume(capture, current);
        if (ok && same(current, lastCapture)) ok = backend.writeVolume(capture, before);
        active = false;
        if (ok) ok = journal.deleteFile();
        return ok;
    }
    float inputCompensation() const { return compensation; }
    static float compensationFor(const DeviceVolume& state)
    {
        if (!valid(state) || !state.hasDecibels || !std::isfinite(state.decibels)) return 1.0f;
        if (state.muted || state.scalar <= 0.0f) return 0.0f;
        return std::pow(10.0f, juce::jlimit(0.0f, 64.0f, -state.decibels) / 20.0f);
    }
private:
    static bool valid(const DeviceVolume& v) { return std::isfinite(v.scalar) && v.scalar >= 0 && v.scalar <= 1; }
    static bool same(const DeviceVolume& a, const DeviceVolume& b)
    { return std::abs(a.scalar - b.scalar) < 0.0001f && a.muted == b.muted; }
    bool refreshCompensation()
    {
        DeviceVolume current;
        if (!backend.readVolume(capture, current) || !current.hasDecibels || !valid(current)) return false;
        compensation = compensationFor(current);
        return true;
    }
    bool saveJournal()
    {
        juce::DynamicObject::Ptr data = new juce::DynamicObject();
        data->setProperty("capture", capture);
        data->setProperty("beforeScalar", before.scalar); data->setProperty("beforeMuted", before.muted);
        data->setProperty("ownedScalar", lastCapture.scalar); data->setProperty("ownedMuted", lastCapture.muted);
        return journal.getParentDirectory().createDirectory().wasOk()
            && journal.replaceWithText(juce::JSON::toString(juce::var(data)));
    }
    VolumeBridgeBackend& backend;
    juce::File journal;
    juce::String capture, playback;
    DeviceVolume before, lastCapture, lastPhysical;
    bool active = false;
    float compensation = 1.0f;
};

class CoreAudioVolumeBackend final : public VolumeBridgeBackend
{
public:
    bool readVolume(const juce::String&, DeviceVolume&) const override;
    bool writeVolume(const juce::String&, const DeviceVolume&) override;
};
