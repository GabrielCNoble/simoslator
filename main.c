#include <stdio.h>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb_image.h"
#include <stdint.h>

#include "draw.h"
#include "ui.h"
#include "in.h"
#include "dev.h"
#include "sim.h"
#include "list.h"

SDL_Window *                    m_window;
SDL_GLContext *                 m_context;
uint32_t                        m_window_width = 800;
uint32_t                        m_window_height = 600;
float                           m_zoom = 0.4f;
float                           m_offset_x = 0.0f;
float                           m_offset_y = 0.0f;
GLuint                          m_cursor_texture;
GLuint                          m_play_texture;
GLuint                          m_pause_texture;
GLuint                          m_wire_texture;
GLuint                          m_move_texture;
GLuint                          m_rotate_texture;
GLuint                          m_fliph_texture;
GLuint                          m_flipv_texture;
int32_t                         m_mouse_x;
int32_t                         m_mouse_y;
int32_t                         m_place_device_x;
int32_t                         m_place_device_y;
int32_t                         m_snapped_mouse_x;
int32_t                         m_snapped_mouse_y;
uint32_t                        m_run_simulation;
uint32_t                        m_cur_edit_func;
uint32_t                        m_wire_func_stage;
uint32_t                        m_selected_device_type;
struct dev_t *                  m_selected_device;
struct list_t                   m_selections;
struct list_t                   m_wire_segments;
struct wire_segment_pos_t *     m_cur_wire_segment;

extern float                    in_mouse_x;
extern float                    in_mouse_y;
extern struct pool_t            dev_devices;
extern struct list_t            dev_pin_blocks;
extern GLuint                   dev_devices_texture;
extern uint32_t                 dev_devices_texture_width;
extern uint32_t                 dev_devices_texture_height;
extern struct dev_desc_t        dev_device_descs[];

extern struct list_t            w_wire_segment_positions;
extern float                    d_model_view_projection_matrix[];

#define M_SNAP_VALUE 20

enum M_EDIT_FUNCS
{
    M_EDIT_FUNC_PLACE,
    M_EDIT_FUNC_SELECT,
    M_EDIT_FUNC_MOVE,
    M_EDIT_FUNC_WIRE
};

struct m_selected_pin_t
{
    struct dev_t *  device;
    uint16_t        pin;
};

enum M_OBJECT_TYPES
{
    M_OBJECT_TYPE_DEVICE,
    M_OBJECT_TYPE_WIRE,
};

struct m_object_t
{
    void *      object;
    uint32_t    type;
};

struct m_selected_pin_t m_GetPinUnderMouse()
{
    struct m_selected_pin_t selected_pin = {};

    for(uint32_t device_index = 0; device_index < dev_devices.cursor; device_index++)
    {
        struct dev_t *device = pool_GetValidElement(&dev_devices, device_index);
        if(device != NULL)
        {
            struct dev_desc_t *desc = dev_device_descs + device->type;
            for(uint32_t pin_index = 0; pin_index < desc->pin_count; pin_index++)
            {
                struct dev_pin_desc_t *pin_desc = desc->pins + pin_index;
                int32_t pin_position[2];

                dev_GetDevicePinLocalPosition(device, pin_index, pin_position);

                pin_position[0] += device->position[0];
                pin_position[1] += device->position[1];

                if(m_mouse_x >= pin_position[0] - DEV_DEVICE_PIN_PIXEL_WIDTH && 
                    m_mouse_x <= pin_position[0] + DEV_DEVICE_PIN_PIXEL_WIDTH)
                {
                    if(m_mouse_y >= pin_position[1] - DEV_DEVICE_PIN_PIXEL_WIDTH && 
                        m_mouse_y <= pin_position[1] + DEV_DEVICE_PIN_PIXEL_WIDTH)
                    {
                        selected_pin.device = device;
                        selected_pin.pin = pin_index;
                        device_index = dev_devices.cursor;
                        break;
                    }    
                }
            }
            
        }
    }

    return selected_pin;
}

