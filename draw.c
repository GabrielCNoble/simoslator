#include "draw.h"
#include "pool.h"
#include "list.h"
#include "dev.h"
#include "wire.h"
#include "sim.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define D_VERSION_EXTS                                              \
    "#version 400 core\n"                                           \
    "#extension GL_ARB_shader_storage_buffer_object : require\n"    \
    "#extension GL_ARB_arrays_of_arrays : require\n"                \

#define D_DEVICE_DATA_BUFFER                                        \
    "struct d_device_data_t\n"                                      \
    "{\n"                                                           \
        "float   device_quad_size[2];\n"                            \
        "float   device_tex_size[2];\n"                             \
        "float   tex_coord_offset[2];\n"                            \
        "float   position[2];\n"                                    \
        "int     tex_coord_set;\n"                                  \
        "int     flip_size;\n"                                      \
        "int     selected;\n"                                       \
    "};\n"                                                          \
                                                                    \
    "layout (std430) buffer d_device_data_buffer\n"                 \
    "{\n"                                                           \
        "d_device_data_t d_device_data[];\n"                        \
    "};\n"                                                          \

#define D_DEVICE_TEX_COORDS_BUFFER                                  \
    "struct d_device_tex_coords_t\n"                                \
    "{\n"                                                           \
        "float tex_coords[4][2];"                                   \
    "};\n"                                                          \
    "layout (std430) buffer d_device_tex_coords_buffer\n"           \
    "{\n"                                                           \
        "d_device_tex_coords_t d_device_tex_coords[];"              \
    "};\n"                                                          \


#define D_WIRE_COLOR_BUFFER                                         \
    "layout (std430) buffer d_wire_colors\n"                        \
    "{\n"                                                           \
        "vec4 wire_colors[];\n"                                     \
    "};\n"                                                          \

// const char *d_draw_device_vertex_shader =
//     "#version 400 core\n"
//     "#extension GL_ARB_shader_storage_buffer_object : require\n"
//     "layout (location = 0) in vec4 d_position;\n"
//     "layout (location = 1) in vec2 d_tex_coords;\n"

//     "#define DEV_DEVICE_ROTATION_0      0\n"
//     "#define DEV_DEVICE_ROTATION_90     1\n"
//     "#define DEV_DEVICE_ROTATION_180    2\n"
//     "#define DEV_DEVICE_ROTATION_270    3\n"
    
//     "#define DEV_DEVICE_FLIP_X    (1 << 1)\n"
//     "#define DEV_DEVICE_FLIP_Y    (1 << 0)\n"

//     "struct d_device_data_t\n"
//     "{\n"
//         "int pos_x;\n"
//         "int pos_y;\n"
//         "int origin_x;\n"
//         "int origin_y;\n"
//         "int size;\n"
//         "int coord_offset;\n"
//         "int rot_flip_sel;\n"
//     "};\n"

//     "layout (std430) buffer d_device_data_buffer\n"
//     "{\n"
//         "d_device_data_t d_device_data[];\n"
//     "};\n"

//     "out vec2 tex_coords;\n"
//     "flat out vec2 size;\n"
//     "flat out vec2 coord_offset;\n"
//     "flat out int selected;\n"
//     "uniform mat4 d_model_view_projection_matrix;\n"

//     "void main()\n"
//     "{\n"
//         "d_device_data_t data = d_device_data[gl_InstanceID];"
//         "size = vec2(float(data.size & 0xffff), float((data.size >> 16) & 0xffff));\n"
//         "tex_coords = d_tex_coords;\n"
//         "coord_offset = vec2(float(data.coord_offset & 0xffff), float((data.coord_offset >> 16) & 0xffff));\n"
//         "vec4 position = vec4(d_position.x * float(size.x), d_position.y * float(size.y), 0.1f, 1);\n"
//         "int rotation = data.rot_flip_sel & 0xffff;\n"
//         "int flip_sel = (data.rot_flip_sel >> 16) & 0xffff;\n"

//         "position.x -= float(data.origin_x);\n"
//         "position.y -= float(data.origin_y);\n"

//         "bool flip_x = bool(flip_sel & DEV_DEVICE_FLIP_X);\n"
//         "bool flip_y = bool(flip_sel & DEV_DEVICE_FLIP_Y);\n"

//         "if(flip_x)\n"
//         "{\n"
//             "position.x = -position.x;\n"
//         "}\n"

//         "if(flip_y)\n"
//         "{\n"
//             "position.y = -position.y;\n"
//         "}\n"

//         "switch(rotation)\n"
//         "{\n"
//             "case DEV_DEVICE_ROTATION_90:\n"
//             "{\n"
//                 "float temp = position.y;\n"
//                 "position.y = position.x;\n"
//                 "position.x = -temp;\n"
//             "}\n"
//             "break;\n"

//             "case DEV_DEVICE_ROTATION_180:\n"
//             "{\n"
//                 "position.y = -position.y;\n"
//                 "position.x = -position.x;\n"
//             "}\n"
//             "break;\n"
    
//             "case DEV_DEVICE_ROTATION_270:\n"
//             "{\n"
//                 "float temp = position.y;\n"
//                 "position.y = -position.x;\n"
//                 "position.x = temp;\n"
//             "}\n"
//             "break;\n"
//         "}\n"

//         "selected = flip_sel & 4;\n"

//         "position.x += float(data.pos_x);\n"
//         "position.y += float(data.pos_y);\n"
//         "gl_Position = d_model_view_projection_matrix * position;\n"
//     "}\n";

