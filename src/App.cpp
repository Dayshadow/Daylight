#include "App.hpp"

App::App()
{
}

void App::startClient()
{
    client.start(m_exceptionQueue);
}

void App::startServer()
{
    localServer.start(m_exceptionQueue);
}

void App::pollEvents()
{
    static SDL_Event event;
    static SharedQueue<MouseEvent>& s_mouseQueue = SharedQueue<MouseEvent>::Get();
    static SharedQueue<KeyEvent>& s_keyMessenger = SharedQueue<KeyEvent>::Get();

    // workaround
    static SharedQueue<SDL_Event>& s_SDLEventMessenger = SharedQueue<SDL_Event>::Get();
    while (SDL_PollEvent(&event)) {
        s_SDLEventMessenger.push((SDL_Event)event);
        switch (event.type) {
        case SDL_WINDOWEVENT:
            if (!(event.window.event == SDL_WINDOWEVENT_RESIZED)) break;
            LOG("Window " << event.window.windowID << " resized to " << event.window.data1 << "x" << event.window.data2);
            if (event.window.windowID != client.getWindowID()) break;
            client.newWidth = event.window.data1;
            client.newHeight = event.window.data2;
            client.flagResize = true;
            break;
        case SDL_QUIT:
            appActive = false;
            break;
        case SDL_KEYDOWN:
            //client.inp.processKeyDown(event.key.keysym.sym); // i'll leave this to rot for now
            s_keyMessenger.push(KeyEvent(true, event.key.keysym.sym, true));
            break;
        case SDL_KEYUP:
            //client.inp.processKeyUp(event.key.keysym.sym);
            s_keyMessenger.push(KeyEvent(false, event.key.keysym.sym, true));
            if (event.key.keysym.sym == SDLK_ESCAPE) appActive = false;
            break;
        case SDL_MOUSEMOTION:
            if (event.window.windowID != client.getWindowID()) break;
            s_mouseQueue.push(MouseEvent{
                    (float)event.motion.x / client.window.width,
                    (float)event.motion.y / client.window.height,
                    (float)event.motion.x,
                    (float)event.motion.y,
                    false,
                    false,
                    true,
                    false,
                    0,
                    event.motion.state
                });
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.window.windowID != client.getWindowID()) break;
            s_mouseQueue.push(MouseEvent{
                    (float)event.motion.x / client.window.width,
                    (float)event.motion.y / client.window.height,
                    (float)event.motion.x,
                    (float)event.motion.y,
                    true,
                    false,
                    false,
                    false,
                    event.button.button,
                    0
                });
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.window.windowID != client.getWindowID()) break;
            s_mouseQueue.push(MouseEvent{
                    (float)event.motion.x / client.window.width,
                    (float)event.motion.y / client.window.height,
                    (float)event.motion.x,
                    (float)event.motion.y,
                    false,
                    true,
                    false,
                    false,
                    event.button.button,
                    0
                });
            break;
        case SDL_MOUSEWHEEL:
            if (event.window.windowID != client.getWindowID()) break;
            s_mouseQueue.push(MouseEvent{
                    (float)event.wheel.x,
                    (float)event.wheel.y,
                    0.0f,
                    0.0f,
                    false,
                    false,
                    false,
                    true,
                    0,
                    event.wheel.which
                });
        }
    }
}

void App::run()
{
    startClient();
    startServer();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        pollEvents();
        // handle thread exceptions
        if (auto exp = m_exceptionQueue.tryPop()) {
            try {
                std::rethrow_exception(exp.value());
            }
            catch (std::exception& e) {
                ERROR_LOG("Main thread caught exception " << e.what());
                localServer.stateManager.close();
                client.stop();
                localServer.stop();
            }
            break;
        }

        if (!appActive) break;
    }
    localServer.stateManager.close();
    client.stop();
    localServer.stop();
}
