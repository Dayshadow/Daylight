#include "Globals.hpp"

namespace Globals {
	Subject<MouseEvent> mouseSubject; // keeping these global fixes issues
	Subject<KeyEvent> keySubject;
	ALCcontext* alctx;
	std::vector<std::string> audioDeviceNames;
	std::string defaultDeviceName;

	ALCdevice* currentAudioDevice;
	// used for functions to detect changes in audio device, simplest way
	size_t audioStateChangeCount = 0;

	float updateFPS = 0.f;
	float drawFPS = 0.f;
	int refresh_rate = 0;
}
