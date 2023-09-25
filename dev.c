#include "dev.h"
#include "GL/glew.h"
#include "stb_image.h"
#include "list.h"
#include "wire.h"


// struct pool_t dev_primitives;
struct pool_t           dev_devices;
struct list_t           dev_pin_blocks;
extern struct pool_t    w_wires;

const char *dev_device_texture_names[] = {
    // [DEV_PRIMITIVE_PNMOS]  = "res/pnmos.png",
    // [DEV_PRIMITIVE_PMOS]   = "res/pmos.png",
    // [DEV_PRIMITIVE_NMOS]   = "res/nmos.png",
    // [DEV_PRIMITIVE_GND]    = "res/gnd.png",
    // [DEV_PRIMITIVE_POW]    = "res/pow.png"
};



// GLuint dev_device_textures[DEV_DEVICE_TYPE_LAST];
GLuint dev_devices_texture;

uint32_t dev_device_texture_offsets[DEV_DEVICE_TYPE_LAST][2] = {
    [DEV_DEVICE_TYPE_PMOS] = {42, 2},
    [DEV_DEVICE_TYPE_NMOS] = {6, 2},
};

void (*dev_DeviceFuncs[DEV_DEVICE_TYPE_LAST])(struct dev_t *device) = {
    [DEV_DEVICE_TYPE_PMOS] = dev_PMosStep,
    [DEV_DEVICE_TYPE_NMOS] = dev_NMosStep
};

#define DEV_MOS_PIN_SOURCE  0
#define DEV_MOS_PIN_DRAIN   1
#define DEV_MOS_PIN_GATE    2

struct dev_desc_t dev_device_descs[DEV_DEVICE_TYPE_LAST] = {
    [DEV_DEVICE_TYPE_PMOS] = {
        .width = 24,
        .height = 42,
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT},
        },
    },

    [DEV_DEVICE_TYPE_NMOS] = {
        .width = 24,
        .height = 42,
        .pin_count = 3,
        .pins = (struct dev_pin_desc_t []) {
            [DEV_MOS_PIN_SOURCE]    = {.type = DEV_PIN_TYPE_IN},
            [DEV_MOS_PIN_GATE]      = {.type = DEV_PIN_TYPE_IN},
            [DEV_MOS_PIN_DRAIN]     = {.type = DEV_PIN_TYPE_OUT},
        },
    },

    [DEV_DEVICE_TYPE_GND] = {
        .pin_count = 1,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT},
        },
    },
    
    [DEV_DEVICE_TYPE_POW] = {
        .pin_count = 1,
        .pins = (struct dev_pin_desc_t []) {
            {.type = DEV_PIN_TYPE_OUT},
        },
    },
};

void dev_Init()
{
    // dev_primitives = pool_CreateTyped(struct dev_primitive_t, 4096);
    dev_devices = pool_CreateTyped(struct dev_t, 4096);
    dev_pin_blocks = list_Create(sizeof(struct dev_pin_block_t), 4096);

    // for(uint32_t index = 0; index < DEV_PRIMITIVE_LAST; index++)
    // {
    const char *texture_name = "res/devices.png";
    int width;
    int height;
    int channels;
    stbi_uc *pixels = stbi_load(texture_name, &width, &height, &channels, STBI_rgb_alpha);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);
    // }
}

void dev_Shutdown()
{
    // pool_Destroy(&dev_primitives);
}

void dev_PMosStep(struct dev_t *device)
{
    struct dev_pin_block_t *pins = list_GetElement(&dev_pin_blocks, device->first_pin_block);
    struct wire_t *gate = pool_GetElement(&w_wires, pins->pins[DEV_MOS_PIN_GATE].wire);
    struct wire_t *source = pool_GetElement(&w_wires, pins->pins[DEV_MOS_PIN_SOURCE].wire);

    if((gate->value == WIRE_VALUE_0S || gate->value == WIRE_VALUE_0W) && (source->value == WIRE_VALUE_1S || source->value == WIRE_VALUE_1W))
    {
        pins->pins[DEV_MOS_PIN_DRAIN].value = source->value;
    }
    else
    {
        pins->pins[DEV_MOS_PIN_DRAIN].value = WIRE_VALUE_Z;
    }
}

void dev_NMosStep(struct dev_t *device)
{
    struct dev_pin_block_t *pins = list_GetElement(&dev_pin_blocks, device->first_pin_block);
    struct wire_t *gate = pool_GetElement(&w_wires, pins->pins[DEV_MOS_PIN_GATE].wire);
    struct wire_t *source = pool_GetElement(&w_wires, pins->pins[DEV_MOS_PIN_SOURCE].wire);

    if((gate->value == WIRE_VALUE_1S || gate->value == WIRE_VALUE_1W) && (source->value == WIRE_VALUE_0S || source->value == WIRE_VALUE_0W))
    {
        pins->pins[DEV_MOS_PIN_DRAIN].value = source->value;
    }
    else
    {
        pins->pins[DEV_MOS_PIN_DRAIN].value = WIRE_VALUE_Z;
    }
}

void dev_DeviceStep(struct dev_t *device)
{
    dev_DeviceFuncs[device->type](device);
}

struct dev_t *dev_CreateDevice(uint32_t type)
{
    struct dev_t *device = pool_AddElement(&dev_devices, NULL);
    device->type = type;

    uint32_t total_pin_count = dev_device_descs[type].pin_count;
    uint32_t block_count = total_pin_count / DEV_PIN_BLOCK_PIN_COUNT;
    // struct dev_pin_desc_t *pins = dev_device_descs[type].pins;

    if(total_pin_count % DEV_PIN_BLOCK_PIN_COUNT)
    {
        block_count++;
    }

    device->first_pin_block = list_AddElement(&dev_pin_blocks, NULL);
    device->pin_block_count = block_count;

    for(uint32_t block_index = 1; block_index < block_count; block_index++)
    {
        list_AddElement(&dev_pin_blocks, NULL);
    }

    // for(uint32_t block_index = 0; block_index < block_count; block_index++)
    // {
    //     uint32_t pin_count = total_pin_count;

    //     if(pin_count > DEV_PIN_BLOCK_PIN_COUNT)
    //     {
    //         pin_count = DEV_PIN_BLOCK_PIN_COUNT;
    //     }

    //     struct dev_pin_block_t *block = list_GetElement(&dev_pin_blocks, device->first_pin_block + block_index);

    //     for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
    //     {
    //         block->pins[pin_index] = *pins;
    //         pins++;
    //     }

    //     total_pin_count -= pin_count;
    // }

    return device;
}

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin)
{
    uint32_t device_pin_block_index = pin / DEV_PIN_BLOCK_PIN_COUNT;
    return list_GetElement(&dev_pin_blocks, device->first_pin_block + device_pin_block_index); 
}