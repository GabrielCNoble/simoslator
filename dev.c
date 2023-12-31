#include "dev.h"
#include "GL/glew.h"
#include "stb_image.h"
#include "list.h"
#include "wire.h"
#include "sim.h"
#include "draw.h"
#include "obj.h"


// struct pool_t dev_primitives;
struct pool_t                       dev_devices;
struct pool_t                       dev_inputs;
struct pool_t                       dev_outputs;
struct pool_t                       dev_clocks;
struct pool_t                       dev_7seg_disps;
struct pool_t                       dev_pin_blocks;
struct dev_pin_t *                  dev_prev_pin_values;

/* from wire.c */
extern struct pool_t                w_wires;

/* from sim.c */
extern struct list_t                sim_wire_data;
extern struct list_t                sim_wire_pins;
extern struct list_t                sim_dev_data;
extern struct list_t                sim_dev_pins;

GLuint                              dev_devices_texture;
uint32_t                            dev_devices_texture_width;
uint32_t                            dev_devices_texture_height;
GLuint                              dev_7seg_mask_texture;
GLuint                              dev_output_mask_texture;

GLuint                              dev_devices_texture_small;
uint32_t                            dev_devices_texture_small_width;
uint32_t                            dev_devices_texture_small_height;

uint32_t dev_tex_coords_lut[4][4] = {
    [DEV_DEVICE_ROTATION_0] = {
        [0]                                     = DEV_DEVICE_ROTATION_0,
        [DEV_DEVICE_FLIP_X]                     = DEV_DEVICE_ROTATION_0   + 4,
        [DEV_DEVICE_FLIP_Y]                     = DEV_DEVICE_ROTATION_180 + 4,
        [DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y] = DEV_DEVICE_ROTATION_180 
    },
    [DEV_DEVICE_ROTATION_90] = {
        [0]                                     = DEV_DEVICE_ROTATION_90,
        [DEV_DEVICE_FLIP_X]                     = DEV_DEVICE_ROTATION_270 + 4,
        [DEV_DEVICE_FLIP_Y]                     = DEV_DEVICE_ROTATION_90 + 4,
        [DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y] = DEV_DEVICE_ROTATION_270
    },
    [DEV_DEVICE_ROTATION_180] = {
        [0]                                     = DEV_DEVICE_ROTATION_180,
        [DEV_DEVICE_FLIP_X]                     = DEV_DEVICE_ROTATION_180 + 4,
        [DEV_DEVICE_FLIP_Y]                     = DEV_DEVICE_ROTATION_0   + 4,
        [DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y] = DEV_DEVICE_ROTATION_0
    },
    [DEV_DEVICE_ROTATION_270] = {
        [0]                                     = DEV_DEVICE_ROTATION_270,
        [DEV_DEVICE_FLIP_X]                     = DEV_DEVICE_ROTATION_90 + 4,
        [DEV_DEVICE_FLIP_Y]                     = DEV_DEVICE_ROTATION_270  + 4,
        [DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y] = DEV_DEVICE_ROTATION_90
    },
};

// uint8_t dev_pmos_table[2][WIRE_VALUE_LAST] = {
//     { [WIRE_VALUE_0S] = 1, [WIRE_VALUE_0W] = 1 },
//     { [WIRE_VALUE_1S] = 1, [WIRE_VALUE_1W] = 1, [WIRE_VALUE_IND] = 1 }
// };



