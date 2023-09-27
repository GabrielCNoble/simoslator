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
struct list_t               w_pin_blocks;
struct list_t               w_dev_blocks;
struct list_t               w_wire_segments;
extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    w_pin_blocks = list_Create(sizeof(struct wire_pin_block_t), 16384);
    w_dev_blocks = list_Create(sizeof(struct wire_dev_block_t), 16384);
    w_wire_segments = list_Create(sizeof(struct wire_segment_t), 16384);
}

void w_Shutdown()
{

}

struct wire_t *w_CreateWire()
{
    struct wire_t *wire = pool_AddElement(&w_wires, NULL);
    wire->pin_block_count = 0;
    wire->value = WIRE_VALUE_Z;
    return wire;
}

struct wire_t *w_GetWire(uint32_t wire_index)
{
    return pool_GetValidElement(&w_wires, wire_index);
}

uint32_t w_AddPinBlocksAt(uint32_t index, uint32_t count)
{
    for(uint32_t add_index = 0; add_index < count; add_index++)
    {
        list_AddElement(&w_pin_blocks, NULL);
    }

    uint32_t first_block = w_pin_blocks.cursor - count;

    if(index < first_block)
    {
        for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
        {
            struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

            if(wire != NULL && wire->first_pin_block >= index)
            {
                wire->first_pin_block += count;
            }
        }

        for(int32_t move_index = w_pin_blocks.cursor - 1; move_index >= (int32_t)index; move_index--)
        {
            struct wire_pin_block_t *dst_block = list_GetElement(&w_pin_blocks, move_index);
            struct wire_pin_block_t *src_block = list_GetElement(&w_pin_blocks, move_index - (int32_t)count);
            *dst_block = *src_block;
        }
    }

    return first_block;
}

void w_RemovePinBlocksAt(uint32_t index, uint32_t count)
{

}

uint32_t w_AddDevBlocksAt(uint32_t index, uint32_t count)
{
    for(uint32_t add_index = 0; add_index < count; add_index++)
    {
        list_AddElement(&w_dev_blocks, NULL);
    }

    uint32_t first_block = w_dev_blocks.cursor - count;

    if(index < first_block)
    {
        for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
        {
            struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

            if(wire != NULL && wire->first_dev_block >= index)
            {
                wire->first_dev_block += count;
            }
        }

        for(int32_t move_index = w_dev_blocks.cursor - 1; move_index >= (int32_t)index; move_index--)
        {
            struct wire_dev_block_t *dst_block = list_GetElement(&w_dev_blocks, move_index);
            struct wire_dev_block_t *src_block = list_GetElement(&w_dev_blocks, move_index - (int32_t)count);
            *dst_block = *src_block;
        }
    }

    return first_block;
}

