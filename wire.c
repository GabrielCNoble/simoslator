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
struct pool_t               w_wire_pin_blocks[2];
struct pool_t               w_wire_seg_pos;
struct pool_t               w_wire_junc_pos;
struct pool_t               w_wire_segs;
struct pool_t               w_wire_juncs;

extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    w_wire_pin_blocks[0] = pool_CreateTyped(struct wire_junc_pin_block_t, 16384);
    w_wire_pin_blocks[1] = pool_CreateTyped(struct wire_junc_pin_block_t, 16384);
    w_wire_seg_pos = pool_CreateTyped(struct wire_seg_pos_block_t, 16384);
    w_wire_junc_pos = pool_CreateTyped(struct wire_junc_pos_block_t, 16384);
    w_wire_segs = pool_Create(sizeof(struct wire_seg_t), 16384, offsetof(struct wire_seg_t, base.POOL_ELEMENT_NAME));
    w_wire_juncs = pool_Create(sizeof(struct wire_junc_t), 16384, offsetof(struct wire_junc_t, base.POOL_ELEMENT_NAME));
}

void w_Shutdown()
{
    pool_Destroy(&w_wires);
    pool_Destroy(&w_wire_pin_blocks[0]);
    pool_Destroy(&w_wire_pin_blocks[1]);
    pool_Destroy(&w_wire_seg_pos);
    pool_Destroy(&w_wire_junc_pos);
    pool_Destroy(&w_wire_segs);
    pool_Destroy(&w_wire_juncs);
}

struct wire_t *w_AllocWire()
{
    struct wire_t *wire = pool_AddElement(&w_wires, NULL);

    for(uint32_t index = 0; index < 2; index++)
    {
        wire->wire_pins[index].first_block = pool_AddElement(&w_wire_pin_blocks[index], NULL);
        wire->wire_pins[index].last_block = wire->wire_pins[index].first_block;
        wire->wire_pins[index].pin_count = 0;

        struct wire_junc_pin_block_t *pin_block = wire->wire_pins[index].first_block;
        pin_block->next = NULL;
        pin_block->prev = NULL;
    }

    wire->first_junction = NULL;
    wire->last_junction = NULL;
    wire->first_segment = NULL;
    wire->last_segment = NULL;


    // if(wire->last_segment_pos == NULL)
    // {
    wire->first_segment_pos = pool_AddElement(&w_wire_seg_pos, NULL);
    wire->last_segment_pos = wire->first_segment_pos;
    wire->segment_pos_count = 0;

    // if(wire->last_junction_pos == NULL)
    // {
    wire->first_junction_pos = pool_AddElement(&w_wire_junc_pos, NULL);
    wire->last_junction_pos = wire->first_junction_pos;
    wire->junction_pos_count = 0;
    // }
    // }
    

    // wire->wire_outputs.first_block = list_AddElement(&w_output_pin_blocks, NULL);
    // wire->wire_outputs.block_count = 1;

    // pin_block = list_GetElement(&w_output_pin_blocks, wire->wire_outputs.first_block);
    // for(uint32_t index = 0; index < WIRE_PIN_BLOCK_PIN_COUNT; index++)
    // {
    //     pin_block->pins[index].device = DEV_INVALID_DEVICE;
    // }
    // wire->
    // wire->pin_block_count = list_AddElement(&w_);
    // wire->dev_block_count = 0;
    // wire->first_segment_pos = pool_AddElement(&w_wire_seg_pos, NULL);
    // wire->last_segment_pos = wire->first_segment_pos;
    // wire->segment_pos_count = 0;
    // wire->first_segment_pos_block = 0;
    // wire->segment_pos_block_count = 0;
    // wire->segment_pos_count = 0;
    // wire->value = WIRE_VALUE_Z;
    return wire;
}

struct wire_t *w_FreeWire(struct wire_t *wire)
{

}

struct wire_t *w_GetWire(uint32_t wire_index)
{
    return pool_GetValidElement(&w_wires, wire_index);
}

struct wire_junc_t *w_AddJunction(struct wire_seg_t *segment, int32_t *position)
{
    struct wire_t *wire = segment->base.wire;
    struct wire_junc_t *junction = NULL;
    uint32_t coord0;
    uint32_t coord1;

    coord0 = segment->pos->start[0] != segment->pos->end[0];
    coord1 = !coord0;