struct dev_mos_table_t dev_mos_tables[2] = {
    [DEV_DEVICE_TYPE_PMOS] = {
        .gate_table = {
            [WIRE_VALUE_0S] = 1,
            [WIRE_VALUE_0W] = 1        
        },
        .source_table = {
            [WIRE_VALUE_0S]     = WIRE_VALUE_Z,
            [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
            [WIRE_VALUE_0W]     = WIRE_VALUE_Z,
            [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
            [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
            [WIRE_VALUE_U]      = WIRE_VALUE_U,
            [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
            [WIRE_VALUE_IND]    = WIRE_VALUE_IND,
        }
    },

    [DEV_DEVICE_TYPE_NMOS] = {
        .gate_table = {
            [WIRE_VALUE_1S] = 1,
            [WIRE_VALUE_1W] = 1
        },
        .source_table = {
            [WIRE_VALUE_0S]     = WIRE_VALUE_0S,
            [WIRE_VALUE_1S]     = WIRE_VALUE_Z,
            [WIRE_VALUE_0W]     = WIRE_VALUE_0W,
            [WIRE_VALUE_1W]     = WIRE_VALUE_Z,
            [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
            [WIRE_VALUE_U]      = WIRE_VALUE_U,
            [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
            [WIRE_VALUE_IND]    = WIRE_VALUE_IND,
        }
    }
};

// uint8_t dev_pmos_gate_table[WIRE_VALUE_LAST] = {
//     [WIRE_VALUE_0S] = 1,
//     [WIRE_VALUE_0W] = 1
// };

// uint8_t dev_pmos_source_table[WIRE_VALUE_LAST] = {
//     [WIRE_VALUE_0S]     = WIRE_VALUE_Z,
//     [WIRE_VALUE_1S]     = WIRE_VALUE_1S,
//     [WIRE_VALUE_0W]     = WIRE_VALUE_Z,
//     [WIRE_VALUE_1W]     = WIRE_VALUE_1W,
//     [WIRE_VALUE_Z]      = WIRE_VALUE_Z,
//     [WIRE_VALUE_U]      = WIRE_VALUE_U,
//     [WIRE_VALUE_ERR]    = WIRE_VALUE_ERR,
//     [WIRE_VALUE_IND]    = WIRE_VALUE_IND,
// };



void dev_PMosStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *gate_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_GATE);
    struct sim_dev_pin_t *source_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_SOURCE);
    struct sim_dev_pin_t *drain_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_DRAIN);
    struct sim_wire_data_t *gate_wire = sim_GetWireSimData(gate_pin->wire, DEV_PIN_TYPE_IN);
    struct sim_wire_data_t *source_wire = sim_GetWireSimData(source_pin->wire, DEV_PIN_TYPE_IN);
    struct sim_wire_data_t *drain_wire = sim_GetWireSimData(drain_pin->wire, DEV_PIN_TYPE_OUT);
    // uint8_t prev_value = drain_pin->value;

    // if((gate_wire->value == WIRE_VALUE_0S || gate_wire->value == WIRE_VALUE_0W) && 
    //    (source_wire->value == WIRE_VALUE_1S || source_wire->value == WIRE_VALUE_1W || source_wire->value == WIRE_VALUE_IND))
    // {
    //     drain_pin->value = source_wire->value;
    // }
    // else
    // {
    //     drain_pin->value = WIRE_VALUE_Z;
    // }

    // if(dev_pmos_table[0][gate_wire->value] && dev_pmos_table[1][source_wire->value])
    // {
    //     drain_pin->value = source_wire->value;
    // }
    // else
    // {
    //     drain_pin->value = WIRE_VALUE_Z;
    // }

    // drain_pin->value = dev_pmos_gate_table[gate_wire->value] ? dev_pmos_source_table[source_wire->value] : WIRE_VALUE_Z;
    gate_pin->value = gate_wire->value;
    source_pin->value = source_wire->value;
    drain_pin->value = dev_mos_tables[DEV_DEVICE_TYPE_PMOS].gate_table[gate_wire->value] ? 
                       dev_mos_tables[DEV_DEVICE_TYPE_PMOS].source_table[source_wire->value] : WIRE_VALUE_Z;

    if(drain_pin->value != drain_wire->value)
    {
        sim_QueueWire(drain_wire);
    }
}

void dev_NMosStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *gate_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_GATE);
    struct sim_dev_pin_t *source_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_SOURCE);
    struct sim_dev_pin_t *drain_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_MOS_PIN_DRAIN);
    struct sim_wire_data_t *gate_wire = sim_GetWireSimData(gate_pin->wire, DEV_PIN_TYPE_IN);
    struct sim_wire_data_t *source_wire = sim_GetWireSimData(source_pin->wire, DEV_PIN_TYPE_IN);
    struct sim_wire_data_t *drain_wire = sim_GetWireSimData(drain_pin->wire, DEV_PIN_TYPE_OUT);
    uint8_t prev_value = drain_pin->value;

    // if((gate_wire->value == WIRE_VALUE_1S || gate_wire->value == WIRE_VALUE_1W) && 
    //    (source_wire->value == WIRE_VALUE_0S || source_wire->value == WIRE_VALUE_0W || source_wire->value == WIRE_VALUE_IND))
    // {
    //     drain_pin->value = source_wire->value;
    // }
    // else
    // {
    //     drain_pin->value = WIRE_VALUE_Z;
    // }

    gate_pin->value = gate_wire->value;
    source_pin->value = source_wire->value;
    drain_pin->value = dev_mos_tables[DEV_DEVICE_TYPE_NMOS].gate_table[gate_wire->value] ? 
                       dev_mos_tables[DEV_DEVICE_TYPE_NMOS].source_table[source_wire->value] : WIRE_VALUE_Z;

    if(prev_value != drain_pin->value)
    {
        sim_QueueWire(drain_wire);
    }
}

