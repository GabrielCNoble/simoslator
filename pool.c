#include "pool.h" 
#include <stdlib.h>
#include <string.h>

struct pool_t pool_Create(size_t elem_size, size_t buffer_elem_count, size_t index_offset)
{
    struct pool_t pool = {};

    pool.elem_size = elem_size;
    pool.buffer_elem_count = buffer_elem_count;
    pool.index_offset = index_offset;
    pool.free_indices_top = INVALID_POOL_INDEX;

    return pool;
}

void pool_Destroy(struct pool_t *pool)
{
    if(pool != NULL)
    {
        if(pool->buffer_count > 0)
        {
            for(size_t index = 0; index < pool->buffer_count; index++)
            {
                free(pool->buffers[index]);
            }

            free(pool->buffers);
            free(pool->free_indices);
        }
    }
}

void pool_Reset(struct pool_t *pool)
{
    if(pool != NULL)
    {
        pool->cursor = 0;
        pool->free_indices_top = INVALID_POOL_INDEX;
    }
}

void *pool_AddElement(struct pool_t *pool, void *element)
{
    uint64_t index = INVALID_POOL_INDEX;

    void *element_address = NULL;

    if(pool != NULL)
    {
        if(pool->free_indices_top != INVALID_POOL_INDEX)
        {
            index = pool->free_indices[pool->free_indices_top];
            pool->free_indices_top--;
        }
        else
        {
            index = pool->cursor;
            pool->cursor++;

            size_t last_cursor = pool->buffer_elem_count * pool->buffer_count;

            if(index >= last_cursor)
            {
                void **buffers = calloc(pool->buffer_count + 1, sizeof(void *));
               
                if(pool->buffers != NULL)
                {
                    memcpy(buffers, pool->buffers, sizeof(void *) * pool->buffer_count);
                    free(pool->buffers);
                    free(pool->free_indices);
                }

                pool->buffers = buffers;
                pool->buffers[pool->buffer_count] = calloc(pool->elem_size, pool->buffer_elem_count);
                pool->buffer_count++;
                pool->free_indices = calloc(pool->buffer_count * pool->buffer_elem_count, sizeof(uint64_t));
            }
        }

        uint64_t buffer_index = index / pool->buffer_elem_count;
        uint64_t element_index = index % pool->buffer_elem_count;

        void *buffer = pool->buffers[buffer_index];
        element_address = (void *)((uintptr_t)buffer + (element_index * pool->elem_size));

        if(element != NULL)
        {
            memcpy(element_address, element, pool->elem_size);
        }

        uint64_t *index_address = (uint64_t *)((uintptr_t)element_address + pool->index_offset);
        *index_address = index;
    }

    return element_address;
}

void *pool_GetElement(struct pool_t *pool, uint64_t index)
{
    void *element = NULL;

    if(pool != NULL && index != INVALID_POOL_INDEX && index < pool->cursor)
    {
        uint64_t buffer_index = index / pool->buffer_elem_count;
        uint64_t element_index = index % pool->buffer_elem_count;

        void *buffer = pool->buffers[buffer_index];
        element = (void *)((uintptr_t)buffer + (element_index * pool->elem_size));
    }

    return element;
}

void *pool_GetValidElement(struct pool_t *pool, uint64_t index)
{
    void *element = pool_GetElement(pool, index);

    if(element != NULL)
    {
        uint64_t *element_index = (uint64_t *)((uintptr_t)element + pool->index_offset);

        if(*element_index == INVALID_POOL_INDEX)
        {
            element = NULL;
        }
    }

    return element;
}

void pool_RemoveElement(struct pool_t *pool, uint64_t index)
{
    void *element = pool_GetValidElement(pool, index);

    if(element)
    {
        uint64_t *element_index = (uint64_t *)((uintptr_t)element + pool->index_offset);
        *element_index = INVALID_POOL_INDEX;

        pool->free_indices_top++;
        pool->free_indices[pool->free_indices_top] = index;
    }
}


