#ifndef ELEM_H
#define ELEM_H

#include <stdint.h>
#include "pool.h"
#include "list.h"
#include "dbvt.h"
#include "maths.h"

#define ELEM_DEVICE_PIN_PIXEL_WIDTH    8
#define ELEM_WIRE_PIXEL_WIDTH          4

enum ELEM_TYPES
{
    ELEM_TYPE_DEVICE,
    ELEM_TYPE_SEGMENT,
    ELEM_TYPE_JUNCTION,
    ELEM_TYPE_LAST
};

/* forward declaration */
struct elem_t;

struct elem_funcs_t
{
    void (*Update)(void *base_elem, struct elem_t *elem);
    void (*Translate)(void *base_elem, ivec2_t *translation);
    void (*Rotate)(void *base_elem, ivec2_t *pivot, uint32_t ccw);
    void (*FlipVertical)(void *base_elem, ivec2_t *pivot);
    void (*FlipHorizontal)(void *base_elem, ivec2_t *pivot);
};

struct elem_link_t
{
    struct elem_t *         element;
    struct elem_link_t *    next;
    struct elem_link_t *    prev;
};

struct elem_t
{
    POOL_ELEMENT;
    void *                      base_object; 
    uint32_t                    type;
    struct dbvt_node_t *        node;
    ivec2_t                     position;
    // int32_t                     position[2];
    // int32_t                     size[2];
    struct m_object_link_t *    first_link;
    struct m_object_link_t *    last_link;
    uint64_t                    selection_index;
};

struct elem_create_args_t
{
    uint32_t type;

    union
    {
        struct
        {
            uint32_t type;
        } device;

        struct 
        {

        } segment;
    };
};

struct elem_t *elem_CreateElement(uint32_t type, void *base_object);

void elem_FreeElement(struct elem_t *element);

void elem_DestroyElement(struct elem_t *element);

void elem_UpdateElement(struct elem_t *element);

void elem_GetTypedElementsInsideBox(uint32_t type, vec2_t *box_min, vec2_t *box_max, struct list_t *elements);

void elem_Translate(struct elem_t *element, ivec2_t *translation);

void elem_Rotate(struct elem_t *element, ivec2_t *pivot, uint32_t ccw);

void elem_FlipVertically(struct elem_t *element, ivec2_t *pivot);

void elem_FlipHorizontally(struct elem_t *element, ivec2_t *pivot);

#endif