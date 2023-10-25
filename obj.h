#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>
#include "pool.h"
#include "list.h"

#define OBJ_DEVICE_PIN_PIXEL_WIDTH    8
#define OBJ_WIRE_PIXEL_WIDTH          4

enum OBJECT_TYPES
{
    OBJECT_TYPE_DEVICE,
    OBJECT_TYPE_SEGMENT,
    OBJECT_TYPE_JUNCTION,
    OBJECT_TYPE_LAST
};

/* forward declaration */
struct obj_t;

struct obj_link_t
{
    struct obj_t *         object;
    struct obj_link_t *    next;
    struct obj_link_t *    prev;
};

struct obj_t
{
    POOL_ELEMENT;
    void *                      base_object; 
    uint32_t                    type;
    int32_t                     position[2];
    int32_t                     size[2];
    struct m_object_link_t *    first_link;
    struct m_object_link_t *    last_link;
    uint64_t                    selection_index;
};

struct obj_t *obj_CreateObject(uint32_t type, void *base_object);

void obj_FreeObject(struct obj_t *object);

void obj_DestroyObject(struct obj_t *object);

void obj_UpdateObject(struct obj_t *object);

void obj_GetTypedObjectsInsideBox(uint32_t type, int32_t *box_min, int32_t *box_max);

#endif