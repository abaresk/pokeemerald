// MIT License
//
// Copyright(c) 2021 hondew
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

Bitset *ecs_bitset_copy(Bitset *src);

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
