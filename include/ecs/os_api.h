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
 * @file os_api.h
 * @brief Operationg system abstractions.
 *
 * This file contains the operating system abstraction API. The flecs core 
 * library avoids OS/runtime specific API calls as much as possible. Instead it
 * provides an interface that can be implemented by applications.
 *
 * Examples for how to implement this interface can be found in the 
 * examples/os_api folder.
 */

#ifndef FLECS_OS_API_H
#define FLECS_OS_API_H

#include <string.h>
#include "ecs/api_defines.h"
#include "malloc.h"

/* Memory management */
#define ecs_os_malloc(size) Alloc(size)
#define ecs_os_free(ptr) Free(ptr)
#define ecs_os_calloc(size) AllocZeroed(size)
#define ecs_os_realloc(ptr, size) Realloc(ptr, size)

/* Strings */
// ONLY IMPLEMENT strdup IF NECESSARY
// #define ecs_os_strdup(str) ecs_os_api.strdup_(str)
#define ecs_os_strlen(str) (ecs_size_t) strlen(str)
#define ecs_os_strcmp(str1, str2) strcmp(str1, str2)
#define ecs_os_strncmp(str1, str2, num) strncmp(str1, str2, (size_t)(num))
#define ecs_os_memcmp(ptr1, ptr2, num) memcmp(ptr1, ptr2, (size_t)(num))
#define ecs_os_memcpy(ptr1, ptr2, num) memcpy(ptr1, ptr2, (size_t)(num))
#define ecs_os_memset(ptr, value, num) memset(ptr, value, (size_t)(num))
#define ecs_os_memmove(ptr, value, num) memmove(ptr, value, (size_t)(num))

#endif // FLECS_OS_API_H