void dev_PowerStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    pin->value = WIRE_VALUE_1S;
    struct sim_wire_data_t *wire = sim_GetWireSimData(pin->wire, DEV_PIN_TYPE_OUT);
    sim_QueueWire(wire);
}

void dev_GroundStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    pin->value = WIRE_VALUE_0S;
    struct sim_wire_data_t *wire = sim_GetWireSimData(pin->wire, DEV_PIN_TYPE_OUT);
    sim_QueueWire(wire);
}

void dev_ClockStep(struct sim_dev_data_t *device)
{

}

void dev_InputStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    struct sim_wire_data_t *wire = sim_GetWireSimData(pin->wire, DEV_PIN_TYPE_OUT);
    sim_QueueWire(wire);
}

void dev_OutputStep(struct sim_dev_data_t *device)
{
    struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
    struct sim_wire_data_t *wire = sim_GetWireSimData(pin->wire, DEV_PIN_TYPE_IN);
    pin->value = wire->value;
}

void dev_NullStep(struct sim_dev_data_t *device)
{

}

void (*dev_DeviceFuncs[DEV_DEVICE_TYPE_LAST])(struct sim_dev_data_t *device) = {
    [DEV_DEVICE_TYPE_PMOS]      = dev_PMosStep,
    [DEV_DEVICE_TYPE_NMOS]      = dev_NMosStep,
    [DEV_DEVICE_TYPE_POW]       = dev_PowerStep,
    [DEV_DEVICE_TYPE_GND]       = dev_GroundStep,
    [DEV_DEVICE_TYPE_CLOCK]     = dev_InputStep,
    [DEV_DEVICE_TYPE_INPUT]     = dev_InputStep,
    [DEV_DEVICE_TYPE_7SEG]      = dev_NullStep,
    [DEV_DEVICE_TYPE_OUTPUT]    = dev_OutputStep
};

struct dev_desc_t dev_device_descs[DEV_DEVICE_TYPE_LAST] = {
    [DEV_DEVICE_TYPE_PMOS] = {
        .width = 42,
        .height = 80,
        .tex_offset = {50, 0},
        .origin = {0, 0},
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN,  .offset = {-20, -40}, .immediate = 1},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN,  .offset = {20, 0}},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT, .offset = {-20,  40}},
        },
    },

    [DEV_DEVICE_TYPE_NMOS] = {
        .width = 42,
        .height = 80,
        .tex_offset = {0, 0},
        .origin = {0, 0},
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN,  .offset = {-20, -40}, .immediate = 1},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN,  .offset = {20, 0}},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT, .offset = {-20,  40}},
        },
    },

    [DEV_DEVICE_TYPE_GND] = {
        .pin_count = 1,
        .width = 20,
        .height = 29,
        .tex_offset = {41, 92}, 
        .origin = {0, 15},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {0, 0}},
        },
    },
    
    [DEV_DEVICE_TYPE_POW] = {
        .pin_count = 1,
        .width = 22,
        .height = 32,
        .tex_offset = {10, 88}, 
        .origin = {0, -16},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {0, 0}},
        },
    },

    [DEV_DEVICE_TYPE_CLOCK] = {
        .pin_count = 1,
        .width = 70,
        .height = 57,
        .tex_offset = {104, 71}, 
        .origin = {15, 0},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {20, 0}},
        },
    },

    [DEV_DEVICE_TYPE_INPUT] = {
        .pin_count = 1,
        .width = 38,
        .height = 22,
        .tex_offset = {104, 30}, 
        .origin = {18, 0},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT, .offset = {0, 0}},
        },
    },

    [DEV_DEVICE_TYPE_7SEG] = {
        .pin_count = 8,
        .width = 94,
        .height = 160,
        .tex_offset = {196, 0},
        .origin = {-2, 0},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_IN, .offset = {  0,  80}},

            {.type = DEV_PIN_TYPE_IN, .offset = {-30, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = {-20, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = {-10, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = {  0, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = { 10, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = { 20, -80}},
            {.type = DEV_PIN_TYPE_IN, .offset = { 30, -80}},
        }
    },

    [DEV_DEVICE_TYPE_OUTPUT] = {
        .pin_count = 1,
        .width = 38,
        .height = 22,
        .tex_offset = {114, 150}, 
        .origin = {18, 0},
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_IN, .offset = {0, 0}},
        },
    },
};

