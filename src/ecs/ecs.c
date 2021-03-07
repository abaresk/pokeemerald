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