struct dev_t *m_GetDeviceUnderMouse()
{
    for(uint32_t device_index = 0; device_index < dev_devices.cursor; device_index++)
    {
        struct dev_t *device = pool_GetValidElement(&dev_devices, device_index);
        if(device != NULL)
        {
            struct dev_desc_t *desc = dev_device_descs + device->type;
            if(m_mouse_x >= device->position[0] - desc->width && m_mouse_x <= device->position[0] + desc->width)
            {
                if(m_mouse_y >= device->position[1] - desc->height && m_mouse_y <= device->position[1] + desc->height)
                {
                    return device;
                }
            }
        }        
    }

    return NULL;
}

struct dev_input_t *m_GetInputUnderMouse()
{
    struct dev_input_t *input = NULL;
    struct dev_t *device = m_GetDeviceUnderMouse();
    if(device != NULL && device->type == DEV_DEVICE_TYPE_INPUT)
    {
        input = device->data;
    }

    return input;
}

void m_ClearSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);
        device->selection_index = INVALID_LIST_INDEX;
    }

    m_selections.cursor = 0;
}

void m_DeleteSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);
        dev_DestroyDeviced(device);
    }

    m_selections.cursor = 0;
}

void m_SelectDevice(struct dev_t *device, uint32_t multiple)
{
    if(device->selection_index != INVALID_LIST_INDEX)
    {
        uint64_t index = device->selection_index;
        device->selection_index = INVALID_LIST_INDEX;   

        if(multiple)
        {
            list_RemoveElement(&m_selections, index);
            if(device->selection_index < m_selections.cursor)
            {
                struct dev_t *moved_device = *(struct dev_t **)list_GetElement(&m_selections, index);
                moved_device->selection_index = index;
            }

            return;
        }
    }

    if(!multiple)
    {
        m_ClearSelections();
    }

    device->selection_index = list_AddElement(&m_selections, &device);
}

