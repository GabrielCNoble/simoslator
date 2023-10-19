#include <stdio.h>
#include "wire.h"
#include "list.h"
#include "dev.h"
#include "sim.h"

uint8_t w_wire_value_resolution[WIRE_VALUE_LAST][WIRE_VALUE_LAST] = {
    [WIRE_VALUE_0S] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1W]     = WIRE_VALUE_0S,
        [WIRE_VALUE_Z]      = WIRE_VALUE_0S,
        [WIRE_VALUE_U]      = WIRE_VALUE_0S,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_1S] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_1S,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1S,
        [WIRE_VALUE_Z]      = WIRE_VALUE_1S,
        [WIRE_VALUE_U]      = WIRE_VALUE_1S,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_0W] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
        [WIRE_VALUE_1W]     = WIRE_VALUE_U,
        [WIRE_VALUE_Z]      = WIRE_VALUE_0W,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_1W] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_U,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
        [WIRE_VALUE_Z]      = WIRE_VALUE_1W,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_Z] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
        [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
        [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_U] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
        [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
        [WIRE_VALUE_0W]     = WIRE_VALUE_U,
        [WIRE_VALUE_1W]     = WIRE_VALUE_U,
        [WIRE_VALUE_Z]      = WIRE_VALUE_U,
        [WIRE_VALUE_U]      = WIRE_VALUE_U,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },

    [WIRE_VALUE_ERR] = {
        [WIRE_VALUE_0S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1S]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_0W]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_1W]     = WIRE_VALUE_ERR,
        [WIRE_VALUE_Z]      = WIRE_VALUE_ERR,
        [WIRE_VALUE_U]      = WIRE_VALUE_ERR,
        [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
    },
};

struct pool_t               w_wires;
// struct pool_t               w_wire_pin_blocks[2];
struct pool_t               w_wire_seg_pos;
// struct pool_t               w_wire_junc_pos;
struct pool_t               w_wire_segs;
struct pool_t               w_wire_juncs;
uint64_t                    w_traversal_id;
uint64_t                    w_seg_junc_count;
// struct 

extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    // w_wire_pin_blocks[0] = pool_CreateTyped(struct wire_junc_pin_block_t, 16384);
    // w_wire_pin_blocks[1] = pool_CreateTyped(struct wire_junc_pin_block_t, 16384);
    w_wire_seg_pos = pool_CreateTyped(struct wire_seg_pos_block_t, 16384);
    // w_wire_junc_pos = pool_CreateTyped(struct wire_junc_pos_block_t, 16384);
    w_wire_segs = pool_Create(sizeof(struct wire_seg_t), 16384, offsetof(struct wire_seg_t, base.POOL_ELEMENT_NAME));
    w_wire_juncs = pool_Create(sizeof(struct wire_junc_t), 16384, offsetof(struct wire_junc_t, base.POOL_ELEMENT_NAME));
}

void w_Shutdown()
{
    pool_Destroy(&w_wires);
    // pool_Destroy(&w_wire_pin_blocks[0]);
    // pool_Destroy(&w_wire_pin_blocks[1]);
    pool_Destroy(&w_wire_seg_pos);
    // pool_Destroy(&w_wire_junc_pos);
    pool_Destroy(&w_wire_segs);
    pool_Destroy(&w_wire_juncs);
}

struct wire_t *w_AllocWire()
{
    struct wire_t *wire = pool_AddElement(&w_wires, NULL);

    // for(uint32_t index = 0; index < 2; index++)
    // {
    //     wire->wire_pins[index].first_block = pool_AddElement(&w_wire_pin_blocks[index], NULL);
    //     wire->wire_pins[index].last_block = wire->wire_pins[index].first_block;
    //     wire->wire_pins[index].pin_count = 0;

    //     struct wire_junc_pin_block_t *pin_block = wire->wire_pins[index].first_block;
    //     pin_block->next = NULL;
    //     pin_block->prev = NULL;
    // }

    wire->first_junction = NULL;
    wire->last_junction = NULL;
    wire->first_segment = NULL;
    wire->last_segment = NULL;

    wire->first_segment_pos = pool_AddElement(&w_wire_seg_pos, NULL);
    wire->last_segment_pos = wire->first_segment_pos;
    wire->segment_pos_count = 0;
    wire->input_count = 0;
    wire->output_count = 0;

    // wire->first_junction_pos = pool_AddElement(&w_wire_junc_pos, NULL);
    // wire->last_junction_pos = wire->first_junction_pos;
    // wire->junction_pos_count = 0;
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
    pool_Reset(&w_wire_seg_pos);
    w_seg_junc_count = 0;
}

