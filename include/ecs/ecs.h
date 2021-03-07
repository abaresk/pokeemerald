#ifndef GUARD_ECS_H
#define GUARD_ECS_H

#include "api_defines.h"

typedef struct {
    u8 val;
} EcsWorld;

typedef u16 EcsEntity;

/** Declare a component.
 * Example:
 *   ECS_COMPONENT(world, Position);
 */
#define ECS_COMPONENT(world, id)                                                        \
    ECS_ENTITY_VAR(id) = ecs_new_component(world, 0, #id, sizeof(id), ECS_ALIGNOF(id)); \
    // ECS_VECTOR_STACK(FLECS__T##id, EcsEntity, &FLECS__E##id, 1);                     \
    // (void)ecs_typeid(id);                                                               \
    // (void)ecs_type(id)

/* -- EcsEntity interface -- */

/** Create a new entity.
 * This operation creates a new entity with a single component in its type. This
 * operation accepts variables created with ECS_COMPONENT, ECS_TYPE and ECS_TAG.
 * This operation recycles ids.
 * 
 * @param world The world.
 * @param type The component type. (REMOVED)
 * @return The new entity.
 */
#define EcsNew(world, type) \
    ecs_new_w_type(world)

/** Get an immutable pointer to a component.
 * Same as ecs_get_w_entity, but accepts the typename of a component.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component to obtain.
 * @return The component pointer, NULL if the entity does not have the component.
 */
#define EcsGet(world, entity, component) \
    ((const component *)ecs_get_w_entity(world, entity, ecs_typeid(component)))

/** Get a mutable pointer to a component.
 * Same as ecs_get_mut_w_entity but accepts a component typename.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component to obtain.
 * @param is_added Out parameter that returns true if the component was added.
 * @return The component pointer.
 */
#define EcsGetMut(world, entity, component, is_added) \
    ((component *)ecs_get_mut_w_entity(world, entity, ecs_typeid(component), is_added))

/** Returns true if the entity has the component.
 * Same as ecs_get_w_entity, but accepts the typename of a component.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component to obtain.
 * @return boolean
 */
#define EcsHas(world, entity, component) \
    (const component *) comp = EcsGet(world, entity, component); \
    comp != NULL;

/** Add a component, type or tag to an entity.
 * This operation adds a type to an entity. The resulting type of the entity
 * will be the union of the previous type and the provided type. If the added
 * type did not have new components, this operation will have no side effects.
 *
 * This operation accepts variables declared by ECS_COMPONENT, ECS_TYPE and
 * ECS_TAG.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component, type or tag to add.
 */
#define EcsAdd(world, entity, component) \
    ecs_add_type(world, entity, ecs_type(component))

/** Set the value of a component.
 * Same as ecs_set_ptr, but accepts a value instead of a pointer to a value.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component to set.
 * @param size The size of the pointer to the value.
 * @return The entity. A new entity if no entity was provided.
 */
#define EcsSet(world, entity, component, ...) \
    ecs_set_ptr_w_entity(world, entity, ecs_typeid(component), sizeof(component), &(component)__VA_ARGS__)

/** Remove a component, type or tag from an entity.
 * This operation removes a type to an entity. The resulting type of the entity
 * will be the difference of the previous type and the provided type. If the 
 * type did not overlap with the entity type, this operation has no side effects.
 *
 * This operation accepts variables declared by ECS_COMPONENT, ECS_TYPE and
 * ECS_TAG.
 *
 * @param world The world.
 * @param entity The entity.
 * @param component The component, type or tag to remove.
 */
#define EcsRemove(world, entity, type) \
    ecs_remove_type(world, entity, ecs_type(type))

/** Delete an entity.
 * This operation will delete an entity and all of its components. The entity id
 * will be recycled. Repeatedly calling ecs_delete without ecs_new, 
 * ecs_new_w_entity or ecs_new_w_type will cause a memory leak as it will cause
 * the list with ids that can be recycled to grow unbounded.
 *
 * @param world The world.
 * @param entity The entity.
 */
void EcsDelete(
    EcsWorld *world,
    EcsEntity entity);

/** Clear all components.
 * This operation will clear all components from an entity but will not delete
 * the entity itself. This effectively prevents the entity id from being 
 * recycled.
 *
 * @param world The world.
 * @param entity The entity.
 */
void EcsClear(
    EcsWorld *world,
    EcsEntity entity);

/* -- System interface -- */

#define ECS_SYSTEM(world, name, kind, ...)                                                          \
    EcsIteratorAction ecs_iter_action(name) = name;                                                 \
    EcsEntity name = ecs_new_system(world, 0, #name, kind, #__VA_ARGS__, ecs_iter_action(name)); \
    (void)ecs_iter_action(name);                                                                    \
    (void)name;


/* -- System iterator -- */

/** The EcsIterator struct allows applications to iterate tables.
 * Queries and filters, among others, allow an application to iterate entities
 * that match a certain set of components. Because of how data is stored 
 * internally, entities with a given set of components may be stored in multiple
 * consecutive arrays, stored across multiple tables. The EcsIterator type 
 * enables iteration across tables. */
typedef struct
{
    // See flecs.h for full definiton.
    EcsWorld *world;      /**< The world */
    EcsEntity system;     /**< The current system (if applicable) */
} EcsIterator;

/** Declare entity variable */
#define ECS_ENTITY_VAR(id) \
    EcsEntity ecs_typeid(id)

/** Action callback for systems and triggers */
typedef void (*EcsIteratorAction)(
    EcsIterator *it);

#endif // GUARD_ECS_H