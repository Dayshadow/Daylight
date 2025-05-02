#pragma once
#include "GameStates.hpp"

class ServerTestingState : public GameState {
public:
	ServerTestingState() {}

	void init() override {
		LOG("Server state test started!");
	};
	void update() override {};
	void suspend() override {};
	void resume() override {};
	void close() override {};

private:

};