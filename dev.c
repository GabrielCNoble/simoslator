#include "dev.h"
#include "GL/glew.h"
#include "stb_image.h"
#include "list.h"
#include "wire.h"
#include "sim.h"


// struct pool_t dev_primitives;
struct pool_t                       dev_devices;
struct pool_t                       dev_inputs;
struct pool_t                       dev_pin_blocks;
struct dev_pin_t *                  dev_prev_pin_values;
extern struct pool_t                w_wires;
extern struct list_t                sim_wire_data;
extern struct list_t                sim_wire_pins;
extern struct list_t                sim_dev_data;
extern struct list_t                sim_dev_pins;

const char *dev_device_texture_names[] = {
    // [DEV_PRIMITIVE_PNMOS]  = "res/pnmos.png",
    // [DEV_PRIMITIVE_PMOS]   = "res/pmos.png",
    // [DEV_PRIMITIVE_NMOS]   = "res/nmos.png",
    // [DEV_PRIMITIVE_GND]    = "res/gnd.png",
    // [DEV_PRIMITIVE_POW]    = "res/pow.png"
};



// GLuint dev_device_textures[DEV_DEVICE_TYPE_LAST];
GLuint      dev_devices_texture;
uint32_t    dev_devices_texture_width;
uint32_t    dev_devices_texture_height;

// uint32_t dev_device_texture_offsets[DEV_DEVICE_TYPE_LAST][2] = {
//     [DEV_DEVICE_TYPE_PMOS] = {38, 2},
//     [DEV_DEVICE_TYPE_NMOS] = {6, 2},
//     [DEV_DEVICE_TYPE_GND] = {0, 58},
//     [DEV_DEVICE_TYPE_POW] = {20, 52}
// };

void dev_PMosStep(struct sim_dev_data_t *device)
{
    struct dev_pin_t *gate_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_GATE);
    struct dev_pin_t *source_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_SOURCE);
    struct dev_pin_t *drain_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_DRAIN);
    struct sim_wire_data_t *gate_wire = list_GetElement(&sim_wire_data, gate_pin->wire);
    struct sim_wire_data_t *source_wire = list_GetElement(&sim_wire_data, source_pin->wire);
    struct sim_wire_data_t *drain_wire = list_GetElement(&sim_wire_data, drain_pin->wire);
    uint8_t prev_value = drain_pin->value;

    if((gate_wire->value == WIRE_VALUE_0S || gate_wire->value == WIRE_VALUE_0W) && (source_wire->value == WIRE_VALUE_1S || source_wire->value == WIRE_VALUE_1W))
    {
        drain_pin->value = source_wire->value;
    }
    else
    {
        drain_pin->value = WIRE_VALUE_Z;
    }

    if(prev_value != drain_pin->value)
    {
        sim_QueueWire(drain_wire);
    }
}

void dev_NMosStep(struct sim_dev_data_t *device)
{
    struct dev_pin_t *gate_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_GATE);
    struct dev_pin_t *source_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_SOURCE);
    struct dev_pin_t *drain_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_DRAIN);
    struct sim_wire_data_t *gate_wire = list_GetElement(&sim_wire_data, gate_pin->wire);
    struct sim_wire_data_t *source_wire = list_GetElement(&sim_wire_data, source_pin->wire);
    struct sim_wire_data_t *drain_wire = list_GetElement(&sim_wire_data, drain_pin->wire);
    uint8_t prev_value = drain_pin->value;

    if((gate_wire->value == WIRE_VALUE_1S || gate_wire->value == WIRE_VALUE_1W) && (source_wire->value == WIRE_VALUE_0S || source_wire->value == WIRE_VALUE_0W))
    {
        drain_pin->value = source_wire->value;
    }
    else
    {
        drain_pin->value = WIRE_VALUE_Z;
    }

    if(prev_value != drain_pin->value)
    {
        sim_QueueWire(drain_wire);
    }
}

void dev_PowerStep(struct sim_dev_data_t *device)
{
    struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    pin->value = WIRE_VALUE_1S;
    struct sim_wire_data_t *wire = list_GetElement(&sim_wire_data, pin->wire);
    sim_QueueWire(wire);
}

void dev_GroundStep(struct sim_dev_data_t *device)
{
    struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    pin->value = WIRE_VALUE_0S;
    struct sim_wire_data_t *wire = list_GetElement(&sim_wire_data, pin->wire);
    sim_QueueWire(wire);
}

