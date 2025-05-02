#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "SDL.h"
#include "Framework/Log.hpp"
#include "App.hpp"
// BEGIN EPIC SPAGHETTI CODE
#define STB_IMAGE_IMPLEMENTATION 

int main(int argc, char* argv[]) {

#ifdef _DEBUG
	LOG("Debug mode active!");
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // memory leak detection
	flag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(flag);
#endif

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

	App app;
	app.run();

	LOG("hello from the application");

	return 0;
}
// END EPIC SPAGHETTI CODE