void m_SetEditFunc(uint32_t func)
{
    m_cur_edit_func = func;
    switch(m_cur_edit_func)
    {
        case M_EDIT_FUNC_SELECT:
        case M_EDIT_FUNC_MOVE:
            m_selected_device_type = DEV_DEVICE_TYPE_LAST;
            m_wire_func_stage = 0;
            m_wire_segments.cursor = 0;
            m_cur_wire_segment = NULL;
            ui_SetCursor(SDL_SYSTEM_CURSOR_ARROW);
        break;

        case M_EDIT_FUNC_PLACE:
            m_wire_func_stage = 0;
            m_wire_segments.cursor = 0;
            m_cur_wire_segment = NULL;
            m_ClearSelections();
            ui_SetCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;

        case M_EDIT_FUNC_WIRE:
            m_selected_device_type = DEV_DEVICE_TYPE_LAST;
            m_ClearSelections();
            ui_SetCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;
    }
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("oh, shit...\n");
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow("simoslator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
    m_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1);

    GLenum status = glewInit();
    if(status != GLEW_OK)
    {
        printf("oh, crap...\n%s\n", glewGetErrorString(status));
        return -2;
    }


    int channels;
    int width;
    int height;
    stbi_uc *pixels = stbi_load("res/cursor.png", &width, &height, &channels, STBI_rgb_alpha);

    glGenTextures(1, &m_cursor_texture);
    glBindTexture(GL_TEXTURE_2D, m_cursor_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/play.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_play_texture);
    glBindTexture(GL_TEXTURE_2D, m_play_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/pause.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_pause_texture);
    glBindTexture(GL_TEXTURE_2D, m_pause_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/connected.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_wire_texture);
    glBindTexture(GL_TEXTURE_2D, m_wire_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/rotate.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_rotate_texture);
    glBindTexture(GL_TEXTURE_2D, m_rotate_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/fliph.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_fliph_texture);
    glBindTexture(GL_TEXTURE_2D, m_fliph_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/flipv.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_flipv_texture);
    glBindTexture(GL_TEXTURE_2D, m_flipv_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);

    pixels = stbi_load("res/move.png", &width, &height, &channels, STBI_rgb_alpha);
    glGenTextures(1, &m_move_texture);
    glBindTexture(GL_TEXTURE_2D, m_move_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);


    // struct list_t list = list_Create(sizeof(uint32_t), 8);
    // list_AddElement(&list, &(uint32_t){0});
    // list_AddElement(&list, &(uint32_t){1});
    // list_AddElement(&list, &(uint32_t){2});
    // list_AddElement(&list, &(uint32_t){3});
    // list_AddElement(&list, &(uint32_t){4});
    // list_AddElement(&list, &(uint32_t){5});
    // list_AddElement(&list, &(uint32_t){6});
    // list_AddElement(&list, &(uint32_t){7});

    // list_AddElement(&list, &(uint32_t){8});
    // list_AddElement(&list, &(uint32_t){9});
    // list_AddElement(&list, &(uint32_t){10});

    // list_ShiftAndInsertAt(&list, 0, 8);

    // for(uint32_t index = 0; index < list.cursor; index++)
    // {
    //     uint32_t value = *(uint32_t *)list_GetElement(&list, index);
    //     printf("%d\n", value);
    // }

    // list_RemoveAtAndShift(&list, 2, 3);
    
    // for(uint32_t index = 0; index < list.cursor; index++)
    // {
    //     uint32_t value = *(uint32_t *)list_GetElement(&list, index);
    //     printf("%d\n", value);
    // }

    m_selections = list_Create(sizeof(struct m_object_t), 512);
    m_wire_segments = list_Create(sizeof(struct wire_segment_pos_t), 512);

    d_Init();
    ui_Init();
    dev_Init();
    w_Init();
    sim_Init();

    glClearColor(0.8, 0.8, 0.8, 1);
    glClearDepth(1);

    // struct dev_t *power = dev_CreateDevice(DEV_DEVICE_TYPE_POW);
    // struct dev_t *gnd = dev_CreateDevice(DEV_DEVICE_TYPE_GND);
    // struct dev_t *nmos = dev_CreateDevice(DEV_DEVICE_TYPE_NMOS);
    // struct dev_t *pmos = dev_CreateDevice(DEV_DEVICE_TYPE_PMOS);
    // struct wire_t *wire0 = w_CreateWire();
    // struct wire_t *wire1 = w_CreateWire();
    // struct wire_t *wire2 = w_CreateWire();
    // struct wire_t *wire3 = w_CreateWire();

    // struct dev_pin_block_t *pin_block = list_GetElement(&dev_pin_blocks, power->first_pin_block);
    // pin_block->pins[0].value = WIRE_VALUE_1S;

    // pin_block = list_GetElement(&dev_pin_blocks, gnd->first_pin_block);
    // pin_block->pins[0].value = WIRE_VALUE_0S;

    // pin_block = list_GetElement(&dev_pin_blocks, nmos->first_pin_block);
    // pin_block->pins[DEV_MOS_PIN_DRAIN].value = WIRE_VALUE_U;

    // pin_block = list_GetElement(&dev_pin_blocks, pmos->first_pin_block);
    // pin_block->pins[DEV_MOS_PIN_DRAIN].value = WIRE_VALUE_U;

    // gnd->position[0] = 120;
    // nmos->position[0] = -120;

    // w_ConnectWire(wire0, nmos, DEV_MOS_PIN_GATE);
    // w_ConnectWire(wire0, pmos, DEV_MOS_PIN_GATE);
    // w_ConnectWire(wire0, power, 0);

    // w_ConnectWire(wire1, gnd, 0);
    // w_ConnectWire(wire1, nmos, DEV_MOS_PIN_SOURCE);

    // w_ConnectWire(wire2, power, 0);
    // w_ConnectWire(wire2, pmos, DEV_MOS_PIN_SOURCE);

    // w_ConnectWire(wire3, nmos, DEV_MOS_PIN_DRAIN);
    // w_ConnectWire(wire3, pmos, DEV_MOS_PIN_DRAIN);

    // sim_QueueWire(wire0);
    // sim_QueueWire(wire1);
    // sim_QueueWire(wire2);

    m_selected_device_type = DEV_DEVICE_TYPE_LAST;
    m_cur_edit_func = M_EDIT_FUNC_SELECT;
    // uint32_t run_simulation = 0;

    // struct m_selected_pin_t first_pin = {};
    // struct m_selected_pin_t second_pin = {};
    struct m_selected_pin_t pins[2] = {};

    // struct dev_t *clock = NULL;
    // struct dev_t *nmos = NULL;

    // struct dev_t *first_device = NULL;
    // uint32_t first_pin;
    // struct dev_t *second_device = NULL;
    // uint32_t second_pin;

    while(!in_ReadInput())
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        ui_BeginFrame();
        if(igBeginMainMenuBar())
        {
            igEndMainMenuBar();
        }

        // m_prev_mouse_x = m_mouse_x;
        // m_prev_mouse_y = m_mouse_y;
        m_mouse_x = in_mouse_x / d_model_view_projection_matrix[0] + d_model_view_projection_matrix[12];
        m_mouse_y = in_mouse_y / d_model_view_projection_matrix[5] + d_model_view_projection_matrix[13];

        m_snapped_mouse_x = M_SNAP_VALUE * (m_mouse_x / M_SNAP_VALUE);
        m_snapped_mouse_y = M_SNAP_VALUE * (m_mouse_y / M_SNAP_VALUE);

        if(igIsKeyPressed_Bool(ImGuiKey_Delete, 0))
        {
            m_DeleteSelections();
        }

        igSetNextWindowPos((ImVec2){0, 18}, 0, (ImVec2){});
        igSetNextWindowSize((ImVec2){m_window_width, 32}, 0);
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){2, 2});
        igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){2, 2});
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowMinSize, (ImVec2){});
        ImGuiStyle *style = igGetStyle();
        ImVec4 button_active_color = (ImVec4){1, 0, 0, 1};
        ImVec4 button_color = style->Colors[ImGuiCol_Button];
        
        if(igBegin("##stuff", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
        {
            if(igButton("+", (ImVec2){28, 28}))
            {
                if(m_zoom < 10.0f)
                {
                    m_zoom += 0.1f;
                }
            }

            igSameLine(0, -1);

            if(igButton("-", (ImVec2){28, 28}))
            {
                if(m_zoom > 0.1f)
                {
                    m_zoom -= 0.1f;
                }
            }

            igSameLine(0, -1);
            if(m_run_simulation)
            {
                if(igImageButton("##stop_simulation", (void *)(uintptr_t)m_pause_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    m_run_simulation = 0;
                    sim_StopSimulation();
                }
            }
            else
            {
                if(igImageButton("##begin_simulation", (void *)(uintptr_t)m_play_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    m_run_simulation = 1;
                    sim_BeginSimulation();
                }
            }

            igSameLine(0, -1);
            igSeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);

            igSameLine(0, -1);
            igPushStyleColor_Vec4(ImGuiCol_Button, (m_cur_edit_func == M_EDIT_FUNC_SELECT) ? button_active_color : button_color);
            if(igImageButton("##select_mode", (void *)(uintptr_t)m_cursor_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }
            igPopStyleColor(1);

            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Select");
                    igEndTooltip();
                }
            }

            igSameLine(0, -1);
            if(igImageButton("##rotate_ccw", (void *)(uintptr_t)m_rotate_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                for(uint32_t index = 0; index < m_selections.cursor; index++)
                {
                    struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);
                    device->rotation = (device->rotation + 1) % 4;
                }
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }
            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Rotate selection CCW");
                    igEndTooltip();
                }
            }

            igSameLine(0, -1);
            if(igImageButton("##rotate_cw", (void *)(uintptr_t)m_rotate_texture, (ImVec2){24, 24}, (ImVec2){1, 0}, (ImVec2){0, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                for(uint32_t index = 0; index < m_selections.cursor; index++)
                {
                    struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);
                    if(device->rotation == 0)
                    {
                        device->rotation = 4;
                    }
                    device->rotation--;
                }
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }
            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Rotate selection CW");
                    igEndTooltip();
                }
            }


            igSameLine(0, -1);
            if(igImageButton("##flip_h", (void *)(uintptr_t)m_fliph_texture, (ImVec2){24, 24}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                for(uint32_t index = 0; index < m_selections.cursor; index++)
                {
                    struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);

                    if(device->rotation == DEV_DEVICE_ROTATION_90 || device->rotation == DEV_DEVICE_ROTATION_270)
                    {
                        device->flip ^= 1 << DEV_DEVICE_FLIP_X;
                    }
                    else
                    {
                        device->flip ^= 1 << DEV_DEVICE_FLIP_Y;
                    }
                }
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }
            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Mirror selection vertically");
                    igEndTooltip();
                }
            }


            igSameLine(0, -1);
            if(igImageButton("##flip_v", (void *)(uintptr_t)m_flipv_texture, (ImVec2){24, 24}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                for(uint32_t index = 0; index < m_selections.cursor; index++)
                {
                    struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);

                    if(device->rotation == DEV_DEVICE_ROTATION_90 || device->rotation == DEV_DEVICE_ROTATION_270)
                    {
                        device->flip ^= 1 << DEV_DEVICE_FLIP_Y;
                    }
                    else
                    {
                        device->flip ^= 1 << DEV_DEVICE_FLIP_X;
                    }
                }
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }
            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Mirror selection horizontally");
                    igEndTooltip();
                }
            }

            igSameLine(0, -1);
            igSeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);
            igSameLine(0, -1);
            igPushStyleColor_Vec4(ImGuiCol_Button, (m_cur_edit_func == M_EDIT_FUNC_WIRE) ? button_active_color : button_color);
            if(igImageButton("##wire_mode", (void *)(uintptr_t)m_wire_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                m_SetEditFunc(M_EDIT_FUNC_WIRE);
            }
            igPopStyleColor(1);
            if(igIsItemHovered(0))
            {
                if(igBeginTooltip())
                {
                    igText("Wire");
                    igEndTooltip();
                }
            }

            for(uint32_t device_type = 0; device_type < DEV_DEVICE_TYPE_LAST; device_type++)
            {
                struct dev_desc_t *desc = dev_device_descs + device_type;
                ImVec2 uv0 = (ImVec2){(float)desc->offset_x / (float)dev_devices_texture_width,
                                      (float)desc->offset_y / (float)dev_devices_texture_height};
                
                ImVec2 uv1 = (ImVec2){(float)(desc->offset_x + desc->width) / (float)dev_devices_texture_width,
                                      (float)(desc->offset_y + desc->height) / (float)dev_devices_texture_height};
                igSameLine(0, -1);
                igPushID_Int(device_type);
                igPushStyleColor_Vec4(ImGuiCol_Button, (m_selected_device_type == device_type) ? button_active_color : button_color);
                if(igImageButton("##button", (void *)(uintptr_t)dev_devices_texture, (ImVec2){24, 24}, uv0, uv1, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    m_selected_device_type = device_type;
                    m_SetEditFunc(M_EDIT_FUNC_PLACE);
                }
                igPopStyleColor(1);
                igPopID();
            }

            if(igIsKeyPressed_Bool(ImGuiKey_Escape, 0))
            {
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
            }

            switch(m_cur_edit_func)
            { 
                case M_EDIT_FUNC_PLACE:
                    m_place_device_x = m_snapped_mouse_x;
                    m_place_device_y = m_snapped_mouse_y;
                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable() && !m_run_simulation)
                    {
                        if(m_selected_device_type != DEV_DEVICE_TYPE_LAST)
                        {
                            struct dev_t *device = dev_CreateDevice(m_selected_device_type);
                            device->position[0] = m_place_device_x;
                            device->position[1] = m_place_device_y;
                        }
                    }
                break;
  
                case M_EDIT_FUNC_SELECT:

                    if(ui_IsMouseAvailable())
                    {
                        if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0))
                        {
                            if(!m_run_simulation)
                            {
                                struct dev_t *device = m_GetDeviceUnderMouse();
                                uint32_t multiple = igIsKeyDown_Nil(ImGuiKey_LeftShift);
                                if(device != NULL)
                                {
                                    m_SelectDevice(device, multiple);
                                }
                            }
                            else
                            {
                                struct dev_input_t *input = m_GetInputUnderMouse();
                                dev_ToggleInput(input);
                            }
                        }
                        if(!m_run_simulation && igIsMouseDown_Nil(ImGuiMouseButton_Right))
                        {
                            int32_t snapped_mouse_x = M_SNAP_VALUE * (m_mouse_x / M_SNAP_VALUE);
                            int32_t snapped_mouse_y = M_SNAP_VALUE * (m_mouse_y / M_SNAP_VALUE);
                            int32_t dx = snapped_mouse_x - m_place_device_x;
                            int32_t dy = snapped_mouse_y - m_place_device_y;
                            
                            if(dx != 0)
                            {
                                m_place_device_x = snapped_mouse_x;
                            }

                            if(dy != 0)
                            {
                                m_place_device_y = snapped_mouse_y;
                            }

                            if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0))
                            {
                                dx = 0;
                                dy = 0;
                            }

                            for(uint32_t index = 0; index < m_selections.cursor; index++)
                            {
                                struct dev_t *device = *(struct dev_t **)list_GetElement(&m_selections, index);
                                device->position[0] += dx;
                                device->position[1] += dy;
                            }
                        }
                    }
                break;

                case M_EDIT_FUNC_WIRE:

                    if(m_cur_wire_segment != NULL)
                    {
                        int32_t dx = m_cur_wire_segment->start[0] - m_snapped_mouse_x;
                        int32_t dy = m_cur_wire_segment->start[1] - m_snapped_mouse_y;
                        if(abs(dx) > abs(dy))
                        {
                            m_cur_wire_segment->end[0] = m_snapped_mouse_x;
                            m_cur_wire_segment->end[1] = m_cur_wire_segment->start[1];
                        }
                        else
                        {
                            m_cur_wire_segment->end[0] = m_cur_wire_segment->start[0];
                            m_cur_wire_segment->end[1] = m_snapped_mouse_y;
                        }
                    }

                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
                    {
                        uint64_t segment_index = list_AddElement(&m_wire_segments, NULL);
                        struct wire_segment_pos_t *segment = list_GetElement(&m_wire_segments, segment_index);

                        if(m_cur_wire_segment != NULL)
                        {
                            segment->start[0] = m_cur_wire_segment->end[0];
                            segment->start[1] = m_cur_wire_segment->end[1];
                        }
                        else
                        {
                            segment->start[0] = m_snapped_mouse_x;
                            segment->start[1] = m_snapped_mouse_y;
                        }

                        segment->end[0] = m_snapped_mouse_x;
                        segment->end[1] = m_snapped_mouse_y;

                        m_cur_wire_segment = segment;

                        pins[m_wire_func_stage] = m_GetPinUnderMouse();
                        if(pins[m_wire_func_stage].device != NULL)
                        {
                            printf("clicked on pin %d of device %d\n", pins[m_wire_func_stage].pin, pins[m_wire_func_stage].device->element_index);
                            m_wire_func_stage++;

                            if(m_wire_func_stage == 2)
                            {
                                if(pins[0].device != pins[1].device || pins[0].pin != pins[1].pin)
                                {
                                    struct wire_t *wire = w_ConnectPins(pins[0].device, pins[0].pin, pins[1].device, pins[1].pin, &m_wire_segments);
                                    printf("wire %p (%d) between pins %d and %d of devices %p and %p\n", wire, wire->element_index, pins[0].pin, pins[1].pin, pins[0].device, pins[1].device);
                                }
                                m_wire_segments.cursor = 0;
                                m_cur_wire_segment = NULL;
                                m_wire_func_stage = 0;
                            }
                        }
                    }
                break;
            }

            // style->Colors[ImGuiCol_Button] = button_color;
        }
        igEnd();
        igPopStyleVar(3);
        ui_EndFrame();

        if(m_run_simulation)
        {
            sim_Step();
        }

        d_DrawDevices();

        SDL_GL_SwapWindow(m_window);
    }

    ui_Shutdown();
}



