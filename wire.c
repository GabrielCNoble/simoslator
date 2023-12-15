#include <stdio.h>
#include "wire.h"
#include "list.h"
#include "dev.h"
#include "sim.h"
#include "obj.h"

uint8_t w_wire_value_resolution[WIRE_VALUE_LAST][WIRE_VALUE_LAST] = {
    [WIRE_VALUE_0S] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1W]     = WIRE_VALUE_0S,
        [WIRE_VALUE_Z]      = WIRE_VALUE_0S,
        [WIRE_VALUE_U]      = WIRE_VALUE_0S,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_0S,
    },

    [WIRE_VALUE_1S] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_1S,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1S,
        [WIRE_VALUE_Z]      = WIRE_VALUE_1S,
        [WIRE_VALUE_U]      = WIRE_VALUE_1S,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_1S,
    },

    [WIRE_VALUE_0W] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
        [WIRE_VALUE_1W]     = WIRE_VALUE_U,
        [WIRE_VALUE_Z]      = WIRE_VALUE_0W,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_0W,
    },

    [WIRE_VALUE_1W] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_U,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
        [WIRE_VALUE_Z]      = WIRE_VALUE_1W,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_1W,
    },

    [WIRE_VALUE_Z] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
        [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_Z,
    },

    [WIRE_VALUE_U] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_U,
        [WIRE_VALUE_1W]     = WIRE_VALUE_U,
        [WIRE_VALUE_Z]      = WIRE_VALUE_U,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_U,
    },

    [WIRE_VALUE_ERR] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_0W]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1W]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_Z]      = WIRE_VALUE_ERR,
        [WIRE_VALUE_U]      = WIRE_VALUE_ERR,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_IND] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
        [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
        [WIRE_VALUE_IND]    = WIRE_VALUE_IND,
    },
};

struct pool_t               w_wires;
struct pool_t               w_wire_seg_pos;
struct pool_t               w_wire_segs;
struct pool_t               w_wire_juncs;
uint64_t                    w_traversal_id;
uint64_t                    w_seg_junc_count;

extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    w_wire_seg_pos = pool_CreateTyped(struct wire_seg_pos_block_t, 16384);
    w_wire_segs = pool_Create(sizeof(struct wire_seg_t), 16384, offsetof(struct wire_seg_t, base.POOL_ELEMENT_NAME));
    w_wire_juncs = pool_Create(sizeof(struct wire_junc_t), 16384, offsetof(struct wire_junc_t, base.POOL_ELEMENT_NAME));
}

void w_Shutdown()
{
    pool_Destroy(&w_wires);
    pool_Destroy(&w_wire_seg_pos);
    pool_Destroy(&w_wire_segs);
    pool_Destroy(&w_wire_juncs);
}

struct wire_t *w_AllocWire()
{
    struct wire_t *wire = pool_AddElement(&w_wires, NULL);

    wire->first_junction = NULL;
    wire->last_junction = NULL;
    wire->first_segment = NULL;
    wire->last_segment = NULL;

    wire->segment_count = 0;
    wire->junction_count = 0;
    wire->input_count = 0;
    wire->output_count = 0;

    return wire;
}

struct wire_t *w_FreeWire(struct wire_t *wire)
{

}

void w_ClearWires()
{
    pool_Reset(&w_wires);
    pool_Reset(&w_wire_segs);
    pool_Reset(&w_wire_juncs);
    w_seg_junc_count = 0;
    w_traversal_id = 0;
}

struct wire_t *w_GetWire(uint32_t wire_index)
{
    return pool_GetValidElement(&w_wires, wire_index);
}

void w_LinkSegmentToWire(struct wire_t *wire, struct wire_seg_t *segment)
{
    if(wire != NULL && segment->base.wire != wire)
    {
        w_UnlinkSegmentFromWire(segment);

        segment->base.wire = wire;
        
        if(wire->first_segment == NULL)
        {
            wire->first_segment = segment;
        }
        else
        {
            wire->last_segment->wire_next = segment;
            segment->wire_prev = wire->last_segment;
        }

        wire->last_segment = segment;
        wire->segment_count++;
    }
}

void w_UnlinkSegmentFromWire(struct wire_seg_t *segment)
{
    struct wire_t *wire = segment->base.wire;

    if(wire != NULL)
    {
        segment->base.wire = NULL;

        if(wire->first_segment == segment)
        {
            wire->first_segment = segment->wire_next;

            if(wire->first_segment != NULL)
            {
                wire->first_segment->wire_prev = NULL;
            }
        }
        else
        {
            segment->wire_prev->wire_next = segment->wire_next;
        }

        if(wire->last_segment == segment)
        {
            wire->last_segment = segment->wire_prev;

            if(wire->last_segment != NULL)
            {
                wire->last_segment->wire_next = NULL;
            }
        }
        else
        {
            segment->wire_next->wire_prev = segment->wire_prev;
        }

        segment->wire_next = NULL;
        segment->wire_prev = NULL;

        wire->segment_count--;
    }
}

