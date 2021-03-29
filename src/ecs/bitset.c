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

/* -- Logical operators -- */
Bitset *ecs_bitset_and(Bitset *first, Bitset *second) {
    if (first->length != second->length) {
        return NULL;
    }

    Bitset *result = ecs_bitset_new(first->length);
    uint32_t n_chunks = num_chunks(first->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        result->chunks[i] = first->chunks[i] & second->chunks[i];
    }
    return result;
}

Bitset *ecs_bitset_or(Bitset *first, Bitset *second) {
    if (first->length != second->length) {
        return NULL;
    }

    Bitset *result = ecs_bitset_new(first->length);
    uint32_t n_chunks = num_chunks(first->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        result->chunks[i] = first->chunks[i] | second->chunks[i];
    }
    return result;
}

Bitset *ecs_bitset_not(Bitset *bitset) {
    Bitset *result = ecs_bitset_new(bitset->length);
    uint32_t n_chunks = num_chunks(bitset->length);
    for (uint32_t i = 0; i < n_chunks; i++) {
        result->chunks[i] = ~bitset->chunks[i];
    }
    return result;
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
