/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Portable implementation of a tiny subset of the c++ crypto APIs.
/*****************************************************************************/
#include "capi_sass.hpp"

#include <thread>
// Always needed (the PRNG below uses std::mt19937)
#include <random>
#include <ctime>
#include <atomic>
#include <mutex>
#ifdef USE_WIN_CRYPT
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <wincrypt.h>
#endif

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Read a truly random seed
  // This is probably expensive
  // Meant to seed PRNG hasher
  // Not needed if a static seed is configured
  #ifndef SassStaticHashSeed
  static uint32_t readHashSeed()
  {
    // Our hash seed
    uint32_t seed = 0;
    // Load optional fixed seed from environment
    // Mainly used to pass the seed to plugins
    if (const char* envseed = GET_ENV("SASS_HASH_SEED")) {
      seed = std::atol(envseed);
    }
    else {
      #ifdef SassStaticHashSeed
      seed = SassStaticHashSeed;
      #elif defined(USE_WIN_CRYPT)
      // Get seed via MS API
      HCRYPTPROV hp = 0;
      CryptAcquireContext(&hp, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
      CryptGenRandom(hp, sizeof(seed), (BYTE*)&seed);
      CryptReleaseContext(hp, 0);
      #else
      // Try to get random number from system
      try {
        // Get seed via C++11 API
        std::random_device rd;
        seed = rd();
      }
      // On certain system this can throw since either
      // underlying hardware or software can be buggy.
      // https://github.com/sass/libsass/issues/3151
      catch (std::exception&) {
      }
      // Don't trust anyone to be random, so we
      // add a little entropy of our own.
      seed ^= std::time(NULL) ^ std::clock() ^
        std::hash<std::thread::id>{}
        (std::this_thread::get_id());
      // Return entropy
      #endif
    }
    // Use some sensible default
    if (seed == 0) {
      // Fibonacci/Golden Ratio Hashing
      seed = 0x9e3779b9;
    }
    // Return the seed
    return seed;
  }
  #endif
  // EO readHashSeed

  // Just a static wrapped around random device
  // Creates one true random number to seed us
  uint32_t getHashSeed(uint32_t* preset)
  {
    // Optionally use static seed
    #ifdef SassStaticHashSeed
    (void)preset; // unused with a static seed
    return SassStaticHashSeed;
    #else
    // Initialization of a function local static is thread safe. The value
    // is atomic, since a preset (e.g. from a plugin) may overwrite it while
    // other threads read it (relaxed is enough, it's a single value).
    static std::atomic<uint32_t> seed(preset
      ? *preset : readHashSeed());
    if (preset) seed.store(*preset, std::memory_order_relaxed);
    return seed.load(std::memory_order_relaxed);
    #endif
  }
  // EO getHashSeed

  // Random number generator only needed in eval phase
  // This makes it safe to reset the hash seed before.
  // There is one generator for the whole process (seeded once), which
  // keeps the sequence reproducible for a given seed. Threads compiling
  // at the same time share it, so every use must hold `getRngMutex()`:
  // `std::mt19937` changes its state on every call (unsynchronized use
  // was a data race, e.g. `unique-id()` returned duplicates).
  static std::mt19937& getRng()
  {
    static std::mt19937 rng(getHashSeed());
    return rng;
  }
  // EO getRng

  // Allocated once and never destroyed (no static destruction order)
  static std::mutex& getRngMutex()
  {
    static std::mutex* mutex = new std::mutex();
    return *mutex;
  }
  // EO getRngMutex

  // Create a random `int` between [low] and [high]
  int getRandomInt(int low, int high)
  {
    std::uniform_int_distribution<int> urd(low, high);
    std::lock_guard<std::mutex> lock(getRngMutex());
    return urd(getRng());
  }
  // EO getRandomInt

  // Create a random `float` between [low] and [high]
  float getRandomFloat(float low, float high)
  {
    std::uniform_real_distribution<float> urd(low, high);
    std::lock_guard<std::mutex> lock(getRngMutex());
    return urd(getRng());
  }
  // EO getRandomFloat

  // Create a random `double` between [low] and [high]
  double getRandomDouble(double low, double high)
  {
    std::uniform_real_distribution<double> urd(low, high);
    std::lock_guard<std::mutex> lock(getRngMutex());
    return urd(getRng());
  }
  // EO getRandomDouble

  // Get full 32bit random data
  uint32_t getRandomUint32()
  {
    std::lock_guard<std::mutex> lock(getRngMutex());
    return getRng()();
  }
  // EO getRandomUint32

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