struct wire_seg_t *w_AllocWireSegment(struct wire_t *wire)
{
    struct wire_seg_t *segment = pool_AddElement(&w_wire_segs, NULL);
    segment->base.wire = NULL;
    segment->segments[0] = NULL;
    segment->segments[1] = NULL;
    segment->junctions[0] = (struct wire_seg_junc_t){};
    segment->junctions[1] = (struct wire_seg_junc_t){};
    segment->selection_index = 0xffffffff;
    segment->wire_next = NULL;
    segment->wire_prev = NULL;
    segment->traversal_id = 0;

    segment->object = obj_CreateObject(OBJECT_TYPE_SEGMENT, segment);
    w_LinkSegmentToWire(wire, segment);    

    return segment;
}

void w_FreeWireSegment(struct wire_seg_t *segment)
{
    struct wire_t *wire = segment->base.wire;
    obj_FreeObject(segment->object);
    w_UnlinkSegmentFromWire(segment);
    pool_RemoveElement(&w_wire_segs, segment->base.element_index);
}

void w_LinkJunctionToWire(struct wire_t *wire, struct wire_junc_t *junction)
{
    if(wire != NULL && junction->base.wire != wire)
    {
        struct wire_pin_t pin = junction->pin;
        w_UnlinkJunctionFromWire(junction);

        junction->base.wire = wire;

        if(wire->first_junction == NULL)
        {
            wire->first_junction = junction;
        }
        else
        {
            wire->last_junction->wire_next = junction;
            junction->wire_prev = wire->last_junction;
        }

        wire->last_junction = junction;
        wire->junction_count++;

        if(pin.device != DEV_INVALID_DEVICE)
        {
            struct dev_t *device = dev_GetDevice(pin.device);
            w_ConnectPinToJunction(junction, device, pin.pin);
        }
    }
}

void w_UnlinkJunctionFromWire(struct wire_junc_t *junction)
{
    struct wire_t *wire = junction->base.wire;

    if(wire != NULL)
    {
        w_DisconnectJunctionFromPin(junction);

        junction->base.wire = NULL;

        if(wire->first_junction == junction)
        {
            wire->first_junction = junction->wire_next;

            if(wire->first_junction != NULL)
            {
                wire->first_junction->wire_prev = NULL;
            }
        }
        else
        {
            junction->wire_prev->wire_next = junction->wire_next;
        }

        if(wire->last_junction == junction)
        {
            wire->last_junction = junction->wire_prev;

            if(wire->last_junction != NULL)
            {
                wire->last_junction->wire_next = NULL;
            }
        }
        else
        {
            junction->wire_next->wire_prev = junction->wire_prev;
        }

        junction->wire_next = NULL;
        junction->wire_prev = NULL;

        wire->junction_count--;
    }
}

struct wire_junc_t *w_AllocWireJunction(struct wire_t *wire)
{
    struct wire_junc_t *junction = pool_AddElement(&w_wire_juncs, NULL);
    junction->base.wire = NULL;
    junction->pin.device = DEV_INVALID_DEVICE;
    junction->pin.pin = DEV_INVALID_PIN;
    junction->first_segment = NULL;
    junction->last_segment = NULL;
    junction->selection_index = 0xffffffff;
    junction->traversal_id = 0;
    junction->wire_next = NULL;
    junction->wire_prev = NULL;
    junction->segment_count = 0;

    w_LinkJunctionToWire(wire, junction);

    return junction;
}

void w_FreeWireJunction(struct wire_junc_t *junction)
{
    if(junction->pin.device != DEV_INVALID_DEVICE)
    {
        w_DisconnectJunctionFromPin(junction);
    }

    w_UnlinkJunctionFromWire(junction);
    pool_RemoveElement(&w_wire_juncs, junction->base.element_index);
}

struct wire_junc_t *w_GetWireJunction(uint64_t junction_index)
{
    return pool_GetValidElement(&w_wire_juncs, junction_index);
}

