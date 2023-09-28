#ifndef WIRE_H
#define WIRE_H

#include <stdint.h>
#include "pool.h"
#include "dev.h"

#define WIRE_MAX_WIRES 0xffffffffffff
#define WIRE_INVALID_WIRE WIRE_MAX_WIRES

enum WIRE_VALUES
{
    /* strong 0 */
    WIRE_VALUE_0S = 0,
    /* strong 1 */
    WIRE_VALUE_1S,
    /* weak 0 */
    WIRE_VALUE_0W,
    /* weak 1 */
    WIRE_VALUE_1W,
    /* high impedance */
    WIRE_VALUE_Z,
    /* weak unknown */
    WIRE_VALUE_U,
    /* error */
    WIRE_VALUE_ERR,

    WIRE_VALUE_LAST,
};

// #define WIRE_INVALID_DEVICE 0xffffffffffff
// #define WIRE_INVALID_PIN    0xffff

struct wire_pin_t
{
    uint64_t device:    48;
    uint64_t pin:       16;
};

#define WIRE_PIN_BLOCK_PIN_COUNT 8

struct wire_pin_block_t
{
    struct wire_pin_t pins[WIRE_PIN_BLOCK_PIN_COUNT];
};

#define WIRE_DEV_BLOCK_DEV_COUNT 8
struct wire_dev_block_t
{
    // uint64_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
    struct wire_pin_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
};

struct wire_segment_t
{
    int32_t start[2];
    int32_t end[2];
};

struct wire_t
{
    POOL_ELEMENT;
    int32_t     position[2];
    uint32_t    first_pin_block;
    uint32_t    pin_block_count;
    uint32_t    first_dev_block;
    uint32_t    dev_block_count;
    uint32_t    first_segment;
    uint32_t    segment_count;
    uint32_t    queued;
    uint8_t     value;
};

void w_Init();

void w_Shutdown();

struct wire_t *w_CreateWire();

struct wire_t *w_GetWire(uint32_t wire_index);

void w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin);

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin);

void w_WireStep(struct wire_t *wire);


#endif