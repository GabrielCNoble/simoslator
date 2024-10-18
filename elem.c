#include "elem.h"
#include "dev.h"
#include "wire.h"

struct pool_t                   elem_elements[ELEM_TYPE_LAST];
struct dbvt_t                   elem_dbvt[ELEM_TYPE_LAST];
struct list_t                   elem_elements_in_box;

extern struct dev_desc_t        dev_device_descs[];
extern struct pool_t            dev_devices;

struct elem_funcs_t e_elem_funcs[] = {
    [ELEM_TYPE_DEVICE] = {
        .Update         = dev_UpdateDevice,
        .Translate      = dev_TranslateDevice,
        .Rotate         = dev_RotateDevice,
        .FlipVertical   = dev_FlipDeviceV,
        .FlipHorizontal = dev_FlipDeviceH
    },

    [ELEM_TYPE_SEGMENT] = {
        .Update         = w_UpdateWire,
        .Translate      = w_TranslateWire,
        .Rotate         = w_RotateWire,
        .FlipVertical   = w_FlipWireV,
        .FlipHorizontal = w_FlipWireH
    }
};

struct elem_t *elem_CreateElement(uint32_t type, void *base_object)
{
    struct elem_t *element = pool_AddElement(&elem_elements[type], NULL);
    element->base_object = base_object;
    element->type = type;
    element->selection_index = INVALID_LIST_INDEX;
    element->node = NULL;
    elem_UpdateElement(element);
    return element;
}

void elem_FreeElement(struct elem_t *element)
{
    pool_RemoveElement(&elem_elements[element->type], element->element_index);
}

void elem_DestroyElement(struct elem_t *element)
{
    switch(element->type)
    {
        case ELEM_TYPE_DEVICE:
        {
            struct dev_t *device = element->base_object;
            dev_DestroyDevice(device);
        }
        break;

        case ELEM_TYPE_SEGMENT:
        {
            struct wire_seg_t *segment = element->base_object;
            if(segment->base.element_index != INVALID_POOL_INDEX)
            {
                w_DisconnectSegment(segment);
                w_FreeSegment(segment);
            }
        }
        break;
    }
}

