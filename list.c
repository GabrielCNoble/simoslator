#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct list_t list_Create(size_t elem_size, size_t buffer_elem_count)
{
    struct list_t list = {};

    list.elem_size = elem_size;
    list.buffer_elem_count = buffer_elem_count;

    return list;
}

void list_Destroy(struct list_t *list)
{
    if(list != NULL && list->buffers != NULL)
    {
        void *first_buffer = (void *)((uintptr_t)list->buffers[0] - list->elem_size);
        free(first_buffer);

        for(size_t index = 1; index < list->buffer_count; index++)
        {
            free(list->buffers[index]);
        }

        free(list->buffers);
        list->buffers = NULL;
    }
}

uint64_t list_AddElement(struct list_t *list, void *element)
{
    uint64_t index = INVALID_LIST_INDEX;

    if(list != NULL)
    {
        index = list->cursor;
        list->cursor++;

        size_t last_cursor = list->buffer_elem_count * list->buffer_count;

        if(index >= last_cursor)
        {
            void **buffers = calloc(list->buffer_count + 1, sizeof(void *));
            
            if(list->buffers != NULL)
            {
                memcpy(buffers, list->buffers, sizeof(void *) * list->buffer_count);
                free(list->buffers);
            }

            list->buffers = buffers;
            void *new_buffer = NULL;

            if(list->buffer_count == 0)
            {
                /* first buffer allocated, so alloc some extra space for the temporary element used during sorting */
                new_buffer = calloc(list->elem_size, list->buffer_elem_count + 1);
                list->temp_elem = new_buffer;
                new_buffer = (void *)((uintptr_t)new_buffer + list->elem_size);
                
            }
            else
            {
                new_buffer = calloc(list->elem_size, list->buffer_elem_count);
            }

            list->buffers[list->buffer_count] = new_buffer;
            list->buffer_count++;
        }

        if(element != NULL)
        {
            uint64_t buffer_index = index / list->buffer_elem_count;
            uint64_t element_index = index % list->buffer_elem_count;

            void *buffer = list->buffers[buffer_index];
            void *element_address = (void *)((uintptr_t)buffer + (element_index * list->elem_size));
            memcpy(element_address, element, list->elem_size);
        }
    }

    return index;
}

void *list_GetElement(struct list_t *list, uint64_t index)
{
    void *element = NULL;

    if(list != NULL)
    {
        uint64_t buffer_index = index / list->buffer_elem_count;
        uint64_t element_index = index % list->buffer_elem_count;

        void *buffer = list->buffers[buffer_index];
        element = (void *)((uintptr_t)buffer + (element_index * list->elem_size));
    }

    return element;
}

void *list_GetValidElement(struct list_t *list, uint64_t index)
{
    void *element = NULL;

    if(list != NULL && index < list->cursor)
    {
        element = list_GetElement(list, index);
    }

    return element;
}

void list_RemoveElement(struct list_t *list, uint64_t index)
{
    if(list != NULL && index < list->cursor)
    {
        uint64_t buffer_index = index / list->buffer_elem_count;
        uint64_t element_index = index % list->buffer_elem_count;

        list->cursor--;

        if(index < list->cursor)
        {
            uint64_t last_buffer_index = list->buffer_count - 1;
            uint64_t last_element_index = list->cursor % list->buffer_elem_count;
            void *elem = (void *)((uintptr_t)list->buffers[buffer_index] + element_index * list->elem_size);
            void *last_elem = (void *)((uintptr_t)list->buffers[last_buffer_index] + last_element_index * list->elem_size);
            memcpy(elem, last_elem, list->elem_size);
        }
    }
}

uint64_t list_ShiftAndInsertAt(struct list_t *list, uint64_t index, uint64_t count)
{
    uint64_t prev_last_element = list->cursor - 1;

    for(uint64_t add_index = 0; add_index < count; add_index++)
    {
        list_AddElement(list, NULL);
    }

    uint64_t first_element = list->cursor - count;

    if(index < first_element)
    {
        first_element = index;
        uint64_t src_element = prev_last_element;
        uint64_t dst_element = list->cursor - 1;

        uint64_t dst_buffer_index = dst_element / list->buffer_elem_count;
        uint64_t dst_buffer_offset = dst_element % list->buffer_elem_count;

        uint64_t src_buffer_index = src_element / list->buffer_elem_count;
        uint64_t src_buffer_offset = src_element % list->buffer_elem_count;

        uint64_t total_move_size = 1 + (src_element - first_element);
        uint64_t buffer_size = list->elem_size * (list->buffer_elem_count - 1);
        uint64_t move_size;

        if(src_buffer_offset > dst_buffer_offset)
        {
            move_size = dst_buffer_offset;
        }
        else
        {
            move_size = src_buffer_offset;
        }

        do
        {
            if(move_size >= total_move_size)
            {
                move_size = total_move_size - 1;
            }

            src_buffer_offset -= move_size;
            dst_buffer_offset -= move_size;
            move_size++;
            total_move_size -= move_size;

            uintptr_t dst = (uintptr_t)list->buffers[dst_buffer_index] + dst_buffer_offset * list->elem_size;
            uintptr_t src = (uintptr_t)list->buffers[src_buffer_index] + src_buffer_offset * list->elem_size;
            
            memmove((void *)dst, (const void *)src, move_size * list->elem_size);

            if(dst_buffer_offset == src_buffer_offset)
            {
                src_buffer_index--;
                src_buffer_offset = list->buffer_elem_count - 1;
                dst_buffer_index--;
                dst_buffer_offset = list->buffer_elem_count - 1;
                move_size = src_buffer_offset;
            }
            else if(dst_buffer_offset == 0)
            {
                src_buffer_offset--;
                move_size = src_buffer_offset;
                dst_buffer_index--;
                dst_buffer_offset = list->buffer_elem_count - 1;
            }
            else if(src_buffer_offset == 0)
            {
                dst_buffer_offset--;
                move_size = dst_buffer_offset;
                src_buffer_index--;
                src_buffer_offset = list->buffer_elem_count - 1;
            }
            // else
            // {
            //     move_size = src_buffer_offset;
            //     src_buffer_offset = list->buffer_elem_count - 1;
            //     dst_buffer_offset = list->buffer_elem_count - 1;
            // }
        }
        while(total_move_size != 0);
    }

    return first_element;
}

