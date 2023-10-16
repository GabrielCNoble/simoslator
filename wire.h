#ifndef WIRE_H
#define WIRE_H

#include <stdint.h>
#include "pool.h"
#include "list.h"
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

/* forward declarations */
struct wire_t;
struct wire_seg_t;
struct wire_junc_t;

// #define WIRE_INVALID_DEVICE 0xffffffffffff
// #define WIRE_INVALID_PIN    0xffff

struct wire_pin_t
{
    uint64_t        device:    48;
    uint64_t        pin:       16;
};

struct wire_junc_pin_t
{
    struct wire_pin_t    pin;
    struct wire_junc_t * junction;
};

#define WIRE_JUNC_PIN_BLOCK_PIN_COUNT 8

struct wire_junc_pin_block_t
{
    POOL_ELEMENT;
    struct wire_junc_pin_block_t *   next;
    struct wire_junc_pin_block_t *   prev;
    struct wire_junc_pin_t           pins[WIRE_JUNC_PIN_BLOCK_PIN_COUNT];
};

// #define WIRE_DEV_BLOCK_DEV_COUNT 8
// struct wire_dev_block_t
// {
//     // uint64_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
//     struct wire_pin_t devices[WIRE_DEV_BLOCK_DEV_COUNT];
// };

struct wire_junc_pins_t
{
    struct wire_junc_pin_block_t *   first_block;
    struct wire_junc_pin_block_t *   last_block;
    uint32_t                         pin_count;
};

struct wire_seg_pos_t 
{
    struct wire_seg_t *         segment;
    int32_t                     ends[2][2];
};

struct wire_junc_pos_t
{
    struct wire_junc_t *        junction;
    int32_t                     pos[2];
};

#define WIRE_SEGMENT_POS_BLOCK_SIZE 16
#define WIRE_JUNCTION_POS_BLOCK_SIZE 8

struct wire_seg_pos_block_t 
{
    POOL_ELEMENT;
    struct wire_seg_pos_block_t *   prev;
    struct wire_seg_pos_block_t *   next;
    struct wire_seg_pos_t           segments[WIRE_SEGMENT_POS_BLOCK_SIZE];
};

struct wire_junc_pos_block_t
{
    POOL_ELEMENT;
    struct wire_junc_pos_block_t *  next;
    struct wire_junc_pos_block_t *  prev;
    struct wire_junc_pos_t          junctions[WIRE_JUNCTION_POS_BLOCK_SIZE];
};

enum WIRE_SEGMENT_TYPES
{
    WIRE_SEGMENT_TYPE_SEGMENT,
    WIRE_SEGMENT_TYPE_JUNCTION,
    WIRE_SEGMENT_TYPE_LAST,
};

#define WIRE_SEG_START_INDEX 0
#define WIRE_SEG_END_INDEX   1

struct wire_seg_junc_t
{
    struct wire_junc_t *     junction;
    struct wire_seg_t *      next;
    struct wire_seg_t *      prev;
};

struct wire_elem_t
{
    POOL_ELEMENT;
    struct wire_t *wire;
};

struct wire_junc_t
{
    struct wire_elem_t              base;
    // struct wire_junc_pos_t *        pos;
    // struct wire_junc_pos_block_t *  pos_block;
    // struct wire_seg_pos_t *         pos;
    int32_t *                       pos;
    struct wire_seg_t *             first_segment;
    struct wire_seg_t *             last_segment;
    struct wire_junc_pin_t *        pin;
    struct wire_junc_pin_block_t *  pin_block;
    struct wire_junc_t *            wire_next;
    struct wire_junc_t *            wire_prev;
    uint32_t                        selection_index;
};

struct wire_seg_t
{
    struct wire_elem_t              base;
    struct wire_seg_pos_t *         pos;
    struct wire_seg_pos_block_t *   pos_block;
    struct wire_seg_junc_t          junctions[2];

    /* segments may be linked to one another in whatever orientation */
    struct wire_seg_t *             segments[2];
    struct wire_seg_t *             wire_next;
    struct wire_seg_t *             wire_prev;
    uint32_t                        selection_index;
};

struct wire_t
{
    POOL_ELEMENT;

    union
    {
        struct
        {
            struct wire_junc_pins_t wire_inputs;
            struct wire_junc_pins_t wire_outputs;
        };

        struct wire_junc_pins_t     wire_pins[2];
    };

    uint64_t                        sim_data;

    struct wire_seg_pos_block_t *   first_segment_pos;
    struct wire_seg_pos_block_t *   last_segment_pos;
    uint32_t                        segment_pos_count;

    uint32_t                        junction_count;

    // struct wire_junc_pos_block_t *  first_junction_pos;
    // struct wire_junc_pos_block_t *  last_junction_pos;
    // uint32_t                        junction_pos_count;
    
    struct wire_seg_t *             first_segment;
    struct wire_seg_t *             last_segment;

    struct wire_junc_t *            first_junction;
    struct wire_junc_t *            last_junction;
};

void w_Init();

void w_Shutdown();

struct wire_t *w_AllocWire();

struct wire_t *w_FreeWire(struct wire_t *wire);

struct wire_t *w_GetWire(uint32_t wire_index);

struct wire_seg_t *w_AllocWireSegment(struct wire_t *wire);

void w_FreeWireSegment(struct wire_seg_t *segment);

struct wire_junc_t *w_AllocWireJunction(struct wire_t *wire); 

void w_FreeWireJunction(struct wire_junc_t *junction);

void w_LinkSegmentToJunction(struct wire_seg_t *segment, struct wire_junc_t *junction, uint32_t link_index);

void w_UnlinkSegmentFromJunction(struct wire_seg_t *segment, struct wire_junc_t *junction);

void w_UnlinkSegmentLinkIndex(struct wire_seg_t *segment, uint32_t link_index);

struct wire_junc_t *w_AddJunction(struct wire_seg_t *segment, int32_t *position);

void w_RemoveJunction(struct wire_junc_t *junction);

struct wire_t *w_MergeWires(struct wire_t *wire_a, struct wire_t *wire_b);

void w_SplitWire(struct wire_t *wire, struct wire_seg_t *segment);

struct wire_junc_pin_t *w_ConnectWire(struct wire_junc_t *junction, struct dev_t *device, uint16_t pin);

void w_DisconnectWire(struct wire_t *wire, struct wire_pin_t *pin);

struct wire_t *w_ConnectPins(struct dev_t *device_a, uint16_t pin_a, struct dev_t *device_b, uint16_t pin_b, struct list_t *wire_segments);

struct wire_t *w_CreateWireConnections(struct list_t *wire_segments);


#endif