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
u32 gRngValueOld;
u32 gRng2Value;
u32 gRngTinyMT[4];

u16 RandomOld(void)
{
    gRngValueOld = 1103515245 * gRngValueOld + 24691;
    sRandCount++;
    return gRngValueOld >> 16;
}

void SeedRngOld(u16 seed)
{
    gRngValueOld = seed;
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
void period_certification(void)
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
        gRngTinyMT[1] ^= MAT1;
        gRngTinyMT[2] ^= MAT2;
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