#pragma once
#include <iostream>
#include <thread>
#include "Framework/Window/GameWindow.hpp"
#include "Framework/Graphics/GUI_Experimental/GUI.hpp"
#include "Framework/Globals.hpp"
#include "util/ext/AL/alc.h"
#include "util/SharedQueue.hpp"
#include "GameStateLogic.hpp"
#include <functional>

// this should all be standardized eventually, especially the audio handling
struct ClientStartupState {
    // these are both static, should never invalidate
    Subject<MouseEvent>* mouseSubject = nullptr;
    Subject<KeyEvent>* keySubject = nullptr;

    // for updating game stats externally
    float* frameratePtr = nullptr;
    uint32_t* refreshRatePtr = nullptr;
};

class GameClient
{
    void run(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ClientStartupState& p_state);
    void resizeWindow(uint32_t p_w, uint32_t p_h);
    // core systems
    GameWindow m_window{ "Game" };

    // utility/testing
    fpsGauge renderFPSGauge;

    // thread management
    std::thread clientThread;
    GameStateManager& stateManager = GameStateManager::Get();
    GUI& m_gui = GUI::Get();

    void clean_up();

    // client-side input handler
    InputHandler m_clientInp;
    ImGuiContext* m_imctx = nullptr;
    std::atomic_bool m_clientStopping = false;
    ClientStartupState m_settings;

public:
    GameClient();
    ~GameClient();
    void imgui_init() const;
    void imgui_update();
    void start(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ClientStartupState& p_params);
    void stop();
    bool joinable();

    inline GameWindow& get_window() { return m_window; };
    inline uint32_t get_window_id() { return m_window.getWindowID(); }
    InputHandler& get_client_input();

    // for access by the main thread
    uint32_t newWidth = 0;
    uint32_t newHeight = 0;

    std::atomic<bool> flagResize = false;

};