struct wire_t *w_GetWire(uint32_t wire_index)
{
    return pool_GetValidElement(&w_wires, wire_index);
}

void w_LinkWireSegment(struct wire_t *wire, struct wire_seg_t *segment)
{
    
}

struct wire_seg_t *w_AllocWireSegment(struct wire_t *wire)
{
    struct wire_seg_t *segment = pool_AddElement(&w_wire_segs, NULL);
    segment->segments[0] = NULL;
    segment->segments[1] = NULL;
    segment->junctions[0] = (struct wire_seg_junc_t){};
    segment->junctions[1] = (struct wire_seg_junc_t){};
    segment->base.wire = wire;
    segment->selection_index = 0xffffffff;
    segment->wire_next = NULL;
    segment->wire_prev = NULL;

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
    
    segment->pos = wire->last_segment_pos->segments + wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE;
    segment->pos_block = wire->last_segment_pos;
    wire->segment_pos_count++;
    
    segment->pos->segment = segment;

    if(wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE == 0)
    {
        struct wire_seg_pos_block_t *new_block = pool_AddElement(&w_wire_seg_pos, NULL);
        new_block->prev = wire->last_segment_pos;
        wire->last_segment_pos->next = new_block;
        wire->last_segment_pos = new_block;
    }

    return segment;
}

void w_FreeWireSegment(struct wire_seg_t *segment)
{
    struct wire_t *wire = segment->base.wire;
    struct wire_seg_pos_block_t *pos_block = segment->pos_block;
    uint32_t pos_index = (segment->pos - pos_block->segments) % WIRE_SEGMENT_POS_BLOCK_SIZE;
    struct wire_seg_pos_block_t *last_block = wire->last_segment_pos;
    uint32_t last_pos_index = wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE;

    if(last_pos_index == WIRE_SEGMENT_POS_BLOCK_SIZE - 1)
    {
        wire->last_segment_pos = last_block->prev;
        wire->last_segment_pos->next = NULL;

        pool_RemoveElement(&w_wire_seg_pos, last_block->element_index);
        last_block = wire->last_segment_pos;
    }

    pool_RemoveElement(&w_wire_segs, segment->base.element_index);

    pos_block->segments[pos_index] = last_block->segments[last_pos_index];
    wire->last_segment_pos--;

    struct wire_seg_t *moved_segment = pos_block->segments[pos_index].segment;
    moved_segment->pos = pos_block->segments + pos_index;
    moved_segment->pos_block = pos_block;

    for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    {
        struct wire_junc_t *junction = moved_segment->junctions[tip_index].junction;
        if(junction != NULL && junction->first_segment == moved_segment)
        {
            junction->pos = moved_segment->pos->ends[tip_index];
        }
    }
}

void w_LinkWireJunction(struct wire_t *wire, struct wire_junc_t *junction)
{
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
}

void w_UnlinkWireJunction(struct wire_junc_t *junction)
{
    struct wire_t *wire = junction->base.wire;
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

struct wire_junc_t *w_AllocWireJunction(struct wire_t *wire)
{
    struct wire_junc_t *junction = pool_AddElement(&w_wire_juncs, NULL);
    junction->pin.device = DEV_INVALID_DEVICE;
    junction->pin.pin = DEV_INVALID_PIN;
    junction->first_segment = NULL;
    junction->last_segment = NULL;
    junction->selection_index = 0xffffffff;
    junction->wire_next = NULL;
    junction->wire_prev = NULL;

    w_LinkWireJunction(wire, junction);

    return junction;
}

void w_FreeWireJunction(struct wire_junc_t *junction)
{
    struct wire_t *wire = junction->base.wire;

    if(junction->pin.device != DEV_INVALID_DEVICE)
    {
        w_DisconnectPin(junction);
    }

    w_UnlinkWireJunction(junction);
    
    pool_RemoveElement(&w_wire_juncs, junction->base.element_index);
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
        junction->pos = segment->pos->ends[link_index];
    }
    else
    { 
        junction->last_segment->junctions[junction->last_segment->junctions[1].junction == junction].next = segment;
        seg_junc->prev = junction->last_segment;
    }
    junction->last_segment = segment;
    w_seg_junc_count++;
}