const char *d_device_vertex_shader =
    D_VERSION_EXTS

    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in vec2 d_tex_coords;\n"

    D_DEVICE_DATA_BUFFER

    D_DEVICE_TEX_COORDS_BUFFER

    "out vec2 tex_coords;\n"
    "flat out int selected;\n"
    "uniform mat4 d_model_view_projection_matrix;\n"
    "uniform sampler2D d_texture;\n"

    "void main()\n"
    "{\n"
        "d_device_data_t data = d_device_data[gl_InstanceID];\n"
        "d_device_tex_coords_t device_tex_coords = d_device_tex_coords[data.tex_coord_set];\n"
        "vec2 texture_size = textureSize(d_texture, 0);\n"
        "vec2 device_size = vec2(data.device_quad_size[0], data.device_quad_size[1]);\n"
        "vec2 coord_offset = vec2(data.tex_coord_offset[0], data.tex_coord_offset[1]);\n"
        "tex_coords = vec2(device_tex_coords.tex_coords[gl_VertexID][0], device_tex_coords.tex_coords[gl_VertexID][1]);\n"
        "tex_coords = coord_offset + vec2(data.device_tex_size[0] * tex_coords.x, data.device_tex_size[1] * tex_coords.y);\n"
        "vec4 position = vec4(d_position.x * device_size[data.flip_size], d_position.y * device_size[data.flip_size ^ 1], 0.1f, 1);\n"
        "gl_Position = d_model_view_projection_matrix * (position + vec4(data.position[0], data.position[1], 0, 0));\n"
        "selected = data.selected;\n"
    "}\n";

const char *d_device_fragment_shader = 
    D_VERSION_EXTS

    "in vec2 tex_coords;\n"
    // "flat in ivec2 device_size;\n"
    // "flat in ivec2 coord_offset;\n"
    "flat in int selected;\n"
    "uniform sampler2D d_texture;\n"
    "void main()\n"
    "{\n"
        // "ivec2 coords = ivec2(float(coord_offset.x) + float(size.x * 2.0f) * tex_coords.x, float(coord_offset.y) + float(size.y * 2.0f) * tex_coords.y);\n"
        // "vec4 color = texelFetch(d_texture, coords, 0);\n"

        // "vec2 tex_size = textureSize(d_texture, 0);\n"
        // "vec2 coords = vec2(((float(device_size.x) * 2.0f) / tex_size.x) * tex_coords.x, ((float(device_size.y) * 2.0f) / tex_size.y) * tex_coords.y);\n"
        // "coords.x += float(coord_offset.x) / tex_size.x;\n"
        // "coords.y += float(coord_offset.y) / tex_size.y;\n"
        "vec4 color = texture(d_texture, tex_coords);\n"
        "if(color.a == 0.0f)\n"
        "{\n"
            "discard;\n"
        "}\n"
        "if(selected != 0)\n"
        "{\n"
            "color = vec4(0.0f, 0.5f, 0.8f, color.a);\n"
        "}\n"
        "gl_FragColor = color;\n"
    "}\n";



const char *d_7seg_mask_vertex_shader = 
    D_VERSION_EXTS

    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in vec2 d_tex_coords;\n"

    D_DEVICE_DATA_BUFFER

    D_DEVICE_TEX_COORDS_BUFFER

    "uniform mat4 d_model_view_projection_matrix;\n"
    "out vec2 tex_coords;\n"
    "flat out int display_index;\n"

    "void main()\n"
    "{\n"
        "d_device_data_t data = d_device_data[gl_InstanceID];\n"
        "d_device_tex_coords_t device_tex_coords = d_device_tex_coords[data.tex_coord_set];\n"
        "vec2 device_size = vec2(data.device_quad_size[0], data.device_quad_size[1]);\n"
        "tex_coords = vec2(device_tex_coords.tex_coords[gl_VertexID][0], device_tex_coords.tex_coords[gl_VertexID][1]);\n"
        "display_index = gl_InstanceID;\n"
        "vec4 position = vec4(d_position.x * device_size[data.flip_size], d_position.y * device_size[data.flip_size ^ 1], 0.1f, 1);\n"
        "gl_Position = d_model_view_projection_matrix * (position + vec4(data.position[0], data.position[1], 0, 0));\n"
    "}\n";

const char *d_7seg_mask_fragment_shader = 
    D_VERSION_EXTS

    "uniform usampler2D d_texture;\n"
    "in vec2 tex_coords;\n"
    "flat in int display_index;\n"

    "layout (std430) buffer d_7seg_data_buffer\n"
    "{\n"
        "int d_7seg_data[];\n"
    "};\n"

    "void main()\n"
    "{\n"
        "int data = (d_7seg_data[display_index >> 2] >> ((display_index % 4) << 3)) & 0xff;\n"
        "vec2 tex_size = textureSize(d_texture, 0);\n"
        "uvec4 mask = texture(d_texture, tex_coords /* + vec2(1.0f / tex_size.x, 1.0f / tex_size.y) */);\n"
        "if(mask.a == 0 || (data & (1 << mask.r)) == 0)\n"
        "{\n"
            "discard;"
        "}\n"

        "gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
    "}\n";

const char *d_output_mask_fragment_shader = 
    D_VERSION_EXTS

    "uniform usampler2D d_texture;\n"
    "in vec2 tex_coords;\n"
    "flat in int display_index;\n"

    "layout (std430) buffer d_output_data_buffer\n"
    "{\n"
        "int d_output_data[];\n"
    "};\n"

    D_WIRE_COLOR_BUFFER

    "void main()\n"
    "{\n"
        "int data = (d_output_data[display_index >> 2] >> ((display_index % 4) << 3)) & 0xff;\n"
        "vec2 tex_size = textureSize(d_texture, 0);\n"
        "uvec4 mask = texture(d_texture, tex_coords /* + vec2(1.0f / tex_size.x, 1.0f / tex_size.y) */);\n"
        "if(mask.a == 0)\n"
        "{\n"
            "discard;"
        "}\n"

        "gl_FragColor = wire_colors[data];\n"
    "}\n";

