#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include "GL/glew.h"
#include "maths.h"
#include "list.h"

struct d_color_vert_t
{
    float position[2];
    float color[4];
};

struct d_device_vert_t
{
    float position[2];
    float tex_coords[2];
};

struct d_wire_vert_t
{
    float       position[2];
    uint32_t    value_sel;
};

struct d_device_tex_coords_t
{
    float tex_coords[4][2];  
};

#define D_DEVICE_DATA_BUFFER_SIZE       0x1000000
#define D_PIN_DATA_BUFFER_SIZE          0x1000000
#define D_JUNCTION_DATA_BUFFER_SIZE     0xc00000
#define D_SEGMENT_DATA_BUFFER_SIZE      0xc00000

#define D_DEVICE_DATA_BINDING           0
#define D_DEVICE_TEX_COORDS_BINDING     1
#define D_7SEG_DATA_BINDING             2
#define D_OUTPUT_DATA_BINDING           3
#define D_WIRE_COLOR_BINDING            4
#define D_DEVICE_TYPE_DEF_BINDING       5
#define D_DEVICE_PIN_DEF_BINDING        6
#define D_PIN_DATA_BINDING              7
#define D_JUNCTION_DATA_BINDING         8
#define D_SEGMENT_DATA_BINDING          9

struct d_device_type_def_t
{
    vec2_t      tex_size;
    vec2_t      tex_coords;
    uint32_t    first_pin;
    uint32_t    pad[3];
};

struct d_device_pin_def_t
{
    vec4_t      offset;
};

#define D_DEVICE_DATA_SLOT_INVALID_INDEX        0xffffffff
#define D_DEVICE_DATA_SLOT_INVALID_UPDATE_INDEX 0xffffffff

struct d_draw_data_slot_t
{
    union
    {
        void *                  object;
        struct dev_t *          device;
        struct wire_junc_t *    junction;
        struct wire_seg_t *     segment;
    };
    uint32_t                    data_index;
    uint32_t                    update_index;
};

struct d_device_data_slot_t
{
    struct dev_t *          device;
    uint32_t                data_index;
    uint32_t                update_index;
};

struct d_junction_data_slot_t
{
    struct wire_junc_t *    junction;
    uint32_t                data_index;
    uint32_t                update_index;
};

struct d_segment_data_update_t
{
    struct wire_seg_t *     segment;
    uint32_t                data_index;
    uint32_t                update_index;
};

struct d_device_data_t
{
    vec2_t      position;
    mat2_t      orientation;
    int32_t     selected;
    int32_t     type; 
};

struct d_pin_data_t
{
    uint32_t device_index;
    uint32_t pin_index;
};

struct d_junction_data_t
{
    float position[2];
};

struct d_segment_data_t
{
    uint32_t ends[2];
    uint32_t selected;
};

void d_Init();

void d_Shutdown();

GLuint d_CreateShader(const char *vertex_program, const char *fragment_program, const char *name);

GLuint d_CreateTexture(int32_t width, int32_t height, uint32_t internal_format, uint32_t min_filter, uint32_t mag_filter, uint32_t max_level, int32_t data_format, void *data);

struct d_draw_data_slot_t *d_AllocDrawDataSlot(struct list_t *slots, void *object);

uint32_t d_FreeDrawDataSlot(struct d_draw_data_slot_t *slot, struct list_t *slots, struct list_t *queue);

void d_QueueDrawDataUpdate(struct d_draw_data_slot_t *slot, struct list_t *queue);

struct d_draw_data_slot_t *d_AllocDeviceData(struct dev_t *device);

void d_FreeDeviceData(struct d_draw_data_slot_t *slot);

void d_QueueDeviceDataUpdate(struct d_draw_data_slot_t *slot);

struct d_draw_data_slot_t *d_AllocJunctionData(struct wire_junc_t *junction);

void d_FreeJunctionData(struct d_draw_data_slot_t *slot);

void d_QueueJunctionDataUpdate(struct d_draw_data_slot_t *slot);

struct d_draw_data_slot_t *d_AllocSegmentData(struct wire_seg_t *segment);

void d_FreeSegmentData(struct d_draw_data_slot_t *slot);

void d_QueueSegmentDataUpdate(struct d_draw_data_slot_t *slot);

void d_UpdateDeviceData();

void d_UpdateWireData();

void d_Draw();

void d_DrawGrid();

void d_DrawDevices();

void d_DrawWires();

void d_DrawPins();


#endif