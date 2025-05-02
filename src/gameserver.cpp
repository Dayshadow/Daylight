#include "GameServer.hpp"

// This is mostly just boilerplate from my basic engine, it may not be needed
GameServer::GameServer()
{
	stateManager.bindServerState(GameStateEnum::NO_STATE, (GameState*)&State_None);
	stateManager.bindServerState(GameStateEnum::MENU, (GameState*)&State_Menu);
	stateManager.bindServerState(GameStateEnum::TESTING, (GameState*)&State_Testing);
	stateManager.setStateByForce(GameStateEnum::TESTING);
}

void GameServer::start(SharedQueue<std::exception_ptr>& p_exceptionQueue) {
	serverThread = std::thread(&GameServer::run, this, std::ref(p_exceptionQueue));
}

void GameServer::stop() {
	serverStopping = true;
	serverThread.join();
}
void GameServer::run(SharedQueue<std::exception_ptr>& p_exceptionQueue) {

	Observer<MouseEvent> mouseObserver{ Globals::mouseSubject };

	try {

		while (true) {

			ts.processFrameStart();
			while (ts.accumulatorFull()) {
				ts.drain();

				stateManager.serverUpdate();

				tickGauge.update(0.9f);

				Globals::updateFPS = (float)(1.0 / tickGauge.getFrametimeAverage());

				ts.processFrameStart();
			}
			// prevent it from overworking
			std::this_thread::sleep_for(std::chrono::microseconds(1000000 / (UPDATE_RATE_FPS * 2)));
			;
			if (stateManager.maybeStopServer()) serverStopping = true;

			if (serverStopping) break;
		}
	}
	catch (std::exception& ex) {
		ERROR_LOG("Exception in " << __FILE__ << " at " << __LINE__ << ": " << ex.what());
		p_exceptionQueue.push(std::current_exception());
	}
}