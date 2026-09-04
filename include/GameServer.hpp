#pragma once
#include <iostream>
#include <thread>
#include "util/utils.hpp"
#include "util/Messenger.hpp"
#include "util/SharedQueue.hpp"
#include "GameStateLogic.hpp"
#include "Timestepper.hpp"
#include "Framework/Globals.hpp"
#include <functional>

// could be simplified
struct ServerStartupState {
    uint32_t fixedUpdateRate;
    // these are both static, should never invalidate
    Subject<MouseEvent>* mouseSubject = nullptr;
    Subject<KeyEvent>* keySubject = nullptr;

    // for updating game stats externally
    float* frameratePtr;
};
class GameServer
{
    void run(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ServerStartupState& p_state);

    Timestepper ts;
    fpsGauge tickGauge;

    // thread management
    std::thread m_serverThread;
    InputHandler m_serverInp;
    ServerStartupState m_settings;

public:
    // NOTE: game states must be initialized at some point before running the server. This is done by fetching the state manager beforehand.
    GameStateManager& stateManager = GameStateManager::Get();
    // server-side input handler
    GameServer();
    void start(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ServerStartupState& params);
    void stop();
    bool joinable();
    InputHandler& get_server_input();
    std::atomic_bool serverStopping = false;
};

