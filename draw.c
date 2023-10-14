#include "draw.h"
#include "GL/glew.h"
#include "pool.h"
#include "list.h"
#include "dev.h"
#include "wire.h"
#include "sim.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const char *d_draw_device_vertex_shader =
    "#version 400 core\n"
    "#extension GL_ARB_shader_storage_buffer_object : require\n"
    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in vec2 d_tex_coords;\n"

    "#define DEV_DEVICE_ROTATION_0      0\n"
    "#define DEV_DEVICE_ROTATION_90     1\n"
    "#define DEV_DEVICE_ROTATION_180    2\n"
    "#define DEV_DEVICE_ROTATION_270    3\n"
    
    "#define DEV_DEVICE_FLIP_X    (1 << 1)\n"
    "#define DEV_DEVICE_FLIP_Y    (1 << 0)\n"

    "struct d_device_data_t\n"
    "{\n"
        "int pos_x;\n"
        "int pos_y;\n"
        "int origin_x;\n"
        "int origin_y;\n"
        "int size;\n"
        "int coord_offset;\n"
        "int rot_flip_sel;\n"
    "};\n"

    "layout (std430) buffer d_device_data_buffer\n"
    "{\n"
        "d_device_data_t d_device_data[];\n"
    "};\n"

    "out vec2 tex_coords;\n"
    "flat out ivec2 size;\n"
    "flat out ivec2 coord_offset;\n"
    "flat out int selected;\n"
    "uniform mat4 d_model_view_projection_matrix;\n"

    "void main()\n"
    "{\n"
        "d_device_data_t data = d_device_data[gl_InstanceID];"
        "size = ivec2(data.size & 0xffff, (data.size >> 16) & 0xffff);\n"
        "tex_coords = d_tex_coords;\n"
        "coord_offset = ivec2(data.coord_offset & 0xffff, (data.coord_offset >> 16) & 0xffff);\n"
        "vec4 position = vec4(d_position.x * float(size.x), d_position.y * float(size.y), 0.1f, 1);\n"
        "int rotation = data.rot_flip_sel & 0xffff;\n"
        "int flip_sel = (data.rot_flip_sel >> 16) & 0xffff;\n"

        "position.x -= float(data.origin_x);\n"
        "position.y -= float(data.origin_y);\n"

        "bool flip_x = bool(flip_sel & DEV_DEVICE_FLIP_X);\n"
        "bool flip_y = bool(flip_sel & DEV_DEVICE_FLIP_Y);\n"

        "if(flip_x)\n"
        "{\n"
            "position.x = -position.x;\n"
        "}\n"

        "if(flip_y)\n"
        "{\n"
            "position.y = -position.y;\n"
        "}\n"

        "switch(rotation)\n"
        "{\n"
            "case DEV_DEVICE_ROTATION_90:\n"
            "{\n"
                "float temp = position.y;\n"
                "position.y = position.x;\n"
                "position.x = -temp;\n"
            "}\n"
            "break;\n"

            "case DEV_DEVICE_ROTATION_180:\n"
            "{\n"
                "position.y = -position.y;\n"
                "position.x = -position.x;\n"
            "}\n"
            "break;\n"
    
            "case DEV_DEVICE_ROTATION_270:\n"
            "{\n"
                "float temp = position.y;\n"
                "position.y = -position.x;\n"
                "position.x = temp;\n"
            "}\n"
            "break;\n"
        "}\n"

        "selected = flip_sel & 4;\n"

        "position.x += float(data.pos_x);\n"
        "position.y += float(data.pos_y);\n"
        "gl_Position = d_model_view_projection_matrix * position;\n"
    "}\n";

const char *d_draw_device_fragment_shader = 
    "#version 400 core\n"
    "in vec2 tex_coords;\n"
    "flat in ivec2 size;\n"
    "flat in ivec2 coord_offset;\n"
    "flat in int selected;\n"
    "uniform sampler2D d_texture;\n"
    "void main()\n"
    "{\n"
        "ivec2 coords = ivec2(float(coord_offset.x) + float(size.x * 2.0f) * tex_coords.x, float(coord_offset.y) + float(size.y * 2.0f) * tex_coords.y);\n"
        "vec4 color = texelFetch(d_texture, coords, 0);\n"
        "if(selected != 0)\n"
        "{\n"
            "color = vec4(0.0f, 0.5f, 0.8f, color.a);\n"
        "}\n"
        "gl_FragColor = color;\n"
    "}\n";


