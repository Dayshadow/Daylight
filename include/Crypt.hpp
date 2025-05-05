#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <random>
#include <exception>
#include <type_traits>
#include "Framework/Log.hpp"

#include "Primes.hpp"

template <typename T>
concept KeyType = requires(T t) {
    //requires boost::multiprecision::is_unsigned_number<T>::value;
    requires std::numeric_limits<T>::is_integer;
};

namespace Crypt {

    // Populates a given boost multiprecision type with values taken from std::random_device. Should be ran infrequently.
    template <KeyType T>
    T generateHardwareKey() {
        constexpr size_t keySize = std::numeric_limits<T>::digits;

        // note: if there is no hardware-implemented random number generator, this might be deterministic.
        std::random_device rnd;
        T key = 0;
        constexpr size_t hardwareKeySize = sizeof(std::random_device::result_type) * 8;

        // Populate the entire key with hardware-random numbers, since the value returned by std::random_device is typically 32bit
        for (size_t bit = 0; bit < keySize; bit += hardwareKeySize) {
            key = key ^ rnd();
            // Do not bitshift after filling in the final value
            if (bit != keySize - hardwareKeySize) key = key << hardwareKeySize;
        }

        return key;
    };

    // A faster way of generating new randomized keys, using a different key as a base.
    template <KeyType T>
    T cycleKey(const T& p_seedKey) {
        constexpr size_t keySize = std::numeric_limits<T>::digits;

        // We will use the least significant 32 bits as a seed to cycle the rest of the key
        // It doesn't have to have the full key size, because we're only using it to re-attempt primality.
        uint32_t trunc_seed = (uint32_t)p_seedKey;
        std::mt19937 psuedo{ trunc_seed };
        T newKey = p_seedKey;

        // Close to what I did before, but now using the existing seed as a base for the psuedorandom generator.
        for (size_t bit = 0; bit < keySize; bit += 32) {
            newKey = newKey ^ psuedo();
            // Do not bitshift after filling in the final value
            if (bit != keySize - 32) newKey = newKey << 32;
        }

        return newKey;
    }

    // I need to do some preprocessing here to prevent overflow, this is the easiest way to do so (doubling the size of the operand datatype during intermediate calculation)
    template <KeyType T>
    T mult_mod_huge(const T& a, const T& b, const T& n) {
        constexpr size_t HUGESIZE = std::numeric_limits<T>::digits * 2;
        using HUGE = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<HUGESIZE, HUGESIZE, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
        return (((HUGE)a * (HUGE)b) % (HUGE)n).convert_to<T>();
    }

    // need to use a modular algo involving repeated squaring when using numbers this big
    template <KeyType T>
    T mod_pow_large(const T& p_base, const T& p_exp, const T& n) {
        if (n < 1) throw std::exception("Bad modulo divisor");

        // This method works by breaking down the expression into powers of two multiplied together, which is much easier to calculate and gives the same result
        // ex. 24 ^ 57 mod 19:
        // 57 = 0b0011 1001
        // 57 = 2^1 + 2^4 + 2^5 + 2^6
        //    =  1     8    16    32
        // calculate 24^1 mod 19, 24^8 mod 19, 24^16 mod 19, 24^32 mod 19 through repeated self-multiplication
        // multiply each of those factors for the end result

        T ret = 1;
        T e = p_exp;
        T b = p_base;

        while (e > 0) {
            // if the current leading exp bit we are reading is a 1, include it in the final product
            if (e % 2 == 1)
                ret = mult_mod_huge(ret, b, n);
            // update base to be b^a mod n, crawling until we hit the last bit of the exponent
            b = mult_mod_huge(b, b, n);
            e >>= 1;
        }

        return ret % n;
    }

    // A more involved algo for checking the primality of a large number. More expensive than the simple case, but provides a probabilistic manner of ensuring the key is prime for longer than the first few hundred.
    // https://www.geeksforgeeks.org/primality-test-set-3-miller-rabin/
    template <KeyType T>
    bool MillerRabinPrimeTest(const T& n, T d, size_t s) {
        // doesn't have to be an advanced seed function, just needed for picking a random number in range
        std::mt19937 psuedo{ static_cast<uint32_t>(rand()) };
        boost::random::uniform_int_distribution<T> dist(2, n - 2);
        T&& rand = dist(psuedo);
        T x = mod_pow_large(rand, d, n);

        if (x == 1 || x == n - 1) return true;

        while (d != n - 1) {
            x = mult_mod_huge(x, x, n);
            d *= 2;
            if (x == 1) return false;
            if (x == n - 1) return true;
        }
        return false;
    }

    template <KeyType T>
    bool isProbablyPrime(T p_input, size_t p_iterations) {
        // these are disasterously unlikely but might as well include them for the purpose of being pedantic
        if (p_input <= 1 || p_input == 4) return false;
        if (p_input <= 3) return true; // case 2 and 3

        T d = p_input - 1; // an even number
        size_t s = 0;
        while (d % 2 == 0) {
            d /= 2;
            s += 1;
        }

        for (size_t i = 0; i < p_iterations; i++) {
            if (!MillerRabinPrimeTest<T>(p_input, d, s))
                return false; // number is composite
        }

        return true; // was unable to disprove it being a prime
    }

    // RSA requires large primes with sufficient confidence. An iterative approach can be used to find probabilistically good primes.
    template <KeyType T>
    T generateRSAPrime() {

        // Create a base for the large, prime number. Making sure it's odd, because its easy to do.
        T candidate = generateHardwareKey<T>() | 0b1;

        size_t attempt = 0;
        while (true) {
            // Basic primality test.
            bool simpleScreenPassed = false;
            while (!simpleScreenPassed) {
                //LOG("Cycling key, Attempt #" << attempt);
                simpleScreenPassed = true;
                for (uint64_t smallPrime : GlobalPrimeList) {
                    if (candidate % smallPrime == 0) {
                        //LOG("Divisor found. Key was divisible by " << smallPrime);
                        simpleScreenPassed = false;
                        // regenerate key and try again
                        candidate = cycleKey(candidate) | 0b0;
                        attempt++;
                        break;
                    }
                }

            }
            //LOG(std::hex << std::showbase << candidate);
            //LOG("Initial screening passed, key is prime up to n=500");
            if (isProbablyPrime(candidate, 30)) {
                break; // We found a sufficiently good prime!
            }
            else {
                candidate = cycleKey(candidate) | 0b0;
                attempt++;
            }
        }

        LOG("High quality prime found: " << std::hex << candidate);
        LOG("Took " << std::fixed << attempt << " attempts to find.");

        return candidate;
    }



}