#include "Client.hpp"
#include "Framework/Graphics/Sprite.hpp"
#include "Framework/Graphics/GenericShaders.hpp"
#include "Framework/Audio/wav.hpp"
#include "Framework/Audio/AudioImmediate.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIDragBar.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIContainer.hpp"
Client::Client() {
	stateManager.bindClientState(GameStateEnum::NO_STATE, (GameState*)&State_None);
	stateManager.bindClientState(GameStateEnum::MENU, (GameState*)&State_Menu);
	stateManager.bindClientState(GameStateEnum::TESTING, (GameState*)&State_Testing);
	stateManager.setStateByForce(GameStateEnum::TESTING);
	window.setVSync(true);
	imctx = ImGui::CreateContext();
}
Client::~Client() {
	window.cleanUp();
}

void Client::start(SharedQueue<std::exception_ptr>& p_exceptionQueue) {
	// important
	window.unbindFromThisThread();
	clientThread = std::thread(&Client::run, this, std::ref(p_exceptionQueue));
}

void Client::stop() {
	clientStopping = true;
	clientThread.join();
}

void audioInit();

void Client::run(SharedQueue<std::exception_ptr>& p_exceptionQueue) {
	SharedQueue<MouseEvent>& s_mouseQueue = SharedQueue<MouseEvent>::Get(); // one-way messenger for capturing mouse events
	SharedQueue<KeyEvent>& s_keyQueue = SharedQueue<KeyEvent>::Get(); // one-way messenger for capturing mouse events
	SharedQueue<SDL_Event>& s_SDLEventMessenger = SharedQueue<SDL_Event>::Get();

	Subject<MouseEvent>& mouseSubject = Globals::mouseSubject; // for re-transmitting mouse events from client
	Subject<KeyEvent>& keySubject = Globals::keySubject; // for re-transmitting key events from client

	window.bindToThisThread();

	ImGui::SetCurrentContext(imctx);
	IMGUI_CHECKVERSION();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window.m_window, window.m_glContext);
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);

	audioInit();

	try {
		while (true) {

			if (window.hasChangedFullscreenState()) {
				resizeWindow(window.width, window.height);
			}
			if (flagResize) {
				flagResize = false;
				resizeWindow(newWidth, newHeight);
			}

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			while (auto e = s_SDLEventMessenger.tryPop()) {
				ImGui_ImplSDL2_ProcessEvent(&e.value());
			}

			renderFPSGauge.update(0.99f);

			static GUIEvent e; // static so it keeps mouse pos between updates
			e.key.valid = false;
			while (auto opt = s_keyQueue.tryPop()) {
				if (io.WantCaptureKeyboard) {
					break;
				}
				e.key.valid = true;
				keySubject.notifyAll(opt.value());
				if (opt.value().wasDown) {
					inp.processKeyDown(opt.value().keyCode);
				}
				else {
					inp.processKeyUp(opt.value().keyCode);
				}
				e.key = opt.value();
				gui.update(e);
			}

			while (auto opt = s_mouseQueue.tryPop()) {
				e.mouse = opt.value();
				if (!io.WantCaptureMouse && !gui.update(e)) {
					mouseSubject.notifyAll(e.mouse);
				};
			}

			window.clear();
			stateManager.clientUpdate();
			gui.draw(window);
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

			window.displayNewFrame();

			if (stateManager.maybeStopClient()) clientStopping = true;
			if (clientStopping) break;
		}

	}
	catch (std::exception& ex) {
		ERROR_LOG("Exception in " << __FILE__ << " at " << __LINE__ << ": " << ex.what());
		p_exceptionQueue.push(std::current_exception());
		cleanUp();
		return;
	}
	cleanUp();
}


void Client::cleanUp()
{
	alcMakeContextCurrent(NULL);
	alcDestroyContext(Globals::alctx);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext(imctx);
}

void Client::resizeWindow(uint32_t p_w, uint32_t p_h)
{
	window.width = p_w;
	window.height = p_h;
	window.setViewport(0, 0, p_w, p_h);
}

void audioInit() {

	const ALchar* allDevice_ptr = nullptr;
	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE) { // able to get every device via openAL
		// openAL context owns this string so we don't need to free
		allDevice_ptr = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		if (*allDevice_ptr != '\0') {
			Globals::audioDeviceNames.push_back(reinterpret_cast<const char*>(allDevice_ptr));
		}
		// world's worst failsafe but at least it stops a runoff
		constexpr uint16_t MAX_DEVICE_CHARLEN = 1024;
		for (uint16_t itr = 0; !(allDevice_ptr[itr] == '\0' && allDevice_ptr[itr + 1] == '\0') && itr < MAX_DEVICE_CHARLEN; ++itr) {
			if (allDevice_ptr[itr] == '\0') {
				// copy from cstr to vector (maybe unsafe)
				Globals::audioDeviceNames.push_back(reinterpret_cast<const char*>(allDevice_ptr + itr + 1));
			}
		}
	}

	const ALchar* defaultDevice_str = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice* defaultDevice = alcOpenDevice(defaultDevice_str);
	std::cout << "Got \"" << alcGetString(defaultDevice, ALC_DEVICE_SPECIFIER) << "\" as the default sound device.\n";

	Globals::defaultDeviceName = defaultDevice_str;
	Globals::currentAudioDevice = defaultDevice;

	auto vecPos = std::find(Globals::audioDeviceNames.begin(), Globals::audioDeviceNames.end(), defaultDevice_str);
	if (vecPos != Globals::audioDeviceNames.end())
		std::iter_swap(Globals::audioDeviceNames.begin() + 1, vecPos);

	Globals::alctx = alcCreateContext(defaultDevice, nullptr);

	if (!alcMakeContextCurrent(Globals::alctx)) {
		std::cerr << "Failed to set active context.\n";
	}

}