void w_UnlinkSegmentFromJunction(struct wire_seg_t *segment, struct wire_junc_t *junction)
{
    uint32_t link_index = segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
    w_UnlinkSegmentLinkIndex(segment, link_index);
    // struct wire_seg_junc_t *seg_junc = &segment->junctions[segment->junctions[WIRE_SEG_END_INDEX].junction == junction];

    // if(seg_junc->junction != junction)
    // {
    //     printf("w_UnlinkSegmentFromJunction: segment is not linked to junction\n");
    //     return;
    // }

    // if(seg_junc->prev == NULL)
    // {
    //     junction->first_segment = seg_junc->next;
    //     if(junction->first_segment != NULL)
    //     {
    //         struct wire_seg_t *first_segment = junction->first_segment;
    //         junction->pos = first_segment->pos->ends[(first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction)];
    //     }
    // }
    // else
    // {
    //     struct wire_seg_t *prev_segment = seg_junc->prev;
    //     prev_segment->junctions[prev_segment->junctions[1].junction == junction].next = seg_junc->next;
    // }

    // if(seg_junc->next == NULL)
    // {
    //     junction->last_segment = seg_junc->prev;
    // }
    // else
    // {
    //     struct wire_seg_t *next_segment = seg_junc->next;
    //     next_segment->junctions[next_segment->junctions[1].junction == junction].prev = seg_junc->prev;
    // }

    // seg_junc->junction = NULL;
    // seg_junc->next = NULL;
    // seg_junc->prev = NULL;
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

    if(seg_junc->prev == NULL)
    {
        junction->first_segment = seg_junc->next;
        if(junction->first_segment != NULL)
        {
            struct wire_seg_t *first_segment = junction->first_segment;
            junction->pos = first_segment->pos->ends[(first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction)];
        }
    }
    else
    {
        struct wire_seg_t *prev_segment = seg_junc->prev;
        prev_segment->junctions[prev_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].next = seg_junc->next;
    }

    if(seg_junc->next == NULL)
    {
        junction->last_segment = seg_junc->prev;
    }
    else
    {
        struct wire_seg_t *next_segment = seg_junc->next;
        next_segment->junctions[next_segment->junctions[WIRE_SEG_END_INDEX].junction == junction].prev = seg_junc->prev;
    }

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

    coord0 = segment->pos->ends[WIRE_SEG_START_INDEX][0] != segment->pos->ends[WIRE_SEG_END_INDEX][0];
    coord1 = !coord0;

    if(position[coord0] == segment->pos->ends[WIRE_SEG_START_INDEX][coord0])
    {
        uint32_t link_index;
        
        if(position[coord1] == segment->pos->ends[WIRE_SEG_START_INDEX][coord1])
        {
            /* junction is to be created at the start of the segment */
            if(segment->junctions[WIRE_SEG_START_INDEX].junction != NULL)
            {
                /* wire already has a junction at the start, so just return it */
                junction = segment->junctions[WIRE_SEG_START_INDEX].junction;
            }
            else
            {
                junction = w_AllocWireJunction(wire);

                if(segment->segments[WIRE_SEG_START_INDEX] != NULL)
                {
                    /* there's another segment after the current one, so link it to the 
                    junction as well */
                    struct wire_seg_t *prev_segment = segment->segments[WIRE_SEG_START_INDEX];
                    uint32_t prev_segment_tip_index = prev_segment->segments[WIRE_SEG_END_INDEX] == segment;
                    w_LinkSegmentToJunction(prev_segment, junction, prev_segment_tip_index);
                    prev_segment->segments[prev_segment_tip_index] = NULL;
                    segment->segments[WIRE_SEG_START_INDEX] = NULL;
                }

                w_LinkSegmentToJunction(segment, junction, WIRE_SEG_START_INDEX);
            }
        }
        else if(position[coord1] == segment->pos->ends[WIRE_SEG_END_INDEX][coord1])
        {
            /* junction is to be created at the end of the segment */
            if(segment->junctions[WIRE_SEG_END_INDEX].junction != NULL)
            {
                /* wire already has a junction at the end, so just return it */
                junction = segment->junctions[WIRE_SEG_END_INDEX].junction;
            }
            else
            {
                junction = w_AllocWireJunction(wire);

                if(segment->segments[WIRE_SEG_END_INDEX] != NULL)
                {
                    /* there's another segment after the current one, so link it to the 
                    junction as well */
                    struct wire_seg_t *next_segment = segment->segments[WIRE_SEG_END_INDEX];
                    uint32_t next_segment_tip_index = next_segment->segments[WIRE_SEG_END_INDEX] == segment;
                    w_LinkSegmentToJunction(next_segment, junction, next_segment_tip_index);
                    next_segment->segments[next_segment_tip_index] = NULL;
                    segment->segments[WIRE_SEG_END_INDEX] = NULL;
                }

                w_LinkSegmentToJunction(segment, junction, WIRE_SEG_END_INDEX);
            }
        }
        else
        {
            /* junction is to be created at the middle of the segment */
            junction = w_AllocWireJunction(wire);
            struct wire_seg_t *split = w_AllocWireSegment(wire);
            split->pos->ends[WIRE_SEG_START_INDEX][0] = position[0];
            split->pos->ends[WIRE_SEG_START_INDEX][1] = position[1];
            split->pos->ends[WIRE_SEG_END_INDEX][0] = segment->pos->ends[WIRE_SEG_END_INDEX][0];
            split->pos->ends[WIRE_SEG_END_INDEX][1] = segment->pos->ends[WIRE_SEG_END_INDEX][1];
            segment->pos->ends[WIRE_SEG_END_INDEX][0] = position[0];
            segment->pos->ends[WIRE_SEG_END_INDEX][1] = position[1];

            split->segments[WIRE_SEG_END_INDEX] = segment->segments[WIRE_SEG_END_INDEX];
            
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
        }
    }

    return junction;
}