void w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin)
{
    uint32_t block_index = pin / DEV_PIN_BLOCK_PIN_COUNT;
    uint32_t dev_pin_index = pin % DEV_PIN_BLOCK_PIN_COUNT;
    struct dev_pin_block_t *dev_pin_block = list_GetElement(&dev_pin_blocks, device->first_pin_block + block_index);
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;

    if(pin_desc->type == DEV_PIN_TYPE_IN)
    {
        struct wire_dev_block_t *wire_dev_block = NULL;
        uint32_t wire_dev_index = 0;

        if(wire->dev_block_count == 0)
        {
            wire->first_dev_block = w_AddDevBlocksAt(0xffffffff, 1);
            wire->dev_block_count++;
            wire_dev_block = list_GetElement(&w_dev_blocks, wire->first_dev_block);
            for(uint32_t index = 0; index < WIRE_DEV_BLOCK_DEV_COUNT; index++)
            {
                wire_dev_block->devices[index].device = WIRE_INVALID_DEVICE;
            }
        }
        else
        {
            wire_dev_block = list_GetElement(&w_dev_blocks, wire->first_dev_block + (wire->dev_block_count - 1));

            for(wire_dev_index = 0; wire_dev_index < WIRE_DEV_BLOCK_DEV_COUNT; wire_dev_index++)
            {
                if(wire_dev_block->devices[wire_dev_index].device == WIRE_INVALID_DEVICE)
                {
                    break;
                }
            }

            if(wire_dev_index == WIRE_INVALID_DEVICE)
            {
                uint32_t block_index = w_AddDevBlocksAt(wire->first_dev_block + wire->dev_block_count, 1);
                wire->dev_block_count++;
                wire_dev_block = list_GetElement(&w_dev_blocks, block_index);
                for(uint32_t index = 0; index < WIRE_DEV_BLOCK_DEV_COUNT; index++)
                {
                    wire_dev_block->devices[index].device = WIRE_INVALID_DEVICE;
                }   
                wire_dev_index = 0;
            }
        }

        wire_dev_block->devices[wire_dev_index].device = device->element_index;
        wire_dev_block->devices[wire_dev_index].pin = pin;
    }
    else if(pin_desc->type == DEV_PIN_TYPE_OUT)
    {
        struct wire_pin_block_t *wire_pin_block = NULL;
        uint32_t wire_pin_index = 0;

        if(wire->pin_block_count == 0)
        {
            wire->first_pin_block = w_AddPinBlocksAt(0xffffffff, 1);
            wire->pin_block_count++;
            wire_pin_block = list_GetElement(&w_pin_blocks, wire->first_pin_block);
            for(uint32_t index = 0; index < WIRE_PIN_BLOCK_PIN_COUNT; index++)
            {
                wire_pin_block->pins[index].device = WIRE_INVALID_DEVICE;
            }
        }
        else
        {
            wire_pin_block = list_GetElement(&w_pin_blocks, wire->first_pin_block + (wire->pin_block_count - 1));

            for(wire_pin_index = 0; wire_pin_index < WIRE_PIN_BLOCK_PIN_COUNT; wire_pin_index++)
            {
                if(wire_pin_block->pins[wire_pin_index].device == WIRE_INVALID_DEVICE)
                {
                    break;
                }
            }

            if(wire_pin_index == WIRE_PIN_BLOCK_PIN_COUNT)
            {
                uint64_t block_index = w_AddPinBlocksAt(wire->first_pin_block + wire->pin_block_count, 1);
                wire->pin_block_count++;
                wire_pin_block = list_GetElement(&w_pin_blocks, block_index);
                for(uint32_t index = 0; index < WIRE_PIN_BLOCK_PIN_COUNT; index++)
                {
                    wire_pin_block->pins[index].device = WIRE_INVALID_DEVICE;
                }
                wire_pin_index = 0;
            }
        }

        wire_pin_block->pins[wire_pin_index].device = device->element_index;
        wire_pin_block->pins[wire_pin_index].pin = pin;
    }

    dev_pin_block->pins[dev_pin_index].wire = wire->element_index;
}

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin)
{

}

void w_WireStep(struct wire_t *wire)
{
    uint8_t wire_value = WIRE_VALUE_Z;

    for(uint32_t block_index = 0; block_index < wire->pin_block_count; block_index++)
    {
        struct wire_pin_block_t *block = list_GetElement(&w_pin_blocks, wire->first_pin_block + block_index);
        uint32_t pin_index = 0;

        while(block->pins[pin_index].device != WIRE_INVALID_DEVICE && block->pins[pin_index].pin != WIRE_INVALID_PIN)
        {
            struct dev_t *device = pool_GetElement(&dev_devices, block->pins[pin_index].device);
            // struct dev_pin_block_t *pin_block = dev_GetDevicePinBlock(device, block->pins[pin_index].pin);
            uint32_t device_pin_index = block->pins[pin_index].pin % DEV_PIN_BLOCK_PIN_COUNT;
            uint32_t device_pin_block_index = block->pins[pin_index].pin / DEV_PIN_BLOCK_PIN_COUNT;
            struct dev_pin_block_t *pin_block = list_GetElement(&dev_pin_blocks, device->first_pin_block + device_pin_block_index);
            wire_value = w_wire_value_resolution[wire_value][pin_block->pins[device_pin_index].value];
            pin_index++;
        }
    }

    if(wire_value != wire->value)
    {
        for(uint32_t block_index = 0; block_index < wire->dev_block_count; block_index++)
        {
            struct wire_dev_block_t *block = list_GetElement(&w_dev_blocks, wire->first_dev_block + block_index);
            uint32_t device_index = 0;

            while(block->devices[device_index].device != WIRE_INVALID_DEVICE)
            {
                struct dev_t *device = pool_GetElement(&dev_devices, block->devices[device_index].device);
                sim_QueueDevice(device);
                device_index++;
            }
        }
    }

    wire->value = wire_value;
}