#ifndef DEV_H
#define DEV_H

#include <stdint.h>
#include "pool.h"

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
    uint8_t type;
};

struct dev_desc_t
{
    uint16_t                    width;
    uint16_t                    height;
    uint32_t                    pin_count;
    struct dev_pin_desc_t *     pins;
};

#define DEV_PIN_BLOCK_PIN_COUNT 6

struct dev_pin_t
{
    uint32_t wire:  28;
    uint32_t value: 4;
};

struct dev_pin_block_t
{
    uint64_t            device;
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
};

void dev_Init();

void dev_Shutdown();

void dev_PMosStep(struct dev_t *device);

void dev_NMosStep(struct dev_t *device);

void dev_DeviceStep(struct dev_t *device);

struct dev_t *dev_CreateDevice(uint32_t type);

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin);


#endif