void dev_ClockStep(struct sim_dev_data_t *device)
{
    // struct dev_pin_block_t *pin = list_GetElement(&dev_pin_blocks, device->first_pin_block);

    // struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, device->sim_data);
    // struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, dev_sim_data->first_pin);

    // if(pin->value == WIRE_VALUE_0S)
    // {
    //     pin->value = WIRE_VALUE_1S;
    // }
    // else
    // {
    //     pin->value = WIRE_VALUE_0S;
    // }

    // struct sim_wire_data_t *wire_sim_data = list_GetElement(&sim_wire_data, pin->wire);

    // // struct wire_t *wire = pool_GetElement(&w_wires, pin->pins[0].wire);
    // sim_QueueWire(wire_sim_data);
}

void dev_InputStep(struct sim_dev_data_t *device)
{
    // struct dev_pin_block_t *pin = list_GetElement(&dev_pin_blocks, device->first_pin_block);
    // struct dev_pin_t *pin = dev_GetDevicePin(device, 0);
    // struct wire_t *wire = pool_GetElement(&w_wires, pin->wire);
    // sim_QueueWire(wire);

    struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    struct sim_wire_data_t *wire = list_GetElement(&sim_wire_data, pin->wire);
    sim_QueueWire(wire);
}

void (*dev_DeviceFuncs[DEV_DEVICE_TYPE_LAST])(struct sim_dev_data_t *device) = {
    [DEV_DEVICE_TYPE_PMOS]  = dev_PMosStep,
    [DEV_DEVICE_TYPE_NMOS]  = dev_NMosStep,
    [DEV_DEVICE_TYPE_POW]   = dev_PowerStep,
    [DEV_DEVICE_TYPE_GND]   = dev_GroundStep,
    [DEV_DEVICE_TYPE_CLOCK] = dev_ClockStep,
    [DEV_DEVICE_TYPE_INPUT] = dev_InputStep
};

struct dev_desc_t dev_device_descs[DEV_DEVICE_TYPE_LAST] = {
    [DEV_DEVICE_TYPE_PMOS] = {
        .width = 35,
        .height = 61,
        .offset_x = 56,
        .offset_y = 2,
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN,  .offset = {-30, -54}},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN,  .offset = {38, 0}},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT, .offset = {-30,  54}},
        },
    },

    [DEV_DEVICE_TYPE_NMOS] = {
        .width = 35,
        .height = 61,
        .offset_x = 6,
        .offset_y = 2,
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN,  .offset = {-30, -54}},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN,  .offset = {38, 0}},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT, .offset = {-30,  54}},
        },
    },

    [DEV_DEVICE_TYPE_GND] = {
        .pin_count = 1,
        .width = 17,
        .height = 17,
        .offset_x = 0,
        .offset_y = 77,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {0, 18}},
        },
    },
    
    [DEV_DEVICE_TYPE_POW] = {
        .pin_count = 1,
        .width = 15,
        .height = 24,
        .offset_x = 28,
        .offset_y = 70,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {0, -28}},
        },
    },

    [DEV_DEVICE_TYPE_CLOCK] = {
        .pin_count = 1,
        .width = 52,
        .height = 34,
        .offset_x = 49,
        .offset_y = 63,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {54, -2}},
        },
    },

    [DEV_DEVICE_TYPE_INPUT] = {
        .pin_count = 1,
        .width = 29,
        .height = 18,
        .offset_x = 96,
        .offset_y = 22,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {36, -2}},
        },
    }
};

void dev_Init()
{
    // dev_primitives = pool_CreateTyped(struct dev_primitive_t, 4096);
    dev_devices = pool_CreateTyped(struct dev_t, 4096);
    dev_inputs = pool_CreateTyped(struct dev_input_t, 4096);
    dev_pin_blocks = pool_CreateTyped(struct dev_pin_block_t, 4096);
    dev_prev_pin_values = calloc(DEV_MAX_DEVICE_PINS, sizeof(struct dev_pin_t));

    // for(uint32_t index = 0; index < DEV_PRIMITIVE_LAST; index++)
    // {
    const char *texture_name = "res/devices.png";
    // int width;
    // int height;
    int channels;
    stbi_uc *pixels = stbi_load(texture_name, &dev_devices_texture_width, &dev_devices_texture_height, &channels, STBI_rgb_alpha);

    if(pixels == NULL)
    {
        printf("couldn't load texture %s!\n", texture_name);
        return;
        // continue;
    }

    glGenTextures(1, &dev_devices_texture);
    glBindTexture(GL_TEXTURE_2D, dev_devices_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dev_devices_texture_width, dev_devices_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);
    // }
}

