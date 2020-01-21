#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

extern u32 gRngValue;
extern u32 gRng2Value;
extern struct Bit64 gRngXoshiro[4];
extern u32 gRngTinyMT[4];

//Returns a 16-bit pseudorandom number
u16 Random(void);
u16 Random2(void);
u16 RandomXoshiro(void);
u16 RandomTinyMT(void);

//Returns a 32-bit pseudorandom number
#define Random32() (RandomTinyMT() | (RandomTinyMT() << 16))

//Sets the initial seed value of the pseudorandom number generator
void SeedRng(u16 seed);
void SeedRng2(u16 seed);
void SeedRngXoshiro(u16 seed);
void SeedRngTinyMT(u32 seed);

struct Bit64
{
    u32 upper;
    u32 lower;
};

struct Bit64 xoshiroNext(void);

void tinymt32_init(u32 seed);
void tinymt32_next_state(void);
u32 tinymt32_generate_uint32(void);
void period_certification(void);

#endif // GUARD_RANDOM_H