void w_LinkSegmentToJunction(struct wire_seg_t *segment, struct wire_junc_t *junction, uint32_t link_index)
{
    struct wire_seg_junc_t *seg_junc = &segment->junctions[link_index];

    if(seg_junc->junction != NULL)
    {
        printf("w_LinkSegmentToJunction: segment %s is already is connected to a junction\n", (link_index == 0) ? "start" : "end");
        return;
    }

    seg_junc->junction = junction;

    if(junction->first_segment == NULL)
    {
        junction->first_segment = segment;
        junction->pos = segment->ends[link_index];
    }
    else
    { 
        junction->last_segment->junctions[junction->last_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].next = segment;
        seg_junc->prev = junction->last_segment;
    }
    junction->last_segment = segment;
    junction->segment_count++;
    printf("junction %d now has %d segments\n", junction->base.element_index, junction->segment_count);
    w_seg_junc_count++;
}

void w_UnlinkSegmentFromJunction(struct wire_seg_t *segment, struct wire_junc_t *junction)
{
    uint32_t link_index = segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
    w_UnlinkSegmentLinkIndex(segment, link_index);
}

void w_UnlinkSegmentLinkIndex(struct wire_seg_t *segment, uint32_t link_index)
{
    struct wire_seg_junc_t *seg_junc = &segment->junctions[link_index];

    if(seg_junc->junction == NULL)
    {
        printf("w_UnlinkSegmentLinkIndex: segment isn't linked at the %s to a junction\n", (link_index == WIRE_SEG_START_INDEX) ? "start" : "end");
        return;
    }

    struct wire_junc_t *junction = seg_junc->junction;

    if(junction->first_segment == segment)
    {
        junction->first_segment = seg_junc->next;
        if(junction->first_segment != NULL)
        {
            struct wire_seg_t *first_segment = junction->first_segment;
            first_segment->junctions[first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].prev = NULL;
            junction->pos = first_segment->ends[(first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction)];
        }
    }
    else
    {
        struct wire_seg_t *prev_segment = seg_junc->prev;
        prev_segment->junctions[prev_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].next = seg_junc->next;
    }

    if(junction->last_segment == segment)
    {
        junction->last_segment = seg_junc->prev;
        if(junction->last_segment != NULL)
        {
            struct wire_seg_t *last_segment = junction->last_segment;
            last_segment->junctions[last_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].next = NULL;
        }
    }
    else
    {
        struct wire_seg_t *next_segment = seg_junc->next;
        next_segment->junctions[next_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].prev = seg_junc->prev;
    }

    junction->segment_count--;
    printf("junction %d now has %d segments\n", junction->base.element_index, junction->segment_count);
    seg_junc->junction = NULL;
    seg_junc->next = NULL;
    seg_junc->prev = NULL;

    w_seg_junc_count--;
}

struct wire_junc_t *w_AddJunction(struct wire_seg_t *segment, int32_t *position)
{
    struct wire_t *wire = segment->base.wire;
    struct wire_junc_t *junction = NULL;
    uint32_t coord0;
    uint32_t coord1;

    coord0 = segment->ends[WIRE_SEG_START_INDEX][0] != segment->ends[WIRE_SEG_END_INDEX][0];
    coord1 = !coord0;

    if(position[coord0] == segment->ends[WIRE_SEG_START_INDEX][coord0])
    {
        uint32_t link_index;
        
        if(position[coord1] == segment->ends[WIRE_SEG_START_INDEX][coord1])
        {
            junction = w_AddJunctionAtTip(segment, WIRE_SEG_START_INDEX);
        }
        else if(position[coord1] == segment->ends[WIRE_SEG_END_INDEX][coord1])
        {
            junction = w_AddJunctionAtTip(segment, WIRE_SEG_END_INDEX);
        }
        else
        {
            junction = w_AddJunctionAtMiddle(segment, position);
        }
    }

    return junction;
}

struct wire_junc_t *w_AddJunctionAtMiddle(struct wire_seg_t *segment, int32_t *position)
{
    struct wire_junc_t *junction = w_AllocWireJunction(segment->base.wire);
    struct wire_seg_t *split = w_AllocWireSegment(segment->base.wire);
    split->ends[WIRE_SEG_START_INDEX][0] = position[0];
    split->ends[WIRE_SEG_START_INDEX][1] = position[1];
    split->ends[WIRE_SEG_END_INDEX][0] = segment->ends[WIRE_SEG_END_INDEX][0];
    split->ends[WIRE_SEG_END_INDEX][1] = segment->ends[WIRE_SEG_END_INDEX][1];
    segment->ends[WIRE_SEG_END_INDEX][0] = position[0];
    segment->ends[WIRE_SEG_END_INDEX][1] = position[1];

    split->segments[WIRE_SEG_END_INDEX] = segment->segments[WIRE_SEG_END_INDEX];
    obj_UpdateObject(split->object);
    obj_UpdateObject(segment->object);

