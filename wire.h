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
    POOL_ELEMENT;
    struct wire_pin_block_t *   next;
    struct wire_pin_block_t *   prev;
    struct wire_pin_t           pins[WIRE_PIN_BLOCK_PIN_COUNT];
};

// #define WIRE_DEV_BLOCK_DEV_COUNT 8
// struct wire_dev_block_t
// {
//     // uint64_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
//     struct wire_pin_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
// };

/* forward declaration */
struct wire_t;

struct wire_pins_t
{
    struct wire_pin_block_t *   first_block;
    struct wire_pin_block_t *   last_block;
    uint32_t                    pin_count;
};

struct wire_segment_pos_t
{
    int32_t                     start[2];
    int32_t                     end[2];
};

#define WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT 16

struct wire_segment_pos_block_t 
{
    POOL_ELEMENT;
    struct wire_segment_pos_block_t *   prev;
    struct wire_segment_pos_block_t *   next;
    struct wire_segment_pos_t           segments[WIRE_SEGMENT_POS_BLOCK_SEGMENT_COUNT];
}; 

enum WIRE_SEGMENT_TYPES
{
    WIRE_SEGMENT_TYPE_SEGMENT,
    WIRE_SEGMENT_TYPE_JUNCTION,
};

struct wire_segment_t
{
    POOL_ELEMENT;
    uint32_t                   type;
    uint32_t                   position_index;
    struct wire_segment_t *    next_segment;
    struct wire_segment_t *    prev_segment;
    struct wire_segment_t *    next_junction;
    struct wire_segment_t *    prev_junction;
};

// struct wire_sim_data_t
// {
//     uint32_t first_input_pin;
//     uint32_t first_output_pin;
//     uint16_t input_pin_count;
//     uint16_t output_pin_count;
//     uint16_t queued;
//     uint16_t value;
// };

struct wire_t
{
    POOL_ELEMENT;

    union
    {
        struct
        {
            struct wire_pins_t          wire_inputs;
            struct wire_pins_t          wire_outputs;
        };

        struct wire_pins_t              wire_pins[2];
    };

    uint64_t                            sim_data;

    // uint32_t                first_segment_pos;
    struct wire_segment_pos_block_t *   first_segment_pos;
    struct wire_segment_pos_block_t *   last_segment_pos;
    // uint32_t                            first_segment_pos_block;
    // uint32_t                            segment_pos_block_count;
    uint32_t                            segment_pos_count;

    struct wire_segment_t *             segments;

    uint32_t                            queued;
    uint8_t                             value;
};

void w_Init();

void w_Shutdown();

struct wire_t *w_CreateWire();

struct wire_t *w_GetWire(uint32_t wire_index);

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b);

void w_SplitWire(struct wire_t *wire, struct wire_segment_t *segment);

void w_ConnectWire(struct wire_t *wire, struct dev_t *device, uint16_t pin);

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin);

struct wire_t *w_ConnectPins(struct dev_t *device_a, uint16_t pin_a, struct dev_t *device_b, uint16_t pin_b);

void w_WireStep(struct wire_t *wire);


#endif