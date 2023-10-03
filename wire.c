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
// struct list_t               w_wire_sim_data;
// struct list_t               w_wire_pins;
// struct list_t               w_input_pin_blocks;
// struct list_t               w_output_pin_blocks;w_wire_segment_copy_buffer
struct pool_t               w_wire_pin_blocks[2];
struct pool_t               w_wire_segment_positions;
// struct list_t               w_wire_segment_copy_buffer;
struct pool_t               w_wire_segments;
extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    // w_wire_sim_data = list_Create(sizeof(struct wire_sim_data_t), 16384);
    // w_wire_pins = list_Create(sizeof(struct wire_pin_t), 16384);
    w_wire_pin_blocks[0] = pool_CreateTyped(struct wire_pin_block_t, 16384);
    w_wire_pin_blocks[1] = pool_CreateTyped(struct wire_pin_block_t, 16384);
    w_wire_segment_positions = pool_CreateTyped(struct wire_segment_pos_block_t, 16384);
    // w_wire_segment_positions = list_Create(sizeof(struct wire_segment_pos_t), 16384);
    // w_wire_segment_copy_buffer = list_Create(sizeof(struct wire_segment_pos_t), 16384);
    w_wire_segments = pool_CreateTyped(struct wire_segment_t, 16384);
}

void w_Shutdown()
{

}

struct wire_t *w_CreateWire()
{
    struct wire_t *wire = pool_AddElement(&w_wires, NULL);

    for(uint32_t index = 0; index < 2; index++)
    {
        wire->wire_pins[index].first_block = pool_AddElement(&w_wire_pin_blocks[index], NULL);
        wire->wire_pins[index].last_block = wire->wire_pins[index].first_block;
        wire->wire_pins[index].pin_count = 0;

        struct wire_pin_block_t *pin_block = wire->wire_pins[index].first_block;
        pin_block->next = NULL;
        pin_block->prev = NULL;
    }
    

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
    wire->first_segment_pos = pool_AddElement(&w_wire_segment_positions, NULL);
    wire->last_segment_pos = wire->first_segment_pos;
    wire->segment_pos_count = 0;
    // wire->first_segment_pos_block = 0;
    // wire->segment_pos_block_count = 0;
    // wire->segment_pos_count = 0;
    wire->value = WIRE_VALUE_Z;
    return wire;
}

struct wire_t *w_GetWire(uint32_t wire_index)
{
    return pool_GetValidElement(&w_wires, wire_index);
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

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b)
{
    if(wire_a != NULL && wire_b != NULL && wire_a != wire_b)
    {
        for(uint32_t pin_type = 0; pin_type < 2; pin_type++)
        {
            struct wire_pins_t *wire_a_pins = &wire_a->wire_pins[pin_type];
            struct wire_pins_t *wire_b_pins = &wire_b->wire_pins[pin_type];

            if(wire_a_pins->pin_count != 0 || wire_b_pins->pin_count != 0)
            {
                struct wire_pin_block_t *wire_b_pin_block = wire_b_pins->first_block;
                uint32_t total_pin_count = wire_b_pins->pin_count;

                while(wire_b_pin_block != NULL)
                {
                    uint32_t pin_count = WIRE_PIN_BLOCK_PIN_COUNT;
                    if(pin_count > total_pin_count)
                    {
                        pin_count = total_pin_count;
                    }

                    total_pin_count -= pin_count;

                    for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
                    {
                        struct wire_pin_t *wire_pin = wire_b_pin_block->pins + pin_index;
                        struct dev_t *device = dev_GetDevice(wire_pin->device);
                        struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin->pin);
                        device_pin->wire = wire_a->element_index;
                    }

                    if(wire_b_pin_block->next == NULL)
                    {
                        break;
                    }

                    wire_b_pin_block = wire_b_pin_block->next;
                }
                
                struct wire_pin_block_t *wire_a_pin_block = wire_a_pins->last_block;
                uint32_t wire_a_pin_index = wire_a_pins->pin_count % WIRE_PIN_BLOCK_PIN_COUNT;
                uint32_t wire_b_pin_index = (wire_b_pins->pin_count - 1) % WIRE_PIN_BLOCK_PIN_COUNT;
                uint32_t move_size = WIRE_PIN_BLOCK_PIN_COUNT - (wire_a_pin_index - 1);

                if(move_size >= wire_b_pin_index)
                {
                    move_size = wire_b_pin_index + 1;
                }

                while(move_size)
                {
                    wire_a_pin_block->pins[wire_a_pin_index] = wire_b_pin_block->pins[wire_b_pin_index];
                    wire_a_pin_index++;
                    wire_b_pin_index--;
                    move_size--;
                }

                if(wire_b_pin_index == 0xffffffff)
                {
                    if(wire_b_pins->first_block == wire_b_pin_block)
                    {
                        wire_b_pins->first_block = NULL;
                        wire_b_pins->last_block = NULL;
                    }
                    else
                    {
                        wire_b_pins->last_block = wire_b_pins->last_block->prev;
                        wire_b_pins->last_block->next = NULL;
                    }

                    pool_RemoveElement(&w_wire_pin_blocks[pin_type], wire_b_pin_block->element_index);
                }

                if(wire_b_pins->first_block != NULL)
                {
                    wire_a_pins->last_block->next = wire_b_pins->first_block;
                    wire_a_pins->last_block = wire_b_pins->last_block;
                }

                wire_a_pins->pin_count += wire_b_pins->pin_count;
            }
        }

        struct wire_segment_pos_block_t *wire_a_last_block = wire_a->last_segment_pos;
        struct wire_segment_pos_block_t *wire_b_last_block = wire_b->first_segment_pos;

        uint32_t wire_a_block_offset = wire_a->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
        uint32_t wire_b_block_offset = (wire_b->segment_pos_count - 1) % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
        uint32_t move_size = WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT - wire_a_block_offset;
        if(move_size >= wire_b_block_offset)
        {
            move_size = wire_b_block_offset + 1;
        }

        while(move_size)
        {
            wire_a_last_block->segments[wire_a_block_offset] = wire_b_last_block->segments[wire_b_block_offset];
            wire_a_block_offset++;
            wire_b_block_offset--;
            move_size--;
        }

        if(wire_b_block_offset == 0xffffffff)
        {
            if(wire_b_last_block == wire_b->first_segment_pos)
            {
                wire_b->first_segment_pos = NULL;
                wire_b->last_segment_pos = NULL;
            }
            else
            {
                wire_b->last_segment_pos = wire_b->last_segment_pos->prev;
                wire_b->last_segment_pos->next = NULL;
            }

            pool_RemoveElement(&w_wire_segment_positions, wire_b_last_block->element_index);
        }

        if(wire_b->first_segment_pos != NULL)
        {
            wire_a->last_segment_pos->next = wire_b->first_segment_pos;
            wire_a->last_segment_pos = wire_b->last_segment_pos;
        }
        
        wire_a->segment_pos_count += wire_b->segment_pos_count;
        pool_RemoveElement(&w_wires, wire_b->element_index);
    }

    return wire_a;
}

