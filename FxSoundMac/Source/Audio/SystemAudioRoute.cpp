// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "AudioRouteLease.h"
#include "SystemVolumeBridge.h"
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>

namespace {
juce::String stringProperty(AudioObjectID object, AudioObjectPropertySelector selector)
{
    AudioObjectPropertyAddress address { selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    CFStringRef value = nullptr;
    UInt32 size = sizeof(value);
    if (AudioObjectGetPropertyData(object, &address, 0, nullptr, &size, &value) != noErr || !value) return {};
    char text[4096] {};
    const bool ok = CFStringGetCString(value, text, sizeof(text), kCFStringEncodingUTF8);
    CFRelease(value);
    return ok ? juce::String::fromUTF8(text) : juce::String{};
}
std::vector<AudioObjectID> devices()
{
    AudioObjectPropertyAddress address { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size) != noErr) return {};
    std::vector<AudioObjectID> result(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, result.data()) != noErr) return {};
    result.resize(size / sizeof(AudioObjectID));
    return result;
}
AudioObjectID deviceForUID(const juce::String& uid)
{
    if (!uid.isEmpty()) for (auto device : devices())
        if (stringProperty(device, kAudioDevicePropertyDeviceUID) == uid) return device;
    return kAudioObjectUnknown;
}
}
juce::String CoreAudioRoutingBackend::defaultOutput() const
{
    AudioObjectPropertyAddress address { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    AudioObjectID device = kAudioObjectUnknown;
    UInt32 size = sizeof(device);
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, &device) != noErr) return {};
    return stringProperty(device, kAudioDevicePropertyDeviceUID);
}
bool CoreAudioRoutingBackend::available(const juce::String& uid) const { return deviceForUID(uid) != kAudioObjectUnknown; }
bool CoreAudioRoutingBackend::setDefaultOutput(const juce::String& uid)
{
    AudioObjectID device = deviceForUID(uid);
    if (device == kAudioObjectUnknown) return false;
    AudioObjectPropertyAddress address { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    return AudioObjectSetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, sizeof(device), &device) == noErr;
}
juce::String CoreAudioRoutingBackend::uidForName(const juce::String& name) const
{
    for (auto device : devices()) if (stringProperty(device, kAudioObjectPropertyName) == name)
        return stringProperty(device, kAudioDevicePropertyDeviceUID);
    return {};
}
juce::String CoreAudioRoutingBackend::fallbackOutput() const
{
    juce::String fallback;
    for (auto device : devices())
    {
        AudioObjectPropertyAddress outputs { kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain };
        UInt32 bytes = 0;
        if (AudioObjectGetPropertyDataSize(device, &outputs, 0, nullptr, &bytes) != noErr || bytes == 0) continue;
        AudioObjectPropertyAddress transport { kAudioDevicePropertyTransportType, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
        UInt32 kind = 0, size = sizeof(kind);
        if (AudioObjectGetPropertyData(device, &transport, 0, nullptr, &size, &kind) != noErr) continue;
        if (kind == kAudioDeviceTransportTypeVirtual || kind == kAudioDeviceTransportTypeAggregate) continue;
        const auto uid = stringProperty(device, kAudioDevicePropertyDeviceUID);
        if (kind == kAudioDeviceTransportTypeBuiltIn) return uid;
        if (fallback.isEmpty()) fallback = uid;
    }
    return fallback;
}
bool CoreAudioRoutingBackend::isPhysicalOutput(const juce::String& name) const
{
    const auto device = deviceForUID(uidForName(name));
    if (device == kAudioObjectUnknown) return false;
    AudioObjectPropertyAddress output { kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(device, &output, 0, nullptr, &size) != noErr || size == 0) return false;
    AudioObjectPropertyAddress transport { kAudioDevicePropertyTransportType, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    UInt32 kind = 0; size = sizeof(kind);
    return AudioObjectGetPropertyData(device, &transport, 0, nullptr, &size, &kind) == noErr
        && kind != kAudioDeviceTransportTypeVirtual && kind != kAudioDeviceTransportTypeAggregate;
}

namespace {
template <typename T> bool readOutputProperty(AudioObjectID device, AudioObjectPropertySelector selector, UInt32 channel, T& value)
{
    AudioObjectPropertyAddress a { selector, kAudioDevicePropertyScopeOutput, channel };
    UInt32 size = sizeof(value);
    return AudioObjectGetPropertyData(device, &a, 0, nullptr, &size, &value) == noErr;
}
template <typename T> bool writeOutputProperty(AudioObjectID device, AudioObjectPropertySelector selector, UInt32 channel, T value)
{
    AudioObjectPropertyAddress a { selector, kAudioDevicePropertyScopeOutput, channel };
    return AudioObjectSetPropertyData(device, &a, 0, nullptr, sizeof(value), &value) == noErr;
}
std::vector<UInt32> volumeChannels(AudioObjectID device)
{
    for (UInt32 channel : {0u, 1u, 2u})
    {
        AudioObjectPropertyAddress a { kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, channel };
        Boolean writable = false;
        if (AudioObjectIsPropertySettable(device, &a, &writable) == noErr && writable)
        {
            if (channel == 0) return {0};
            std::vector<UInt32> channels {channel};
            a.mElement = 2;
            if (channel == 1 && AudioObjectIsPropertySettable(device, &a, &writable) == noErr && writable) channels.push_back(2);
            return channels;
        }
    }
    return {};
}
}
bool CoreAudioVolumeBackend::readVolume(const juce::String& uid, DeviceVolume& result) const
{
    const auto device = deviceForUID(uid);
    const auto channels = volumeChannels(device);
    if (channels.empty()) return false;
    float sum = 0; bool muted = false;
    for (auto channel : channels)
    {
        Float32 scalar = 0; UInt32 mute = 0;
        if (!readOutputProperty(device, kAudioDevicePropertyVolumeScalar, channel, scalar)) return false;
        sum += scalar;
        if (readOutputProperty(device, kAudioDevicePropertyMute, channel, mute)) muted = muted || mute != 0;
    }
    UInt32 masterMute = 0;
    if (readOutputProperty(device, kAudioDevicePropertyMute, 0, masterMute)) muted = masterMute != 0;
    result.scalar = sum / (float)channels.size(); result.muted = muted;
    result.hasDecibels = channels.size() == 1 && readOutputProperty(device, kAudioDevicePropertyVolumeDecibels, channels.front(), result.decibels);
    return true;
}
bool CoreAudioVolumeBackend::writeVolume(const juce::String& uid, const DeviceVolume& target)
{
    if (!std::isfinite(target.scalar) || target.scalar < 0 || target.scalar > 1) return false;
    const auto device = deviceForUID(uid);
    const auto channels = volumeChannels(device);
    DeviceVolume previous;
    if (channels.empty() || !readVolume(uid, previous)) return false;
    // Equal scalar deltas preserve existing left/right balance where possible.
    for (auto channel : channels)
    {
        Float32 scalar = 0;
        if (!readOutputProperty(device, kAudioDevicePropertyVolumeScalar, channel, scalar)
            || !writeOutputProperty(device, kAudioDevicePropertyVolumeScalar, channel,
                  juce::jlimit(0.0f, 1.0f, scalar + target.scalar - previous.scalar))) return false;
        AudioObjectPropertyAddress mute { kAudioDevicePropertyMute, kAudioDevicePropertyScopeOutput, channel };
        Boolean writable = false;
        if (AudioObjectIsPropertySettable(device, &mute, &writable) == noErr && writable
            && !writeOutputProperty(device, kAudioDevicePropertyMute, channel, (UInt32)target.muted)) return false;
    }
    AudioObjectPropertyAddress mute { kAudioDevicePropertyMute, kAudioDevicePropertyScopeOutput, 0 };
    Boolean writable = false;
    if (AudioObjectIsPropertySettable(device, &mute, &writable) == noErr && writable
        && !writeOutputProperty(device, kAudioDevicePropertyMute, 0, (UInt32)target.muted)) return false;
    return true;
}
