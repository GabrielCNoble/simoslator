#include "obj.h"
#include "dev.h"
#include "wire.h"

struct pool_t                   obj_objects[OBJECT_TYPE_LAST];
struct list_t                   obj_objects_in_box;

extern struct dev_desc_t        dev_device_descs[];
extern struct pool_t            dev_devices;

struct obj_t *obj_CreateObject(uint32_t type, void *base_object)
{
    struct obj_t *object = pool_AddElement(&obj_objects[type], NULL);
    object->base_object = base_object;
    object->type = type;
    object->selection_index = INVALID_LIST_INDEX;
    obj_UpdateObject(object);
    return object;
}

void obj_FreeObject(struct obj_t *object)
{
    pool_RemoveElement(&obj_objects[object->type], object->element_index);
}

void obj_DestroyObject(struct obj_t *object)
{
    switch(object->type)
    {
        case OBJECT_TYPE_DEVICE:
        {
            struct dev_t *device = object->base_object;
            dev_DestroyDevice(device);
        }
        break;

        case OBJECT_TYPE_SEGMENT:
        {
            struct wire_seg_t *segment = object->base_object;
            if(segment->base.element_index != INVALID_POOL_INDEX)
            {
                w_DisconnectSegment(segment);
                w_FreeWireSegment(segment);
            }
        }
        break;
    }
}

void obj_UpdateObject(struct obj_t *object)
{
    switch(object->type)
    {
        case OBJECT_TYPE_DEVICE:
        {
            struct dev_t *device = object->base_object;
            struct dev_desc_t *dev_desc = dev_device_descs + device->type;
            int32_t box_min[2];
            int32_t box_max[2];
            dev_GetDeviceLocalBoxPosition(device, box_min, box_max);

            for(uint32_t pin_index = 0; pin_index < dev_desc->pin_count; pin_index++)
            {
                int32_t pin_min[2];
                int32_t pin_max[2];

                dev_GetDeviceLocalPinPosition(device, pin_index, pin_min);

                pin_max[0] = pin_min[0] + OBJ_DEVICE_PIN_PIXEL_WIDTH;
                pin_max[1] = pin_min[1] + OBJ_DEVICE_PIN_PIXEL_WIDTH;

                pin_min[0] -= OBJ_DEVICE_PIN_PIXEL_WIDTH;
                pin_min[1] -= OBJ_DEVICE_PIN_PIXEL_WIDTH;

                if(pin_min[0] < box_min[0])
                {
                    box_min[0] = pin_min[0];
                }

                if(pin_min[1] < box_min[1])
                {
                    box_min[1] = pin_min[1];
                }

                if(pin_max[0] > box_max[0])
                {
                    box_max[0] = pin_max[0];
                }

                if(pin_max[1] > box_max[1])
                {
                    box_max[1] = pin_max[1];
                }
            }

            object->size[0] = (box_max[0] - box_min[0]) / 2;
            object->size[1] = (box_max[1] - box_min[1]) / 2;

            box_min[0] += device->position[0];
            box_min[1] += device->position[1];

            box_max[0] += device->position[0];
            box_max[1] += device->position[1];

            object->position[0] = (box_max[0] + box_min[0]) / 2;
            object->position[1] = (box_max[1] + box_min[1]) / 2;

            device->selection_index = object->selection_index;
        }
        break;

        case OBJECT_TYPE_SEGMENT:
        {
            struct wire_seg_t *segment = object->base_object;
            for(uint32_t index = 0; index < 2; index++)
            {
                if(segment->ends[WIRE_SEG_END_INDEX][index] > segment->ends[WIRE_SEG_START_INDEX][index])
                {
                    object->size[index] = segment->ends[WIRE_SEG_END_INDEX][index] - segment->ends[WIRE_SEG_START_INDEX][index]; 
                }
                else
                {
                    object->size[index] = segment->ends[WIRE_SEG_START_INDEX][index] - segment->ends[WIRE_SEG_END_INDEX][index]; 
                }

                object->size[index] = object->size[index] / 2 + OBJ_WIRE_PIXEL_WIDTH;
            }

            object->position[0] = (segment->ends[WIRE_SEG_START_INDEX][0] + segment->ends[WIRE_SEG_END_INDEX][0]) / 2;
            object->position[1] = (segment->ends[WIRE_SEG_START_INDEX][1] + segment->ends[WIRE_SEG_END_INDEX][1]) / 2;
            segment->selection_index = object->selection_index;
            segment->object = object;
        }
        break;
    }
}

void obj_GetTypedObjectsInsideBox(uint32_t type, int32_t *box_min, int32_t *box_max)
{
    struct pool_t *object_list = &obj_objects[type];
    obj_objects_in_box.cursor = 0;

    for(uint32_t index = 0; index < object_list->cursor; index++)
    {
        struct obj_t *object = pool_GetValidElement(object_list, index);

        if(object != NULL)
        {
            if(box_max[0] >= object->position[0] - object->size[0] && box_min[0] <= object->position[0] + object->size[0])
            {
                if(box_max[1] >= object->position[1] - object->size[1] && box_min[1] <= object->position[1] + object->size[1])
                {
                    list_AddElement(&obj_objects_in_box, &object);
                }
            }
        }
    }
}