void elem_UpdateElement(struct elem_t *element)
{
    if(element->node == NULL)
    {
        element->node = dbvt_AllocNode(&elem_dbvt[element->type]);
        element->node->data = (uintptr_t)element;
    }

    struct dbvt_node_t *node = element->node;
    node->min.z = -0.1f;
    node->max.z = 0.1f;

    // switch(element->type)
    // {
    //     case ELEM_TYPE_DEVICE:
    //     {
    //         struct dev_t *device = element->base_object;
    //         struct dev_desc_t *dev_desc = dev_device_descs + device->type;

    //         dev_GetDeviceLocalBoxPosition(device, &node->min.xy, &node->max.xy);

    //         node->min.x += device->position.x;
    //         node->min.y += device->position.y;
    //         node->max.x += device->position.x;
    //         node->max.y += device->position.y;
    //         // for(uint32_t pin_index = 0; pin_index < dev_desc->pin_count; pin_index++)
    //         // {
    //         //     int32_t pin_min[2];
    //         //     int32_t pin_max[2];

    //         //     dev_GetDeviceLocalPinPosition(device, pin_index, pin_min);

    //         //     pin_max[0] = pin_min[0] + ELEM_DEVICE_PIN_PIXEL_WIDTH;
    //         //     pin_max[1] = pin_min[1] + ELEM_DEVICE_PIN_PIXEL_WIDTH;

    //         //     pin_min[0] -= ELEM_DEVICE_PIN_PIXEL_WIDTH;
    //         //     pin_min[1] -= ELEM_DEVICE_PIN_PIXEL_WIDTH;

    //         //     if(pin_min[0] < box_min[0])
    //         //     {
    //         //         box_min[0] = pin_min[0];
    //         //     }

    //         //     if(pin_min[1] < box_min[1])
    //         //     {
    //         //         box_min[1] = pin_min[1];
    //         //     }

    //         //     if(pin_max[0] > box_max[0])
    //         //     {
    //         //         box_max[0] = pin_max[0];
    //         //     }

    //         //     if(pin_max[1] > box_max[1])
    //         //     {
    //         //         box_max[1] = pin_max[1];
    //         //     }
    //         // }

    //         // element->size[0] = (box_max[0] - box_min[0]) / 2;
    //         // element->size[1] = (box_max[1] - box_min[1]) / 2;

    //         // box_min[0] += device->position[0];
    //         // box_min[1] += device->position[1];

    //         // box_max[0] += device->position[0];
    //         // box_max[1] += device->position[1];

    //         // element->position[0] = (box_max[0] + box_min[0]) / 2;
    //         // element->position[1] = (box_max[1] + box_min[1]) / 2;

    //         dev_UpdateDevice(device);

    //         device->selection_index = element->selection_index;
    //     }
    //     break;

    //     case ELEM_TYPE_SEGMENT:
    //     {
    //         struct wire_seg_t *segment = element->base_object;
    //         // node->min.x = FLT_MAX;
    //         // node->min.y = FLT_MAX;

    //         // node->max.x = -FLT_MAX;
    //         // node->max.y = -FLT_MAX;

    //         for(uint32_t index = 0; index < 2; index++)
    //         {
    //             float length;
    //             if(segment->ends[WIRE_SEG_END_INDEX].comps[index] > segment->ends[WIRE_SEG_START_INDEX].comps[index])
    //             {
    //                 node->max.comps[index] = (float)segment->ends[WIRE_SEG_END_INDEX].comps[index];
    //                 node->min.comps[index] = (float)segment->ends[WIRE_SEG_START_INDEX].comps[index];
    //                 // element->size[index] = segment->ends[WIRE_SEG_END_INDEX][index] - segment->ends[WIRE_SEG_START_INDEX][index];
    //                 // length = (float)segment->ends[WIRE_SEG_END_INDEX][index] - (float)segment->ends[WIRE_SEG_START_INDEX][index];
    //             }
    //             else
    //             {
    //                 // element->size[index] = segment->ends[WIRE_SEG_START_INDEX][index] - segment->ends[WIRE_SEG_END_INDEX][index];
    //                 // length = (float)segment->ends[WIRE_SEG_START_INDEX][index] - (float)segment->ends[WIRE_SEG_END_INDEX][index]; 
    //                 node->min.comps[index] = (float)segment->ends[WIRE_SEG_END_INDEX].comps[index];
    //                 node->max.comps[index] = (float)segment->ends[WIRE_SEG_START_INDEX].comps[index];
    //             }

    //             // length = length / 2.0f + ELEM_WIRE_PIXEL_WIDTH;

    //             // node->min.comps[index] = fminf(node->min.comps[index], -length);
    //             // node->max.comps[index] = fmaxf(node->max.comps[index], length);

    //             // element->size[index] = element->size[index] / 2 + ELEM_WIRE_PIXEL_WIDTH;
    //         }

    //         // element->position[0] = (segment->ends[WIRE_SEG_START_INDEX][0] + segment->ends[WIRE_SEG_END_INDEX][0]) / 2;
    //         // element->position[1] = (segment->ends[WIRE_SEG_START_INDEX][1] + segment->ends[WIRE_SEG_END_INDEX][1]) / 2;
    //         segment->selection_index = element->selection_index;
    //         segment->element = element;
    //     }
    //     break;
    // }

    e_elem_funcs[element->type].Update(element->base_object, element);
    dbvt_UpdateNode(&elem_dbvt[element->type], node);
}

void elem_GetTypedElementsInsideBox(uint32_t type, vec2_t *box_min, vec2_t *box_max, struct list_t *elements)
{
    // struct pool_t *element_list = &elem_elements[type];
    dbvt_BoxOnDbvtContents(&elem_dbvt[type], elements, &(vec3_t){box_min->x, box_min->y, -0.1f}, &(vec3_t){box_max->x, box_max->y, 0.1f});
    
    // elem_elements_in_box.cursor = 0;
    // for(uint32_t index = 0; index < element_list->cursor; index++)
    // {
    //     struct elem_t *element = pool_GetValidElement(element_list, index);

    //     if(element != NULL)
    //     {
    //         if(box_max[0] >= element->position[0] - element->size[0] && box_min[0] <= element->position[0] + element->size[0])
    //         {
    //             if(box_max[1] >= element->position[1] - element->size[1] && box_min[1] <= element->position[1] + element->size[1])
    //             {
    //                 list_AddElement(&elem_elements_in_box, &element);
    //             }
    //         }
    //     }
    // }
}

void elem_Translate(struct elem_t *element, ivec2_t *translation)
{
    e_elem_funcs[element->type].Translate(element->base_object, translation);
    elem_UpdateElement(element);
}

void elem_Rotate(struct elem_t *element, ivec2_t *pivot, uint32_t ccw)
{
    e_elem_funcs[element->type].Rotate(element->base_object, pivot, ccw);
    elem_UpdateElement(element);
}

void elem_FlipVertically(struct elem_t *element, ivec2_t *pivot)
{
    e_elem_funcs[element->type].FlipVertical(element->base_object, pivot);
    elem_UpdateElement(element);
}

void elem_FlipHorizontally(struct elem_t *element, ivec2_t *pivot)
{
    e_elem_funcs[element->type].FlipHorizontal(element->base_object, pivot);
    elem_UpdateElement(element);
}