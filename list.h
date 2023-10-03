#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>

struct list_t
{
    size_t      elem_size;
    size_t      buffer_elem_count;
    size_t      buffer_count;
    size_t      cursor;
    void **     buffers;
    /* temporary element used during sorting, allocated from the first buffer */
    void *      temp_elem;
};

#define INVALID_LIST_INDEX 0xffffffffffffffff

#ifdef __cplusplus
extern "C"
{
#endif

struct list_t list_Create(size_t elem_size, size_t buffer_elem_count);

void list_Destroy(struct list_t *list);

uint64_t list_AddElement(struct list_t *list, void *element);

void *list_GetElement(struct list_t *list, uint64_t index);

void *list_GetValidElement(struct list_t *list, uint64_t index);

void list_RemoveElement(struct list_t *list, uint64_t index);

uint64_t list_ShiftAndInsertAt(struct list_t *list, uint64_t index, uint64_t count);

void list_RemoveAtAndShift(struct list_t *list, uint64_t index, uint64_t count);

void list_Qsort(struct list_t *list, int32_t (*predicate)(const void *a, const void *b));

void list_QsortRange(struct list_t *list, int32_t (*predicate)(const void *a, const void *b), int64_t left, int64_t right);

#ifdef __cplusplus
}
#endif


#endif