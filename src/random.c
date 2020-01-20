#include "global.h"
#include "random.h"

#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000

// The number 1103515245 comes from the example implementation of rand and srand
// in the ISO C standard.

EWRAM_DATA static u8 sUnknown = 0;
EWRAM_DATA static u32 sRandCount = 0;

// IWRAM common
u32 gRngValue;
u32 gRng2Value;
struct Bit64 gRngXoshiro[4];
u32 gRngTinyMT[4];

// Changes will be made here
u16 Random(void)
{
    gRngValue = 1103515245 * gRngValue + 24691;
    sRandCount++;
    return gRngValue >> 16;
}

void SeedRng(u16 seed)
{
    gRngValue = seed;
    sUnknown = 0;
}

void SeedRng2(u16 seed)
{
    gRng2Value = seed;
}

u16 Random2(void)
{
    gRng2Value = 1103515245 * gRng2Value + 24691;
    return gRng2Value >> 16;
}

void SeedRngXoshiro(u16 seed)
{
    return;
}

u16 RandomXoshiro(void)
{
    struct Bit64 val;
    val = xoshiroNext();
    return val.upper >> 16;
}

void SeedRngTinyMT(u32 seed)
{
    tinymt32_init(seed);
}

u16 RandomTinyMT(void)
{
    u32 val;
    val = tinymt32_generate_uint32();
    return val >> 16;
}

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

// #include <stdint.h>

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

struct Bit64 rotl(struct Bit64 x, u32 k) {
    struct Bit64 z;
    u32 hi_shift;
    u32 lo_shift;
    u32 hi_rotated;
    u32 lo_rotated;

    hi_shift = x.upper << k;
    hi_rotated = x.upper >> (32 - k);

    lo_shift = x.lower << k;
    lo_rotated = x.lower >> (32 - k);

    z.upper = hi_shift | lo_rotated;
    z.lower = lo_shift | hi_rotated;
    return z;
}

struct Bit64 leftShift(struct Bit64 x, u32 k)
{
    struct Bit64 z;
    u32 hi_shift;
    u32 lo_shift;
    u32 lo_rotated;

    hi_shift = x.upper << k;

    lo_shift = x.lower << k;
    lo_rotated = x.lower >> (32 - k);

    z.upper = hi_shift | lo_rotated;
    z.lower = lo_shift;
    return z;
}

struct Bit64 rightShift(struct Bit64 x, u32 k)
{
    struct Bit64 z;
    u32 hi_shift;
    u32 lo_shift;
    u32 hi_rotated;

    hi_shift = x.upper >> k;

    lo_shift = x.lower >> k;
    hi_rotated = x.upper << (32 - k);

    z.upper = hi_shift;
    z.lower = lo_shift | hi_rotated;
    return z;
}

struct Bit64 add(struct Bit64 x, struct Bit64 y)
{
    struct Bit64 z;
    u32 carry;

    z.lower = x.lower + y.lower;
    carry = (z.lower < x.lower && z.lower < y.lower ) ? 1 : 0;
    z.upper = x.upper + y.upper + carry;
    return z;
}

struct Bit64 mult(struct Bit64 x, struct Bit64 y)
{
    // Break up 64-bit int's into 4 16-bit components:

    //   64B (x)
    // * 64B (y)
    // -------
    // =
    //   |         |         |         |         | 16B (a) | 16B (b) | 16B (c) | 16B (d) |
    // * |         |         |         |         | 16B (s) | 16B (t) | 16B (u) | 16B (v) |
    // -----------------------------------------------------------------------------------
    // +                                                             [       d * v       ] (32-bit partial product)
    // +                                                   [       c * v       ]
    // +                                                   [       d * u       ]
    // +                                         [       b * v       ]
    // +                                         [       c * u       ]
    // +                                         [       d * t       ]
    // +                               [       a * v       ]
    // +                               [       b * u       ]
    // +                               [       c * t       ]
    // +                               [       d * s       ]
    // +                     [       a * u       ]
    // +                     [       b * t       ]
    // +                     [       c * s       ]
    // +           [       a * t       ]
    // +           [       b * s       ]
    // + [       a * s       ]
    // -----------------------------------------------------------------------------------
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    u32 s;
    u32 t;
    u32 u;
    u32 v;
    struct Bit64 dv;
    struct Bit64 cv;
    struct Bit64 du;
    struct Bit64 bv;
    struct Bit64 cu;
    struct Bit64 dt;
    struct Bit64 av;
    struct Bit64 bu;
    struct Bit64 ct;
    struct Bit64 ds;
    struct Bit64 z;

    a = x.upper >> 16;
    b = (u32)((u16)(x.upper));
    c = x.lower >> 16;
    d = (u32)((u16)(x.lower));

    s = y.upper >> 16;
    t = (u32)((u16)(y.upper));
    u = y.lower >> 16;
    v = (u32)((u16)(y.lower));

    dv.lower = d * v;
    
    cv.upper = (c * v) >> 16; 
    cv.lower = (u32)((u16)(c * v));

    du.upper = (d * u) >> 16;
    du.lower = (u32)((u16)(d * u));

    bv.upper = b * v;
    cu.upper = c * u;
    dt.upper = d * t;

    av.upper = (u32)((u16)(a * v));
    bu.upper = (u32)((u16)(b * u));
    ct.upper = (u32)((u16)(c * t));
    ds.upper = (u32)((u16)(d * s));

    z = add(dv, cv);
    z = add(z, du);
    z = add(z, bv);
    z = add(z, cu);
    z = add(z, dt);
    z = add(z, av);
    z = add(z, bu);
    z = add(z, ct);
    z = add(z, ds);
    return z;
}

