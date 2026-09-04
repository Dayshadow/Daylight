#pragma once
#include <string>
#include <vector>
#include <AL/alc.h>

extern struct ClientAudioState {
    std::string defaultAudioDeviceName;
    std::vector<std::string> audioDeviceNames;
    ALCdevice* currentAudioDevice;
    ALCcontext* alctx;
} g_ClientAudioState;