void w_SplitWire(struct wire_t *wire, struct wire_segment_t *segment)
{

}

void w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin)
{
    struct dev_pin_t *device_pin = dev_GetDevicePin(device, pin);
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;
    struct pool_t *wire_pin_block_list;
    struct wire_pins_t *wire_pins;

    wire_pin_block_list = &w_wire_pin_blocks[pin_desc->type];
    wire_pins = &wire->wire_pins[pin_desc->type];

    struct wire_pin_t *wire_pin = wire_pins->last_block->pins + wire_pins->pin_count % WIRE_PIN_BLOCK_PIN_COUNT;
    wire_pins->pin_count++;

    if(wire_pins->pin_count % WIRE_PIN_BLOCK_PIN_COUNT == 0)
    {
        struct wire_pin_block_t *new_block = pool_AddElement(wire_pin_block_list, NULL);
        new_block->prev = wire_pins->last_block;
        wire_pins->last_block->next = new_block;
        wire_pins->last_block = new_block;
    }

    // if(wire_pins->pin_count / WIRE_PIN_BLOCK_PIN_COUNT >= wire_pins->block_count)
    // {
    //     w_AddWirePinBlocks(wire, wire_pins->block_count, 1, pin_desc->type);
    // }

    // uint32_t wire_pin_index = wire_pins->pin_count % WIRE_PIN_BLOCK_PIN_COUNT;
    // struct wire_pin_block_t *wire_pin_block = list_GetElement(wire_pin_block_list, wire_pins->first_block + (wire_pins->block_count - 1));
    // wire_pin_block->pins[wire_pin_index].device = device->element_index;
    // wire_pin_block->pins[wire_pin_index].pin = pin;
    // wire_pins->pin_count++;
    wire_pin->device = device->element_index;
    wire_pin->pin = pin;
    device_pin->wire = wire->element_index;    
}

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin)
{

}

struct wire_t *w_ConnectPins(struct dev_t *device_a, uint16_t pin_a, struct dev_t *device_b, uint16_t pin_b)
{
    struct dev_pin_t *pin0 = dev_GetDevicePin(device_a, pin_a);
    struct dev_pin_t *pin1 = dev_GetDevicePin(device_b, pin_b);
    struct wire_t *wire;
    
    if(pin0->wire == WIRE_INVALID_WIRE && pin1->wire == WIRE_INVALID_WIRE)
    {
        wire = w_CreateWire();
        w_ConnectWire(wire, device_a, pin_a);
        w_ConnectWire(wire, device_b, pin_b);
    }
    else if(pin0->wire == WIRE_INVALID_WIRE)
    {
        wire = w_GetWire(pin1->wire);
        w_ConnectWire(wire, device_a, pin_a);
    }
    else if(pin1->wire == WIRE_INVALID_WIRE)
    {
        wire = w_GetWire(pin0->wire);
        w_ConnectWire(wire, device_b, pin_b);
    }
    else
    {
        struct wire_t *wire_a = w_GetWire(pin0->wire);
        struct wire_t *wire_b = w_GetWire(pin1->wire);
        wire = w_MergeWires(wire_a, wire_b);
    }