    // list_AddElement(&w_created_segments, &split);
    // obj_CreateObject(OBJECT_TYPE_SEGMENT, split);
    
    if(segment->segments[WIRE_SEG_END_INDEX] != NULL)
    {
        struct wire_seg_t *next_segment = segment->segments[WIRE_SEG_END_INDEX];
        uint32_t next_segment_tip_index = next_segment->segments[WIRE_SEG_END_INDEX] == segment;
        next_segment->segments[next_segment_tip_index] = split;
    }

    segment->segments[WIRE_SEG_END_INDEX] = NULL;

    struct wire_junc_t *end_junction = segment->junctions[WIRE_SEG_END_INDEX].junction;
    if(end_junction != NULL)
    {
        w_UnlinkSegmentLinkIndex(segment, WIRE_SEG_END_INDEX);
        w_LinkSegmentToJunction(split, end_junction, WIRE_SEG_END_INDEX);
    }            

    w_LinkSegmentToJunction(segment, junction, WIRE_SEG_END_INDEX);
    w_LinkSegmentToJunction(split, junction, WIRE_SEG_START_INDEX);

    return junction;
}

struct wire_junc_t *w_AddJunctionAtTip(struct wire_seg_t *segment, uint32_t tip_index)
{
    struct wire_junc_t *junction = NULL;
    /* junction is to be created at the start of the segment */
    if(segment->junctions[tip_index].junction != NULL)
    {
        /* wire already has a junction at the start, so just return it */
        junction = segment->junctions[tip_index].junction;
    }
    else
    {
        junction = w_AllocWireJunction(segment->base.wire);

        if(segment->segments[tip_index] != NULL)
        {
            /* there's another segment after the current one, so link it to the 
            junction as well */
            struct wire_seg_t *other_segment = segment->segments[tip_index];
            uint32_t other_segment_tip_index = other_segment->segments[WIRE_SEG_END_INDEX] == segment;
            w_LinkSegmentToJunction(other_segment, junction, other_segment_tip_index);
            other_segment->segments[other_segment_tip_index] = NULL;
            segment->segments[tip_index] = NULL;
        }

        w_LinkSegmentToJunction(segment, junction, tip_index);
    }

    return junction;
}

struct wire_seg_t *w_RemoveJunction(struct wire_junc_t *junction)
{
    struct wire_seg_t *segment = NULL;

    if(junction->first_segment != NULL)
    {
        struct wire_seg_t *first_segment = junction->first_segment;
        uint32_t first_link_index = first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
        struct wire_seg_junc_t *seg_junc = &first_segment->junctions[first_link_index];
        struct wire_seg_t *second_segment = seg_junc->next;

        if(second_segment != NULL && junction->last_segment != second_segment)
        {
            // printf("can't remove junction with more than two segments\n");
            return NULL;
        }

        w_UnlinkSegmentLinkIndex(first_segment, first_link_index);

        if(second_segment != NULL)
        {
            uint32_t second_link_index = second_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
            w_UnlinkSegmentLinkIndex(second_segment, second_link_index);

            uint32_t coord0 = first_segment->ends[WIRE_SEG_START_INDEX][1] == first_segment->ends[WIRE_SEG_END_INDEX][1];
            uint32_t coord1 = second_segment->ends[WIRE_SEG_START_INDEX][1] == second_segment->ends[WIRE_SEG_END_INDEX][1];

            if(coord0 == coord1)
            {
                /* segments are collinear, so merge them */
                first_segment->ends[first_link_index][0] = second_segment->ends[!second_link_index][0];
                first_segment->ends[first_link_index][1] = second_segment->ends[!second_link_index][1];

                if(second_segment->junctions[!second_link_index].junction != NULL)
                {
                    /* second segment is connected to a junction, so disconnect it and connect the 
                    first one to it */
                    struct wire_junc_t *second_junction = second_segment->junctions[!second_link_index].junction;
                    w_UnlinkSegmentLinkIndex(second_segment, !second_link_index);
                    w_LinkSegmentToJunction(first_segment, second_junction, first_link_index);
                }
                else
                {
                    struct wire_seg_t *third_segment = second_segment->segments[!second_link_index];
                    first_segment->segments[first_link_index] = third_segment;
                    third_segment->segments[third_segment->segments[WIRE_SEG_END_INDEX] == second_segment] = first_segment;
                }

                // list_AddElement(&w_deleted_segments, &second_segment->object);
                struct obj_t *object = second_segment->object;
                w_FreeWireSegment(second_segment);
                // obj_DestroyObject(object);
            }
            else
            {
                /* segments aren't collinear, so just link them together */
                first_segment->segments[first_segment->segments[WIRE_SEG_END_INDEX] == NULL] = second_segment;
                second_segment->segments[second_segment->segments[WIRE_SEG_END_INDEX] == NULL] = first_segment;
            }
        }

        segment = first_segment;
    }

