#pragma once
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <Framework/Log.hpp>
#include <assert.h>
#include <optional>
#include "Framework/Input/InputHandler.hpp"

enum class GameStateEnum {
    NO_STATE,
    MENU,
    TESTING,
    GAME
};

// this is a bit silly but it disambiguates server/client states upon creation
enum class GameStateRole {
    SERVER,
    CLIENT
};

class GameState {
public:
    GameState() = delete;
    GameState(GameStateEnum p_binding);
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual void close() = 0; // must not require thread-specific functions

    GameStateEnum get_binding() const;

    friend struct GameStatePair;
    friend class GameStateManager;
private:
    // managed externally by GameStatePair and GameStateManager
    bool initialized = false;
    const GameStateEnum m_binding;
};
struct GameStatePair {
    std::weak_ptr<GameState> client;
    std::weak_ptr<GameState> server;

    void close();
    bool ready() const;

    GameStateEnum ID = GameStateEnum::NO_STATE;
};

class GameStateManager {
public:
    GameStateManager() = default;

    void client_update();
    void server_update();

    void bind_client_state(GameStateEnum p_ID, std::weak_ptr<GameState> p_clientState);
    void bind_server_state(GameStateEnum p_ID, std::weak_ptr<GameState> p_serverState);

    // only to be used for setting the very first state, does not handle changeover logic
    void set_state_by_force(GameStateEnum p_ID);
    void swap(GameStateEnum newState);
    void close();

    // ideally these should be only accessible by client and server, but threads make it non-obvious
    bool client_should_close();
    bool server_should_close();

    static GameStateManager& Get();

    /*
     * @remarks 
     * This function works on the assumption that the amount of game states does not vary during runtime.
     * Failure to acknowledge could lead to pointer invalidation.
     * Please initialize all states before doing any game logic involving this function
     * This method will throw if you provide an invalid ID
     */
    std::weak_ptr<GameStatePair> get_state(GameStateEnum p_ID);
    bool state_exists(GameStateEnum p_ID);
    
private:

    std::optional<std::weak_ptr<GameStatePair>> find_state(GameStateEnum p_ID);

    std::vector<std::shared_ptr<GameStatePair>> m_allStates;
    std::weak_ptr<GameStatePair> m_activeState;
    volatile bool m_stateStopping = false;
    std::shared_mutex m_stateLock;
};



