#include "draw.h"
#include "GL/glew.h"
#include "pool.h"
#include "dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define D_VERTEX_BUFFER_SIZE    4
#define D_INDEX_BUFFER_SIZE     6
#define D_PRIMITIVE_STORAGE_BUFFER_SIZE 0xffff

const char *d_draw_primitive_vertex_shader =
    "#version 400 core\n"
    "#extension GL_ARB_shader_storage_buffer_object : require\n"
    "layout (location = 0) in vec4 d_position;\n"
    "layout (location = 1) in vec2 d_tex_coords;\n"
    "struct d_primitive_data_t\n"
    "{\n"
    "   int pos_x;\n"
    "   int pos_y;\n"
    "   int size;\n"
    "   int coord_offset;\n"
    "};\n"
    "layout (std430) buffer d_primitive_data_buffer\n"
    "{\n"
    "   d_primitive_data_t d_primitive_data[];\n"
    "};\n"
    "out vec2 tex_coords;\n"
    "flat out ivec2 size;\n"
    "flat out ivec2 coord_offset;\n"
    "uniform mat4 d_model_view_projection_matrix;\n"
    "void main()\n"
    "{\n"
    "   d_primitive_data_t data = d_primitive_data[gl_InstanceID];"
    "   size = ivec2(data.size & 0xffff, (data.size >> 16) & 0xffff);\n"
    "   tex_coords = d_tex_coords;\n"
    "   coord_offset = ivec2(data.coord_offset & 0xffff, (data.coord_offset >> 16) & 0xffff);\n"
    "   vec4 position = vec4(d_position.x * float(size.x) + float(data.pos_x), d_position.y * float(size.y) + float(data.pos_y), 0, 1);\n"
    "   gl_Position = d_model_view_projection_matrix * position;\n"
    "}\n";

const char *d_draw_primitive_fragment_shader = 
    "#version 400 core\n"
    "in vec2 tex_coords;\n"
    "flat in ivec2 size;\n"
    "flat in ivec2 coord_offset;\n"
    "uniform sampler2D d_texture;\n"
    "void main()\n"
    "{\n"
    "   ivec2 coords = ivec2(float(coord_offset.x) + float(size.x) * tex_coords.x, float(coord_offset.y) + float(size.y) * tex_coords.y);\n"
    "   vec4 color = texelFetch(d_texture, coords, 0);\n"
    "   gl_FragColor = vec4(vec3(1.0f - color.r) * vec3(0, color.g, 0), color.a);\n"
    "}\n";

GLuint                      d_draw_primitive_shader;
GLuint                      d_vao;
GLuint                      d_vertex_buffer;
GLuint                      d_index_buffer;
GLuint                      d_primitive_storage_buffer;
uint32_t                    d_draw_primitive_shader_model_view_projection_matrix;
uint32_t                    d_draw_primitive_shader_texture;
struct d_primitive_data_t * d_primitive_data;

float                       d_model_view_projection_matrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

extern struct pool_t    dev_devices;
extern GLuint           dev_devices_texture;
extern uint32_t         dev_device_texture_offsets[][2];
extern uint32_t         m_window_width;
extern uint32_t         m_window_height;


void d_Init()
{
    glGenVertexArrays(1, &d_vao);
    glBindVertexArray(d_vao);

    glGenBuffers(1, &d_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, d_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, D_VERTEX_BUFFER_SIZE * sizeof(struct d_vert_t), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &d_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, D_INDEX_BUFFER_SIZE * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &d_primitive_storage_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_primitive_storage_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, D_PRIMITIVE_STORAGE_BUFFER_SIZE * sizeof(struct d_primitive_data_t), NULL, GL_DYNAMIC_DRAW);

    struct d_vert_t vertices[] = {
        {.position = {-1.0, 1.0}, .tex_coords = {0.0, 1.0}},
        {.position = {-1.0,-1.0}, .tex_coords = {0.0, 0.0}},
        {.position = { 1.0,-1.0}, .tex_coords = {1.0, 0.0}},
        {.position = { 1.0, 1.0}, .tex_coords = {1.0, 1.0}},
    };

    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0
    };

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &d_draw_primitive_vertex_shader, NULL);
    glCompileShader(vertex_shader);

    d_primitive_data = calloc(D_PRIMITIVE_STORAGE_BUFFER_SIZE, sizeof(struct d_primitive_data_t));

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
    glShaderSource(fragment_shader, 1, &d_draw_primitive_fragment_shader, NULL);
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

    d_draw_primitive_shader = glCreateProgram();
    glAttachShader(d_draw_primitive_shader, vertex_shader);
    glAttachShader(d_draw_primitive_shader, fragment_shader);
    glLinkProgram(d_draw_primitive_shader);
    glGetProgramiv(d_draw_primitive_shader, GL_LINK_STATUS, &status);
    if(!status)
    {
        GLint log_length;
        glGetProgramiv(d_draw_primitive_shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = calloc(log_length, 1);
        glGetProgramInfoLog(d_draw_primitive_shader, log_length, NULL, info_log);
        printf("shader link error:\n%s\n", info_log);
        free(info_log);
        exit(-1);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    d_draw_primitive_shader_model_view_projection_matrix = glGetUniformLocation(d_draw_primitive_shader, "d_model_view_projection_matrix");
    d_draw_primitive_shader_texture = glGetUniformLocation(d_draw_primitive_shader, "d_texture");
    GLuint primitive_data_block = glGetUniformBlockIndex(d_draw_primitive_shader, "d_primitive_data_buffer");
    glShaderStorageBlockBinding(d_draw_primitive_shader, primitive_data_block, 0);

    // glUseProgram(d_draw_quad_shader);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, position)));
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, tex_coords)));
}

void d_Shutdown()
{

}

void d_DrawDevices()
{
    d_model_view_projection_matrix[0] = 2.0 / (float)m_window_width;
    d_model_view_projection_matrix[5] = 2.0 / (float)m_window_height;

    for(uint32_t index = 0; index < dev_devices.cursor; index++)
    {
        struct dev_t *device = pool_GetElement(&dev_devices, index);
        struct d_primitive_data_t *data = d_primitive_data + index;
        data->pos_x = device->position[0];
        data->pos_y = device->position[1];
        data->size = (42 << 16) | 24;
        data->coord_offset = (dev_device_texture_offsets[device->type][1] << 16) | dev_device_texture_offsets[device->type][0];
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, d_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_index_buffer);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, d_primitive_storage_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, d_primitive_storage_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, D_PRIMITIVE_STORAGE_BUFFER_SIZE * sizeof(struct d_primitive_data_t), d_primitive_data);

    glUseProgram(d_draw_primitive_shader);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct d_vert_t), (void *)(offsetof(struct d_vert_t, tex_coords)));
    glDisableVertexAttribArray(2);

    glUniformMatrix4fv(d_draw_primitive_shader_model_view_projection_matrix, 1, GL_FALSE, d_model_view_projection_matrix);
    glBindTexture(GL_TEXTURE_2D, dev_devices_texture);
    glUniform1i(d_draw_primitive_shader_texture, 0);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0, dev_devices.cursor);

    // for(uint32_t index = 0; index < dev_primitives.cursor; index++)
    // {
    //     struct dev_primitive_t *device = pool_GetElement(&dev_primitives, index);
    //     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);
    // }
}
