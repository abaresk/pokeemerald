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

/**
 * @file log.h
 * @brief Internal logging API.
 *
 * Internal utility functions for tracing, warnings and errors. 
 */

#ifndef FLECS_LOG_H
#define FLECS_LOG_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/** Assert */
void _ecs_assert(
    bool condition,
    int32_t error_code,
    const char *param,
    const char *condition_str,
    const char *file,
    int32_t line);

#ifdef NDEBUG
#define ecs_assert(condition, error_code, param)
#else
#define ecs_assert(condition, error_code, param)\
    _ecs_assert(condition, error_code, param, #condition, __FILE__, __LINE__);\
    assert(condition)
#endif

const char *ecs_strerror(
    int32_t error_code);

#endif // FLECS_LOG_H
