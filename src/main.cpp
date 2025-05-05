#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "SDL.h"
#include "Framework/Log.hpp"
#include "App.hpp"

#define STB_IMAGE_IMPLEMENTATION 

// ENTRYPOINT, most of this is just for establishing a main loop, client, and server threads. This project only really uses the client thread.
// This code adapts from a game framework I made previously
int main(int argc, char* argv[]) {

#ifdef _DEBUG
	LOG("Debug mode active!");
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // memory leak detection
	flag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(flag);
#endif

	SDL_Init(SDL_INIT_VIDEO);

	App app;
	app.run();

	return 0;
}