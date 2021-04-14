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

#include "global.h"
#include "ecs/bitset.h"
#include "ecs/os_api.h"

#define CHAR_BITS (8)

static uint32_t num_chunks(uint32_t length);

Bitset *ecs_bitset_new(uint32_t length) {
    if (length == 0) {
        return NULL;
    }

    uint32_t n_chunks = num_chunks(length);
    Bitset *bitset = ecs_os_calloc(sizeof(Bitset));

    bitset->length = length;
    bitset->chunks = ecs_os_calloc(n_chunks);
    return bitset;
}

void ecs_bitset_free(Bitset *bitset) {
    if (bitset) {
        ecs_os_free(bitset->chunks);
        ecs_os_free(bitset);
    }
}

void ecs_bitset_set(Bitset *bitset, uint32_t index) {
    if (index >= bitset->length) {
        return;
    }

    uint32_t chunk_idx = index / CHAR_BITS;
    bitset->chunks[chunk_idx] |= (1 << (index % CHAR_BITS));
}

uint8_t ecs_bitset_get(Bitset *bitset, uint32_t index) {
    if (index >= bitset->length) {
        return 0;
    }

    uint32_t chunk_idx = index / CHAR_BITS;
    return (bitset->chunks[chunk_idx] >> (index % CHAR_BITS)) & 1;
}

void ecs_bitset_clear(Bitset *bitset, uint32_t index) {
    if (index >= bitset->length) {
        return;
    }

    uint32_t chunk_idx = index / CHAR_BITS;
    bitset->chunks[chunk_idx] &= ~(1 << (index % CHAR_BITS));
}

void ecs_bitset_replace(Bitset **dest, Bitset **src) {
    if (dest == NULL || *dest == NULL || src == NULL || *src == NULL) {
        return NULL;
    }

    ecs_bitset_free(*dest);
    *dest = *src;
    *src = NULL;
}

Bitset *ecs_bitset_copy(Bitset *src) {
    if (src == NULL) {
        return NULL;
    }

    Bitset *result = ecs_bitset_new(src->length);
    uint32_t n_chunks = num_chunks(src->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        result->chunks[i] = src->chunks[i];
    }

    return result;
}

/* -- Logical operators -- */
void ecs_bitset_and(Bitset *first, Bitset *second, Bitset **result) {
    if (first->length != second->length) {
        ecs_bitset_free(*result);
        return;
    }

    Bitset *product = ecs_bitset_new(first->length);
    uint32_t n_chunks = num_chunks(first->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        product->chunks[i] = first->chunks[i] & second->chunks[i];
    }

    ecs_bitset_replace(result, &product);
}

void ecs_bitset_or(Bitset *first, Bitset *second, Bitset **result) {
    if (first->length != second->length) {
        ecs_bitset_free(*result);
        return;
    }

    Bitset *product = ecs_bitset_new(first->length);
    uint32_t n_chunks = num_chunks(first->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        product->chunks[i] = first->chunks[i] | second->chunks[i];
    }

    ecs_bitset_replace(result, &product);
}

void ecs_bitset_not(Bitset *bitset, Bitset **result) {
    Bitset *product = ecs_bitset_new(bitset->length);
    uint32_t n_chunks = num_chunks(bitset->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        product->chunks[i] = ~bitset->chunks[i];
    }

    ecs_bitset_replace(result, &product);
}

/* -- Helper methods -- */
static uint32_t num_chunks(uint32_t length)
{
    uint32_t ret = length / CHAR_BITS;
    if (length % CHAR_BITS != 0) {
        ret++;
    }
    return ret;
}
