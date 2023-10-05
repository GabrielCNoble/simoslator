#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

struct d_vert_t
{
    float position[2];
    float tex_coords[2];
};

struct d_wire_vert_t
{
    float       position[2];
    uint32_t    value;
};

struct d_device_data_t
{
    int32_t pos_x;
    int32_t pos_y;
    int32_t origin_x;
    int32_t origin_y;
    int32_t size;
    int32_t coord_offset;
    int32_t rot_flip_sel;
};

void d_Init();

void d_Shutdown();

void d_DrawDevices();


#endif