    if(position[coord0] == segment->pos->start[coord0])
    {
        uint32_t link_index;
        
        if(position[coord1] == segment->pos->start[coord1])
        {
            /* junction is to be created at the start of the segment */
            if(segment->junctions[WIRE_SEG_START_LINK].junction != NULL)
            {
                /* wire already has a junction at the start, so just return it */
                junction = segment->junctions[WIRE_SEG_START_LINK].junction;
            }
            else
            {
                junction = w_AllocWireJunction(wire);
                w_LinkSegmentToJunction(segment, junction, WIRE_SEG_START_LINK);
                junction->pos->pos[0] = segment->pos->start[0];
                junction->pos->pos[1] = segment->pos->start[1];
            }
        }
        else if(position[coord1] == segment->pos->end[coord1])
        {
            /* junction is to be created at the end of the segment */
            if(segment->junctions[WIRE_SEG_END_LINK].junction != NULL)
            {
                /* wire already has a junction at the end, so just return it */
                junction = segment->junctions[WIRE_SEG_END_LINK].junction;
            }
            else
            {
                junction = w_AllocWireJunction(wire);
                w_LinkSegmentToJunction(segment, junction, WIRE_SEG_END_LINK);
                junction->pos->pos[0] = segment->pos->end[0];
                junction->pos->pos[1] = segment->pos->end[1];
            }
        }
        else
        {
            /* junction is to be created at the middle of the segment */
            junction = w_AllocWireJunction(wire);
            struct wire_seg_t *split = w_AllocWireSegment(wire);
            split->pos->start[0] = position[0];
            split->pos->start[1] = position[1];
            split->pos->end[0] = segment->pos->end[0];
            split->pos->end[1] = segment->pos->end[1];
            segment->pos->end[0] = position[0];
            segment->pos->end[1] = position[1];

            split->next = segment->next;
            
            if(segment->next != NULL)
            {
                segment->next->prev = split;
            }

            segment->next = NULL;

            struct wire_junc_t *end_junction = segment->junctions[WIRE_SEG_END_LINK].junction;
            if(end_junction != NULL)
            {
                w_UnlinkSegmentLinkIndex(segment, WIRE_SEG_END_LINK);
                w_LinkSegmentToJunction(split, end_junction, WIRE_SEG_END_LINK);
            }            

            w_LinkSegmentToJunction(segment, junction, WIRE_SEG_END_LINK);
            w_LinkSegmentToJunction(split, junction, WIRE_SEG_START_LINK);

            junction->pos->pos[0] = position[0];
            junction->pos->pos[1] = position[1];
        }
    }

    return junction;

    // if(segment->pos->start[0] == segment->pos->end[0])
    // {
    //     /* vertical segment */
    //     coord0 = 0;
    //     coord1 = 
    // }
    // else
    // {

    // }
}

struct wire_seg_t *w_AllocWireSegment(struct wire_t *wire)
{
    struct wire_seg_t *segment = pool_AddElement(&w_wire_segs, NULL);
    segment->next = NULL;
    segment->prev = NULL;
    segment->junctions[0] = (struct wire_seg_junc_t){};
    segment->junctions[1] = (struct wire_seg_junc_t){};
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

    // if(wire->last_segment_pos == NULL)
    // {
    //     wire->first_segment_pos = pool_AddElement(&w_wire_seg_pos, NULL);
    //     wire->last_segment_pos = wire->first_segment_pos;
    //     wire->segment_pos_count = 0;
    // }
    
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

}

struct wire_junc_t *w_AllocWireJunction(struct wire_t *wire)
{
    struct wire_junc_t *junction = pool_AddElement(&w_wire_juncs, NULL);
    junction->pin = NULL;
    junction->first_segment = NULL;
    junction->last_segment = NULL;
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

    // if(wire->last_junction_pos == NULL)
    // {
    //     wire->first_junction_pos = pool_AddElement(&w_wire_junc_pos, NULL);
    //     wire->last_junction_pos = wire->first_junction_pos;
    //     wire->junction_pos_count = 0;
    // }
    
    junction->pos = wire->last_junction_pos->junctions + wire->junction_pos_count % WIRE_JUNCTION_POS_BLOCK_SIZE;
    junction->pos_block = wire->last_junction_pos;
    wire->junction_pos_count++;

    junction->pos->junction = junction;

    if(wire->junction_pos_count % WIRE_JUNCTION_POS_BLOCK_SIZE == 0)
    {
        struct wire_junc_pos_block_t *new_block = pool_AddElement(&w_wire_junc_pos, NULL);
        new_block->prev = wire->last_junction_pos;
        wire->last_junction_pos->next = new_block;
        wire->last_junction_pos = new_block;
    }