struct Bit64 xor(struct Bit64 x, struct Bit64 y)
{
    struct Bit64 z;
    z.upper = x.upper ^ y.upper;
    z.lower = x.lower ^ z.lower;
    return z;
}

// // Given any non-random 64bit int, generate a random 64bit int.
// // Used to seed our Xoroshiro Rng.
// struct Bit64 genBit64(struct Bit64 x) {
//     struct Bit64 a = 
//     {
//         .upper = 2654435769,
//         .lower = 2135587861
//     };

//     struct Bit64 b =
//     {
//         .upper = 3210233709,
//         .lower = 484763065
//     };

//     struct Bit64 c =
//     {
//         .upper = 2496678331,
//         .lower = 321982955
//     };

//     struct Bit64 z = add(x, a);
//     return z;
// }

struct Bit64 xoshiroNext(void)
{
    // struct Bit64 *s = &gRngXoshiro;

    // const uint64_t result = rotl(s[0] + s[3], 23) + s[0];
    struct Bit64 sum = add(gRngXoshiro[0], gRngXoshiro[3]);
    struct Bit64 rot_sum = rotl(sum, 23);
    struct Bit64 result = add(rot_sum, gRngXoshiro[0]);

    // const uint64_t t = s[1] << 17;
    struct Bit64 t = leftShift(gRngXoshiro[1], 17);

    // s[2] ^= s[0];
    // s[3] ^= s[1];
    // s[1] ^= s[2];
    // s[0] ^= s[3];
    gRngXoshiro[2] = xor(gRngXoshiro[2], gRngXoshiro[0]);
    gRngXoshiro[3] = xor(gRngXoshiro[3], gRngXoshiro[1]);
    gRngXoshiro[1] = xor(gRngXoshiro[1], gRngXoshiro[2]);
    gRngXoshiro[0] = xor(gRngXoshiro[0], gRngXoshiro[3]);

    // s[2] ^= t;
    gRngXoshiro[2] = xor(gRngXoshiro[2], t);

    // s[3] = rotl(s[3], 45);
    gRngXoshiro[3] = rotl(gRngXoshiro[3], 45);

    return result;
}

/**
 * @file tinymt32.c
 *
 * @brief Tiny Mersenne Twister only 127 bit internal state
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2011 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and The University of Tokyo.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 */
#define MIN_LOOP 8
#define PRE_LOOP 8

#define MAT1 0x8f7011ee
#define MAT2 0xfc78ff1f
#define TMAT 0x3793fdff

#define TINYMT32_MASK 0x7FFFFFFF
#define TINYMT32_SH0 1
#define TINYMT32_SH1 10
#define TINYMT32_SH8 8

/**
 * This function certificate the period of 2^127-1.
 * @param random tinymt state vector.
 */
static void period_certification(void)
{
    if ((gRngTinyMT[0] & TINYMT32_MASK) == 0 &&
        gRngTinyMT[1] == 0 &&
        gRngTinyMT[2] == 0 &&
        gRngTinyMT[3] == 0)
    {
        gRngTinyMT[0] = 'T';
        gRngTinyMT[1] = 'I';
        gRngTinyMT[2] = 'N';
        gRngTinyMT[3] = 'Y';
    }
}

/**
 * This function initializes the internal state array with a 32-bit
 * unsigned integer seed.
 * @param random tinymt state vector.
 * @param seed a 32-bit unsigned integer used as a seed.
 */
void tinymt32_init(u32 seed)
{
    u16 i;

    gRngTinyMT[0] = seed;
    gRngTinyMT[1] = MAT1;
    gRngTinyMT[2] = MAT2;
    gRngTinyMT[3] = TMAT;

    for (i = 1; i < MIN_LOOP; i++)
    {
        gRngTinyMT[i & 3] ^= i + 1812433253 * (gRngTinyMT[(i - 1) & 3] ^ (gRngTinyMT[(i - 1) & 3] >> 30));
    }
    period_certification();

    for (i = 0; i < PRE_LOOP; i++)
    {
        tinymt32_next_state();
    }
}

/**
 * This function changes internal state of tinymt32.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
void tinymt32_next_state(void)
{
    u32 x;
    u32 y;

    y = gRngTinyMT[3];
    x = (gRngTinyMT[0] & TINYMT32_MASK) ^ gRngTinyMT[1] ^ gRngTinyMT[2];
    x ^= (x << TINYMT32_SH0);
    y ^= (y >> TINYMT32_SH0) ^ x;
    gRngTinyMT[0] = gRngTinyMT[1];
    gRngTinyMT[1] = gRngTinyMT[2];
    gRngTinyMT[2] = x ^ (y << TINYMT32_SH1);
    gRngTinyMT[3] = y;
    if ((y & 1) == 1)
    {
        gRngTinyMT[1] = MAT1;
        gRngTinyMT[2] = MAT2;
    }
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return 32-bit unsigned pseudorandom number
 */
u32 tinymt32_temper(void)
{
    u32 t0, t1;

    t0 = gRngTinyMT[3];
    t1 = gRngTinyMT[0] + (gRngTinyMT[2] >> TINYMT32_SH8);
    t0 ^= t1;
    if ((t1 & 1) == 1) {
        t0 ^= TMAT;
    }
    return t0;
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 32-bit unsigned integer r (0 <= r < 2^32)
 */
u32 tinymt32_generate_uint32(void)
{
    tinymt32_next_state();
    return tinymt32_temper();
}