// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
// Read-only hardware diagnostics, independent Mac preview, 2026-10-28.
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdio>
#include <vector>

static void name(AudioObjectID id, AudioObjectPropertySelector selector, char* text, size_t capacity)
{
    AudioObjectPropertyAddress a { selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    CFStringRef s = nullptr; UInt32 size = sizeof(s);
    text[0] = 0;
    if (AudioObjectGetPropertyData(id, &a, 0, nullptr, &size, &s) == noErr && s)
    { CFStringGetCString(s, text, capacity, kCFStringEncodingUTF8); CFRelease(s); }
}
int main()
{
    AudioObjectPropertyAddress a { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &a, 0, nullptr, &size) != noErr || !size) return 1;
    std::vector<AudioObjectID> devices(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &a, 0, nullptr, &size, devices.data()) != noErr) return 2;
    AudioObjectID current = 0; UInt32 ds = sizeof(current);
    a.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &a, 0, nullptr, &ds, &current);
    for (auto id : devices)
    {
        char label[1024], uid[1024];
        name(id, kAudioObjectPropertyName, label, sizeof(label));
        name(id, kAudioDevicePropertyDeviceUID, uid, sizeof(uid));
        Float64 rate = 0; UInt32 rs = sizeof(rate);
        a.mSelector = kAudioDevicePropertyNominalSampleRate;
        AudioObjectGetPropertyData(id, &a, 0, nullptr, &rs, &rate);
        std::printf("%s%s | %.0f Hz | %s\n", id == current ? "DEFAULT: " : "DEVICE: ", label, rate, uid);
        for (UInt32 element = 0; element <= 2; ++element)
        {
            AudioObjectPropertyAddress volume { kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, element };
            Float32 scalar = 0; UInt32 vs = sizeof(scalar);
            if (AudioObjectGetPropertyData(id, &volume, 0, nullptr, &vs, &scalar) == noErr)
                std::printf("  output volume %u: %.4f\n", element, scalar);
            volume.mSelector = kAudioDevicePropertyVolumeDecibels; vs = sizeof(scalar);
            if (AudioObjectGetPropertyData(id, &volume, 0, nullptr, &vs, &scalar) == noErr)
                std::printf("  output dB %u: %.2f\n", element, scalar);
            volume.mSelector = kAudioDevicePropertyMute; UInt32 mute = 0; vs = sizeof(mute);
            if (AudioObjectGetPropertyData(id, &volume, 0, nullptr, &vs, &mute) == noErr)
                std::printf("  output mute %u: %u\n", element, mute);
        }
    }
}