void w_RemoveJunction(struct wire_junc_t *junction)
{
    if(junction->first_segment != NULL)
    {
        struct wire_seg_t *first_segment = junction->first_segment;
        uint32_t first_link_index = first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
        struct wire_seg_junc_t *seg_junc = &first_segment->junctions[first_link_index];
        struct wire_seg_t *second_segment = seg_junc->next;

        if(second_segment != NULL && junction->last_segment != second_segment)
        {
            printf("can't remove junction with more than two segments\n");
            return;
        }

        w_UnlinkSegmentLinkIndex(first_segment, first_link_index);

        if(second_segment != NULL)
        {
            uint32_t second_link_index = second_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
            w_UnlinkSegmentLinkIndex(second_segment, second_link_index);

            uint32_t coord0 = first_segment->pos->ends[WIRE_SEG_START_INDEX][1] == first_segment->pos->ends[WIRE_SEG_END_INDEX][1];
            uint32_t coord1 = second_segment->pos->ends[WIRE_SEG_START_INDEX][1] == second_segment->pos->ends[WIRE_SEG_END_INDEX][1];

            if(coord0 == coord1)
            {
                /* segments are collinear, so merge them */
                if(second_segment->junctions[!second_link_index].junction != NULL)
                {
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

                w_FreeWireSegment(second_segment);
            }
            else
            {
                /* segments aren't collinear, so just link them together */
                first_segment->segments[first_segment->segments[WIRE_SEG_END_INDEX] == NULL] = second_segment;
                second_segment->segments[second_segment->segments[WIRE_SEG_END_INDEX] == NULL] = first_segment;
            }
        }
        else if(junction->pin.device != DEV_INVALID_DEVICE)
        {
            w_DisconnectPin(junction);
        }

        w_FreeWireJunction(junction);
    }
}

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b)
{
    if(wire_a != NULL && wire_b != NULL && wire_a != wire_b)
    {
        // for(uint32_t pin_type = 0; pin_type < 2; pin_type++)
        // {
        //     struct wire_junc_pins_t *wire_a_pins = &wire_a->wire_pins[pin_type];
        //     struct wire_junc_pins_t *wire_b_pins = &wire_b->wire_pins[pin_type];

        //     if(wire_b_pins->pin_count != 0)
        //     {
        //         struct wire_junc_pin_block_t *wire_b_pin_block = wire_b_pins->first_block;
        //         uint32_t total_pin_count = wire_b_pins->pin_count;

        //         while(wire_b_pin_block != NULL)
        //         {
        //             uint32_t pin_count = WIRE_JUNC_PIN_BLOCK_PIN_COUNT;
        //             if(pin_count > total_pin_count)
        //             {
        //                 pin_count = total_pin_count;
        //             }

        //             total_pin_count -= pin_count;

        //             for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
        //             {
        //                 struct wire_junc_pin_t *wire_pin = wire_b_pin_block->pins + pin_index;
        //                 struct dev_t *device = dev_GetDevice(wire_pin->pin.device);
        //                 struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin->pin.pin);
        //                 device_pin->wire = wire_a->element_index;
        //             }

        //             if(wire_b_pin_block->next == NULL)
        //             {
        //                 break;
        //             }

        //             wire_b_pin_block = wire_b_pin_block->next;
        //         }
                
        //         struct wire_junc_pin_block_t *wire_a_pin_block = wire_a_pins->last_block;
        //         uint32_t wire_a_pin_index = wire_a_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT;
        //         uint32_t wire_b_pin_index = (wire_b_pins->pin_count - 1) % WIRE_JUNC_PIN_BLOCK_PIN_COUNT;
        //         uint32_t move_size = WIRE_JUNC_PIN_BLOCK_PIN_COUNT - (wire_a_pin_index - 1);

        //         if(move_size >= wire_b_pin_index)
        //         {
        //             move_size = wire_b_pin_index + 1;
        //         }

        //         while(move_size)
        //         {
        //             wire_a_pin_block->pins[wire_a_pin_index] = wire_b_pin_block->pins[wire_b_pin_index];
        //             wire_a_pin_block->pins[wire_a_pin_index].junction->pin = wire_a_pin_block->pins + wire_a_pin_index;
        //             wire_a_pin_block->pins[wire_a_pin_index].junction->pin_block = wire_a_pin_block;
        //             wire_a_pin_index++;
        //             wire_b_pin_index--;
        //             move_size--;
        //         }

        //         if(wire_b_pin_index == 0xffffffff)
        //         {
        //             if(wire_b_pins->first_block == wire_b_pin_block)
        //             {
        //                 wire_b_pins->first_block = NULL;
        //                 wire_b_pins->last_block = NULL;
        //             }
        //             else
        //             {
        //                 wire_b_pins->last_block = wire_b_pins->last_block->prev;
        //                 wire_b_pins->last_block->next = NULL;
        //             }

        //             pool_RemoveElement(&w_wire_pin_blocks[pin_type], wire_b_pin_block->element_index);
        //         }

        //         if(wire_b_pins->first_block != NULL)
        //         {
        //             wire_a_pins->last_block->next = wire_b_pins->first_block;
        //             wire_a_pins->last_block = wire_b_pins->last_block;
        //         }

        //         wire_a_pins->pin_count += wire_b_pins->pin_count;
        //     }
        // }

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

        struct wire_seg_pos_block_t *wire_a_last_seg_pos_block = wire_a->last_segment_pos;
        struct wire_seg_pos_block_t *wire_b_last_seg_pos_block = wire_b->first_segment_pos;

        uint32_t wire_a_block_offset = wire_a->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE;
        uint32_t wire_b_block_offset = (wire_b->segment_pos_count - 1) % WIRE_SEGMENT_POS_BLOCK_SIZE;
        uint32_t move_size = WIRE_SEGMENT_POS_BLOCK_SIZE - wire_a_block_offset;
        if(move_size >= wire_b_block_offset)
        {
            move_size = wire_b_block_offset + 1;
        }

        while(move_size)
        {
            wire_a_last_seg_pos_block->segments[wire_a_block_offset] = wire_b_last_seg_pos_block->segments[wire_b_block_offset];
            wire_a_last_seg_pos_block->segments[wire_a_block_offset].segment->pos = wire_a_last_seg_pos_block->segments + wire_a_block_offset;
            wire_a_last_seg_pos_block->segments[wire_a_block_offset].segment->pos_block = wire_a_last_seg_pos_block;
            wire_a_block_offset++;
            wire_b_block_offset--;
            move_size--;
        }

        if(wire_b_block_offset == 0xffffffff)
        {
            /* there were less elements in wire_b_last_seg_pos_block, so get rid of it */
            if(wire_b_last_seg_pos_block == wire_b->first_segment_pos)
            {
                wire_b->first_segment_pos = NULL;
                wire_b->last_segment_pos = NULL;
            }
            else
            {
                wire_b->last_segment_pos = wire_b->last_segment_pos->prev;
                wire_b->last_segment_pos->next = NULL;
            }

            pool_RemoveElement(&w_wire_seg_pos, wire_b_last_seg_pos_block->element_index);
        }

        if(wire_b->first_segment_pos != NULL)
        {
            wire_a->last_segment_pos->next = wire_b->first_segment_pos;
            wire_a->last_segment_pos = wire_b->last_segment_pos;
        }
        wire_a->segment_pos_count += wire_b->segment_pos_count;


        /* move junction stuff over */
        struct wire_junc_t *wire_b_junctions = wire_b->first_junction;
        struct wire_junc_t *junction = wire_b_junctions;
        while(junction != NULL)
        {
            struct wire_seg_t *first_segment = junction->first_segment;
            uint32_t tip_index = first_segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
            junction->base.wire = wire_a;
            junction->pos = first_segment->pos->ends[tip_index];

            if(junction->pin.device != DEV_INVALID_DEVICE)
            {
                struct dev_t *device = dev_GetDevice(junction->pin.device);
                struct dev_pin_t *pin = dev_GetDevicePin(device, junction->pin.pin);
                pin->wire = wire_a->element_index;
            }

            junction = junction->wire_next;
        }

        wire_a->last_junction->wire_next = wire_b_junctions;
        wire_b_junctions->wire_prev = wire_a->last_junction;
        wire_a->last_junction = wire_b->last_junction;
        wire_a->input_count += wire_b->input_count;
        wire_a->output_count += wire_b->output_count;

        // struct wire_junc_pos_block_t *wire_a_last_junc_pos_block = wire_a->last_junction_pos;
        // struct wire_junc_pos_block_t *wire_b_last_junc_pos_block = wire_b->first_junction_pos;

        // wire_a_block_offset = wire_a->junction_pos_count % WIRE_JUNCTION_POS_BLOCK_SIZE;
        // wire_b_block_offset = (wire_b->junction_pos_count - 1) % WIRE_JUNCTION_POS_BLOCK_SIZE;
        // move_size = WIRE_JUNCTION_POS_BLOCK_SIZE - wire_a_block_offset;
        // if(move_size >= wire_b_block_offset)
        // {
        //     move_size = wire_b_block_offset + 1;
        // }

        // while(move_size)
        // {
        //     wire_a_last_junc_pos_block->junctions[wire_a_block_offset] = wire_b_last_junc_pos_block->junctions[wire_b_block_offset];
        //     wire_a_last_junc_pos_block->junctions[wire_a_block_offset].junction->pos = wire_a_last_junc_pos_block->junctions + wire_a_block_offset;
        //     wire_a_last_junc_pos_block->junctions[wire_a_block_offset].junction->pos_block = wire_a_last_junc_pos_block;
        //     wire_a_block_offset++;
        //     wire_b_block_offset--;
        //     move_size--;
        // }

        // if(wire_b_block_offset == 0xffffffff)
        // {
        //     /* there were less elements in wire_b_last_junc_pos_block, so get rid of it */
        //     if(wire_b_last_junc_pos_block == wire_b->first_junction_pos)
        //     {
        //         wire_b->first_junction_pos = NULL;
        //         wire_b->last_junction_pos = NULL;
        //     }
        //     else
        //     {
        //         wire_b->last_junction_pos = wire_b->last_junction_pos->prev;
        //         wire_b->last_junction_pos->next = NULL;
        //     }

        //     pool_RemoveElement(&w_wire_junc_pos, wire_b_last_junc_pos_block->element_index);
        // }

        // if(wire_b->first_junction_pos != NULL)
        // {
        //     wire_a->last_junction_pos->next = wire_b->first_junction_pos;
        //     wire_a->last_junction_pos = wire_b->last_junction_pos;
        // }
        // wire_a->junction_pos_count += wire_b->junction_pos_count;
        
        pool_RemoveElement(&w_wires, wire_b->element_index);
    }

    return wire_a;
}

