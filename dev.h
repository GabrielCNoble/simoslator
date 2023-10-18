#ifndef DEV_H
#define DEV_H

#include <stdint.h>
#include "pool.h"

#define DEV_MAX_DEVICES     0xffffffffffff
#define DEV_MAX_DEVICE_PINS 0xffff
#define DEV_INVALID_DEVICE  DEV_MAX_DEVICES
#define DEV_INVALID_PIN     DEV_MAX_DEVICE_PINS

#define DEV_MOS_PIN_SOURCE  0
#define DEV_MOS_PIN_DRAIN   1
#define DEV_MOS_PIN_GATE    2

enum DEV_DEVICE_TYPES
{
    DEV_DEVICE_TYPE_PMOS,
    DEV_DEVICE_TYPE_NMOS,
    DEV_DEVICE_TYPE_GND,
    DEV_DEVICE_TYPE_POW,
    DEV_DEVICE_TYPE_CLOCK,
    DEV_DEVICE_TYPE_INPUT,
    DEV_DEVICE_TYPE_LAST,
};

enum DEV_PIN_TYPES
{
    DEV_PIN_TYPE_IN,
    DEV_PIN_TYPE_OUT,
    DEV_PIN_TYPE_INOUT
};

struct dev_pin_desc_t
{
    int32_t offset[2];
    uint8_t type;
};

struct dev_desc_t
{
    uint16_t                    width;
    uint16_t                    height;
    // int32_t                     offset_x;
    // int32_t                     offset_y;
    int32_t                     offset[2];
    int32_t                     origin[2];
    uint32_t                    pin_count;
    struct dev_pin_desc_t *     pins;
};

#define DEV_PIN_BLOCK_PIN_COUNT 4
#define DEV_MAX_DEVICE_PINS 0xffff
// #define DEV_INVALID_WIRE 0xffffff

enum DEV_DEVICE_ROTATION
{
    DEV_DEVICE_ROTATION_0,
    DEV_DEVICE_ROTATION_90,
    DEV_DEVICE_ROTATION_180,
    DEV_DEVICE_ROTATION_270,
};

enum DEV_DEVICE_FLIP
{
    DEV_DEVICE_FLIP_Y,
    DEV_DEVICE_FLIP_X,
};

struct dev_pin_t
{
    uint64_t wire:  48;
    uint64_t value: 4;
};

struct dev_pin_block_t
{
    // uint64_t            device;
    POOL_ELEMENT;
    struct dev_pin_block_t *    next;
    struct dev_pin_t            pins[DEV_PIN_BLOCK_PIN_COUNT];
};

struct dev_t
{
    POOL_ELEMENT;
    void *                      data;
    uint64_t                    sim_data;
    uint64_t                    selection_index;
    int32_t                     position[2];
    uint32_t                    type;
    struct dev_pin_block_t *    pin_blocks;
    uint16_t                    rotation;
    uint16_t                    flip;
    uint32_t                    regions[4];
    uintptr_t                   serialized_index;
};

struct dev_clock_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint32_t        frequency;
};

struct dev_input_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint8_t         init_value;
};

void dev_Init();

void dev_Shutdown();

struct dev_t *dev_CreateDevice(uint32_t type);

void dev_DestroyDevice(struct dev_t *device);

struct dev_t *dev_GetDevice(uint64_t device_index);

void dev_GetDeviceLocalPinPosition(struct dev_t *device, uint16_t pin, int32_t *pin_position);

void dev_GetDeviceLocalBoxPosition(struct dev_t *device, int32_t *min, int32_t *max);

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin);

struct dev_pin_t *dev_GetDevicePin(struct dev_t *device, uint16_t pin);

void dev_ToggleInput(struct dev_input_t *input);


#endif