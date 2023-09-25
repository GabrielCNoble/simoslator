#ifndef POOL_H
#define POOL_H

#include <stdint.h>
#include <stddef.h>

#define POOL_ELEMENT_NAME   element_index
#define POOL_ELEMENT        uint64_t POOL_ELEMENT_NAME
#define INVALID_POOL_INDEX  0xffffffffffffffff

struct pool_t
{
    size_t      elem_size;
    size_t      buffer_elem_count;
    size_t      index_offset;
    size_t      buffer_count;

    uint64_t    cursor;
    uint64_t    free_indices_top;
    uint64_t *  free_indices;

    void **     buffers;
};

struct pool_t pool_Create(size_t elem_size, size_t buffer_elem_count, size_t index_offset);

#define pool_CreateTyped(type, buffer_elem_count) pool_Create(sizeof(type), buffer_elem_count, offsetof(type, POOL_ELEMENT_NAME))

void pool_Destroy(struct pool_t *pool);

void *pool_AddElement(struct pool_t *pool, void *element);

void *pool_GetElement(struct pool_t *pool, uint64_t index);

void *pool_GetValidElement(struct pool_t *pool, uint64_t index);

void pool_RemoveElement(struct pool_t *pool, uint64_t index);


#endif