    return junction;
}

void w_FreeWireJunction(struct wire_junc_t *junction)
{

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
    }
    else
    { 
        junction->last_segment->junctions[junction->last_segment->junctions[1].junction == junction].next = segment;
        seg_junc->prev = junction->last_segment;
    }
    junction->last_segment = segment;
}

void w_UnlinkSegmentFromJunction(struct wire_seg_t *segment, struct wire_junc_t *junction)
{
    struct wire_seg_junc_t *seg_junc = &segment->junctions[segment->junctions[1].junction == junction];

    if(seg_junc->junction != junction)
    {
        printf("w_UnlinkSegmentFromJunction: segment is not linked to junction\n");
        return;
    }

    if(seg_junc->prev == NULL)
    {
        junction->first_segment = seg_junc->next;
    }
    else
    {
        seg_junc->prev->next = seg_junc->next;
    }

    if(seg_junc->next == NULL)
    {
        junction->last_segment = seg_junc->prev;
    }
    else
    {
        seg_junc->next->prev = seg_junc->prev;
    }

    seg_junc->junction = NULL;
    seg_junc->next = NULL;
    seg_junc->prev = NULL;
}

void w_UnlinkSegmentLinkIndex(struct wire_seg_t *segment, uint32_t link_index)
{
    struct wire_seg_junc_t *seg_junc = &segment->junctions[link_index];

    if(seg_junc->junction != NULL)
    {
        printf("w_UnlinkSegmentLinkIndex: segment isn't linked at the %s to a junction\n", (link_index == WIRE_SEG_START_LINK) ? "start" : "end");
        return;
    }

    struct wire_junc_t *junction = seg_junc->junction;

    if(seg_junc->prev == NULL)
    {
        junction->first_segment = seg_junc->next;
    }
    else
    {
        seg_junc->prev->next = seg_junc->next;
    }

    if(seg_junc->next == NULL)
    {
        junction->last_segment = seg_junc->prev;
    }
    else
    {
        seg_junc->next->prev = seg_junc->prev;
    }

    seg_junc->junction = NULL;
    seg_junc->next = NULL;
    seg_junc->prev = NULL;
}

// uint32_t w_AddPinBlocksAt(uint32_t index, uint32_t count)
// {
//     for(uint32_t add_index = 0; add_index < count; add_index++)
//     {
//         list_AddElement(&w_pin_blocks, NULL);
//     }

//     uint32_t first_block = w_pin_blocks.cursor - count;

//     if(index < first_block)
//     {
//         for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
//         {
//             struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

//             if(wire != NULL && wire->first_pin_block >= index)
//             {
//                 wire->first_pin_block += count;
//             }
//         }

//         for(int32_t move_index = w_pin_blocks.cursor - 1; move_index >= (int32_t)index; move_index--)
//         {
//             struct wire_pin_block_t *dst_block = list_GetElement(&w_pin_blocks, move_index);
//             struct wire_pin_block_t *src_block = list_GetElement(&w_pin_blocks, move_index - (int32_t)count);
//             *dst_block = *src_block;
//         }
//     }

//     return first_block;
// }

// void w_RemovePinBlocksAt(uint32_t index, uint32_t count)
// {

// }

// uint32_t w_AddDevBlocksAt(uint32_t index, uint32_t count)
// {
//     for(uint32_t add_index = 0; add_index < count; add_index++)
//     {
//         list_AddElement(&w_dev_blocks, NULL);
//     }

//     uint32_t first_block = w_dev_blocks.cursor - count;

//     if(index < first_block)
//     {
//         for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
//         {
//             struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

//             if(wire != NULL && wire->first_dev_block >= index)
//             {
//                 wire->first_dev_block += count;
//             }
//         }

//         for(int32_t move_index = w_dev_blocks.cursor - 1; move_index >= (int32_t)index; move_index--)
//         {
//             struct wire_dev_block_t *dst_block = list_GetElement(&w_dev_blocks, move_index);
//             struct wire_dev_block_t *src_block = list_GetElement(&w_dev_blocks, move_index - (int32_t)count);
//             *dst_block = *src_block;
//         }
//     }

//     return first_block;
// }

