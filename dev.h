#ifndef DEV_H
#define DEV_H

#include <stdint.h>
#include "pool.h"
#include "elem.h"
#include "draw.h"

#define DEV_MAX_DEVICES     0xffffffffffff
#define DEV_MAX_DEVICE_PINS 0xffff
#define DEV_INVALID_DEVICE  DEV_MAX_DEVICES
#define DEV_INVALID_PIN     DEV_MAX_DEVICE_PINS

#define DEV_MOS_PIN_SOURCE  0
#define DEV_MOS_PIN_DRAIN   1
#define DEV_MOS_PIN_GATE    2

enum DEV_DEVICE_CLASS
{
    DEV_DEVICE_CLASS_MOSFET,
    DEV_DEVICE_CLASS_GATE,
    DEV_DEVICE_CLASS_LAST,
}; 

enum DEV_DEVICE
{
    DEV_DEVICE_PMOS,
    DEV_DEVICE_NMOS,
    DEV_DEVICE_GND,
    DEV_DEVICE_POW,
    DEV_DEVICE_CLOCK,
    DEV_DEVICE_INPUT,
    DEV_DEVICE_7SEG,
    DEV_DEVICE_OUTPUT,
    // DEV_DEVICE_NOT,
    // DEV_DEVICE_AND,
    // DEV_DEVICE_NAND,
    // DEV_DEVICE_OR,
    // DEV_DEVICE_NOR,
    // DEV_DEVICE_XOR,
    // DEV_DEVICE_XNOR,
    DEV_DEVICE_LAST,
};

enum DEV_PIN_TYPES
{
    DEV_PIN_TYPE_IN,
    DEV_PIN_TYPE_OUT,
    DEV_PIN_TYPE_INOUT
};

struct dev_pin_desc_t
{
    // ivec2_t  offset;
    vec2_t   offset;
    uint8_t  direction;
    uint8_t  type;
};

struct dev_desc_t
{
    // uint16_t                    width;
    // uint16_t                    height;
    ivec2_t                     size;
    ivec2_t                     origin;
    ivec2_t                     tex_coords;
    // ivec2_t                     tex_size;
    float                       angle;
    // int32_t                     tex_coords[2];
    uint32_t                    pin_count;
    struct dev_pin_desc_t *     pins;
};

struct dev_mos_table_t
{
    uint8_t gate_table[8];
    uint8_t source_table[8];
};

#define DEV_PIN_BLOCK_PIN_COUNT 4
#define DEV_MAX_DEVICE_PINS 0xffff
// #define DEV_INVALID_WIRE 0xffffff

// enum DEV_DEVICE_ROTATION
// {
//     DEV_DEVICE_ROTATION_0,
//     DEV_DEVICE_ROTATION_90,
//     DEV_DEVICE_ROTATION_180,
//     DEV_DEVICE_ROTATION_270,
// };

// enum DEV_DEVICE_FLIP
// {
//     DEV_DEVICE_FLIP_X = 1,
//     DEV_DEVICE_FLIP_Y = 1 << 1,
// };

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

#define DEV_DEVICE_AXIS_NEG_MASK 0x2
#define DEV_POSITION_GRANULARITY 10

enum DEV_DEVICE_AXIS
{
    DEV_DEVICE_AXIS_POS_X,
    DEV_DEVICE_AXIS_POS_Y,
    DEV_DEVICE_AXIS_NEG_X = DEV_DEVICE_AXIS_POS_X | DEV_DEVICE_AXIS_NEG_MASK,
    DEV_DEVICE_AXIS_NEG_Y = DEV_DEVICE_AXIS_POS_Y | DEV_DEVICE_AXIS_NEG_MASK
};

struct dev_t
{
    POOL_ELEMENT;
    void *                          data;
    struct elem_t *                 element;

    uint64_t                        selection_index;

    /* TODO: those two could probably be put inside an union */
    uint64_t                        sim_data;
    uintptr_t                       serialized_index;
    // vec2_t                          position;
    // vec2_t                          origin;
    // float                       origin[2];
    // int32_t                     origin[2];
    // float                       orientation[2][2];
    // mat2_t                          orientation;
    struct d_device_data_slot_t *   draw_data;
    struct dev_pin_block_t *        pin_blocks;
    uint32_t                        type : 28;
    uint32_t                        x_axis : 2;
    uint32_t                        y_axis : 2;

    ivec2_t                         position;
    ivec2_t                         origin;
    
    // uint8_t                         rotation;
    // uint8_t                         flip;
    // uint8_t                     tex_coords;
    
    // uint8_t                     rotation;
    // uint8_t                     flip;
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

struct dev_output_t
{
    POOL_ELEMENT;
    struct dev_t *  device;
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

// void dev_UpdateDeviceElement(struct elem_t *element);

// void dev_UpdateDeviceRotation(struct dev_t *device);

void dev_TranslateDevice(void *element, ivec2_t *translation);

void dev_RotateDevice(void *element, ivec2_t *pivot, uint32_t ccw);

void dev_FlipDeviceV(void *element, ivec2_t *pivot);

void dev_FlipDeviceH(void *element, ivec2_t *pivot);

void dev_UpdateDevice(struct dev_t *device);

void dev_ClearDevices();

struct dev_t *dev_GetDevice(uint64_t device_index);

void dev_UpdateDeviceOrigin(struct dev_t *device);

void dev_GetDeviceLocalPinPosition(struct dev_t *device, uint16_t pin, ivec2_t *pin_position);

void dev_GetDeviceLocalBoxPosition(struct dev_t *device, vec2_t *min, vec2_t *max);

struct dev_pin_block_t *dev_GetDevicePinBlock(struct dev_t *device, uint16_t pin);

struct dev_pin_t *dev_GetDevicePin(struct dev_t *device, uint16_t pin);

void dev_ToggleInput(struct dev_input_t *input);

void dev_Update7SegDisplay(struct dev_7seg_disp_t *display);


#endif