void list_RemoveAtAndShift(struct list_t *list, uint64_t index, uint64_t count)
{
    // uint64_t prev_last_element = list->cursor - 1;

    // for(uint64_t add_index = 0; add_index < count; add_index++)
    // {
    //     list_AddElement(list, NULL);
    // }

    // uint64_t first_element = list->cursor - count;

    if(list->cursor > 0)
    {
        if(count > list->cursor)
        {
            count = list->cursor;   
        }
        else
        {
            uint64_t last_element = list->cursor - 1;

            if(index < last_element)
            {
                // uint64_t src_element = list->cursor - count;
                uint64_t src_element = index + count;
                uint64_t dst_element = index;

                uint64_t dst_buffer_index = dst_element / list->buffer_elem_count;
                uint64_t dst_buffer_offset = dst_element % list->buffer_elem_count;

                uint64_t src_buffer_index = src_element / list->buffer_elem_count;
                uint64_t src_buffer_offset = src_element % list->buffer_elem_count;

                uint64_t total_move_size = list->cursor - src_element;
                // uint64_t buffer_size = list->elem_size * (list->buffer_elem_count - 1);
                uint64_t move_size;

                if(src_buffer_offset > dst_buffer_offset)
                {
                    move_size = list->buffer_elem_count - src_buffer_offset;
                }
                else
                {
                    move_size = list->buffer_elem_count - dst_buffer_offset;
                }

                do
                {
                    if(move_size >= total_move_size)
                    {
                        move_size = total_move_size;
                    }

                    total_move_size -= move_size;

                    uintptr_t dst = (uintptr_t)list->buffers[dst_buffer_index] + dst_buffer_offset * list->elem_size;
                    uintptr_t src = (uintptr_t)list->buffers[src_buffer_index] + src_buffer_offset * list->elem_size;

                    src_buffer_offset += move_size;
                    dst_buffer_offset += move_size;
                    
                    memmove((void *)dst, (const void *)src, move_size * list->elem_size);

                    if(dst_buffer_offset == src_buffer_offset)
                    {
                        src_buffer_index++;
                        src_buffer_offset = 0;
                        dst_buffer_index++;
                        dst_buffer_offset = 0;
                        move_size = list->buffer_elem_count;
                    }
                    else if(src_buffer_offset >= list->buffer_elem_count)
                    {
                        move_size = list->buffer_elem_count - dst_buffer_offset;
                        src_buffer_index++;
                        src_buffer_offset = 0;
                    }
                    else
                    {
                        move_size = list->buffer_elem_count - src_buffer_offset;
                        dst_buffer_index++;
                        dst_buffer_offset = 0;
                    }
                }
                while(total_move_size != 0);
            }
        }

        list->cursor -= count;
    }
}

void list_Qsort(struct list_t *list, int32_t (*predicate)(const void *a, const void *b))
{
    list_QsortRange(list, predicate, 0, (int64_t)list->cursor - 1);
}

void list_QsortRange(struct list_t *list, int32_t (*predicate)(const void *a, const void *b), int64_t left, int64_t right)
{
    uint64_t middle_index = (right + left) / 2;
    int64_t cur_left = left;
    int64_t cur_right = right;

    void *middle_element = list_GetValidElement(list, middle_index);
    void *left_element = list_GetValidElement(list, cur_left);
    void *right_element = list_GetValidElement(list, cur_right);

    while(left_element != NULL && right_element != NULL)
    {
        while(cur_left < right && predicate(left_element, middle_element) < 0)
        {
            cur_left++;
            left_element = list_GetElement(list, cur_left);
        }
        
        while(cur_right > left && predicate(middle_element, right_element) < 0)
        {
            cur_right--;
            right_element = list_GetElement(list, cur_right);
        }
        
        if(cur_left > cur_right)
        {
            break;
        }

        memcpy(list->temp_elem, left_element, list->elem_size);
        memcpy(left_element, right_element, list->elem_size);
        memcpy(right_element, list->temp_elem, list->elem_size);

        cur_left++;
        cur_right--;

        left_element = list_GetValidElement(list, cur_left);
        right_element = list_GetValidElement(list, cur_right); 
    }

    if(left < cur_right)
    {
        list_QsortRange(list, predicate, left, cur_right);
    }

    if(cur_left < right)
    {
        list_QsortRange(list, predicate, cur_left, right);
    }
}