// uint64_t w_AddWirePinBlocks(struct wire_t *wire, uint64_t index, uint64_t count, uint32_t type)
// {
//     uint32_t first_block_index = list_ShiftAndInsertAt(&w_wire_pin_blocks[type], wire->wire_pins[type].first_block + index, count);

//     for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
//     {
//         /* test which wires have blocks after index and shift them towards the end by count */
//         struct wire_t *shift_wire = pool_GetValidElement(&w_wires, wire_index);

//         if(shift_wire != NULL && shift_wire->wire_pins[type].first_block >= first_block_index)
//         {
//             shift_wire->wire_pins[type].first_block += count;
//         }
//     }

//     for(uint32_t block_index = 0; block_index < count; block_index++)
//     {
//         struct wire_pin_block_t *pin_block = list_GetElement(&w_wire_pin_blocks[type], first_block_index + block_index);
//         for(uint32_t pin_index = 0; pin_index < WIRE_PIN_BLOCK_PIN_COUNT; pin_index++)
//         {
//             pin_block->pins[pin_index].device = DEV_INVALID_DEVICE;
//         }
//     }

//     wire->wire_pins[type].block_count += count;

//     return first_block_index;
// }

// void w_RemoveWirePinBlocks(struct wire_t *wire, uint64_t index, uint64_t count, uint32_t type)
// {
//     // uint32_t first_block_index = list_ShiftAndInsertAt(&w_wire_pin_blocks[type], &wire->wire_pins[type].first_block + index, count);
//     uint32_t first_block_index = wire->wire_pins[type].first_block + index;

//     list_RemoveAtAndShift(&w_wire_pin_blocks[type], wire->wire_pins[type].first_block + index, count);

//     for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
//     {
//         /* test which wires have blocks after index and shift them towards the start by count */
//         struct wire_t *shift_wire = pool_GetValidElement(&w_wires, wire_index);

//         if(shift_wire != NULL && shift_wire->wire_pins[type].first_block >= first_block_index)
//         {
//             shift_wire->wire_pins[type].first_block -= count;
//         }
//     }

//     wire->wire_pins[type].block_count -= count;
// }

