#pragma once

#include "GameStates.hpp"

class TemplateState : public GameState {
public:
	TemplateState() {}

	void init() override {};
	void update() override {};
	void suspend() override {};
	void resume() override {};
	void close() override {};

private:

};