const char *d_draw_wire_vertex_shader = 
    "#version 400 core\n"
    "#extension GL_ARB_shader_storage_buffer_object : require\n"
    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in int d_wire_value;\n"

    "uniform mat4 d_model_view_projection_matrix;\n"
    "flat out int wire_value;\n"

    "void main()\n"
    "{\n"
        "gl_Position = d_model_view_projection_matrix * vec4(d_position.xy, 0.1f, 1.0f);\n"
        "wire_value = d_wire_value;\n"
    "}\n";

const char *d_draw_wire_fragment_shader = 
    "#version 400 core\n"
    "#extension GL_ARB_shader_storage_buffer_object : require\n"
    "layout (std430) buffer d_wire_colors\n"
    "{\n"
        "vec4 wire_colors[];\n"
    "};\n"
    "flat in int wire_value;\n"
    "void main()\n"
    "{\n"
        "gl_FragColor = wire_colors[wire_value];\n"
    "}\n";
 
#define D_DEVICE_VERTEX_BUFFER_SIZE     4
#define D_DEVICE_INDEX_BUFFER_SIZE      6
#define D_DEVICE_STORAGE_BUFFER_SIZE    0xfffff
#define D_WIRE_VERTEX_BUFFER_SIZE       0xfffff

GLuint                      d_vao;

GLuint                      d_wire_value_color_buffer;

GLuint                      d_draw_device_shader;
uint32_t                    d_draw_device_shader_model_view_projection_matrix;
uint32_t                    d_draw_device_shader_texture;
GLuint                      d_device_vertex_buffer;
GLuint                      d_device_index_buffer;
GLuint                      d_device_storage_buffer;
struct d_device_data_t *    d_device_data;

GLuint                      d_draw_wire_shader;
uint32_t                    d_draw_wire_shader_model_view_projection_matrix;
GLuint                      d_wire_vertex_buffer;
struct d_wire_vert_t *      d_wire_vertices;
// GLuint                      d_wire_id_buffer;
// uint32_t *                  d_wire_ids;

float                       d_model_view_projection_matrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

extern struct pool_t        dev_devices;
extern GLuint               dev_devices_texture;
extern uint32_t             dev_device_texture_offsets[][2];
extern struct dev_desc_t    dev_device_descs[];

extern struct pool_t        w_wires;
extern struct list_t        w_wire_seg_pos;
extern struct list_t        w_wire_junc_pos;

extern uint32_t             m_window_width;
extern uint32_t             m_window_height;
extern float                m_zoom;
extern float                m_offset_x;
extern float                m_offset_y;
extern uint32_t             m_run_simulation;
extern uint32_t             m_selected_device_type;
extern struct dev_t *       m_selected_device;
extern int32_t              m_mouse_x;
extern int32_t              m_mouse_y;
extern int32_t              m_place_device_x;
extern int32_t              m_place_device_y;
extern struct list_t        m_wire_seg_pos;
// extern struct list_t        m_wire_segments;

extern struct list_t        sim_wire_data;


