#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include "GL/glew.h"

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

#define D_DEVICE_DATA_BINDING       0
#define D_DEVICE_TEX_COORDS_BINDING 1
#define D_7SEG_DATA_BINDING         2
#define D_OUTPUT_DATA_BINDING       3
#define D_WIRE_COLOR_BINDING        4
// struct d_device_data_t
// {
//     int32_t pos_x;
//     int32_t pos_y;
//     int32_t origin_x;
//     int32_t origin_y;
//     int32_t size;
//     int32_t coord_offset;
//     int32_t rot_flip_sel;
// };

struct d_device_data_t
{
    float       device_quad_size[2];
    float       device_tex_size[2];
    float       tex_coord_offset[2];
    float       position[2];
    int32_t     tex_coord_set;
    int32_t     flip_size;
    int32_t     selected;
}; 

// struct d_wire_data_t
// {

// };

void d_Init();

void d_Shutdown();

GLuint d_CreateShader(const char *vertex_program, const char *fragment_program, const char *name);

GLuint d_CreateTexture(int32_t width, int32_t height, uint32_t internal_format, uint32_t min_filter, uint32_t mag_filter, uint32_t max_level, int32_t data_format, void *data);

void d_Draw();


#endif