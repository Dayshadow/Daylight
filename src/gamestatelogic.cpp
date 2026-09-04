#include "GameStateLogic.hpp"

void GameStatePair::close()
{
    if (std::shared_ptr<GameState> lkServer = server.lock()) {
        lkServer->close();
        lkServer->initialized = false;
        LOG("A Server State is Closing");
    }
    if (std::shared_ptr<GameState> lkClient = client.lock()) {
        lkClient->close();
        lkClient->initialized = false;
        LOG("A Client State is Closing");
    }
}

bool GameStatePair::ready() const
{
    return (!client.expired()) && (!server.expired());
}

void GameStateManager::client_update()
{
    std::shared_lock<std::shared_mutex> lock(m_stateLock); // ensure state is not changing

    // don't crash, just move on
    if (m_activeState.expired())
        return;

    std::shared_ptr<GameStatePair> activeState = m_activeState.lock();

    if (!activeState->ready()) return;
    // if the client is not running, and the server is, start the client
    if (std::shared_ptr<GameState> clientState = activeState->client.lock()) {
        if (!clientState->initialized) {
            if (activeState->server.lock()->initialized) {
                clientState->init();
                clientState->initialized = true;
            }
            return;
        };
        clientState->update();
    }
}

void GameStateManager::server_update()
{
    std::shared_lock<std::shared_mutex> lock(m_stateLock); // ensure state is not changing
  
    // don't crash, just move on
    if (m_activeState.expired())
        return;

    if (std::shared_ptr<GameStatePair> activeState = m_activeState.lock()) {
        // if the server is not running, start it
        if (std::shared_ptr<GameState> serverState = activeState->server.lock()) {
            if (!serverState->initialized) {
                serverState->init();
                serverState->initialized = true;
                return;
            }
            if (!activeState->ready()) return;
            serverState->update();
        }
    }

}

void GameStateManager::bind_client_state(GameStateEnum p_ID, std::weak_ptr<GameState> p_clientState)
{
    std::unique_lock<std::shared_mutex> lock(m_stateLock);
    if (state_exists(p_ID)) {
        // assume that call succeeds, excepts on fail
        std::weak_ptr<GameStatePair> target = get_state(p_ID);
        target.lock()->client = p_clientState;
        return;
    }
    // else, pair does not exist yet, so create it
    m_allStates.push_back(std::make_shared<GameStatePair>());
    auto& lastState = m_allStates.back();
    lastState->client = p_clientState;
    lastState->ID = p_ID;
}

void GameStateManager::bind_server_state(GameStateEnum p_ID, std::weak_ptr<GameState> p_serverState)
{
    std::unique_lock<std::shared_mutex> lock(m_stateLock);
    if (state_exists(p_ID)) {
        // assume that call succeeds, excepts on fail
        std::weak_ptr<GameStatePair> target = get_state(p_ID);
        target.lock()->server = p_serverState;
        return;
    }
    // else, pair does not exist yet, so create it
    m_allStates.push_back(std::make_shared<GameStatePair>());
    auto& lastState = m_allStates.back();
    lastState->server = p_serverState;
    lastState->ID = p_ID;
}

void GameStateManager::set_state_by_force(GameStateEnum p_ID)
{
    std::unique_lock<std::shared_mutex> lock(m_stateLock);
    if (state_exists(p_ID)) {
        m_activeState = get_state(p_ID);
    }
}

void GameStateManager::swap(GameStateEnum newState)
{
    if (m_activeState.expired()) {
        throw std::exception("Tried to swap game states when an initial state has not been set.");
    };
    std::shared_ptr<GameStatePair> activeState = m_activeState.lock();

    if (activeState->ID == newState) return;
    if (!activeState->ready()) throw std::exception("GameState has not been set up! Cannot swap existing state.");

    activeState->close();
    LOG("New game state has been swapped in.");
    m_activeState = get_state(newState);
    if (!activeState->ready()) throw std::exception("An adequate server-client state pair has not been made for swapped state.");
    // init is handled by update function to ensure correct thread
    //m_activeState->init();
};

void GameStateManager::close()
{
    std::shared_lock<std::shared_mutex> lock(m_stateLock);
    if (m_activeState.expired()) return;

    std::shared_ptr<GameStatePair> activeState = m_activeState.lock();
    if (!activeState->ready()) return;
    m_stateStopping = true;
}

bool GameStateManager::client_should_close()
{
    std::unique_lock<std::shared_mutex> lock(m_stateLock);
    
    if (std::shared_ptr<GameStatePair> activeState = m_activeState.lock()) {
        if (activeState->client.expired()) return false;
        if (m_stateStopping) {
            activeState->client.lock()->close();
            activeState->client.reset();
            return true;
        }
    }
    return false;
}

bool GameStateManager::server_should_close()
{
    std::unique_lock<std::shared_mutex> lock(m_stateLock);
    if (std::shared_ptr<GameStatePair> activeState = m_activeState.lock()) {
        if (activeState->server.expired()) return false;
        if (m_stateStopping && activeState->client.expired()) {
            activeState->server.lock()->close();
            activeState->server.reset();
            return true;
        }
    }
    return false;
}

GameStateManager& GameStateManager::Get()
{
    static GameStateManager instance;
    return instance;
}

std::weak_ptr<GameStatePair> GameStateManager::get_state(GameStateEnum p_ID)
{
    auto state = find_state(p_ID);

    if (state)
        return state.value();

    throw std::exception("State not found.");
    return std::weak_ptr<GameStatePair>{};
}
bool GameStateManager::state_exists(GameStateEnum p_ID)
{
    return find_state(p_ID).has_value();
};

std::optional<std::weak_ptr<GameStatePair>> GameStateManager::find_state(GameStateEnum p_ID)
{
    auto stateIt = std::ranges::find_if(m_allStates, 
        [p_ID](const std::shared_ptr<GameStatePair>& state) {
            return state->ID == p_ID;
        });
    if (stateIt != std::end(m_allStates))
        return std::weak_ptr<GameStatePair>{ *stateIt };
    return std::nullopt;
}

GameState::GameState(GameStateEnum p_binding)
      : m_binding(p_binding)
{
}

GameStateEnum GameState::get_binding() const
{
    return m_binding;
}
