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
extern struct pool_t        dev_devices;
extern struct list_t        dev_pin_blocks;
extern struct dev_desc_t    dev_device_descs[];

void w_Init()
{
    w_wires = pool_CreateTyped(struct wire_t, 4096);
    w_pin_blocks = list_Create(sizeof(struct wire_pin_block_t), 4096);
    w_dev_blocks = list_Create(sizeof(struct wire_dev_block_t), 4096);
}

void w_Shutdown()
{

}

struct wire_t *w_CreateWire()
{

}

void w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin)
{
    uint32_t block_index = pin / DEV_PIN_BLOCK_PIN_COUNT;
    uint32_t pin_index = pin % DEV_PIN_BLOCK_PIN_COUNT;
    struct dev_pin_block_t *pin_block = list_GetElement(&dev_pin_blocks, device->first_pin_block + block_index);
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;

    if(pin_desc->type == DEV_PIN_TYPE_IN)
    {

    }
    else if(pin_desc->type == DEV_PIN_TYPE_OUT)
    {
        if(wire->pin_block_count == 0)
        {
            // uint64_t 
        }
        struct w_pin_blocks *last_block = list_GetElement(&w_pin_blocks, wire->first_pin_block + (wire->pin_block_count - 1));
        pin_block->pins[pin_index].wire = wire->element_index;
    }
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

            while(block->devices[device_index] != WIRE_INVALID_DEVICE)
            {
                struct dev_t *device = list_GetElement(&dev_devices, block->devices[device_index]);
                sim_QueueDevice(device);
                device_index++;
            }
        }
    }
}