    if(junction->pin.device != DEV_INVALID_DEVICE)
    {
        w_DisconnectJunctionFromPin(junction);
    }

    w_FreeWireJunction(junction);

    return segment;
}

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b)
{
    if(wire_a != NULL && wire_b != NULL && wire_a != wire_b)
    {
        /* move segment stuff over */
        struct wire_seg_t *wire_b_segments = wire_b->first_segment;
        struct wire_seg_t *segment = wire_b_segments;
        while(segment != NULL)
        {
            segment->base.wire = wire_a;
            segment = segment->wire_next;
        }

        wire_a->last_segment->wire_next = wire_b_segments;
        wire_b_segments->wire_prev = wire_a->last_segment;
        wire_a->last_segment = wire_b->last_segment;
        wire_a->segment_count += wire_b->segment_count;


        /* move junction stuff over */
        struct wire_junc_t *wire_b_junctions = wire_b->first_junction;
        struct wire_junc_t *junction = wire_b_junctions;
        while(junction != NULL)
        {
            struct wire_seg_t *first_segment = junction->first_segment;
            uint32_t tip_index = first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
            junction->base.wire = wire_a;
            // junction->pos = first_segment->pos->ends[tip_index];

            if(junction->pin.device != DEV_INVALID_DEVICE)
            {
                struct dev_t *device = dev_GetDevice(junction->pin.device);
                struct dev_pin_t *pin = dev_GetDevicePin(device, junction->pin.pin);
                pin->junction = junction->base.element_index;
            }

            junction = junction->wire_next;
        }

        wire_a->last_junction->wire_next = wire_b_junctions;
        wire_b_junctions->wire_prev = wire_a->last_junction;
        wire_a->last_junction = wire_b->last_junction;
        wire_a->input_count += wire_b->input_count;
        wire_a->output_count += wire_b->output_count;
        
        pool_RemoveElement(&w_wires, wire_b->element_index);
    }

    return wire_a;
}

void w_MoveSegmentsToWire_r(struct wire_t *wire, struct wire_seg_t *segment, uint32_t tip_index)
{
    if(segment->base.wire == wire)
    {
        return;
    }

    struct wire_seg_t *prev_segment = NULL;
    // uint32_t tip_index = segment->segments[WIRE_SEG_START_INDEX] == NULL;
    while(segment != NULL)
    {
        w_LinkSegmentToWire(wire, segment);

        if(segment->junctions[tip_index].junction != NULL)
        {
            w_MoveJunctionToWire(wire, segment->junctions[tip_index].junction);
            // struct wire_junc_t *junction = segment->junctions[tip_index].junction;
            // w_LinkJunctionToWire(wire, junction);

            // struct wire_seg_t *junction_segment = junction->first_segment;
            // uint32_t recursive_tip_index = !tip_index;
            // while(junction_segment)
            // {
            //     w_MoveSegmentsToWire_r(wire, junction_segment, recursive_tip_index);
            //     recursive_tip_index = junction_segment->junctions[WIRE_SEG_START_INDEX].junction == junction;
            //     junction_segment = junction_segment->junctions[!recursive_tip_index].next;
            // }
        }

        prev_segment = segment;
        segment = segment->segments[tip_index];
        if(segment != NULL)
        {
            tip_index = segment->segments[WIRE_SEG_START_INDEX] == prev_segment;
        }
    }
}

void w_MoveSegmentsToWire(struct wire_t *wire, struct wire_seg_t *segment)
{
    w_LinkSegmentToWire(wire, segment);

    if(segment->segments[WIRE_SEG_START_INDEX] != NULL)
    {
        struct wire_seg_t *prev_segment = segment->segments[WIRE_SEG_START_INDEX];
        w_MoveSegmentsToWire_r(wire, prev_segment, prev_segment->segments[WIRE_SEG_START_INDEX] == segment);
    }
    else if(segment->junctions[WIRE_SEG_START_INDEX].junction != NULL)
    {
        w_MoveJunctionToWire(wire, segment->junctions[WIRE_SEG_START_INDEX].junction);
    }

    if(segment->segments[WIRE_SEG_END_INDEX] != NULL)
    {
        struct wire_seg_t *next_segment = segment->segments[WIRE_SEG_END_INDEX];
        w_MoveSegmentsToWire_r(wire, next_segment, next_segment->segments[WIRE_SEG_START_INDEX] == segment);
    }
    else if(segment->junctions[WIRE_SEG_END_INDEX].junction != NULL)
    {
        w_MoveJunctionToWire(wire, segment->junctions[WIRE_SEG_END_INDEX].junction);
    }
}

