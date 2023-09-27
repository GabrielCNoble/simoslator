#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

struct d_vert_t
{
    float position[2];
    float tex_coords[2];
};

struct d_device_data_t
{
    int32_t pos_x;
    int32_t pos_y;
    int32_t size;
    int32_t coord_offset;
    int32_t rot_flip;
};

void d_Init();

void d_Shutdown();

void d_DrawDevices();


#endif