const char *d_wire_vertex_shader = 
    D_VERSION_EXTS

    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in int d_value_sel;\n"

    "uniform mat4 d_model_view_projection_matrix;\n"
    "flat out int wire_value;\n"
    "flat out int selected;\n"

    "void main()\n"
    "{\n"
        "gl_Position = d_model_view_projection_matrix * vec4(d_position.xy, 0.1f, 1.0f);\n"
        "wire_value = d_value_sel & 0xffff;\n"
        "selected = d_value_sel >> 16;"
    "}\n";

const char *d_wire_fragment_shader = 
    D_VERSION_EXTS
    
    D_WIRE_COLOR_BUFFER

    "flat in int wire_value;\n"
    "flat in int selected;\n"
    "void main()\n"
    "{\n"
        "vec4 color;\n"
        "if(selected != 0)\n"
        "{\n"
            "color = vec4(0.0f, 0.5f, 0.8f, 1.0f);\n"
        "}\n"
        "else\n"
        "{\n"
            "color = wire_colors[wire_value];"
        "}\n"
        "gl_FragColor = color;\n"
    "}\n";

const char *d_selection_box_vertex_shader = 
    D_VERSION_EXTS
    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in vec4 d_color;\n"

    "uniform mat4 d_model_view_projection_matrix;\n"
    "out vec4 color;\n"

    "void main()\n"
    "{\n"
        "color = d_color;\n"
        "gl_Position = d_model_view_projection_matrix * d_position;\n"
    "}\n";

const char *d_selection_box_fragment_shader = 
    D_VERSION_EXTS
    "in vec4 color;\n"

    "void main()\n"
    "{\n"
        "gl_FragColor = color;\n"
    "}\n";
 
#define D_DEVICE_VERTEX_BUFFER_SIZE     4
#define D_DEVICE_INDEX_BUFFER_SIZE      6
#define D_DEVICE_DATA_BUFFER_SIZE       0xfffff
#define D_DEVICE_DATA_BUFFER_COUNT      8

#define D_WIRE_VERTEX_BUFFER_SIZE       0xfffff
#define D_WIRE_VERTEX_BUFFER_COUNT      8

#define D_7SEG_DISPLAY_DATA_BUFFER_SIZE 0xfff

GLuint                      d_vao;

GLuint                      d_wire_value_color_buffer;
GLuint                      d_device_tex_coords_buffer;
GLuint                      d_device_shader;
uint32_t                    d_device_shader_model_view_projection_matrix;
uint32_t                    d_device_shader_texture;
uint32_t                    d_device_data_buffer_index;
GLuint                      d_device_vertex_buffer;
GLuint                      d_device_index_buffer;
GLuint                      d_device_data_buffer[D_DEVICE_DATA_BUFFER_COUNT];
struct d_device_data_t *    d_device_data;

GLuint                      d_wire_shader;
uint32_t                    d_wire_shader_model_view_projection_matrix;
GLuint                      d_wire_vertex_buffer;
struct d_wire_vert_t *      d_wire_vertices;

GLuint                      d_7seg_mask_shader;
uint32_t                    d_7seg_mask_shader_model_view_projection_matrix;
uint32_t                    d_7seg_mask_shader_texture;
GLuint                      d_7seg_display_data_buffer;
uint32_t *                  d_7seg_display_data;

GLuint                      d_output_mask_shader;
uint32_t                    d_output_mask_shader_model_view_projection_matrix;
uint32_t                    d_output_mask_shader_texture;
GLuint                      d_output_data_buffer;
uint32_t *                  d_output_data;

GLuint                      d_selection_box_shader;
GLuint                      d_selection_box_vertex_buffer;
uint32_t                    d_selection_box_shader_model_view_projection_matrix;

// GLuint                      d_wire_id_buffer;
// uint32_t *                  d_wire_ids;

float                       d_model_view_projection_matrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

/* from dev.c */
extern struct pool_t        dev_devices;
extern struct pool_t        dev_outputs;
extern struct pool_t        dev_7seg_disps;
extern GLuint               dev_devices_texture;
extern uint32_t             dev_devices_texture_width;
extern uint32_t             dev_devices_texture_height;
extern GLuint               dev_7seg_mask_texture;
extern GLuint               dev_output_mask_texture;
extern struct dev_desc_t    dev_device_descs[];

/* from wire.c */
extern struct pool_t        w_wires;
extern struct pool_t        w_wire_juncs;
extern struct pool_t        w_wire_segs;
extern struct list_t        w_wire_seg_pos;
extern struct list_t        w_wire_junc_pos;

/* from main.c */
extern uint32_t             m_window_width;
extern uint32_t             m_window_height;
extern float                m_zoom;
extern float                m_offset_x;
extern float                m_offset_y;
extern uint32_t             m_run_simulation;
extern uint32_t             m_single_step;
extern uint32_t             m_selected_device_type;
extern struct dev_t *       m_selected_device;
extern uint32_t             m_draw_selection_box;
extern int32_t              m_selection_box_min[2];
extern int32_t              m_selection_box_max[2];
extern int32_t              m_mouse_pos[2];
extern int32_t              m_place_device_x;
extern int32_t              m_place_device_y;
extern struct list_t        m_wire_seg_pos;
extern struct list_t        m_objects[];
// extern struct list_t        m_wire_segments;

/* from sim.c */
extern struct list_t        sim_wire_data;