void w_RemoveJunction(struct wire_junc_t *junction)
{

}

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b)
{
    // if(wire_a != NULL && wire_b != NULL && wire_a != wire_b)
    // {
    //     for(uint32_t pin_type = 0; pin_type < 2; pin_type++)
    //     {
    //         struct wire_seg_pins_t *wire_a_pins = &wire_a->wire_pins[pin_type];
    //         struct wire_seg_pins_t *wire_b_pins = &wire_b->wire_pins[pin_type];

    //         if(wire_a_pins->pin_count != 0 || wire_b_pins->pin_count != 0)
    //         {
    //             struct wire_seg_pin_block_t *wire_b_pin_block = wire_b_pins->first_block;
    //             uint32_t total_pin_count = wire_b_pins->pin_count;

    //             while(wire_b_pin_block != NULL)
    //             {
    //                 uint32_t pin_count = WIRE_SEG_PIN_BLOCK_PIN_COUNT;
    //                 if(pin_count > total_pin_count)
    //                 {
    //                     pin_count = total_pin_count;
    //                 }

    //                 total_pin_count -= pin_count;

    //                 for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
    //                 {
    //                     struct wire_pin_t *wire_pin = wire_b_pin_block->pins + pin_index;
    //                     struct dev_t *device = dev_GetDevice(wire_pin->device);
    //                     struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin->pin);
    //                     device_pin->wire = wire_a->element_index;
    //                 }

    //                 if(wire_b_pin_block->next == NULL)
    //                 {
    //                     break;
    //                 }

    //                 wire_b_pin_block = wire_b_pin_block->next;
    //             }
                
    //             struct wire_seg_pin_block_t *wire_a_pin_block = wire_a_pins->last_block;
    //             uint32_t wire_a_pin_index = wire_a_pins->pin_count % WIRE_SEG_PIN_BLOCK_PIN_COUNT;
    //             uint32_t wire_b_pin_index = (wire_b_pins->pin_count - 1) % WIRE_SEG_PIN_BLOCK_PIN_COUNT;
    //             uint32_t move_size = WIRE_SEG_PIN_BLOCK_PIN_COUNT - (wire_a_pin_index - 1);

    //             if(move_size >= wire_b_pin_index)
    //             {
    //                 move_size = wire_b_pin_index + 1;
    //             }

    //             while(move_size)
    //             {
    //                 wire_a_pin_block->pins[wire_a_pin_index] = wire_b_pin_block->pins[wire_b_pin_index];
    //                 wire_a_pin_index++;
    //                 wire_b_pin_index--;
    //                 move_size--;
    //             }

    //             if(wire_b_pin_index == 0xffffffff)
    //             {
    //                 if(wire_b_pins->first_block == wire_b_pin_block)
    //                 {
    //                     wire_b_pins->first_block = NULL;
    //                     wire_b_pins->last_block = NULL;
    //                 }
    //                 else
    //                 {
    //                     wire_b_pins->last_block = wire_b_pins->last_block->prev;
    //                     wire_b_pins->last_block->next = NULL;
    //                 }

    //                 pool_RemoveElement(&w_wire_pin_blocks[pin_type], wire_b_pin_block->element_index);
    //             }

    //             if(wire_b_pins->first_block != NULL)
    //             {
    //                 wire_a_pins->last_block->next = wire_b_pins->first_block;
    //                 wire_a_pins->last_block = wire_b_pins->last_block;
    //             }

    //             wire_a_pins->pin_count += wire_b_pins->pin_count;
    //         }
    //     }

    //     struct wire_seg_pos_block_t *wire_a_last_block = wire_a->last_segment_pos;
    //     struct wire_seg_pos_block_t *wire_b_last_block = wire_b->first_segment_pos;

    //     uint32_t wire_a_block_offset = wire_a->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SIZE;
    //     uint32_t wire_b_block_offset = (wire_b->segment_pos_count - 1) % WIRE_SEGMENT_POS_BLOCK_SIZE;
    //     uint32_t move_size = WIRE_SEGMENT_POS_BLOCK_SIZE - wire_a_block_offset;
    //     if(move_size >= wire_b_block_offset)
    //     {
    //         move_size = wire_b_block_offset + 1;
    //     }

    //     while(move_size)
    //     {
    //         wire_a_last_block->segments[wire_a_block_offset] = wire_b_last_block->segments[wire_b_block_offset];
    //         wire_a_last_block->segments[wire_a_block_offset].segment->pos = wire_a_last_block->segments + wire_a_block_offset;
    //         wire_a_last_block->segments[wire_a_block_offset].segment->pos_block = wire_a_last_block;
    //         wire_a_block_offset++;
    //         wire_b_block_offset--;
    //         move_size--;
    //     }

    //     if(wire_b_block_offset == 0xffffffff)
    //     {
    //         if(wire_b_last_block == wire_b->first_segment_pos)
    //         {
    //             wire_b->first_segment_pos = NULL;
    //             wire_b->last_segment_pos = NULL;
    //         }
    //         else
    //         {
    //             wire_b->last_segment_pos = wire_b->last_segment_pos->prev;
    //             wire_b->last_segment_pos->next = NULL;
    //         }

    //         pool_RemoveElement(&w_wire_seg_pos, wire_b_last_block->element_index);
    //     }

    //     if(wire_b->first_segment_pos != NULL)
    //     {
    //         wire_a->last_segment_pos->next = wire_b->first_segment_pos;
    //         wire_a->last_segment_pos = wire_b->last_segment_pos;
    //     }
        
    //     wire_a->segment_pos_count += wire_b->segment_pos_count;
    //     pool_RemoveElement(&w_wires, wire_b->element_index);
    // }

    return wire_a;
}

void w_SplitWire(struct wire_t *wire, struct wire_seg_t *segment)
{

}

struct wire_junc_pin_t *w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin)
{
    struct dev_pin_t *device_pin = dev_GetDevicePin(device, pin);
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;
    struct pool_t *wire_pin_block_list;
    struct wire_junc_pins_t *wire_pins;

    wire_pin_block_list = &w_wire_pin_blocks[pin_desc->type];
    wire_pins = &wire->wire_pins[pin_desc->type];

    struct wire_junc_pin_t *wire_pin = wire_pins->last_block->pins + wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT;
    wire_pins->pin_count++;

    if(wire_pins->pin_count % WIRE_JUNC_PIN_BLOCK_PIN_COUNT == 0)
    {
        struct wire_junc_pin_block_t *new_block = pool_AddElement(wire_pin_block_list, NULL);
        new_block->prev = wire_pins->last_block;
        wire_pins->last_block->next = new_block;
        wire_pins->last_block = new_block;
    }
    
    wire_pin->pin.device = device->element_index;
    wire_pin->pin.pin = pin;
    device_pin->wire = wire->element_index;    

    return wire_pin;
}

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin)
{

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