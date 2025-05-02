#pragma once
#include <iostream>
#include <thread>
#include "Framework/Window/GameWindow.hpp"
#include "Framework/Graphics/GUI_Experimental/GUI.hpp"
#include "Framework/Globals.hpp"
#include "GameStates.hpp"
#include "Game States/TemplateState.hpp"
#include "Game States/ClientMenuState.hpp"
#include "Game States/ClientTestingState.hpp"
#include "Globals.hpp"
#include "util/SharedQueue.hpp"

class GameClient
{
	void run(SharedQueue<std::exception_ptr>& p_exceptionQueue);
	void resizeWindow(uint32_t p_w, uint32_t p_h);
	// core systems
public: GameWindow window{ "DeltaSkill" };
private: 

	   GUI& gui = GUI::Get();

	   // utility/testing
	   fpsGauge renderFPSGauge;

	   // thread management
	   std::thread clientThread;
	   GameStateManager& stateManager = GameStateManager::Get();
	   TemplateState State_None;
	   ClientMenuState State_Menu;
	   ClientTestingState State_Testing{ window };

	   void cleanUp();

public:
	GameClient();
	~GameClient();
	void start(SharedQueue<std::exception_ptr>& p_exceptionQueue);
	void stop();
	uint32_t getWindowID() { return window.getWindowID(); }

	// for access by the main thread
	uint32_t newWidth = 0;
	uint32_t newHeight = 0;
	InputHandler inp;
	ImGuiContext* imctx = nullptr;
	std::mutex inputReadWriteMutex;
	std::atomic<bool> flagResize = false;
	std::atomic_bool clientStopping = false;

};