void d_Init()
{
    glGenVertexArrays(1, &d_vao);
    glBindVertexArray(d_vao);

    glGenBuffers(1, &d_device_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_device_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, D_DEVICE_VERTEX_BUFFER_SIZE * sizeof(struct d_vert_t), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &d_device_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_device_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, D_DEVICE_INDEX_BUFFER_SIZE * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &d_device_storage_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_device_storage_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, D_DEVICE_STORAGE_BUFFER_SIZE * sizeof(struct d_device_data_t), NULL, GL_DYNAMIC_DRAW);
    d_device_data = calloc(D_DEVICE_STORAGE_BUFFER_SIZE, sizeof(struct d_device_data_t));

    struct d_vert_t vertices[] = {
        {.position = {-1.0, 1.0}, .tex_coords = {0.0, 0.0}},
        {.position = {-1.0,-1.0}, .tex_coords = {0.0, 1.0}},
        {.position = { 1.0,-1.0}, .tex_coords = {1.0, 1.0}},
        {.position = { 1.0, 1.0}, .tex_coords = {1.0, 0.0}},
    };

    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glGenBuffers(1, &d_wire_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_wire_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, D_WIRE_VERTEX_BUFFER_SIZE * sizeof(struct d_wire_vert_t), NULL, GL_STATIC_DRAW);
    d_wire_vertices = calloc(D_WIRE_VERTEX_BUFFER_SIZE, sizeof(struct d_wire_vert_t));

    // glGenBuffers(1, &d_wire_id_buffer);
    // glBindBuffer(GL_ARRAY_BUFFER, d_wire_id_buffer);
    // glBufferData(GL_ARRAY_BUFFER, D_WIRE_VERTEX_BUFFER_SIZE * sizeof(uint32_t), NULL, GL_STATIC_DRAW);
    // d_wire_ids = calloc(D_WIRE_VERTEX_BUFFER_SIZE, sizeof(uint32_t));

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &d_draw_device_vertex_shader, NULL);
    glCompileShader(vertex_shader);

    GLint status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(vertex_shader, log_length, NULL, info_log);
        printf("vertex shader error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &d_draw_device_fragment_shader, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(fragment_shader, log_length, NULL, info_log);
        printf("fragment shader error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    d_draw_device_shader = glCreateProgram();
    glAttachShader(d_draw_device_shader, vertex_shader);
    glAttachShader(d_draw_device_shader, fragment_shader);
    glLinkProgram(d_draw_device_shader);
    glGetProgramiv(d_draw_device_shader, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetProgramiv(d_draw_device_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetProgramInfoLog(d_draw_device_shader, log_length, NULL, info_log);
        printf("shader link error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    d_draw_device_shader_model_view_projection_matrix = glGetUniformLocation(d_draw_device_shader, "d_model_view_projection_matrix");
    d_draw_device_shader_texture = glGetUniformLocation(d_draw_device_shader, "d_texture");
    GLuint device_data_block = glGetProgramResourceIndex(d_draw_device_shader, GL_SHADER_STORAGE_BUFFER, "d_device_data_buffer");
    glShaderStorageBlockBinding(d_draw_device_shader, device_data_block, 0);



    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &d_draw_wire_vertex_shader, NULL);
    glCompileShader(vertex_shader);

    status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(vertex_shader, log_length, NULL, info_log);
        printf("vertex shader error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &d_draw_wire_fragment_shader, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetShaderInfoLog(fragment_shader, log_length, NULL, info_log);
        printf("fragment shader error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    d_draw_wire_shader = glCreateProgram();
    glAttachShader(d_draw_wire_shader, vertex_shader);
    glAttachShader(d_draw_wire_shader, fragment_shader);
    glLinkProgram(d_draw_wire_shader);
    glGetProgramiv(d_draw_wire_shader, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetProgramiv(d_draw_wire_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetProgramInfoLog(d_draw_wire_shader, log_length, NULL, info_log);
        printf("shader link error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    d_draw_wire_shader_model_view_projection_matrix = glGetUniformLocation(d_draw_wire_shader, "d_model_view_projection_matrix");
    uint32_t wire_colors_block = glGetProgramResourceIndex(d_draw_wire_shader, GL_SHADER_STORAGE_BLOCK, "d_wire_colors");
    glShaderStorageBlockBinding(d_draw_wire_shader, wire_colors_block, 1);

    float wire_colors[][4] = {
        [WIRE_VALUE_0S]     = {0, 0.5, 0, 1},
        [WIRE_VALUE_1S]     = {0, 1.0, 0, 1},
        [WIRE_VALUE_0W]     = {0.2, 0.1, 0, 1},
        [WIRE_VALUE_1W]     = {0.2, 0.2, 0, 1},
        [WIRE_VALUE_Z]      = {0.3, 0.3, 0.3, 1},
        [WIRE_VALUE_U]      = {0.3, 0.0, 0.5, 1},
        [WIRE_VALUE_ERR]    = {1, 0, 0, 1},
    };


    glGenBuffers(1, &d_wire_value_color_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_wire_value_color_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(wire_colors), wire_colors, GL_STATIC_DRAW);
}

void d_Shutdown()
{

}

void d_DrawDevices()
{
    d_model_view_projection_matrix[0] = 2.0 / ((float)m_window_width / m_zoom);
    d_model_view_projection_matrix[5] = 2.0 / ((float)m_window_height / m_zoom);
    d_model_view_projection_matrix[12] = -m_offset_x * d_model_view_projection_matrix[0];
    d_model_view_projection_matrix[13] = -m_offset_y * d_model_view_projection_matrix[1];

    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, d_device_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_device_index_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, d_device_storage_buffer);
    glUseProgram(d_draw_device_shader);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, tex_coords)));
    glDisableVertexAttribArray(2);

    glUniformMatrix4fv(d_draw_device_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
    glBindTexture(GL_TEXTURE_2D, dev_devices_texture);
    glUniform1i(d_draw_device_shader_texture, 0);

    uint32_t device_count = dev_devices.cursor - (dev_devices.free_indices_top + 1);
    uint32_t update_count = device_count / D_DEVICE_STORAGE_BUFFER_SIZE;

    if(device_count % D_DEVICE_STORAGE_BUFFER_SIZE)
    {
        update_count++;
    }

    uint32_t first_device = 0;
    uint32_t last_device = D_DEVICE_STORAGE_BUFFER_SIZE;

    for(uint32_t update_index = 0; update_index < update_count; update_index++)
    {
        uint32_t buffer_offset = 0;

        for(uint32_t device_index = first_device; device_index < last_device; device_index++)
        {
            struct dev_t *device = pool_GetValidElement(&dev_devices, device_index);

            if(device != NULL)
            {
                struct d_device_data_t *data = d_device_data + buffer_offset;
                struct dev_desc_t *desc = dev_device_descs + device->type;
                int32_t width = desc->width >> 1;
                int32_t height = desc->height >> 1;
                data->pos_x = device->position[0];
                data->pos_y = device->position[1];
                data->origin_x = desc->origin[0];
                data->origin_y = desc->origin[1];
                data->size = (height << 16) | width;
                data->rot_flip_sel = (device->flip << 16) | device->rotation | ((device->selection_index != INVALID_LIST_INDEX) << 18);
                data->coord_offset = (desc->offset[1] << 16) | desc->offset[0];
                buffer_offset++;
            }
        }

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffer_offset * sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, buffer_offset);

        first_device += D_DEVICE_STORAGE_BUFFER_SIZE;
        last_device += D_DEVICE_STORAGE_BUFFER_SIZE;

        if(last_device > dev_devices.cursor)
        {
            last_device = dev_devices.cursor;
        }
    }

    if(m_selected_device_type != DEV_DEVICE_TYPE_LAST)
    {
        struct dev_desc_t *desc = dev_device_descs + m_selected_device_type;
        int32_t width = desc->width >> 1;
        int32_t height = desc->height >> 1;
        d_device_data->pos_x = m_place_device_x;
        d_device_data->pos_y = m_place_device_y;
        d_device_data->origin_x = desc->origin[0];
        d_device_data->origin_y = desc->origin[1];
        d_device_data->size = (height << 16) | width;
        d_device_data->rot_flip_sel = 0;
        d_device_data->coord_offset = (desc->offset[1] << 16) | desc->offset[0];

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(struct d_device_data_t), d_device_data);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, d_wire_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, d_wire_value_color_buffer);
    glUseProgram(d_draw_wire_shader);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_wire_vert_t), (void *)(offsetof(struct d_wire_vert_t, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_INT, sizeof(struct d_wire_vert_t), (void *)(offsetof(struct d_wire_vert_t, value)));

    glUniformMatrix4fv(d_draw_wire_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
    glLineWidth(2.0f);
    glPointSize(4.0f);

    uint32_t vertex_count = w_wire_seg_pos.cursor * 2;
    update_count = vertex_count / D_WIRE_VERTEX_BUFFER_SIZE;

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
                if(wire->segment_pos_count * 2 + buffer_offset > D_WIRE_VERTEX_BUFFER_SIZE)
                {
                    break;
                }

                struct wire_seg_pos_block_t *segment_block = wire->first_segment_pos;
                struct sim_wire_data_t *wire_data = list_GetValidElement(&sim_wire_data, wire->sim_data);
                uint8_t wire_value = m_run_simulation ? wire_data->value : WIRE_VALUE_Z;
                uint32_t total_segment_count = wire->segment_pos_count;

                while(segment_block != NULL)
                {
                    uint32_t segment_count = WIRE_SEGMENT_POS_BLOCK_SIZE;
                    if(segment_count > total_segment_count)
                    {
                        segment_count = total_segment_count;
                    }
                    total_segment_count -= segment_count;

                    for(uint32_t segment_index = 0; segment_index < segment_count; segment_index++)
                    {
                        struct wire_seg_pos_t *segment = segment_block->segments + segment_index;
                        d_wire_vertices[buffer_offset].position[0] = segment->start[0];
                        d_wire_vertices[buffer_offset].position[1] = segment->start[1];
                        d_wire_vertices[buffer_offset].value = wire_value;
                        buffer_offset++;

                        d_wire_vertices[buffer_offset].position[0] = segment->end[0];
                        d_wire_vertices[buffer_offset].position[1] = segment->end[1];
                        d_wire_vertices[buffer_offset].value = wire_value;
                        buffer_offset++;
                    }
                    segment_block = segment_block->next;
                }
            }
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct d_wire_vert_t) * buffer_offset, d_wire_vertices);
        glDrawArrays(GL_LINES, 0, buffer_offset);
    }

    vertex_count = w_wire_junc_pos.cursor;
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
                if(wire->junction_pos_count + buffer_offset > D_WIRE_VERTEX_BUFFER_SIZE)
                {
                    break;
                }

                struct wire_junc_pos_block_t *junction_block = wire->first_junction_pos;
                struct sim_wire_data_t *wire_data = list_GetValidElement(&sim_wire_data, wire->sim_data);
                uint8_t wire_value = m_run_simulation ? wire_data->value : WIRE_VALUE_Z;
                uint32_t total_junction_count = wire->junction_pos_count;

                while(junction_block != NULL)
                {
                    uint32_t junction_count = WIRE_JUNCTION_POS_BLOCK_SIZE;
                    if(junction_count > total_junction_count)
                    {
                        junction_count = total_junction_count;
                    }
                    total_junction_count -= junction_count;

                    for(uint32_t junction_index = 0; junction_index < junction_count; junction_index++)
                    {
                        struct wire_junc_pos_t *junction = junction_block->junctions + junction_index;
                        d_wire_vertices[buffer_offset].position[0] = junction->pos[0];
                        d_wire_vertices[buffer_offset].position[1] = junction->pos[1];
                        d_wire_vertices[buffer_offset].value = wire_value;
                        buffer_offset++;
                    }
                    junction_block = junction_block->next;
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
            d_wire_vertices[buffer_offset].position[0] = segment->seg_pos.start[0];
            d_wire_vertices[buffer_offset].position[1] = segment->seg_pos.start[1];
            d_wire_vertices[buffer_offset].value = WIRE_VALUE_Z;
            buffer_offset++;

            d_wire_vertices[buffer_offset].position[0] = segment->seg_pos.end[0];
            d_wire_vertices[buffer_offset].position[1] = segment->seg_pos.end[1];
            d_wire_vertices[buffer_offset].value = WIRE_VALUE_Z;
            buffer_offset++;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct d_wire_vert_t) * buffer_offset, d_wire_vertices);
        glDrawArrays(GL_LINES, 0, buffer_offset);
    }    
}
