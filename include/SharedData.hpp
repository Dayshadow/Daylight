#pragma once
#include <memory>
#include <type_traits>

template <typename T>
concept HasSharedClass = requires {
    typename T::Shared;
};


template <HasSharedClass T>
struct SharedData {
    virtual void post_init();
    std::shared_ptr<typename T::Shared> shared;
};

template<HasSharedClass T>
inline void SharedData<T>::post_init()
{
}
