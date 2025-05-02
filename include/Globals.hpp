#pragma once
#include "util/SubjectObserver.hpp"
#include "Framework/Globals.hpp"
#include "util/ext/AL/alc.h"

#define UPDATE_RATE_FPS 60

namespace Globals {
	extern Subject<MouseEvent> mouseSubject;
	extern Subject<KeyEvent> keySubject;
	extern ALCcontext* alctx;
	extern std::vector<std::string> audioDeviceNames;
	extern std::string defaultDeviceName;

	extern ALCdevice* currentAudioDevice;
	// used for functions to detect changes in audio device, simplest way
	extern size_t audioStateChangeCount;

	extern float updateFPS;
	extern float drawFPS;
	extern int refresh_rate;
}