    // uint32_t segment_index = list_ShiftAndInsertAt(&w_wire_segment_positions, wire->first_segment_pos + wire->segment_pos_count, 1);

    // struct wire_segment_pos_block_t *segment_block;
    struct wire_segment_pos_t *segment_pos = wire->last_segment_pos->segments + wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
    wire->segment_pos_count++;

    if(wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT == 0)
    {
        struct wire_segment_pos_block_t *new_block = pool_AddElement(&w_wire_segment_positions, NULL);
        new_block->prev = wire->last_segment_pos;
        wire->last_segment_pos->next = new_block;
        wire->last_segment_pos = new_block;
    }

    // if(wire->segment_pos_count / WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT >= wire->segment_pos_block_count)
    // {
    //     uint64_t block_index = list_AddElement(&w_wire_segment_positions, NULL);
    //     segment_block = list_GetElement(&w_wire_segment_positions, block_index);
    //     if(wire->segment_pos == NULL)
    //     {
    //         wire->segment_pos = segment_block;
    //     }
    //     else
    //     {
    //         wire->last_segment_pos->next = segment_block;
    //     }

    //     segment_block->prev = wire->last_segment_pos;
    //     wire->last_segment_pos = segment_block;
    //     wire->segment_pos_block_count++;
    // }

    // segment_block = wire->last_segment_pos;
    // uint32_t segment_index = wire->segment_pos_count % WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT;
    // wire->segment_pos_count++;

    // for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
    // {
    //     struct wire_t *shift_wire = pool_GetValidElement(&w_wires, wire_index);
    //     if(shift_wire != NULL && shift_wire != wire)
    //     {
    //         if(shift_wire->first_segment_pos >= segment_index)
    //         {
    //             shift_wire->first_segment_pos++;
    //         }
    //     }
    // }

    // if(wire->segment_pos_count == 0)
    // {
    //     wire->first_segment_pos = segment_index;
    // }

    // wire->segment_pos_count++;
    
    // struct wire_segment_pos_t *segment = list_GetElement(&w_wire_segment_positions, segment_index);
    // struct wire_segment_pos_t *segment = segment_block->segments + segment_index;
    struct dev_pin_desc_t *desc0 = dev_device_descs[device_a->type].pins + pin_a;
    struct dev_pin_desc_t *desc1 = dev_device_descs[device_b->type].pins + pin_b;
    segment_pos->start[0] = device_a->position[0] + desc0->offset[0];
    segment_pos->start[1] = device_a->position[1] + desc0->offset[1];

    segment_pos->end[0] = device_b->position[0] + desc1->offset[0];
    segment_pos->end[1] = device_b->position[1] + desc1->offset[1];

    return wire;
}

void w_WireStep(struct wire_t *wire)
{
    // uint8_t wire_value = WIRE_VALUE_Z;

    // for(uint32_t block_index = 0; block_index < wire->wire_inputs.block_count; block_index++)
    // {
    //     struct wire_pin_block_t *wire_pin_block = list_GetElement(&w_wire_pin_blocks[DEV_PIN_TYPE_OUT], wire->wire_inputs.first_block + block_index);
    //     uint32_t wire_pin_index = 0;

    //     while(wire_pin_block->pins[wire_pin_index].device != DEV_INVALID_DEVICE)
    //     {
    //         struct dev_t *device = pool_GetElement(&dev_devices, wire_pin_block->pins[wire_pin_index].device);
    //         // uint32_t device_pin_index = wire_pin_block->pins[wire_pin_index].pin % DEV_PIN_BLOCK_PIN_COUNT;
    //         // uint32_t device_pin_block_index = wire_pin_block->pins[wire_pin_index].pin / DEV_PIN_BLOCK_PIN_COUNT;
    //         // struct dev_pin_block_t *device_pin_block = list_GetElement(&dev_pin_blocks, device->first_pin_block + device_pin_block_index);
    //         // wire_value = w_wire_value_resolution[wire_value][device_pin_block->pins[device_pin_index].value];
    //         struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin_block->pins[wire_pin_index].pin);
    //         wire_value = w_wire_value_resolution[wire_value][device_pin->value];
    //         wire_pin_index++;
    //     }
    // }

    // if(wire_value != wire->value)
    // {
    //     for(uint32_t block_index = 0; block_index < wire->wire_outputs.block_count; block_index++)
    //     {
    //         struct wire_pin_block_t *wire_pin_block = list_GetElement(&w_wire_pin_blocks[DEV_PIN_TYPE_IN], wire->wire_outputs.first_block + block_index);
    //         uint32_t device_index = 0;

    //         while(wire_pin_block->pins[device_index].device != DEV_INVALID_DEVICE)
    //         {
    //             struct dev_t *device = pool_GetElement(&dev_devices, wire_pin_block->pins[device_index].device);
    //             sim_QueueDevice(device);
    //             device_index++;
    //         }
    //     }
    // }

    // wire->value = wire_value;
}