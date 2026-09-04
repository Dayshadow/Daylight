#include "GameClient.hpp"
#include "Framework/Graphics/Sprite.hpp"
#include "Framework/Graphics/GenericShaders.hpp"
#include "Framework/Audio/wav.hpp"
#include "Framework/Audio/AudioImmediate.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIDragBar.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIContainer.hpp"
#include <print>
#include <format>

namespace ClientAudioState {
    std::string defaultAudioDeviceName;
    std::vector<std::string> audioDeviceNames;
    ALCdevice* currentAudioDevice = nullptr;
    ALCcontext* alctx = nullptr;
}

GameClient::GameClient() 
{
    stateManager.set_state_by_force(GameStateEnum::TESTING);
    m_window.setVSync(true);
    m_imctx = ImGui::CreateContext();
}
GameClient::~GameClient() {
    m_window.clean_up();
}

void GameClient::imgui_init() const
{
    ImGui::SetCurrentContext(m_imctx);
    IMGUI_CHECKVERSION();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window.m_window, m_window.m_glContext);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void GameClient::imgui_start_frame()
{
    SharedQueue<SDL_Event>& s_SDLEventMessenger = SharedQueue<SDL_Event>::Get();
    GameStateManager& gsm = GameStateManager::Get();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    while (auto e = s_SDLEventMessenger.tryPop()) {
        ImGui_ImplSDL2_ProcessEvent(&e.value());
    }

    ImGui::Begin("Game States");
    const char* state_names[] = {
        "Testing State",
        "Menu State",
        "Game State"
    };
    constexpr GameStateEnum enums[] = {
        GameStateEnum::TESTING,
        GameStateEnum::MENU,
        GameStateEnum::GAME
    };
    static int selected_idx = 0;
    if (ImGui::BeginListBox("Selected Game State")) {
        for (int i = 0; i < sizeof(state_names) / sizeof(const char*); i++) {
            const bool selected = selected_idx == i;
            if (ImGui::Selectable(state_names[i], selected)) {
                selected_idx = i;
                gsm.swap(enums[i]);
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }


    ImGui::End();
}

void GameClient::imgui_end_frame()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui::Render();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameClient::start(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ClientStartupState& p_state) {
    // important
    m_window.unbindFromThisThread();
    clientThread = std::thread(&GameClient::run, this, std::ref(p_exceptionQueue), p_state);
}

void GameClient::stop() {
    m_clientStopping = true;
    clientThread.join();
}
bool GameClient::joinable() {
    return clientThread.joinable();
}

InputHandler& GameClient::get_client_input()
{
    return m_clientInp;
}

// TODO: find a more suitable location for this functionality
static void audio_init() {
    // platform-dependent check, shouldn't ever fail unless I'm proven wrong
    static_assert(sizeof(ALchar) == sizeof(char));

    const ALchar* allDevice_ptr = nullptr;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE) { // able to get every device via OpenAL
        // OpenAL context owns this string so we don't need to free
        allDevice_ptr = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        if (*allDevice_ptr != '\0') {
            ClientAudioState::audioDeviceNames.push_back(static_cast<const char*>(allDevice_ptr));
        }
        // world's worst failsafe but at least it stops a runoff
        constexpr uint16_t MAX_DEVICE_CHARLEN = 1024;
        for (uint16_t itr = 0; !(allDevice_ptr[itr] == '\0' && allDevice_ptr[itr + 1] == '\0') && itr < MAX_DEVICE_CHARLEN; ++itr) {
            if (allDevice_ptr[itr] == '\0') {
                // copy from cstr to vector (maybe unsafe)
                ClientAudioState::audioDeviceNames.push_back(static_cast<const char*>(allDevice_ptr + itr + 1));
            }
        }
    }
    else {
        throw std::exception("Unsupported OpenAL operation: Cannot enumerate all audio devices.");
    }

    const ALchar* defaultDevice_str = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice* defaultDevice = alcOpenDevice(defaultDevice_str);
    if (!defaultDevice) {
        throw std::exception("Failed to create default OpenAL Device. (Could not initialize sound)");
    }
    LOG("Got \"" << alcGetString(defaultDevice, ALC_DEVICE_SPECIFIER) << "\" as the default sound device.\n");

    ClientAudioState::defaultAudioDeviceName = defaultDevice_str;
    ClientAudioState::currentAudioDevice = defaultDevice;

    // If default device is among all audio devices (should always be the case), move it to a known vector location
    auto vecPos = std::ranges::find(ClientAudioState::audioDeviceNames, defaultDevice_str);
    if (vecPos != ClientAudioState::audioDeviceNames.end())
        std::iter_swap(ClientAudioState::audioDeviceNames.begin() + 1, vecPos);

#ifdef _DEBUG
    std::println("Audio Device List: {}", ClientAudioState::audioDeviceNames);
#endif

    alc_check(ClientAudioState::alctx = alcCreateContext(defaultDevice, nullptr), ClientAudioState::currentAudioDevice);

    if (!alcMakeContextCurrent(ClientAudioState::alctx)) {
        throw std::exception("Failed to activate OpenAL context.");
    };

    //play_wav_immediate("./res/flup.wav", 1.f, 1.f, false, DSKGlobals::alctx, DSKGlobals::audioStateChangeCount);
}

void GameClient::run(SharedQueue<std::exception_ptr>& p_exceptionQueue, const ClientStartupState& p_params) {
    m_settings = p_params;
    SharedQueue<MouseEvent>& s_mouseQueue = SharedQueue<MouseEvent>::Get(); // one-way messenger for capturing mouse events
    SharedQueue<KeyEvent>& s_keyQueue = SharedQueue<KeyEvent>::Get(); // one-way messenger for capturing key events

    assert(m_settings.mouseSubject && m_settings.keySubject && "Client startup settings must be configured and contain input subjects");
    Subject<MouseEvent>& mouseSubject = *m_settings.mouseSubject; // for re-transmitting mouse events from client
    Subject<KeyEvent>& keySubject = *m_settings.keySubject; // for re-transmitting key events from client

    m_window.bindToThisThread();

    if (m_settings.refreshRatePtr)
        *m_settings.refreshRatePtr = m_window.getRefreshRate();

    imgui_init();
    audio_init();

    try {
        while (true) {

            if (m_window.hasChangedFullscreenState()) {
                resizeWindow(m_window.width, m_window.height);
            }
            if (flagResize) {
                flagResize = false;
                resizeWindow(newWidth, newHeight);
            }

            renderFPSGauge.update(0.99f);

            imgui_start_frame();

            if (m_settings.frameratePtr)
                *m_settings.frameratePtr = 1.f / renderFPSGauge.getFrametimeAverage();

            m_window.clear();
            stateManager.client_update();

            imgui_end_frame();

            // static so it keeps mouse pos between updates
            static GUIEvent gui_e; 
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            gui_e.key.valid = false;

            // handle keyboard event transmission
            while (auto key_opt = s_keyQueue.tryPop()) {
                if (io.WantCaptureKeyboard) {
                    break;
                }
                gui_e.key.valid = true;
                keySubject.notifyAll(key_opt.value());
                if (key_opt.value().wasDown) {
                    m_clientInp.processKeyDown(key_opt.value().keyCode);
                }
                else {
                    m_clientInp.processKeyUp(key_opt.value().keyCode);
                }
                gui_e.key = key_opt.value();
                m_gui.update(gui_e);
            }

            // handle mouse event transmission
            while (auto mouse_opt = s_mouseQueue.tryPop()) {
                gui_e.mouse = mouse_opt.value();
                // either ImGui or the custom gui may want to prevent mouse events from being transmitted
                if (!io.WantCaptureMouse && !m_gui.update(gui_e)) {
                    mouseSubject.notifyAll(mouse_opt.value());
                };
            }

            m_gui.draw(m_window);
            m_window.displayNewFrame();

            if (stateManager.client_should_close()) m_clientStopping = true;
            if (m_clientStopping) break;
        }

    }
    catch (std::exception& ex) {
        ERROR_LOG("Exception in " << __FILE__ << " at " << __LINE__ << ": " << ex.what());
        p_exceptionQueue.push(std::current_exception());
        clean_up();
        return;
    }
    clean_up();
}


void GameClient::clean_up()
{
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ClientAudioState::alctx);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(m_imctx);
}

void GameClient::resizeWindow(uint32_t p_w, uint32_t p_h)
{
    m_window.width = p_w;
    m_window.height = p_h;
    m_window.setViewport(0, 0, p_w, p_h);
}