void d_Init()
{
    glGenVertexArrays(1, &d_vao);
    glBindVertexArray(d_vao);

    struct d_device_vert_t vertices[] = {
        {.position = {-1.0, 1.0}, .tex_coords = {0.0, 0.0}},
        {.position = {-1.0,-1.0}, .tex_coords = {0.0, 1.0}},
        {.position = { 1.0,-1.0}, .tex_coords = {1.0, 1.0}},
        {.position = { 1.0, 1.0}, .tex_coords = {1.0, 0.0}},
    };

    glGenBuffers(1, &d_device_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_device_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, D_DEVICE_VERTEX_BUFFER_SIZE * sizeof(struct d_device_vert_t), vertices, GL_STATIC_DRAW);

    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0
    };

    glGenBuffers(1, &d_device_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_device_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, D_DEVICE_INDEX_BUFFER_SIZE * sizeof(uint32_t), indices, GL_STATIC_DRAW);

    for(uint32_t index = 0; index < D_DEVICE_DATA_BUFFER_COUNT; index++)
    {
        glGenBuffers(1, &d_device_data_buffer[index]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_device_data_buffer[index]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BUFFER_SIZE * sizeof(struct d_device_data_t), NULL, GL_DYNAMIC_DRAW);
    }
    d_device_data = calloc(D_DEVICE_DATA_BUFFER_SIZE, sizeof(struct d_device_data_t));

    glGenBuffers(1, &d_7seg_display_data_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_7seg_display_data_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, D_7SEG_DISPLAY_DATA_BUFFER_SIZE * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
    d_7seg_display_data = calloc(D_7SEG_DISPLAY_DATA_BUFFER_SIZE, sizeof(uint32_t));

    glGenBuffers(1, &d_output_data_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_output_data_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, D_7SEG_DISPLAY_DATA_BUFFER_SIZE * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
    d_output_data = calloc(D_7SEG_DISPLAY_DATA_BUFFER_SIZE, sizeof(uint32_t));

    glGenBuffers(1, &d_wire_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_wire_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, D_WIRE_VERTEX_BUFFER_SIZE * sizeof(struct d_wire_vert_t), NULL, GL_STATIC_DRAW);
    d_wire_vertices = calloc(D_WIRE_VERTEX_BUFFER_SIZE, sizeof(struct d_wire_vert_t));

    glGenBuffers(1, &d_selection_box_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_selection_box_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct d_color_vert_t) * 4, NULL, GL_STATIC_DRAW);

    d_device_shader = d_CreateShader(d_device_vertex_shader, d_device_fragment_shader, "device_shader");
    d_device_shader_model_view_projection_matrix = glGetUniformLocation(d_device_shader, "d_model_view_projection_matrix");
    d_device_shader_texture = glGetUniformLocation(d_device_shader, "d_texture");
    GLuint device_data_block = glGetProgramResourceIndex(d_device_shader, GL_SHADER_STORAGE_BLOCK, "d_device_data_buffer");
    glShaderStorageBlockBinding(d_device_shader, device_data_block, D_DEVICE_DATA_BINDING);
    GLuint device_tex_coords_block = glGetProgramResourceIndex(d_device_shader, GL_SHADER_STORAGE_BLOCK, "d_device_tex_coords_buffer");
    glShaderStorageBlockBinding(d_device_shader, device_tex_coords_block, D_DEVICE_TEX_COORDS_BINDING);
    
    d_wire_shader = d_CreateShader(d_wire_vertex_shader, d_wire_fragment_shader, "wire_shader");
    d_wire_shader_model_view_projection_matrix = glGetUniformLocation(d_wire_shader, "d_model_view_projection_matrix");
    uint32_t wire_colors_block = glGetProgramResourceIndex(d_wire_shader, GL_SHADER_STORAGE_BLOCK, "d_wire_colors");
    glShaderStorageBlockBinding(d_wire_shader, wire_colors_block, D_WIRE_COLOR_BINDING);

    d_7seg_mask_shader = d_CreateShader(d_7seg_mask_vertex_shader, d_7seg_mask_fragment_shader, "7seg_mask_shader");
    d_7seg_mask_shader_model_view_projection_matrix = glGetUniformLocation(d_7seg_mask_shader, "d_model_view_projection_matrix");
    d_7seg_mask_shader_texture = glGetUniformLocation(d_7seg_mask_shader, "d_texture");
    device_data_block = glGetProgramResourceIndex(d_7seg_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_device_data_buffer");
    glShaderStorageBlockBinding(d_7seg_mask_shader, device_data_block, D_DEVICE_DATA_BINDING);
    device_tex_coords_block = glGetProgramResourceIndex(d_7seg_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_device_tex_coords_buffer");
    glShaderStorageBlockBinding(d_7seg_mask_shader, device_tex_coords_block, D_DEVICE_TEX_COORDS_BINDING);
    uint32_t display_data_block = glGetProgramResourceIndex(d_7seg_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_7seg_data_buffer");
    glShaderStorageBlockBinding(d_7seg_mask_shader, display_data_block, D_7SEG_DATA_BINDING);

    d_output_mask_shader = d_CreateShader(d_7seg_mask_vertex_shader, d_output_mask_fragment_shader, "output_mask_shader");
    d_output_mask_shader_model_view_projection_matrix = glGetUniformLocation(d_output_mask_shader, "d_model_view_projection_matrix");
    d_output_mask_shader_texture = glGetUniformLocation(d_output_mask_shader, "d_texture");
    device_data_block = glGetProgramResourceIndex(d_output_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_device_data_buffer");
    glShaderStorageBlockBinding(d_output_mask_shader, device_data_block, D_DEVICE_DATA_BINDING);
    device_tex_coords_block = glGetProgramResourceIndex(d_output_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_device_tex_coords_buffer");
    glShaderStorageBlockBinding(d_output_mask_shader, device_tex_coords_block, D_DEVICE_TEX_COORDS_BINDING);
    wire_colors_block = glGetProgramResourceIndex(d_output_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_wire_colors");
    glShaderStorageBlockBinding(d_output_mask_shader, wire_colors_block, D_WIRE_COLOR_BINDING);
    uint32_t output_data_block = glGetProgramResourceIndex(d_output_mask_shader, GL_SHADER_STORAGE_BLOCK, "d_output_data_buffer");
    glShaderStorageBlockBinding(d_output_mask_shader, output_data_block, D_OUTPUT_DATA_BINDING);
    
    d_selection_box_shader = d_CreateShader(d_selection_box_vertex_shader, d_selection_box_fragment_shader, "selection_box_shader");
    d_selection_box_shader_model_view_projection_matrix = glGetUniformLocation(d_selection_box_shader, "d_model_view_projection_matrix");


    float wire_colors[][4] = {
        [WIRE_VALUE_0S]     = {0, 0.5, 0, 1},
        [WIRE_VALUE_1S]     = {0, 1.0, 0, 1},
        [WIRE_VALUE_0W]     = {0.2, 0.1, 0, 1},
        [WIRE_VALUE_1W]     = {0.2, 0.2, 0, 1},
        [WIRE_VALUE_Z]      = {0.3, 0.3, 0.3, 1},
        [WIRE_VALUE_U]      = {0.3, 0.0, 0.5, 1},
        [WIRE_VALUE_ERR]    = {1, 0, 0, 1},
        [WIRE_VALUE_IND]    = {0.8f, 1, 0, 1}
    };

    glGenBuffers(1, &d_wire_value_color_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_wire_value_color_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(wire_colors), wire_colors, GL_STATIC_DRAW);

    struct d_device_tex_coords_t device_tex_coords[] = {
        [DEV_DEVICE_ROTATION_0]                                 = {.tex_coords[0] = {0, 0}, .tex_coords[1] = {0, 1}, .tex_coords[2] = {1, 1}, .tex_coords[3] = {1, 0}},
        [DEV_DEVICE_ROTATION_90]                                = {.tex_coords[0] = {1, 0}, .tex_coords[1] = {0, 0}, .tex_coords[2] = {0, 1}, .tex_coords[3] = {1, 1}},
        [DEV_DEVICE_ROTATION_180]                               = {.tex_coords[0] = {1, 1}, .tex_coords[1] = {1, 0}, .tex_coords[2] = {0, 0}, .tex_coords[3] = {0, 1}},
        [DEV_DEVICE_ROTATION_270]                               = {.tex_coords[0] = {0, 1}, .tex_coords[1] = {1, 1}, .tex_coords[2] = {1, 0}, .tex_coords[3] = {0, 0}},     

        [DEV_DEVICE_ROTATION_360 + DEV_DEVICE_ROTATION_0]       = {.tex_coords[0] = {1, 0}, .tex_coords[1] = {1, 1}, .tex_coords[2] = {0, 1}, .tex_coords[3] = {0, 0}},
        [DEV_DEVICE_ROTATION_360 + DEV_DEVICE_ROTATION_90]      = {.tex_coords[0] = {0, 0}, .tex_coords[1] = {1, 0}, .tex_coords[2] = {1, 1}, .tex_coords[3] = {0, 1}},
        [DEV_DEVICE_ROTATION_360 + DEV_DEVICE_ROTATION_180]     = {.tex_coords[0] = {0, 1}, .tex_coords[1] = {0, 0}, .tex_coords[2] = {1, 0}, .tex_coords[3] = {1, 1}},
        [DEV_DEVICE_ROTATION_360 + DEV_DEVICE_ROTATION_270]     = {.tex_coords[0] = {1, 1}, .tex_coords[1] = {0, 1}, .tex_coords[2] = {0, 0}, .tex_coords[3] = {1, 0}},        
    };

    glGenBuffers(1, &d_device_tex_coords_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_device_tex_coords_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(device_tex_coords), device_tex_coords, GL_STATIC_DRAW);
}

void d_Shutdown()
{

}

GLuint d_CreateShader(const char *vertex_program, const char *fragment_program, const char *name)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_program, NULL);
    glCompileShader(vertex_shader);

    GLint status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(vertex_shader, log_length, NULL, info_log);
        printf("[%s] vertex shader error:\n%s\n", name, info_log);
        free(info_log);
        return 0;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_program, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(fragment_shader, log_length, NULL, info_log);
        printf("[%s] fragment shader error:\n%s\n", name, info_log);
        free(info_log);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetProgramInfoLog(program, log_length, NULL, info_log);
        printf("[%s] shader link error:\n%s\n", name, info_log);
        free(info_log);
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

GLuint d_CreateTexture(int32_t width, int32_t height, uint32_t internal_format, uint32_t min_filter, uint32_t mag_filter, uint32_t max_level, int32_t data_format, void *data)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, GL_UNSIGNED_BYTE, data);

    if(max_level > 0)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return texture;
}

void d_Draw()
{
    d_model_view_projection_matrix[0] = 2.0 / ((float)m_window_width / m_zoom);
    d_model_view_projection_matrix[5] = 2.0 / ((float)m_window_height / m_zoom);
    d_model_view_projection_matrix[12] = -m_offset_x * d_model_view_projection_matrix[0];
    d_model_view_projection_matrix[13] = -m_offset_y * d_model_view_projection_matrix[5];

    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    d_DrawWires();
    d_DrawDevices();
    
    if(m_draw_selection_box)
    {
        glUseProgram(d_selection_box_shader);
        glBindBuffer(GL_ARRAY_BUFFER, d_selection_box_vertex_buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_color_vert_t), (void *)(offsetof(struct d_color_vert_t, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct d_color_vert_t), (void *)(offsetof(struct d_color_vert_t, color)));
        glDisableVertexAttribArray(2);
        glUniformMatrix4fv(d_selection_box_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);

        struct d_color_vert_t box_verts[] = {
            {.position = {m_selection_box_min[0], m_selection_box_max[1]}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
            {.position = {m_selection_box_min[0], m_selection_box_min[1]}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
            {.position = {m_selection_box_max[0], m_selection_box_min[1]}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
            {.position = {m_selection_box_max[0], m_selection_box_max[1]}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(box_verts), box_verts);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
}

void d_DrawDevices()
{
    glBindBuffer(GL_ARRAY_BUFFER, d_device_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_device_index_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_TEX_COORDS_BINDING, d_device_tex_coords_buffer);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BINDING, d_device_data_buffer);
    glUseProgram(d_device_shader);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, tex_coords)));
    glDisableVertexAttribArray(2);

    glUniformMatrix4fv(d_device_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
    glBindTexture(GL_TEXTURE_2D, dev_devices_texture);
    glUniform1i(d_device_shader_texture, 0);

    uint32_t device_count = dev_devices.cursor - (dev_devices.free_indices_top + 1);
    uint32_t update_count = device_count / D_DEVICE_DATA_BUFFER_SIZE;

    if(device_count % D_DEVICE_DATA_BUFFER_SIZE)
    {
        update_count++;
    }

    uint32_t first_device = 0;
    uint32_t last_device = D_DEVICE_DATA_BUFFER_SIZE;

    for(uint32_t update_index = 0; update_index < update_count; update_index++)
    {
        uint32_t buffer_offset = 0;

        for(uint32_t device_index = first_device; device_index < last_device; device_index++)
        {
            struct dev_t *device = pool_GetValidElement(&dev_devices, device_index);

            if(device != NULL)
            {
                // struct d_device_data_t *data = d_device_data + buffer_offset;
                // struct dev_desc_t *desc = dev_device_descs + device->type;
                // int32_t width = desc->width >> 1;
                // int32_t height = desc->height >> 1;
                // data->pos_x = device->position[0];
                // data->pos_y = device->position[1];
                // data->origin_x = desc->origin[0];
                // data->origin_y = desc->origin[1];
                // data->size = (height << 16) | width;
                // data->rot_flip_sel = (device->flip << 16) | device->rotation | ((device->selection_index != INVALID_LIST_INDEX) << 18);
                // data->coord_offset = (desc->tex_offset[1] << 16) | desc->tex_offset[0];
                // buffer_offset++;

                struct d_device_data_t *data = d_device_data + buffer_offset;
                struct dev_desc_t *desc = dev_device_descs + device->type;
                data->device_quad_size[0] = desc->width >> 1;
                data->device_quad_size[1] = desc->height >> 1;
                data->device_tex_size[0] = (float)desc->width / (float)dev_devices_texture_width;
                data->device_tex_size[1] = (float)desc->height / (float)dev_devices_texture_height;
                data->tex_coord_set = device->tex_coords;
                data->flip_size = device->rotation == DEV_DEVICE_ROTATION_90 || device->rotation == DEV_DEVICE_ROTATION_270;
                data->position[0] = device->position[0] - device->origin[0];
                data->position[1] = device->position[1] - device->origin[1];
                data->tex_coord_offset[0] = (float)desc->tex_offset[0] / (float)dev_devices_texture_width;
                data->tex_coord_offset[1] = (float)desc->tex_offset[1] / (float)dev_devices_texture_height;
                data->selected = device->selection_index != INVALID_LIST_INDEX;
                buffer_offset++;
            }
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BINDING, d_device_data_buffer[d_device_data_buffer_index]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, buffer_offset);

        first_device += D_DEVICE_DATA_BUFFER_SIZE;
        last_device += D_DEVICE_DATA_BUFFER_SIZE;

        if(last_device > dev_devices.cursor)
        {
            last_device = dev_devices.cursor;
        }

        d_device_data_buffer_index = (d_device_data_buffer_index + 1) % D_DEVICE_DATA_BUFFER_COUNT;
    }

    if(m_selected_device_type != DEV_DEVICE_TYPE_LAST)
    {
        struct dev_desc_t *desc = dev_device_descs + m_selected_device_type;
        // int32_t width = desc->width >> 1;
        // int32_t height = desc->height >> 1;
        // d_device_data->pos_x = m_place_device_x;
        // d_device_data->pos_y = m_place_device_y;
        // d_device_data->origin_x = desc->origin[0];
        // d_device_data->origin_y = desc->origin[1];
        // d_device_data->size = (height << 16) | width;
        // d_device_data->rot_flip_sel = 0;
        // d_device_data->coord_offset = (desc->tex_offset[1] << 16) | desc->tex_offset[0];

        d_device_data->device_quad_size[0] = desc->width >> 1;
        d_device_data->device_quad_size[1] = desc->height >> 1;
        d_device_data->device_tex_size[0] = (float)desc->width / (float)dev_devices_texture_width;
        d_device_data->device_tex_size[1] = (float)desc->height / (float)dev_devices_texture_height;
        d_device_data->position[0] = m_place_device_x - desc->origin[0];
        d_device_data->position[1] = m_place_device_y - desc->origin[1];
        d_device_data->tex_coord_set = 0;
        d_device_data->flip_size = 0;
        d_device_data->tex_coord_offset[0] = (float)desc->tex_offset[0] / (float)dev_devices_texture_width;
        d_device_data->tex_coord_offset[1] = (float)desc->tex_offset[1] / (float)dev_devices_texture_height;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BINDING, d_device_data_buffer[d_device_data_buffer_index]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, 1);
    }

    if(dev_7seg_disps.cursor)
    {
        glUseProgram(d_7seg_mask_shader);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, tex_coords)));
        glDisableVertexAttribArray(2);

        glUniformMatrix4fv(d_7seg_mask_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
        glBindTexture(GL_TEXTURE_2D, dev_7seg_mask_texture);
        glUniform1i(d_7seg_mask_shader_texture, 0);
        glDepthFunc(GL_EQUAL);
        // glDisable(GL_DEPTH_TEST);
        uint32_t buffer_offset = 0;
        for(uint32_t index = 0; index < dev_7seg_disps.cursor; index++)
        {
            struct dev_7seg_disp_t *display = pool_GetValidElement(&dev_7seg_disps, index);

            if(display != NULL)
            {
                uint32_t display_value = (m_run_simulation || m_single_step) ? display->value : 0;
                struct dev_t *device = display->device;
                struct dev_desc_t *desc = dev_device_descs + device->type;
                struct d_device_data_t *device_data = d_device_data + buffer_offset;
                d_7seg_display_data[buffer_offset >> 2] |= display_value << ((buffer_offset % 4) << 3);
                device_data->device_quad_size[0] = desc->width >> 1;
                device_data->device_quad_size[1] = desc->height >> 1;
                // data->device_tex_size[0] = (float)desc->width / (float)dev_devices_texture_width;
                // data->device_tex_size[1] = (float)desc->height / (float)dev_devices_texture_height;
                device_data->tex_coord_set = device->tex_coords;
                device_data->flip_size = device->rotation == DEV_DEVICE_ROTATION_90 || device->rotation == DEV_DEVICE_ROTATION_270;
                device_data->position[0] = device->position[0] - device->origin[0];
                device_data->position[1] = device->position[1] - device->origin[1];
                // data->tex_coord_offset[0] = (float)desc->tex_offset[0] / (float)dev_devices_texture_width;
                // data->tex_coord_offset[1] = (float)desc->tex_offset[1] / (float)dev_devices_texture_height;
                buffer_offset++;
            }
        }        

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_7SEG_DATA_BINDING, d_7seg_display_data_buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(uint32_t), d_7seg_display_data);
        
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BINDING, d_device_data_buffer[d_device_data_buffer_index]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, buffer_offset);
        d_device_data_buffer_index = (d_device_data_buffer_index + 1) % D_DEVICE_DATA_BUFFER_COUNT;
    }

    if(dev_outputs.cursor)
    {
        glUseProgram(d_output_mask_shader);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_device_vert_t), (void *)(offsetof(struct d_device_vert_t, tex_coords)));
        glDisableVertexAttribArray(2);

        glUniformMatrix4fv(d_output_mask_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
        glBindTexture(GL_TEXTURE_2D, dev_output_mask_texture);
        glUniform1i(d_output_mask_shader_texture, 0);
        glDepthFunc(GL_EQUAL);
        // glDisable(GL_DEPTH_TEST);
        uint32_t buffer_offset = 0;
        // printf("%d\n", dev_outputs.cursor);
        for(uint32_t index = 0; index < dev_outputs.cursor; index++)
        {
            // struct dev_7seg_disp_t *display = pool_GetValidElement(&dev_7seg_disps, index);
            struct dev_output_t *output = pool_GetValidElement(&dev_outputs, index);

            if(!(buffer_offset & 0x3))
            {
                d_output_data[buffer_offset >> 2] = 0;    
            }

            if(output != NULL)
            {
                struct dev_t *device = output->device;
                struct sim_dev_data_t *data = sim_GetDevSimData(device->sim_data);
                struct sim_dev_pin_t *pin = sim_GetDevSimPin(data, 0);
                uint32_t output_value = (m_run_simulation || m_single_step) ? pin->value : WIRE_VALUE_Z;
                output_value = output_value << ((buffer_offset % 4) << 3);
                // printf("%d\n", output_value);
                struct dev_desc_t *desc = dev_device_descs + device->type;
                struct d_device_data_t *device_data = d_device_data + buffer_offset;

                d_output_data[buffer_offset >> 2] |= output_value;
                // printf("%d\n", d_output_data[buffer_offset >> 2]);
                device_data->device_quad_size[0] = desc->width >> 1;
                device_data->device_quad_size[1] = desc->height >> 1;
                // data->device_tex_size[0] = (float)desc->width / (float)dev_devices_texture_width;
                // data->device_tex_size[1] = (float)desc->height / (float)dev_devices_texture_height;
                device_data->tex_coord_set = device->tex_coords;
                device_data->flip_size = device->rotation == DEV_DEVICE_ROTATION_90 || device->rotation == DEV_DEVICE_ROTATION_270;
                device_data->position[0] = device->position[0] - device->origin[0];
                device_data->position[1] = device->position[1] - device->origin[1];
                // data->tex_coord_offset[0] = (float)desc->tex_offset[0] / (float)dev_devices_texture_width;
                // data->tex_coord_offset[1] = (float)desc->tex_offset[1] / (float)dev_devices_texture_height;
                buffer_offset++;
            }
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_WIRE_COLOR_BINDING, d_wire_value_color_buffer);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_OUTPUT_DATA_BINDING, d_output_data_buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(uint32_t), d_output_data);
        
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_DEVICE_DATA_BINDING, d_device_data_buffer[d_device_data_buffer_index]);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, buffer_offset);
        d_device_data_buffer_index = (d_device_data_buffer_index + 1) % D_DEVICE_DATA_BUFFER_COUNT;
    }
}

