#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

struct d_vert_t
{
    float position[2];
    float tex_coords[2];
};

struct d_primitive_data_t
{
    int32_t pos_x;
    int32_t pos_y;
    int32_t size;
    int32_t coord_offset;
};

void d_Init();

void d_Shutdown();

void d_DrawDevices();


#endif