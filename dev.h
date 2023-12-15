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
    DEV_DEVICE_TYPE_7SEG,
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
    uint8_t immediate;
};

struct dev_desc_t
{
    uint16_t                    width;
    uint16_t                    height;
    int32_t                     tex_offset[2];
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
    DEV_DEVICE_ROTATION_360
};

enum DEV_DEVICE_FLIP
{
    DEV_DEVICE_FLIP_X = 1,
    DEV_DEVICE_FLIP_Y = 1 << 1,
};

struct dev_pin_t
{
    // uint64_t wire:  48;
    // uint64_t value: 4;
    uint64_t junction;
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
    void *                      object;
    uint64_t                    sim_data;
    uint64_t                    selection_index;
    uintptr_t                   serialized_index;
    int32_t                     position[2];
    int32_t                     origin[2];
    struct dev_pin_block_t *    pin_blocks;
    uint8_t                     tex_coords;
    uint8_t                     type;
    uint8_t                     rotation;
    uint8_t                     flip;
};

struct dev_clock_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint64_t        frequency;
};

struct dev_input_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint8_t         init_value;
};

#define DEV_7SEG_PIN_POW    0
#define DEV_7SEG_PIN_SEG0   1
#define DEV_7SEG_PIN_SEG1   2
#define DEV_7SEG_PIN_SEG2   3
#define DEV_7SEG_PIN_SEG3   4
#define DEV_7SEG_PIN_SEG4   5
#define DEV_7SEG_PIN_SEG5   6
#define DEV_7SEG_PIN_SEG6   7
struct dev_7seg_disp_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint8_t         value;
};

void dev_Init();

void dev_Shutdown();

struct dev_t *dev_CreateDevice(uint32_t type);

void dev_DestroyDevice(struct dev_t *device);

void dev_UpdateDeviceRotation(struct dev_t *device);

void dev_RotateDevice(struct dev_t *device, uint32_t ccw);

void dev_ClearDevices();

struct dev_t *dev_GetDevice(uint64_t device_index);

void dev_GetDeviceLocalPinPosition(struct dev_t *device, uint16_t pin, int32_t *pin_position);

void dev_GetDeviceLocalBoxPosition(struct dev_t *device, int32_t *min, int32_t *max);

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin);

struct dev_pin_t *dev_GetDevicePin(struct dev_t *device, uint16_t pin);

void dev_ToggleInput(struct dev_input_t *input);

void dev_Update7SegDisplay(struct dev_7seg_disp_t *display);


#endif