void dev_Init()
{
    dev_devices = pool_CreateTyped(struct dev_t, 4096);
    dev_inputs = pool_CreateTyped(struct dev_input_t, 512);
    dev_outputs = pool_CreateTyped(struct dev_output_t, 512);
    dev_clocks = pool_CreateTyped(struct dev_clock_t, 8);
    dev_7seg_disps = pool_CreateTyped(struct dev_7seg_disp_t, 8);
    dev_pin_blocks = pool_CreateTyped(struct dev_pin_block_t, 4096);
    dev_prev_pin_values = calloc(DEV_MAX_DEVICE_PINS, sizeof(struct dev_pin_t));

    int channels;
    stbi_uc *pixels = stbi_load("res/devices.png", &dev_devices_texture_width, &dev_devices_texture_height, &channels, STBI_rgb_alpha);
    dev_devices_texture = d_CreateTexture(dev_devices_texture_width, dev_devices_texture_height, GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, 4, GL_RGBA, pixels);
    free(pixels);

    int32_t width;
    int32_t height;
    pixels = stbi_load("res/7seg_mask.png", &width, &height, &channels, STBI_rgb_alpha);
    dev_7seg_mask_texture = d_CreateTexture(width, height, GL_RGBA8UI, GL_NEAREST, GL_NEAREST, 0, GL_RGBA_INTEGER, pixels);
    free(pixels);

    pixels = stbi_load("res/output_mask.png", &width, &height, &channels, STBI_rgb_alpha);
    dev_output_mask_texture = d_CreateTexture(width, height, GL_RGBA8UI, GL_NEAREST, GL_NEAREST, 0, GL_RGBA_INTEGER, pixels);
    free(pixels);
}

void dev_Shutdown()
{
    pool_Destroy(&dev_devices);
    pool_Destroy(&dev_pin_blocks);
}

struct dev_t *dev_CreateDevice(uint32_t type)
{
    struct dev_t *device = pool_AddElement(&dev_devices, NULL);
    device->type = type;
    device->pin_blocks = NULL;
    // device->selection_index = INVALID_LIST_INDEX;
    device->rotation = DEV_DEVICE_ROTATION_0;
    // device->tex_coords[0] = 0.0f;
    // device->tex_coords[1] = 1.0f;
    // device->tex_coords[2] = 0.0f;
    // device->tex_coords[3] = 1.0f;

    uint32_t total_pin_count = dev_device_descs[type].pin_count;
    uint32_t block_count = total_pin_count / DEV_PIN_BLOCK_PIN_COUNT;
    // struct dev_pin_desc_t *pins = dev_device_descs[type].pins;

    if(total_pin_count % DEV_PIN_BLOCK_PIN_COUNT)
    {
        block_count++;
    }

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
            // pin_block->pins[pin_index].wire = WIRE_INVALID_WIRE;
            // pin_block->pins[pin_index].value = WIRE_VALUE_Z;
            pin_block->pins[pin_index].junction = INVALID_POOL_INDEX;
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