void w_MoveJunctionToWire(struct wire_t *wire, struct wire_junc_t *junction)
{
    w_UnlinkWireJunction(junction);
    w_LinkWireJunction(wire, junction);

    struct wire_seg_t *segment = junction->first_segment;
    while(segment)
    {

    }
}

void w_MoveSegmentsToWire(struct wire_t *wire, struct wire_seg_t *segment, uint32_t tip_index)
{
    // while(segment != NULL)
}

struct wire_t *w_SplitWire(struct wire_t *wire, struct wire_seg_t *segment)
{
    struct wire_t *new_wire = NULL;

    if(!w_TryReachSegment(segment))
    {
        new_wire = w_AllocWire();

        if(segment->junctions[WIRE_SEG_START_INDEX].junction != NULL)
        {
            struct wire_junc_t *junction = segment->junctions[WIRE_SEG_START_INDEX].junction;
            w_UnlinkSegmentLinkIndex(segment, WIRE_SEG_START_INDEX);
            w_MoveJunctionToWire(new_wire, junction);
        }
        else if(segment->segments[WIRE_SEG_START_INDEX] != NULL)
        {
            
        }
        
        // if(segment->segments[WIRE_SEG_START_INDEX] == NULL && )
        // {

        // }
    }
    else
    {
        new_wire = wire;

        if(segment->junctions[WIRE_SEG_START_INDEX].junction != NULL)
        {
            w_UnlinkSegmentLinkIndex(segment, WIRE_SEG_START_INDEX);
        }
        else if(segment->segments[WIRE_SEG_START_INDEX] != NULL)
        {
            struct wire_seg_t *prev_segment = segment->segments[WIRE_SEG_START_INDEX];
            prev_segment->segments[prev_segment->segments[WIRE_SEG_START_INDEX] == segment] = NULL;
        }

        if(segment->junctions[WIRE_SEG_END_INDEX].junction != NULL)
        {
            w_UnlinkSegmentLinkIndex(segment, WIRE_SEG_END_INDEX);
        }
        else if(segment->segments[WIRE_SEG_END_INDEX] != NULL)
        {
            struct wire_seg_t *prev_segment = segment->segments[WIRE_SEG_END_INDEX];
            prev_segment->segments[prev_segment->segments[WIRE_SEG_START_INDEX] == segment] = NULL;
        }
    }

