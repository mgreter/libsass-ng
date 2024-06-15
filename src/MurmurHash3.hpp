/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* MurmurHash3 was written by Austin Appleby, and is placed in the public    */
/* domain. The author hereby disclaims copyright to this source code.        */
/*****************************************************************************/
/* LibSass only needs MurmurHash3 32bit, so we made this header only         */
/*****************************************************************************/
#ifndef SASS_MURMURHASH3_HPP
#define SASS_MURMURHASH3_HPP

/////////////////////////////////////////////////////////////////////////
// Platform-specific functions and macros
/////////////////////////////////////////////////////////////////////////

// Microsoft Visual Studio

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

// Other compilers

#else	// defined(_MSC_VER)

#include <stdint.h>

#endif // !defined(_MSC_VER)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

#define FORCE_INLINE	__forceinline

#include <stdlib.h>

#define ROTL32(x,y)	_rotl(x,y)

// Other compilers

#else	// defined(_MSC_VER)

#define	FORCE_INLINE inline __attribute__((always_inline))

inline uint32_t rotl32(uint32_t x, int8_t r)
{
  return (x << r) | (x >> (32 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)

#endif // !defined(_MSC_VER)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

FORCE_INLINE uint32_t MurmurHash3(const void* key, int len, uint32_t seed)
{
  const uint8_t* data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

  for (int i = -nblocks; i; i++)
  {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = ROTL32(k1, 15);
    k1 *= c2;

    h1 ^= k1;
    h1 = ROTL32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;
  }

  //----------
  // tail

  const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

  uint32_t k1 = 0;

  switch (len & 3)
  {
  case 3:
    k1 ^= tail[2] << 16;
    /* fall through */
  case 2:
    k1 ^= tail[1] << 8;
    /* fall through */
  case 1:
    k1 ^= tail[0];
    k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;

  return h1;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif // SASS_MURMURHASH3_HPP
