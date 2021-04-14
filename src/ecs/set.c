#include "global.h"
#include "ecs/map.h"
#include "ecs/os_api.h"
#include "ecs/set.h"

/** Adds an element. */
void *_ecs_set_add(
    ecs_set_t *set,
    ecs_set_key_t key)
{
    uint8_t empty = 1;
    ecs_map_set(set, key, &empty);
}

/** Returns true if the set contains an element. */
bool ecs_set_contains(
    ecs_set_t *set,
    ecs_set_key_t key)
{
    uint8_t *val = ecs_map_get(set, uint8_t, key);
    return val != NULL;
}

/** Returns true if the set is empty. */
bool ecs_set_empty(
    ecs_set_t *set)
{
    return ecs_set_count(set) == 0;
}

/** 
 * Obtain next element in set from iterator. Returns false if iteration is 
 * complete, otherwise true. 
 */
bool ecs_set_next(
    ecs_set_iter_t *iter,
    ecs_set_key_t *key)
{
    uint8_t *val = ecs_map_next(iter, uint8_t, key);
    return val != NULL;
}

void ecs_set_replace(
    ecs_set_t **dest,
    ecs_set_t **src)
{
    if (dest == NULL || *dest == NULL || src == NULL || *src == NULL) {
        return NULL;
    }

    ecs_set_free(*dest);
    *dest = *src;
    *src = NULL;
}

/** Returns the intersection of two sets. */
void ecs_set_intersection(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result)
{
    ecs_set_t *temp;
    // Iterate over the smaller set
    if (ecs_set_count(first) > ecs_set_count(second)) {
        SWAP(first, second, temp);
    }

    ecs_set_t *product = ecs_set_new(0);
    ecs_set_key_t key;
    ecs_set_each(first, key, {
        if (ecs_set_contains(second, key)) {
            ecs_set_add(product, key);
        }
    });

    ecs_set_replace(result, &product);
}

/** Returns the union of two sets. */
void ecs_set_union(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result)
{
    ecs_set_t *product = ecs_set_new(0);
    ecs_set_key_t key;
    ecs_set_each(first, key, {
        ecs_set_add(product, key);
    });
    ecs_set_each(second, key, {
        ecs_set_add(product, key);
    });

    ecs_set_replace(result, &product);
}

/** 
 * Returns the difference of two sets (elements in the first set that aren't 
 * in the second set). 
 */
void ecs_set_difference(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result)
{
    ecs_set_t *product = ecs_set_new(0);
    ecs_set_key_t key;
    ecs_set_each(first, key, {
        if (!ecs_set_contains(second, key)) {
            ecs_set_add(product, key);
        }
    });

    ecs_set_replace(result, &product);
}

/** 
 * Returns the symmetric difference of two sets (all items from both sets 
 * except items present in both). 
 */
void ecs_set_symmetric_difference(
    ecs_set_t *first,
    ecs_set_t *second,
    ecs_set_t **result)
{
    ecs_set_t *set_union = ecs_set_new(0);
    ecs_set_t *set_intersect = ecs_set_new(0);
    ecs_set_t *product = ecs_set_new(0);

    ecs_set_union(first, second, &set_union);
    ecs_set_intersection(first, second, &set_intersect);
    ecs_set_difference(set_union, set_intersect, &product);

    ecs_set_free(set_union);
    ecs_set_free(set_intersect);
    ecs_set_replace(result, &product);
}

bool ecs_set_equal(
    ecs_set_t *first,
    ecs_set_t *second)
{
    ecs_set_t *product = ecs_set_new(0);
    ecs_set_symmetric_difference(first, second, &product);

    int32_t count = ecs_set_count(product);
    ecs_set_free(product);
    return count == 0; 
}

/** Returns true if two sets share no elements. */
bool ecs_set_disjoint(
    ecs_set_t *first,
    ecs_set_t *second)
{
    ecs_set_t *product = ecs_set_new(0);
    ecs_set_intersection(first, second, &product);

    int32_t count = ecs_set_count(product);
    ecs_set_free(product);
    return count == 0;
}

/** 
 * Returns true if the first set is a subset of the second (all the elements 
 * in the first are in the second). 
 */
bool ecs_set_subset(
    ecs_set_t *first,
    ecs_set_t *second)
{
    ecs_set_t *acc = ecs_set_new(0);
    ecs_set_intersection(first, second, &acc);
    bool equal = ecs_set_equal(first, acc);

    ecs_set_free(acc);
    return equal;
}
