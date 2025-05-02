#pragma once
#include "GameStates.hpp"
#include "Framework/Graphics/Sprite.hpp"
#include "Framework/Graphics/GenericShaders.hpp"
#include "Framework/Window/GameWindow.hpp"

class ClientTestingState : public GameState {
public:
	ClientTestingState() = delete;
	ClientTestingState(GameWindow& window) : m_window(window) {};

	GenericShaders& gs = GenericShaders::Get();

	// 0.5x0.5 square centered at 0, 0
	Sprite sillySprite{ glm::vec3(0.f), Rect(-0.25f, -0.25f, 0.5f, 0.5f) };
	// floating 32 z units above the x-y plane
	Camera testCam{ {0.f, 0.f, 32.f} };

	void init() override {
		LOG("Client state test started!");
		// look at origin
		testCam.lookAt({ 0.f, 0.f, 0.f });
		// add basic shader to sprite
		sillySprite.attachShader(&gs.solidColorShader);
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

		// set sprite color and opacity
		gs.solidColorShader.setVec3Uniform(gs.solidColor_colorUniformLoc, { 0.8f, 0.85f, 1.0f });
		gs.solidColorShader.setFloatUniform(gs.solidColor_opacityUniformLoc, 0.9f);

		// self explanitory
		sillySprite.setRotation((float)frame / 60);

		DrawStates d;
		// apply camera transform
		d.setTransform(testCam.getTransform());

		sillySprite.draw(m_window, d);
	};
	void suspend() override {};
	void resume() override {};
	void close() override {};

private:
	GameWindow& m_window;
};