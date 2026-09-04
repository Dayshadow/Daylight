#include "GameServer.hpp"

GameServer::GameServer() 
{
}

void GameServer::start(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ServerStartupState& p_state)
{
    m_serverThread = std::thread(&GameServer::run, this, std::ref(p_exceptionQueue), p_state);
}

void GameServer::stop() 
{
    serverStopping = true;
    m_serverThread.join();
}
bool GameServer::joinable()
{
    return m_serverThread.joinable();
}
InputHandler& GameServer::get_server_input()
{
    return m_serverInp;
}
void GameServer::run(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ServerStartupState& p_state) 
{
    m_settings = p_state;
    ts = Timestepper(m_settings.fixedUpdateRate);
    Observer<MouseEvent> m_mouseObserver{ *m_settings.mouseSubject };
    static Observer<KeyEvent> keyObserver{ *m_settings.keySubject };

    try {

        while (true) {

            ts.process_frame_start();
            while (ts.accumulator_full()) {
                ts.drain();

                while (auto keyOpt = keyObserver.observe()) {
                    if (keyOpt) {
                        KeyEvent key = keyOpt.value();
                        if (key.wasDown)
                            m_serverInp.processKeyDown(key.keyCode);
                        else
                            m_serverInp.processKeyUp(key.keyCode);
                    }
                }
                stateManager.server_update();

                tickGauge.update(0.98f);

                if (m_settings.frameratePtr)
                    *m_settings.frameratePtr = (float)(1.0 / tickGauge.getFrametimeAverage());

                ts.process_frame_start();
            }
            // prevent it from overworking
            std::this_thread::sleep_for(std::chrono::microseconds(1000000 / (m_settings.fixedUpdateRate * 2)));
            ;
            if (stateManager.server_should_close()) serverStopping = true;

            if (serverStopping) break;
        }
    }
    catch (std::exception& ex) {
        ERROR_LOG("Exception in " << __FILE__ << " at " << __LINE__ << ": " << ex.what());
        p_exceptionQueue.push(std::current_exception());
    }
}