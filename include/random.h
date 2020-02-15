#ifndef GUARD_RANDOM_H
#define GUARD_RANDOM_H

extern u32 gRngValueOld;
extern u32 gRng2Value;
extern u32 gRngState[4];

//Returns a 16-bit pseudorandom number
u16 Random(void);
u16 Random2(void);
u16 RandomOld(void);

//Returns a 32-bit pseudorandom number
#define Random32() (Random() | (Random() << 16))

//Sets the initial seed value of the pseudorandom number generator
void SeedRng(u32 seed);
void SeedRng2(u16 seed);
void SeedRngOld(u16 seed);

void tinymt32_init(u32 seed);
void tinymt32_next_state(void);
u32 tinymt32_generate_uint32(void);
void period_certification(void);

#endif // GUARD_RANDOM_H