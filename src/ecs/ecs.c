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
#include "ecs/ecs.h"

/* -- Implementation -- */
EcsEntity ecs_new_w_type(
    EcsWorld *world)
{
    // For reference, look at corresponding definition in flecs.c
}

EcsEntity ecs_new_component(
    EcsWorld *world,
    EcsEntity e,
    const char *name,
    size_t size,
    size_t alignment)
{
    // For reference, look at corresponding definition in flecs.c
}

const void *ecs_get_w_entity(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity component)
{
    // For reference, look at corresponding definition in flecs.c
}

void *ecs_get_mut_w_entity(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity component,
    bool8 *is_added)
{
    // For reference, look at corresponding definition in flecs.c
}

EcsEntity ecs_set_ptr_w_entity(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity component,
    size_t size,
    const void *ptr)
{
    // For reference, look at corresponding definition in flecs.c
}

void ecs_add_type(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity type)
{
    // For reference, look at corresponding definition in flecs.c
}

void ecs_remove_type(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity type)
{
    // For reference, look at corresponding definition in flecs.c
}

void EcsDelete(
    EcsWorld *world,
    EcsEntity entity)
{
    // For reference, look at corresponding definition in flecs.c
}

void EcsClear(
    EcsWorld *world,
    EcsEntity entity)
{
    // For reference, look at corresponding definition in flecs.c
}

EcsEntity ecs_new_system(
    EcsWorld *world,
    EcsEntity e,
    const char *name,
    EcsEntity tag,
    const char *signature,
    EcsIteratorAction action)
{
    // For reference, look at corresponding definition in flecs.c
}