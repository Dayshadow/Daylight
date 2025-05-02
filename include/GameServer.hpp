#pragma once
#include <iostream>
#include <thread>
#include "util/utils.hpp"
#include "util/Messenger.hpp"
#include "util/SharedQueue.hpp"
#include "GameStates.hpp"
#include "Game States/ServerTestingState.hpp"
#include "Game States/ServerMenuState.hpp"
#include "Game States/TemplateState.hpp"
#include "Timestepper.hpp"
#include "Globals.hpp"

class GameServer
{
	void run(SharedQueue<std::exception_ptr>& p_exceptionQueue);

	Timestepper ts {UPDATE_RATE_FPS};
	
	fpsGauge tickGauge;

	TemplateState State_None;
	ServerMenuState State_Menu;
	ServerTestingState State_Testing;

	// thread management
	std::thread serverThread;

public:
	GameStateManager& stateManager = GameStateManager::Get();
	GameServer();
	void start(SharedQueue<std::exception_ptr>& p_exceptionQueue);
	void stop();
	std::atomic_bool serverStopping = false;
};

