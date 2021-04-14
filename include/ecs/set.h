/**
 * @file set.h
 * @brief Set data structure.
 * 
 * Stores unique 64-bit integer keys. Built on top of the ecs_map_t data 
 * structure.
 */

#ifndef ECS_SET_H
#define ECS_SET_H

#include "api_defines.h"
#include "api_support.h"
#include "ecs/map.h"

typedef struct ecs_map_t ecs_set_t;
typedef ecs_map_key_t ecs_set_key_t;
typedef struct ecs_map_iter_t ecs_set_iter_t;

#define ecs_set_new(elem_count) (_ecs_map_new(sizeof(uint8_t), ECS_ALIGNOF(uint8_t), elem_count))
#define ecs_set_free(set) (ecs_map_free(set))
#define ecs_set_remove(set, key) (ecs_map_remove(set, key))
#define ecs_set_count(set) (ecs_map_count(set))
#define ecs_set_clear(set) (ecs_map_clear(set))
#define ecs_set_iter(set) (ecs_map_iter(set))
#define ecs_set_memory(set, allocd, used) (ecs_map_memory(set, allocd, used))

/** Adds an element. */
void* _ecs_set_add(
    ecs_set_t *set,
    ecs_set_key_t key);

#define ecs_set_add(set, key)\
    _ecs_set_add(set, (ecs_set_key_t)key);

/** Returns true if the set contains an element. */
bool ecs_set_contains(
    ecs_set_t *set,
    ecs_set_key_t key);

/** Returns true if the set is empty. */
bool ecs_set_empty(
    ecs_set_t *set);

/** Return iterator to set contents. */
ecs_set_iter_t ecs_set_iter(
    const ecs_set_t *set);

/** 
 * Obtain next element in set from iterator. Returns false if iteration is 
 * complete, otherwise true. 
 */
bool ecs_set_next(
    ecs_set_iter_t *iter,
    ecs_set_key_t *key);

/** Replaces one set for another. Frees dest set if it exists. */
void ecs_set_replace(
    ecs_set_t **dest,
    ecs_set_t **src);

#define ecs_set_each(set, key, ...)\
    {\
        ecs_set_iter_t it = ecs_set_iter(set);\
        ecs_set_key_t key;\
        (void)key;\
        while ((ecs_set_next(&it, &key))) {\
            __VA_ARGS__\
        }\
    }

/** Returns the intersection of two sets. */
void ecs_set_intersection(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result);

/** Returns the union of two sets. */
void ecs_set_union(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result);

/** 
 * Returns the difference of two sets (elements in the first set that aren't 
 * in the second set). 
 */
void ecs_set_difference(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result);

/** 
 * Returns the symmetric difference of two sets (all items from both sets 
 * except items present in both). 
 */
void ecs_set_symmetric_difference(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result);

/** Returns true if two sets share exactly the same elements. */
bool ecs_set_equal(
    ecs_set_t *first,
    ecs_set_t *second);

/** Returns true if two sets share no elements. */
bool ecs_set_disjoint(
    ecs_set_t *first,
    ecs_set_t *second);

/** 
 * Returns true if the first set is a subset of the second (all the elements 
 * in the first are in the second). 
 */
bool ecs_set_subset(
    ecs_set_t *first,
    ecs_set_t *second);

/** 
 * Returns true if the first set is a superset of the second (all the elements 
 * in the second are in the first). 
 */
#define ecs_set_superset(first, second) (ecs_set_subset(second, first))

#endif // ECS_SET_H
