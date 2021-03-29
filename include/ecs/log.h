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
