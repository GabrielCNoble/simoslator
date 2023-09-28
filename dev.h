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
    uint32_t                    offset_x;
    uint32_t                    offset_y;
    uint32_t                    pin_count;
    struct dev_pin_desc_t *     pins;
};

#define DEV_PIN_BLOCK_PIN_COUNT 4
#define DEV_MAX_DEVICE_PINS 0xffff
#define DEV_DEVICE_PIN_PIXEL_WIDTH 8
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
    DEV_DEVICE_FLIP_X,
    DEV_DEVICE_FLIP_Y,
};

struct dev_pin_t
{
    uint64_t wire:  48;
    uint64_t value: 4;
};

struct dev_pin_block_t
{
    // uint64_t            device;
    struct dev_pin_t    pins[DEV_PIN_BLOCK_PIN_COUNT];
};

struct dev_t
{
    POOL_ELEMENT;
    int32_t     position[2];
    uint32_t    type;
    uint32_t    first_pin_block;
    uint32_t    pin_block_count;
    uint32_t    queued;
    uint16_t    rotation;
    uint16_t    flip;
};

struct dev_clock_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
    uint32_t        frequency;
};

void dev_Init();

void dev_Shutdown();

void dev_PMosStep(struct dev_t *device);

void dev_NMosStep(struct dev_t *device);

void dev_PowerStep(struct dev_t *device);

void dev_GroundStep(struct dev_t *device);

void dev_ClockStep(struct dev_t *device);

void dev_DeviceStep(struct dev_t *device);

struct dev_t *dev_CreateDevice(uint32_t type);

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin);

struct dev_pin_t *dev_GetDevicePin(struct dev_t *device, uint16_t pin);


#endif