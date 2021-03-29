
#ifndef FLECS_MISC_H
#define FLECS_MISC_H

#include "global.h"
#include "ecs/api_defines.h"
#include <stdint.h>

int8_t ecs_to_i8(
    int64_t v);

int16_t ecs_to_i16(
    int64_t v);

uint32_t ecs_to_u32(
    uint64_t v);

size_t ecs_to_size_t(
    int64_t size);

ecs_size_t ecs_from_size_t(
    size_t size);

int32_t ecs_next_pow_of_2(
    int32_t n);

void *ecs_os_memdup(
    const void *src,
    ecs_size_t size);

#endif // FLECS_MISC_H