    return new_wire;
}

uint32_t w_TryReachSegment_r(struct wire_seg_t *target_segment, struct wire_seg_t *cur_segment, uint32_t begin_tip)
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
                if(junction_segment != cur_segment && w_TryReachSegment_r(target_segment, junction_segment, junction_segment_tip_index))
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

uint32_t w_TryReachSegment(struct wire_seg_t *target_segment)
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
    return w_TryReachSegment_r(target_segment, target_segment, WIRE_SEG_END_INDEX);
}

void w_ConnectPin(struct wire_junc_t *junction, struct dev_t *device, uint16_t pin)
{
    struct wire_t *wire = junction->base.wire;
    struct dev_pin_t *device_pin = dev_GetDevicePin(device, pin);
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;
    // struct pool_t *wire_pin_block_list;
    // struct wire_junc_pins_t *wire_pins;

    // wire_pin_block_list = &w_wire_pin_blocks[pin_desc->type];
    // wire_pins = &wire->wire_pins[pin_desc->type];

    // junction->pin = wire_pins->last_block->pins + wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT;
    // junction->pin_block = wire_pins->last_block;
    // wire_pins->pin_count++;

    // if(wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT == 0)
    // {
    //     struct wire_junc_pin_block_t *new_block = pool_AddElement(wire_pin_block_list, NULL);
    //     new_block->prev = wire_pins->last_block;
    //     wire_pins->last_block->next = new_block;
    //     wire_pins->last_block = new_block;
    // }
    
    // junction->pin->pin.device = device->element_index;
    // junction->pin->pin.pin = pin;
    // junction->pin->junction = junction;
    junction->pin.device = device->element_index;
    junction->pin.pin = pin;
    device_pin->wire = wire->element_index;    

    wire->input_count += pin_desc->type == DEV_PIN_TYPE_IN;
    wire->output_count += pin_desc->type == DEV_PIN_TYPE_OUT;
}

