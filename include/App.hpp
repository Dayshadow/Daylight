#pragma once
#include "Client.hpp"
#include "Server.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
class App {

	Client client;
	Server localServer;

	SharedQueue<std::exception_ptr> m_exceptionQueue;

	bool appActive = true;

	void startClient();
	void startServer();
	void pollEvents();

public:
	App();

	void run();
};