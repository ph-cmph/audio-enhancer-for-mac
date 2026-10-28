// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_core/juce_core.h>

// Independent macOS reliability changes, 2026-10-28, AGPL-3.0-or-later.
struct AudioRoutingBackend
{
    virtual ~AudioRoutingBackend() = default;
    virtual juce::String defaultOutput() const = 0;
    virtual bool available(const juce::String& uid) const = 0;
    virtual bool setDefaultOutput(const juce::String& uid) = 0;
    virtual juce::String fallbackOutput() const { return {}; }
};

// Control-thread only. Restore only a route this instance owns: a user's later
// output change always wins. A small journal permits recovery after a crash.
class AudioRouteLease
{
public:
    AudioRouteLease(AudioRoutingBackend& b, juce::File journalFile) : backend(b), journal(std::move(journalFile)) {}
    bool begin(const juce::String& capture, const juce::String& playback)
    {
        if (active || capture.isEmpty() || playback.isEmpty() || capture == playback
            || !backend.available(capture) || !backend.available(playback)) return false;
        captureUID = capture;
        playbackUID = playback;
        previousUID = backend.defaultOutput();
        if (previousUID.isEmpty()) return false;
        // Recover a previously stranded virtual route to the chosen speakers.
        if (previousUID == captureUID) previousUID = playbackUID;
        juce::DynamicObject::Ptr data = new juce::DynamicObject();
        data->setProperty("capture", captureUID);
        data->setProperty("previous", previousUID);
        data->setProperty("playback", playbackUID);
        if (journal.getParentDirectory().createDirectory().failed()
            || !journal.replaceWithText(juce::JSON::toString(juce::var(data)))) return false;
        if (!backend.setDefaultOutput(captureUID)) { journal.deleteFile(); return false; }
        active = true;
        return true;
    }
    bool end()
    {
        if (!active) return true;
        if (backend.defaultOutput() == captureUID)
        {
            const auto target = backend.available(previousUID) ? previousUID
                              : backend.available(playbackUID) ? playbackUID : backend.fallbackOutput();
            if (!backend.available(target) || !backend.setDefaultOutput(target)) return false;
        }
        active = false;
        journal.deleteFile();
        return true;
    }
    bool recover()
    {
        if (!journal.existsAsFile()) return true;
        const auto data = juce::JSON::parse(journal);
        if (!data.isObject()) return false;
        captureUID = data["capture"].toString();
        previousUID = data["previous"].toString();
        playbackUID = data["playback"].toString();
        if (captureUID.isEmpty() || previousUID.isEmpty() || playbackUID.isEmpty()
            || previousUID == captureUID || playbackUID == captureUID) return false;
        active = true;
        return end();
    }
    bool isActive() const { return active; }
private:
    AudioRoutingBackend& backend;
    juce::File journal;
    juce::String captureUID, previousUID, playbackUID;
    bool active = false;
};

class CoreAudioRoutingBackend final : public AudioRoutingBackend
{
public:
    juce::String defaultOutput() const override;
    bool available(const juce::String& uid) const override;
    bool setDefaultOutput(const juce::String& uid) override;
    juce::String uidForName(const juce::String& name) const;
    bool isPhysicalOutput(const juce::String& name) const;
    juce::String fallbackOutput() const override;
};