void w_DisconnectPin(struct wire_junc_t *junction)
{
    if(junction->pin.device != DEV_INVALID_DEVICE)
    {
        struct wire_t *wire = junction->base.wire;
        struct dev_t *device = dev_GetDevice(junction->pin.device);
        struct dev_pin_t *device_pin = dev_GetDevicePin(device, junction->pin.pin);
        struct dev_pin_desc_t *pin_desc = dev_device_descs[device->type].pins + junction->pin.pin;
        // struct pool_t *wire_pin_block_list = &w_wire_pin_blocks[pin_desc->type];
        // struct wire_junc_pins_t *wire_pins = &wire->wire_pins[pin_desc->type];

        // struct wire_junc_pin_block_t *pin_block = junction->pin_block;
        // struct wire_junc_pin_block_t *last_block = wire_pins->last_block;
        // uint32_t pin_index = pin - pin_block->pins;
        // uint32_t last_pin_index = wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT;

        // if(pin_block != last_block)
        // {
        //     if((wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT) == WIRE_JUNC_PIN_BLOCK_PIN_COUNT - 1)
        //     {
        //         /* there's free space in the previous block of the last block, so get rid of it */
        //         wire_pins->last_block = last_block->prev;
        //         wire_pins->last_block->next = NULL;
        //         pool_RemoveElement(wire_pin_block_list, last_block->element_index);
        //         last_block = wire_pins->last_block;
        //     }
        // }

        // pin_block->pins[pin_index] = last_block->pins[last_pin_index];
        // wire_pins->pin_count--;

        // junction->pin = NULL;
        // junction->pin_block = NULL;
        
        // pin->junction->pin = pin;
        // pin->junction->pin_block = pin_block;
        device_pin->wire = WIRE_INVALID_WIRE;
        junction->pin.device = DEV_INVALID_DEVICE;
        junction->pin.pin = DEV_INVALID_PIN;

        wire->input_count -= pin_desc->type == DEV_PIN_TYPE_IN;
        wire->output_count -= pin_desc->type == DEV_PIN_TYPE_OUT;
    }
}

struct wire_seg_pos_t *w_AllocWireSegPos(struct wire_t *wire)
{
    if(wire->last_segment_pos == NULL)
    {
        wire->first_segment_pos = pool_AddElement(&w_wire_seg_pos, NULL);
        wire->last_segment_pos = wire->first_segment_pos;
        wire->segment_pos_count = 0;
    }
    
    struct wire_seg_pos_t *segment = wire->last_segment_pos->segments + wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE;
    wire->segment_pos_count++;

    if(wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE == 0)
    {
        struct wire_seg_pos_block_t *new_block = pool_AddElement(&w_wire_seg_pos, NULL);
        new_block->prev = wire->last_segment_pos;
        wire->last_segment_pos->next = new_block;
        wire->last_segment_pos = new_block;
    }

