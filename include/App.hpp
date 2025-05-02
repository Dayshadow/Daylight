#pragma once
#include "GameClient.hpp"
#include "GameServer.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
class App {

	GameClient client;
	GameServer localServer;

	SharedQueue<std::exception_ptr> m_exceptionQueue;

	bool appActive = true;

	void startClient();
	void startServer();
	void pollEvents();

public:
	App();

	void run();
};