void w_MoveJunctionToWire(struct wire_t *wire, struct wire_junc_t *junction)
{
    if(junction->base.wire == wire)
    {
        return;
    }

    w_LinkJunctionToWire(wire, junction);

    struct wire_seg_t *segment = junction->first_segment;
    uint32_t tip_index = segment->junctions[WIRE_SEG_END_INDEX].junction == junction;

    while(segment != NULL)
    {
        // if(segment->segments[!tip_index] != NULL)
        // {
        //     /* walk away from the current junction, moving everything in the way to the 
        //     new wire */
        //     w_MoveSegmentsToWire_r(wire, segment, !tip_index);   
        // }

        w_MoveSegmentsToWire(wire, segment);

        segment = segment->junctions[tip_index].next;

        if(segment != NULL)
        {
            uint32_t tip_index = segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
        }
    }
}

uint32_t w_TryReachOppositeSegmentTip_r(struct wire_seg_t *target_segment, struct wire_seg_t *cur_segment, uint32_t begin_tip)
{
    struct wire_seg_t *prev_segment = NULL;
    uint32_t tip_index = begin_tip;

    while(cur_segment != NULL)
    {
        if(cur_segment->traversal_id == w_traversal_id)
        {
            return cur_segment == target_segment;
        }

        cur_segment->traversal_id = w_traversal_id;

        if(cur_segment->junctions[tip_index].junction != NULL)
        {
            struct wire_junc_t *junction = cur_segment->junctions[tip_index].junction;
            struct wire_seg_t *junction_segment = junction->first_segment;

            while(junction_segment != NULL)
            {
                uint32_t junction_segment_tip_index = junction_segment->junctions[WIRE_SEG_START_INDEX].junction == junction;
                if(junction_segment != cur_segment && w_TryReachOppositeSegmentTip_r(target_segment, junction_segment, junction_segment_tip_index))
                {
                    return 1;
                }

                junction_segment = junction_segment->junctions[!junction_segment_tip_index].next;
            }

            break;
        }

        prev_segment = cur_segment;
        cur_segment = cur_segment->segments[tip_index];
        if(cur_segment != NULL)
        {
            tip_index = cur_segment->segments[WIRE_SEG_START_INDEX] == prev_segment;
        }
    }

    return 0;
}

uint32_t w_TryReachOppositeSegmentTip(struct wire_seg_t *target_segment)
{
    /* try to find if there's a path (other than the segment itself) between its endpoints */
    struct wire_seg_t *prev_segment = NULL;
    struct wire_seg_t *segment = target_segment;
    uint32_t tip_index = WIRE_SEG_END_INDEX;
    while(segment)
    {
        struct wire_junc_t *junction = segment->junctions[tip_index].junction;
        if(junction != NULL && junction->first_segment == junction->last_segment)
        {
            /* this segment is the only one in this junction, which means there's no other
            path that leads to this end point */
            return 0;
        }

        prev_segment = segment;
        segment = segment->segments[tip_index];
        if(segment != NULL)
        {
            tip_index = segment->segments[WIRE_SEG_START_INDEX] == prev_segment;
        }
    }
    
    segment = target_segment;
    prev_segment = NULL;
    tip_index = WIRE_SEG_START_INDEX;
    while(segment)
    {
        struct wire_junc_t *junction = segment->junctions[tip_index].junction;
        if(junction != NULL && junction->first_segment == junction->last_segment)
        {
            /* this segment is the only one in this junction, which means there's no other
            path that leads to this end point */
            return 0;
        }

        prev_segment = segment;
        segment = segment->segments[tip_index];
        if(segment != NULL)
        {
            tip_index = segment->segments[WIRE_SEG_START_INDEX] == prev_segment;
        }
    }

    /* segment is connected to junctions (or to segments connected to junctions) that have
    more than one segment in them, so search all connections for a path. Whatever search
    direction is fine, so we'll start the search by the end tip and try to reach the start 
    tip. */
    w_traversal_id++;
    return w_TryReachOppositeSegmentTip_r(target_segment, target_segment, WIRE_SEG_END_INDEX);
}

