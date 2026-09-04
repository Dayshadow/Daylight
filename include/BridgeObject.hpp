#pragma once
#include <type_traits>
#include "SharedData.hpp"
#include "Framework/Log.hpp"
#include <thread>
#include <stdexcept>

template <typename T>
concept BridgeType = requires {
    typename T::Client;
    typename T::Server;
    typename T::Shared;
    // inherit from SharedData to get access to the shared_ptr in client and server.
    requires std::is_base_of<SharedData<T>, typename T::Client>::value;
    requires std::is_base_of<SharedData<T>, typename T::Server>::value;
};

// This represents any object that must exist on both the client and the server, used for sync.
// Must implement 3 subclasses, Client, Server, and Shared.
// NOTE:: this is for any OBJECT type, it does not represent the actual client and server threads, just object data
template <BridgeType T>
struct BridgeObject {
    BridgeObject();

    void make_client_data();
    void make_server_data();
    void destroy();

    std::shared_ptr<typename T::Client> client();
    std::shared_ptr<typename T::Server> server();
    std::shared_ptr<typename T::Shared> shared();

    bool linked() const;
    const uint64_t UUID() const; // used for keeping track of vector position, even if moved

    bool is_client_initialized();
    bool is_server_initialized();

private:
    std::shared_ptr<typename T::Server> m_server;
    std::shared_ptr<typename T::Shared> m_shared;
    std::shared_ptr<typename T::Client> m_client;

    static uint64_t s_counter;

    uint64_t m_UUID = NULL;
    std::thread::id m_serverThread;
    std::thread::id m_clientThread;

    bool m_serverInitialized = false;
    bool m_clientInitialized = false;
};

template<BridgeType T>
uint64_t BridgeObject<T>::s_counter = 0;

template<BridgeType T>
inline BridgeObject<T>::BridgeObject()
{
    m_UUID = ++s_counter; // may not be atomic, but probably is
    m_shared = std::make_shared<typename T::Shared>();
}

// this destroy is dirty, the objects MUST not be in-use when it occurs. 
template<BridgeType T>
inline void BridgeObject<T>::destroy()
{
    m_UUID = NULL;
    // should represent an invalid thread, might just be uninitialized
    m_serverThread = std::thread::id();
    m_clientThread = std::thread::id();

    //s_counter--;
    m_serverInitialized = false;
    m_clientInitialized = false;

    m_client.reset();
    m_server.reset();
    m_shared.reset();
}

template<BridgeType T>
inline void BridgeObject<T>::make_client_data()
{
    if (m_clientInitialized) {
        WARNING_LOG("Trying to re-init a bridge object's client data");
        return;
    }
    m_client = std::make_shared<typename T::Client>();
    m_client->shared = m_shared;
    m_clientThread = std::this_thread::get_id();
    m_client->post_init();
    m_clientInitialized = true;
}

template<BridgeType T>
inline void BridgeObject<T>::make_server_data()
{
    if (m_serverInitialized) {
        WARNING_LOG("Trying to re-init a bridge object's server data");
        return;
    }
    m_server = std::make_shared<typename T::Server>();
    m_server->shared = m_shared;
    m_serverThread = std::this_thread::get_id();
    m_server->post_init();
    m_serverInitialized = true;
}

template<BridgeType T>
inline bool BridgeObject<T>::is_client_initialized()
{
    return m_clientInitialized;
}

template<BridgeType T>
inline bool BridgeObject<T>::is_server_initialized()
{
    return m_serverInitialized;
}

template<BridgeType T>
inline std::shared_ptr<typename T::Client> BridgeObject<T>::client()
{
    if (std::this_thread::get_id() != m_clientThread) {
        ERROR_LOG("Wrong Thread access in Bridge<T>! Use the thread you called make_client_data() with.");
        return nullptr;
    }
    return m_client;
}

template<BridgeType T>
inline std::shared_ptr<typename T::Server> BridgeObject<T>::server()
{
    if (std::this_thread::get_id() != m_serverThread) {
        ERROR_LOG("Wrong Thread access in Bridge<T>! Use the thread you called make_server_data() with.");
        return nullptr;
    }
    return m_server;
}

template<BridgeType T>
inline std::shared_ptr<typename T::Shared> BridgeObject<T>::shared()
{
    return m_shared;
}

template<BridgeType T>
inline bool BridgeObject<T>::linked() const
{
    return !(m_server == nullptr || m_client == nullptr);
}

template<BridgeType T>
inline const uint64_t BridgeObject<T>::UUID() const
{
    return m_UUID;
}