        case DEV_DEVICE_TYPE_OUTPUT:
        {
            struct dev_output_t *output = pool_AddElement(&dev_outputs, NULL);
            output->device = device;
            device->data = output;
        }
        break;

        case DEV_DEVICE_TYPE_CLOCK:
        {
            struct dev_clock_t *clock = pool_AddElement(&dev_clocks, NULL);
            clock->device = device;
            clock->frequency = 1;
            device->data = clock;
        }
        break;

        case DEV_DEVICE_TYPE_7SEG:
        {
            struct dev_7seg_disp_t *display = pool_AddElement(&dev_7seg_disps, NULL);
            display->device = device;
            display->value = 0;
            device->data = display;
        }
        break;
    }

    dev_UpdateDeviceRotation(device);
    device->object = obj_CreateObject(OBJECT_TYPE_DEVICE, device);

    return device;
}

void dev_DestroyDevice(struct dev_t *device)
{
    if(device != NULL)
    {
        obj_FreeObject(device->object);
        struct dev_desc_t *desc = dev_device_descs + device->type;
        struct dev_pin_block_t *pin_block = device->pin_blocks;
        uint32_t total_pin_count = desc->pin_count;
        while(pin_block)
        {
            uint32_t pin_count = DEV_PIN_BLOCK_PIN_COUNT;
            if(pin_count > total_pin_count)
            {
                pin_count = total_pin_count;
            }

            for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
            {
                struct dev_pin_t *pin = pin_block->pins + pin_index;
                if(pin->junction != INVALID_POOL_INDEX)
                {
                    struct wire_junc_t *junction = w_GetWireJunction(pin->junction);
                    w_DisconnectJunctionFromPin(junction);
                    // struct wire_t *wire = w_GetWire(pin->wire);
                    // w_DisconnectPin(wire, device, )
                }
            }
            pool_RemoveElement(&dev_pin_blocks, pin_block->element_index);
            pin_block = pin_block->next;
        }

        switch(device->type)
        {
            case DEV_DEVICE_TYPE_INPUT:
            {
                struct dev_input_t *input = device->data;
                pool_RemoveElement(&dev_inputs, input->element_index);
            }
            break;

            case DEV_DEVICE_TYPE_OUTPUT:
            {
                struct dev_output_t *output = device->data;
                pool_RemoveElement(&dev_outputs, output->element_index);
            }
            break;

            case DEV_DEVICE_TYPE_CLOCK:
            {
                struct dev_clock_t *clock = device->data;
                pool_RemoveElement(&dev_clocks, clock->element_index);
            }
            break;
        }

        pool_RemoveElement(&dev_devices, device->element_index);
    }
}

void dev_UpdateDeviceRotation(struct dev_t *device)
{
    struct dev_desc_t *desc = dev_device_descs + device->type;
    uint32_t rotation = (device->flip && (device->flip != (DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y))) ? 
        (DEV_DEVICE_ROTATION_360 - device->rotation) % DEV_DEVICE_ROTATION_360 : device->rotation;
    device->tex_coords = dev_tex_coords_lut[rotation][device->flip];

    device->origin[0] = (device->flip & DEV_DEVICE_FLIP_X) ? -desc->origin[0] : desc->origin[0];
    device->origin[1] = (device->flip & DEV_DEVICE_FLIP_Y) ? -desc->origin[1] : desc->origin[1];

    switch(device->rotation)
    {
        case DEV_DEVICE_ROTATION_90:
        {
            int32_t temp = device->origin[0];
            device->origin[0] = -device->origin[1];
            device->origin[1] = temp;
        }
        break;

        case DEV_DEVICE_ROTATION_180:
        {
            device->origin[0] = -device->origin[0];
            device->origin[1] = -device->origin[1];
        }
        break;

        case DEV_DEVICE_ROTATION_270:
        {
            int32_t temp = device->origin[0];
            device->origin[0] = device->origin[1];
            device->origin[1] = -temp;
        }
        break;
    }
}

