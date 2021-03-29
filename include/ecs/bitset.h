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
Bitset *ecs_bitset_and(Bitset *first, Bitset *second);

Bitset *ecs_bitset_or(Bitset *first, Bitset *second);

Bitset *ecs_bitset_not(Bitset *bitset);

#endif // ECS_BITSET_H
