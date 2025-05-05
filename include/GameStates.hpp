#pragma once
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <Framework/Log.hpp>
enum class GameStateEnum {
	NO_STATE,
	MENU,
	TESTING
};

class GameState {
public:
	GameState() {};
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void suspend() = 0;
	virtual void resume() = 0;
	virtual void close() = 0; // must not require thread-specific functions

	bool initialized = false;
};
struct GameStatePair {
	GameState* clientState = nullptr;
	GameState* serverState = nullptr;
	void close() {
		if (serverState) {
			serverState->close();
			serverState->initialized = false;
			LOG("A Server State is Closing");
		}
		if (clientState) {
			clientState->close();
			clientState->initialized = false;
			LOG("A Client State is Closing");
		}
	}

	bool ready() { return clientState && serverState; };
	GameStateEnum ID = GameStateEnum::NO_STATE;
};

class GameStateManager { // todo: un-gunch it by putting stuff in the cpp file
public:
	GameStateManager() { };

	void clientUpdate() {
		std::shared_lock<std::shared_mutex> lock(m_stateLock); // ensure state is not changing
		if (!m_activeState || !m_activeState->ready()) return;
		// if the client is not running, and the server is, start the client
		if (!m_activeState->clientState->initialized) {
			if (m_activeState->serverState->initialized) {
				m_activeState->clientState->init();
				m_activeState->clientState->initialized = true;
			}
			return;
		};
		m_activeState->clientState->update();
	}

	void serverUpdate() {
		std::shared_lock<std::shared_mutex> lock(m_stateLock); // ensure state is not changing
		// if the server is not running, start it
		if (!m_activeState->serverState->initialized) {
			m_activeState->serverState->init();
			m_activeState->serverState->initialized = true;
			return;
		}
		if (!m_activeState || !m_activeState->ready()) return;
		m_activeState->serverState->update();
	}

	void bindClientState(GameStateEnum p_ID, GameState* p_clientState) {
		std::unique_lock<std::shared_mutex> lock(m_stateLock);
		if (stateExists(p_ID)) {
			getState(p_ID)->clientState = p_clientState;
			return;
		}
		m_allStates.push_back(GameStatePair());
		auto& lastState = m_allStates[m_allStates.size() - 1];
		lastState.clientState = p_clientState;
		lastState.ID = p_ID;
	}

	void bindServerState(GameStateEnum p_ID, GameState* p_serverState) {
		std::unique_lock<std::shared_mutex> lock(m_stateLock);
		if (stateExists(p_ID)) {
			getState(p_ID)->serverState = p_serverState;
		}
		m_allStates.push_back(GameStatePair());
		auto& lastState = m_allStates[m_allStates.size() - 1];
		lastState.serverState = p_serverState;
		lastState.ID = p_ID;
	}

	void setStateByForce(GameStateEnum p_ID) { // only to be used for setting the very first state, really. It's a forceful set.
		std::unique_lock<std::shared_mutex> lock(m_stateLock);
		if (stateExists(p_ID)) {
			m_activeState = getState(p_ID);
		}
	}
	void swap(GameStateEnum newState) {
		if (m_activeState->ID == newState) return;
		if (!m_activeState->ready()) throw std::exception("GameState has not been set up! Cannot swap existing state.");

		m_activeState->close();
		LOG("New game state has been swapped in.");
		m_activeState = getState(newState);
		if (!m_activeState->ready()) throw std::exception("An adequate server-client state pair has not been made for swapped state.");
		// init is handled by update function to ensure correct thread
		//m_activeState->init();
	};
	void close() {
		std::shared_lock<std::shared_mutex> lock(m_stateLock);
		if (!m_activeState || !m_activeState->ready()) return;
		stateStopping = true;
	}
	bool maybeStopClient() {
		if (!m_activeState->clientState) return false;
		if (stateStopping) {
			std::unique_lock<std::shared_mutex> lock(m_stateLock);
			m_activeState->clientState->close();
			m_activeState->clientState = nullptr;
			return true;
		}
		return false;
	}
	bool maybeStopServer() {
		if (!m_activeState->serverState) return false;
		if (stateStopping && !m_activeState->clientState) {
			std::unique_lock<std::shared_mutex> lock(m_stateLock);
			m_activeState->serverState->close();
			m_activeState->serverState = nullptr;
			return true;
		}
		return false;
	}
	static GameStateManager& Get() {
		static GameStateManager instance;
		return instance;
	}

	GameStatePair* getState(GameStateEnum p_ID) { // silly linear search, doesn't matter too much
		for (auto& s : m_allStates) {
			if (s.ID == p_ID) return &s;
		}
		throw std::exception("State not found.");
	};

	bool stateExists(GameStateEnum p_ID) {
		for (auto& s : m_allStates) {
			if (s.ID == p_ID) return true;
		}
		return false;
	};
private:
	std::vector<GameStatePair> m_allStates;
	GameStatePair* m_activeState = nullptr;
	volatile bool stateStopping = false;
	std::shared_mutex m_stateLock;
};