void dev_Shutdown()
{
    // pool_Destroy(&dev_primitives);
}

// void dev_DeviceStep(struct dev_t *device)
// {
//     if(dev_DeviceFuncs[device->type] != NULL)
//     {
//         // uint32_t pin_count = dev_device_descs[device->type].pin_count;
//         // struct dev_pin_desc_t *pin_desc = dev_device_descs[device->type].pins;
//         // for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
//         // {

//         // }
//         dev_DeviceFuncs[device->type](device);
//     }
// }

struct dev_t *dev_CreateDevice(uint32_t type)
{
    struct dev_t *device = pool_AddElement(&dev_devices, NULL);
    device->type = type;
    device->pin_blocks = NULL;
    device->selection_index = INVALID_LIST_INDEX;

    uint32_t total_pin_count = dev_device_descs[type].pin_count;
    uint32_t block_count = total_pin_count / DEV_PIN_BLOCK_PIN_COUNT;
    // struct dev_pin_desc_t *pins = dev_device_descs[type].pins;

    if(total_pin_count % DEV_PIN_BLOCK_PIN_COUNT)
    {
        block_count++;
    }

    // device->first_pin_block = list_AddElement(&dev_pin_blocks, NULL);
    // device->pin_block_count = block_count;

    // for(uint32_t block_index = 1; block_index < block_count; block_index++)
    // {
    //     list_AddElement(&dev_pin_blocks, NULL);
    // }

    // struct dev_pin_block_t *last_block = NULL;

    for(uint32_t block_index = 0; block_index < block_count; block_index++)
    {
        struct dev_pin_block_t *pin_block = pool_AddElement(&dev_pin_blocks, NULL);
        pin_block->next = device->pin_blocks;
        device->pin_blocks = pin_block;

        uint32_t pin_count = total_pin_count;

        if(pin_count > DEV_PIN_BLOCK_PIN_COUNT)
        {
            pin_count = DEV_PIN_BLOCK_PIN_COUNT;
        }

        for(uint32_t pin_index = 0; pin_index < DEV_PIN_BLOCK_PIN_COUNT; pin_index++)
        {
            pin_block->pins[pin_index].wire = WIRE_INVALID_WIRE;
            pin_block->pins[pin_index].value = WIRE_VALUE_Z;
        }
    }

    switch(device->type)
    {
        case DEV_DEVICE_TYPE_INPUT:
        {
            struct dev_input_t *input = pool_AddElement(&dev_inputs, NULL);
            input->device = device;
            input->init_value = WIRE_VALUE_0S;
            device->data = input;
        }
        break;
    }

    return device;
}

struct dev_t *dev_GetDevice(uint64_t device_index)
{
    return pool_GetValidElement(&dev_devices, device_index);
}

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin)
{
    uint32_t device_pin_block_index = pin / DEV_PIN_BLOCK_PIN_COUNT;
    struct dev_pin_block_t *block = device->pin_blocks;
    while(device_pin_block_index != 0 && block != NULL)
    {
        block = block->next;
        device_pin_block_index--;
    }
    
    return block;
}

struct dev_pin_t *dev_GetDevicePin(struct dev_t *device, uint16_t pin)
{
    struct dev_pin_block_t *block = dev_GetDevicePinBlock(device, pin);
    return block->pins + (pin % DEV_PIN_BLOCK_PIN_COUNT);
}

void dev_ToggleInput(struct dev_input_t *input)
{
    if(input != NULL)
    {
        struct sim_dev_data_t *sim_data = list_GetElement(&sim_dev_data, input->device->sim_data);
        struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, sim_data->first_pin);

        if(pin->value == WIRE_VALUE_0S)
        {
            pin->value = WIRE_VALUE_1S;
        }
        else
        {
            pin->value = WIRE_VALUE_0S;
        }
        sim_QueueDevice(sim_data);
    }
}