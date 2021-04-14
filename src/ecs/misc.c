// MIT License
//
// Copyright(c) 2019 Sander Mertens
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
#include "ecs/api_support.h"
#include "ecs/api_defines.h"
#include "ecs/log.h"
#include "ecs/os_api.h"

int8_t ecs_to_i8(
    int64_t v)
{
    ecs_assert(v < INT8_MAX, ECS_INTERNAL_ERROR, NULL);
    return (int8_t)v;
}

int16_t ecs_to_i16(
    int64_t v)
{
    ecs_assert(v < INT16_MAX, ECS_INTERNAL_ERROR, NULL);
    return (int16_t)v;
}

uint32_t ecs_to_u32(
    uint64_t v)
{
    ecs_assert(v < UINT32_MAX, ECS_INTERNAL_ERROR, NULL);
    return (uint32_t)v;
}

size_t ecs_to_size_t(
    int64_t size)
{
    ecs_assert(size >= 0, ECS_INTERNAL_ERROR, NULL);
    return (size_t)size;
}

ecs_size_t ecs_from_size_t(
    size_t size)
{
    ecs_assert(size < INT32_MAX, ECS_INTERNAL_ERROR, NULL);
    return (ecs_size_t)size;
}

int32_t ecs_next_pow_of_2(
    int32_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

void *ecs_os_memdup(
    const void *src,
    ecs_size_t size)
{
    if (!src)
    {
        return NULL;
    }

    void *dst = ecs_os_malloc(size);
    ecs_assert(dst != NULL, ECS_OUT_OF_MEMORY, NULL);
    ecs_os_memcpy(dst, src, size);
    return dst;
}
