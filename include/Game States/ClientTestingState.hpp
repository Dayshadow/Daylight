#pragma once
#include "GameStates.hpp"
#include "Framework/Graphics/Sprite.hpp"
#include "Framework/Graphics/GenericShaders.hpp"
#include "Framework/Window/GameWindow.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIContainer.hpp"
#include "Framework/Graphics/GUI_Experimental/GUIDragBar.hpp"

#include <Crypt.hpp>

class ClientTestingState : public GameState {
public:
	ClientTestingState() = delete;
	ClientTestingState(GameWindow& window) : m_window(window) {};

	GenericShaders& gs = GenericShaders::Get();
	GUI& GlobalGUI = GUI::Get();

	// floating 32 z units above the x-y plane
	Camera testCam{ {0.f, 0.f, 32.f} };

	GUIDragBar testBoxDrag{ "bar1" };
	GUIContainer testBox{ "box1" };


	void init() override {
		LOG("Client state test started!");
		// look at origin
		testCam.lookAt({ 0.f, 0.f, 0.f });

		testBox.setAbsoluteBounds(Rect(0.25f, 0.25f, 0.5f, 0.5f));
		testBox.enableBackground();
		testBox.backgroundColor = glm::vec3(0.4f, 0.1f, 0.05f);
		testBox.backgroundOpacity = 1.f;

		testBoxDrag.setLocalBounds(Rect(0.f, 0.f, 1.f, 0.1f));
		testBoxDrag.backgroundColor = glm::vec3(1.f);
		testBoxDrag.enableBackground();
		testBox.addChild(&testBoxDrag);
		
		GlobalGUI.addElement(&testBox);

	};

	void update() override {
		static int64_t frame = 0;
		frame++;
		// set the draw surface to the window and clear with a dark blue grey
		m_window.bind();
		m_window.setClearColor({ 0.2f, 0.2f, 0.24f, 0.0f });
		m_window.clear();

		// adjust camera to window aspect
		testCam.setDimensions(m_window.width, m_window.height);

		bool isP = Crypt::isProbablyPrime<uint32_t>(394696903, 8);

		auto p1 = Crypt::generateRSAPrime<boost::multiprecision::uint1024_t>();
		auto p2 = Crypt::generateRSAPrime<boost::multiprecision::uint1024_t>();


		//LOG(Crypt::generateKey512());

		DrawStates d;
		// apply camera transform
		d.setTransform(testCam.getTransform());
	};
	void suspend() override {};
	void resume() override {};
	void close() override {
		// recursive
		GlobalGUI.removeElement("box1");
	};

private:
	GameWindow& m_window;
};