void dev_RotateDevice(struct dev_t *device, uint32_t ccw)
{
    if(device != NULL)
    {
        // ccw ^= device->flip && (device->flip != (DEV_DEVICE_FLIP_X | DEV_DEVICE_FLIP_Y));

        if(ccw)
        {
            device->rotation = (device->rotation + 1) % DEV_DEVICE_ROTATION_360;
        }
        else
        {
            if(device->rotation == 0)
            {
                device->rotation = 4;
            }
            device->rotation--;
        }

        dev_UpdateDeviceRotation(device);
    }
}

void dev_ClearDevices()
{
    pool_Reset(&dev_devices);
    pool_Reset(&dev_inputs);
    pool_Reset(&dev_outputs);
    pool_Reset(&dev_clocks);
    pool_Reset(&dev_pin_blocks);
}

struct dev_t *dev_GetDevice(uint64_t device_index)
{
    return pool_GetValidElement(&dev_devices, device_index);
}

void dev_GetDeviceLocalPinPosition(struct dev_t *device, uint16_t pin, int32_t *pin_position)
{
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    struct dev_pin_desc_t *pin_desc = dev_desc->pins + pin;

    pin_position[0] = (device->flip & DEV_DEVICE_FLIP_X) ? -pin_desc->offset[0] : pin_desc->offset[0];
    pin_position[1] = (device->flip & DEV_DEVICE_FLIP_Y) ? -pin_desc->offset[1] : pin_desc->offset[1];

    switch(device->rotation)
    {
        case DEV_DEVICE_ROTATION_90:                        
        {
            int32_t temp = pin_position[0];
            pin_position[0] = -pin_position[1];
            pin_position[1] = temp;
        }
        break;

        case DEV_DEVICE_ROTATION_180:
            pin_position[0] = -pin_position[0];
            pin_position[1] = -pin_position[1];
        break;

        case DEV_DEVICE_ROTATION_270:
        {
            int32_t temp = pin_position[0];
            pin_position[0] = pin_position[1];
            pin_position[1] = -temp;
        }
        break;
    }
}

void dev_GetDeviceLocalBoxPosition(struct dev_t *device, int32_t *min, int32_t *max)
{
    struct dev_desc_t *dev_desc = dev_device_descs + device->type;
    int32_t width = dev_desc->width >> 1;
    int32_t height = dev_desc->height >> 1;

    min[0] = -width;
    min[1] = -height;

    max[0] = width;
    max[1] = height;

    if(device->flip & DEV_DEVICE_FLIP_X)
    {
        min[0] += dev_desc->origin[0];
        max[0] += dev_desc->origin[0];
    }
    else
    {
        min[0] -= dev_desc->origin[0];
        max[0] -= dev_desc->origin[0];
    }

    if(device->flip & DEV_DEVICE_FLIP_Y)
    {
        min[1] += dev_desc->origin[1];
        max[1] += dev_desc->origin[1];
    }
    else
    {
        min[1] -= dev_desc->origin[1];
        max[1] -= dev_desc->origin[1];
    }

    // int32_t tx = max[0] - min[0];
    // int32_t ty = max[1] - min[1];
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
        struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, sim_data->first_pin);

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

void dev_Update7SegDisplay(struct dev_7seg_disp_t *display)
{
    // struct dev_7seg_disp_t *display = device->device->data;
    struct sim_dev_data_t *device = list_GetElement(&sim_dev_data, display->device->sim_data);
    display->value = 0;

    struct sim_dev_pin_t *power_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_7SEG_PIN_POW);
    struct sim_wire_data_t *power = sim_GetWireSimData(power_pin->wire, DEV_PIN_TYPE_IN);

    if(power->value == WIRE_VALUE_1S || power->value == WIRE_VALUE_1W)
    {
        for(uint32_t index = 0; index < 7; index++)
        {
            struct sim_dev_pin_t *segment_pin = list_GetElement(&sim_dev_pins, device->first_pin + DEV_7SEG_PIN_SEG0 + index);
            struct sim_wire_data_t *segment_wire = sim_GetWireSimData(segment_pin->wire, DEV_PIN_TYPE_IN);
            display->value |= (segment_wire->value == WIRE_VALUE_0S || segment_wire->value == WIRE_VALUE_0W) << index;
        }
    }
}