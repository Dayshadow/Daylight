#include "Crypt.hpp"
#include <type_traits>

//boost::multiprecision::uint512_t Crypt::generateKey512()
//{
//    // note: if there is no hardware-implemented random number generator, this might be deterministic.
//    std::random_device rnd;
//    uint512_t key = 0;
//    constexpr size_t hardwareKeySize = sizeof(std::random_device::result_type) * 8;
//
//    // Populate the entire key with hardware-random numbers, since the value returned by std::random_device is typically 32bit
//    for (size_t bit = 0; bit < 512; bit += hardwareKeySize) {
//        key = key ^ rnd();
//        // Do not bitshift after filling in the final value
//        if (bit != 512 - hardwareKeySize) key = key << hardwareKeySize;
//    }
//
//    return key;
//}
//
//uint1024_t Crypt::generateKey1024()
//{
//    return uint1024_t();
//}