    return segment;
}

void w_FreeWireSegPos(struct wire_t *wire, struct wire_seg_pos_t *seg_pos)
{

}

struct wire_junc_pos_t *w_AllocWireJuncPos(struct wire_t *wire)
{
    // if(wire->last_junction_pos == NULL)
    // {
    //     wire->first_junction_pos = pool_AddElement(&w_wire_seg_pos, NULL);
    //     wire->last_junction_pos = wire->first_junction_pos;
    //     wire->junction_pos_count = 0;
    // }
    
    // struct wire_junc_pos_t *junction = wire->last_junction_pos->junctions + wire->junction_pos_count % WIRE_SEGMENT_POS_BLOCK_POS_COUNT;
    // wire->segment_pos_count++;

    // if(wire->junction_pos_count % WIRE_SEGMENT_POS_BLOCK_POS_COUNT == 0)
    // {
    //     struct wire_junc_pos_block_t *new_block = pool_AddElement(&w_wire_junc_pos, NULL);
    //     new_block->prev = wire->last_junction_pos;
    //     wire->last_junction_pos->next = new_block;
    //     wire->last_junction_pos = new_block;
    // }

    // return junction;
}

// struct wire_seg_t *w_AllocWireSegment(struct wire_t *wire, uint32_t type)
// {
//     // struct wire_seg_t *segment = wire->last_segment->segments + wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
//     struct wire_seg_t *segment = pool_AddElement(&w_wire_segs, NULL);

//     if(wire->first_segment == NULL)
//     {
//         wire->first_segment = segment;
//     }
//     else
//     {
//         // segment->
//     }

//     wire->last_segment = segment;
//     // wire->segment_pos_count++;

//     // if(wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT == 0)
//     // {
//     //     struct wire_seg_pos_block_t *new_block = pool_AddElement(&w_wire_seg_pos, NULL);
//     //     new_block->prev = wire->last_segment_pos;
//     //     wire->last_segment_pos->next = new_block;
//     //     wire->last_segment_pos = new_block;
//     // }

//     // dst_segment->start[0] = src_segment->start[0];
//     // dst_segment->start[1] = src_segment->start[1];
//     // dst_segment->end[0] = src_segment->end[0];
//     // dst_segment->end[1] = src_segment->end[1];
// }

struct wire_t *w_ConnectPins(struct dev_t *device_a, uint16_t pin_a, struct dev_t *device_b, uint16_t pin_b, struct list_t *wire_segments)
{
    // struct dev_pin_t *pin0 = dev_GetDevicePin(device_a, pin_a);
    // struct dev_pin_t *pin1 = dev_GetDevicePin(device_b, pin_b);
    // struct wire_t *wire;
    
    // if(pin0->wire == WIRE_INVALID_WIRE && pin1->wire == WIRE_INVALID_WIRE)
    // {
    //     wire = w_CreateWire();
    //     w_ConnectWire(wire, device_a, pin_a);
    //     w_ConnectWire(wire, device_b, pin_b);
    // }
    // else if(pin0->wire == WIRE_INVALID_WIRE)
    // {
    //     wire = w_GetWire(pin1->wire);
    //     w_ConnectWire(wire, device_a, pin_a);
    // }
    // else if(pin1->wire == WIRE_INVALID_WIRE)
    // {
    //     wire = w_GetWire(pin0->wire);
    //     w_ConnectWire(wire, device_b, pin_b);
    // }
    // else
    // {
    //     struct wire_t *wire_a = w_GetWire(pin0->wire);
    //     struct wire_t *wire_b = w_GetWire(pin1->wire);
    //     wire = w_MergeWires(wire_a, wire_b);
    // }

    // for(uint32_t index = 0; index < wire_segments->cursor; index++)
    // {
    //     struct wire_seg_t *src_segment = list_GetElement(wire_segments, index);
    //     struct wire_seg_t *dst_segment = wire->last_segment_pos->segments + wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
    //     wire->segment_pos_count++;

    //     if(wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT == 0)
    //     {
    //         struct wire_seg_pos_block_t *new_block = pool_AddElement(&w_wire_seg_pos, NULL);
    //         new_block->prev = wire->last_segment_pos;
    //         wire->last_segment_pos->next = new_block;
    //         wire->last_segment_pos = new_block;
    //     }

    //     dst_segment->start[0] = src_segment->start[0];
    //     dst_segment->start[1] = src_segment->start[1];
    //     dst_segment->end[0] = src_segment->end[0];
    //     dst_segment->end[1] = src_segment->end[1];
    // }

    // return wire;
}

struct wire_t *w_CreateWireConnections(struct list_t *wire_segments)
{

}