void d_DrawWires()
{
    glBindBuffer(GL_ARRAY_BUFFER, d_wire_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, D_WIRE_COLOR_BINDING, d_wire_value_color_buffer);
    glUseProgram(d_wire_shader);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_wire_vert_t), (void *)(offsetof(struct d_wire_vert_t, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_INT, sizeof(struct d_wire_vert_t), (void *)(offsetof(struct d_wire_vert_t, value_sel)));

    glUniformMatrix4fv(d_wire_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
    glLineWidth(2.0f);
    glPointSize(4.0f);
    glDepthFunc(GL_LESS);

    uint32_t vertex_count = (w_wire_segs.cursor - (w_wire_segs.free_indices_top + 1)) * 2;
    uint32_t update_count = vertex_count / D_WIRE_VERTEX_BUFFER_SIZE;

    if(vertex_count % D_WIRE_VERTEX_BUFFER_SIZE)
    {
        update_count++;
    }

    uint32_t wire_index = 0;
    uint32_t last_wire = w_wires.cursor;
    
    for(uint32_t update_index = 0; update_index < update_count; update_index++)
    {
        uint32_t buffer_offset = 0;

        for(; wire_index < last_wire; wire_index++)
        {
            struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

            if(wire != NULL)
            {
                if(wire->segment_count * 2 + buffer_offset > D_WIRE_VERTEX_BUFFER_SIZE)
                {
                    break;
                }


                // struct wire_seg_pos_block_t *segment_block = wire->first_segment_pos;
                struct wire_seg_t *segment = wire->first_segment;
                struct sim_wire_data_t *wire_data = list_GetValidElement(&sim_wire_data, wire->sim_data);
                uint8_t wire_value = (m_run_simulation || m_single_step) ? wire_data->value : WIRE_VALUE_Z;
                // uint32_t total_segment_count = wire->segment_pos_count;

                while(segment != NULL)
                {
                    // uint32_t segment_count = WIRE_SEGMENT_POS_BLOCK_SIZE;
                    // if(segment_count > total_segment_count)
                    // {
                    //     segment_count = total_segment_count;
                    // }
                    // total_segment_count -= segment_count;

                    // for(uint32_t segment_index = 0; segment_index < segment_count; segment_index++)
                    // {
                    // struct wire_seg_pos_t *segment = segment_block->segments + segment_index;
                    d_wire_vertices[buffer_offset].position[0] = segment->ends[WIRE_SEG_START_INDEX][0];
                    d_wire_vertices[buffer_offset].position[1] = segment->ends[WIRE_SEG_START_INDEX][1];
                    d_wire_vertices[buffer_offset].value_sel = wire_value | ((segment->selection_index != 0xffffffff) << 16);
                    buffer_offset++;

                    d_wire_vertices[buffer_offset].position[0] = segment->ends[WIRE_SEG_END_INDEX][0];
                    d_wire_vertices[buffer_offset].position[1] = segment->ends[WIRE_SEG_END_INDEX][1];
                    d_wire_vertices[buffer_offset].value_sel = wire_value | ((segment->selection_index != 0xffffffff) << 16);
                    buffer_offset++;
                    // }
                    segment = segment->wire_next;
                }
            }
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct d_wire_vert_t) * buffer_offset, d_wire_vertices);
        glDrawArrays(GL_LINES, 0, buffer_offset);
    }

    vertex_count = w_wire_juncs.cursor;
    update_count = vertex_count / D_WIRE_VERTEX_BUFFER_SIZE;

    if(vertex_count % D_WIRE_VERTEX_BUFFER_SIZE)
    {
        update_count++;
    }

    wire_index = 0;
    last_wire = w_wires.cursor;
    
    for(uint32_t update_index = 0; update_index < update_count; update_index++)
    {
        uint32_t buffer_offset = 0;

        for(; wire_index < last_wire; wire_index++)
        {
            struct wire_t *wire = pool_GetValidElement(&w_wires, wire_index);

            if(wire != NULL)
            {
                if(wire->junction_count + buffer_offset > D_WIRE_VERTEX_BUFFER_SIZE)
                {
                    break;
                }

                // struct wire_junc_pos_block_t *junction_block = wire->first_junction_pos;
                struct sim_wire_data_t *wire_data = list_GetValidElement(&sim_wire_data, wire->sim_data);
                uint8_t wire_value = (m_run_simulation || m_single_step) ? wire_data->value : WIRE_VALUE_Z;
                uint32_t total_junction_count = wire->junction_count;
                struct wire_junc_t *junction = wire->first_junction;


                while(junction != NULL)
                {
                    // uint32_t junction_count = WIRE_JUNCTION_POS_BLOCK_SIZE;
                    // if(junction_count > total_junction_count)
                    // {
                    //     junction_count = total_junction_count;
                    // }
                    // total_junction_count -= junction_count;

                    // for(uint32_t junction_index = 0; junction_index < junction_count; junction_index++)
                    // {
                    //     struct wire_junc_pos_t *junction = junction_block->junctions + junction_index;
                    d_wire_vertices[buffer_offset].position[0] = junction->pos[0];
                    d_wire_vertices[buffer_offset].position[1] = junction->pos[1];
                    d_wire_vertices[buffer_offset].value_sel = wire_value;
                    buffer_offset++;
                    // }
                    // junction_block = junction_block->next;
                    junction = junction->wire_next;
                }
            }
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct d_wire_vert_t) * buffer_offset, d_wire_vertices);
        glDrawArrays(GL_POINTS, 0, buffer_offset);
    }

    if(m_wire_seg_pos.cursor != 0)
    {
        uint32_t buffer_offset = 0;
        for(uint32_t index = 0; index < m_wire_seg_pos.cursor; index++)
        {
            union m_wire_seg_t *segment = list_GetElement(&m_wire_seg_pos, index);
            d_wire_vertices[buffer_offset].position[0] = segment->seg_pos.ends[WIRE_SEG_START_INDEX][0];
            d_wire_vertices[buffer_offset].position[1] = segment->seg_pos.ends[WIRE_SEG_START_INDEX][1];
            d_wire_vertices[buffer_offset].value_sel = WIRE_VALUE_Z;
            buffer_offset++;

            d_wire_vertices[buffer_offset].position[0] = segment->seg_pos.ends[WIRE_SEG_END_INDEX][0];
            d_wire_vertices[buffer_offset].position[1] = segment->seg_pos.ends[WIRE_SEG_END_INDEX][1];
            d_wire_vertices[buffer_offset].value_sel = WIRE_VALUE_Z;
            buffer_offset++;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct d_wire_vert_t) * buffer_offset, d_wire_vertices);
        glDrawArrays(GL_LINES, 0, buffer_offset);
    }    
}