uint32_t w_ConnectSegments(struct wire_seg_t *to_connect, struct wire_seg_t *connect_to, uint32_t tip_index)
{
    /* this duplicates a bit the logic in w_AddJunction, but it's necessary. We only want to add a junction
    at the tips of a segment if there's another segment after it. */
    uint32_t coord0 = connect_to->ends[WIRE_SEG_START_INDEX][0] != connect_to->ends[WIRE_SEG_END_INDEX][0];
    uint32_t coord1 = !coord0;    
    if(to_connect->ends[tip_index][coord0] == connect_to->ends[WIRE_SEG_START_INDEX][coord0])
    {   
        if(to_connect->base.wire != connect_to->base.wire)
        {
            if(to_connect->base.wire == NULL)
            {
                w_MoveSegmentsToWire(connect_to->base.wire, to_connect);
            }
            else if(connect_to->base.wire == NULL)
            {
                w_MoveSegmentsToWire(to_connect->base.wire, connect_to);
            }
            else
            {
                w_MergeWires(connect_to->base.wire, to_connect->base.wire);
            }
        }

        /* check if we're trying to connect the segment to one of the endpoints of the other */
        if(to_connect->ends[tip_index][coord1] == connect_to->ends[WIRE_SEG_START_INDEX][coord1])
        {
            /* we're trying to connect [to_connect] to the end of [connect_to] */
            if(connect_to->junctions[WIRE_SEG_START_INDEX].junction != NULL)
            {
                /* [connect_to] is connected to a junction at its end, so connect [to_connect] to the same junction */
                w_LinkSegmentToJunction(to_connect, connect_to->junctions[WIRE_SEG_START_INDEX].junction, tip_index);
            }
            else
            {
                if(connect_to->segments[WIRE_SEG_START_INDEX] != NULL)
                {
                    /* [connect_to] has a segment connected to its end, so create a junction at is end and
                    connect [to_connect] to it */
                    struct wire_junc_t *junction = w_AddJunctionAtTip(connect_to, WIRE_SEG_START_INDEX);
                    w_LinkSegmentToJunction(to_connect, junction, tip_index);
                }
                else
                {
                    /* no junctions or segments, so just link the segments together */
                    connect_to->segments[WIRE_SEG_START_INDEX] = to_connect;
                    to_connect->segments[tip_index] = connect_to;
                }
            }
        }
        else if(to_connect->ends[tip_index][coord1] == connect_to->ends[WIRE_SEG_END_INDEX][coord1])
        {
            /* we're trying to connect [to_connect] to the end of [connect_to] */
            if(connect_to->junctions[WIRE_SEG_END_INDEX].junction != NULL)
            {
                /* [connect_to] is connected to a junction at its end, so connect [to_connect] to the same junction */
                w_LinkSegmentToJunction(to_connect, connect_to->junctions[WIRE_SEG_END_INDEX].junction, tip_index);
            }
            else
            {
                if(connect_to->segments[WIRE_SEG_END_INDEX] != NULL)
                {
                    /* [connect_to] has a segment connected to its end, so create a junction at is end and
                    connect [to_connect] to it */
                    struct wire_junc_t *junction = w_AddJunctionAtTip(connect_to, WIRE_SEG_END_INDEX);
                    w_LinkSegmentToJunction(to_connect, junction, tip_index);
                }
                else
                {
                    /* no junctions or segments, so just link the segments together */
                    connect_to->segments[WIRE_SEG_END_INDEX] = to_connect;
                    to_connect->segments[tip_index] = connect_to;
                }
            }
        }
        else
        {
            /* we'll be connecting the segment to somewhere in the middle of the other segment */
            struct wire_junc_t *junction = w_AddJunctionAtMiddle(connect_to, to_connect->ends[tip_index]);
            w_LinkSegmentToJunction(to_connect, junction, tip_index);
        }

        return 1;
    }

    return 0;
}

