/**
 * @file bitset.h
 * @brief Bitset data structure.
 */

#ifndef ECS_BITSET_H
#define ECS_BITSET_H

#include "api_defines.h"

typedef struct {
    uint32_t length;
    uint8_t *chunks;
} Bitset;

Bitset *ecs_bitset_new(uint32_t length);

void ecs_bitset_free(Bitset *bitset);

void ecs_bitset_set(Bitset *bitset, uint32_t index);

uint8_t ecs_bitset_get(Bitset *bitset, uint32_t index);

void ecs_bitset_clear(Bitset *bitset, uint32_t index);

// Replaces one bitset for another
void ecs_bitset_replace(Bitset **dest, Bitset **src);

/* -- Logical operators -- */

// Performs a logical AND on two Bitset operands and stores the result in
// `result`. Frees any pre-exisiting resources used by `result`.
//
// You can chain AND operations as follows:
// Bitset *acc = ecs_bitset_new(length);
// Bitset *second = ecs_bitset_new(length);
// ecs_bitset_and(acc, second, &acc);
void ecs_bitset_and(Bitset *first, Bitset *second, Bitset **result);

// Performs a logical OR on two Bitset operands and stores the result in
// `result`. Frees any pre-exisiting resources used by `result`.
//
// You can chain OR operations as follows:
// Bitset *acc = ecs_bitset_new(length);
// Bitset *second = ecs_bitset_new(length);
// ecs_bitset_or(acc, second, &acc);
void ecs_bitset_or(Bitset *first, Bitset *second, Bitset **result);

// Performs a logical NOT on a Bitset operand and stores the result in
// `result`. Frees any pre-exisiting resources used by `result`.
//
// You can chain NOT operations as follows:
// Bitset *acc = ecs_bitset_new(length);
// ecs_bitset_not(acc, &acc);
void ecs_bitset_not(Bitset *bitset, Bitset **result);

#endif // ECS_BITSET_H