void w_DisconnectSegment(struct wire_seg_t *segment)
{
    struct wire_seg_t *segments[2] = {};
    struct wire_junc_t *junctions[2] = {};

    /* ( ° ͜ʖ °) */
    uint32_t can_reach_around = w_TryReachOppositeSegmentTip(segment);

    for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    {
        if(segment->junctions[tip_index].junction != NULL)
        {
            /* disconnect the segment from this junction, also removing the junction if applicable */
            struct wire_junc_t *junction = segment->junctions[tip_index].junction;
            w_UnlinkSegmentLinkIndex(segment, tip_index);

            /* there's two or more segments in this junction */
            struct wire_seg_t *first_segment = junction->first_segment;
            struct wire_seg_t *last_segment = junction->last_segment;

            if(first_segment == NULL || (first_segment == last_segment && junction->pin.device == DEV_INVALID_DEVICE))
            {
                /* if there's no segment left in this junction, or if there's a single segment
                left and the junction isn't connected to a pin, get rid of it. */
                segments[tip_index] = w_RemoveJunction(junction);
            }
            else if(first_segment->junctions[first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].next == last_segment)
            {
                /* there's two segments in this junction, so check if it's connected to a pin */
                if(junction->pin.device == DEV_INVALID_DEVICE)
                {
                    /* junction is not connected to a device, so get rid of it */
                    segments[tip_index] = w_RemoveJunction(junction);
                }
                else
                {
                    /* junction is connected to a pin, so it survives... this time */
                    junctions[tip_index] = junction;
                }
            }
            else
            {
                junctions[tip_index] = junction;
            }
        }
        else if(segment->segments[tip_index] != NULL)
        {
            /* unlink any remaining segments */
            struct wire_seg_t *linked_segment = segment->segments[tip_index];
            linked_segment->segments[linked_segment->segments[WIRE_SEG_END_INDEX] == segment] = NULL;
            segment->segments[tip_index] = NULL;
            segments[tip_index] = linked_segment;
        }
    }


    if(!can_reach_around)
    {
        /* there's no path in this wire (other than the segment itself) that leads to the opposite side of the segment, so we may 
        need to split this wire, depending on what the segment was connected to. */
        if(segments[WIRE_SEG_START_INDEX] != NULL && (void *)segments[WIRE_SEG_END_INDEX] != (void *)junctions[WIRE_SEG_END_INDEX])
        {
            /* segment has a segment in one side and either a segment or a junction on the other, so split the wire */
            struct wire_t *new_wire = w_AllocWire();
            w_MoveSegmentsToWire(new_wire, segments[WIRE_SEG_START_INDEX]);
        }
        else if(junctions[WIRE_SEG_START_INDEX] != NULL && (void *)segments[WIRE_SEG_END_INDEX] != (void *)junctions[WIRE_SEG_END_INDEX])
        {
            /* segment has a junction in one side and either a segment or a junction on the other, so split the wire */
            struct wire_t *new_wire = w_AllocWire();
            w_MoveJunctionToWire(new_wire, junctions[WIRE_SEG_START_INDEX]);
        }
    }
}

void w_ConnectPinToSegment(struct wire_seg_t *segment, uint32_t tip_index, struct dev_t *device, uint16_t pin)
{
    struct wire_t *wire = segment->base.wire;
    if(segment->junctions[tip_index].junction == NULL)
    {
        w_AddJunctionAtTip(segment, tip_index);
    }

    w_ConnectPinToJunction(segment->junctions[tip_index].junction, device, pin);  
}

void w_ConnectPinToJunction(struct wire_junc_t *junction, struct dev_t *device, uint16_t pin)
{
    junction->pin.device = device->element_index;
    junction->pin.pin = pin;

    if(junction->base.wire != NULL)
    {
        struct dev_pin_t *device_pin = dev_GetDevicePin(device, pin);
        struct dev_desc_t *dev_desc = dev_device_descs + device->type;
        struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;

        struct wire_t *wire = junction->base.wire;
        // device_pin->wire = wire->element_index;    
        device_pin->junction = junction->base.element_index;
        wire->input_count += pin_desc->type == DEV_PIN_TYPE_IN;
        wire->output_count += pin_desc->type == DEV_PIN_TYPE_OUT;
    }
}

void w_DisconnectJunctionFromPin(struct wire_junc_t *junction)
{
    if(junction->pin.device != DEV_INVALID_DEVICE)
    {
        struct wire_t *wire = junction->base.wire;
        struct dev_t *device = dev_GetDevice(junction->pin.device);
        struct dev_pin_t *device_pin = dev_GetDevicePin(device, junction->pin.pin);
        struct dev_pin_desc_t *pin_desc = dev_device_descs[device->type].pins + junction->pin.pin;
        junction->pin.device = DEV_INVALID_DEVICE;
        junction->pin.pin = DEV_INVALID_PIN;

        if(wire != NULL)
        {
            // device_pin->wire = WIRE_INVALID_WIRE;
            device_pin->junction = INVALID_POOL_INDEX;
            wire->input_count -= pin_desc->type == DEV_PIN_TYPE_IN;
            wire->output_count -= pin_desc->type == DEV_PIN_TYPE_OUT;
        }
    }
}

void w_DisconnectPin(struct wire_t *wire, struct dev_t *device, uint16_t pin)
{
    struct wire_junc_t *junction = wire->first_junction;

    while(junction != NULL)
    {
        if(junction->pin.device == device->element_index && junction->pin.pin == pin)
        {
            w_DisconnectJunctionFromPin(junction);
            break;
        }
        junction = junction->wire_next;
    }
}