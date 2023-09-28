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
extern float                    in_mouse_x;
extern float                    in_mouse_y;
extern struct pool_t            dev_devices;
extern struct list_t            dev_pin_blocks;
extern GLuint                   dev_devices_texture;
extern uint32_t                 dev_devices_texture_width;
extern uint32_t                 dev_devices_texture_height;
extern struct dev_desc_t        dev_device_descs[];

extern float                    d_model_view_projection_matrix[];

enum M_EDIT_MODES
{
    M_EDIT_MODE_PLACE,
    M_EDIT_MODE_SELECT,
    M_EDIT_MODE_DRAG_WIRE
};

struct m_selected_pin_t
{
    struct dev_t *  device;
    uint16_t        pin;
};

struct m_selected_pin_t m_GetPinUnderMouse(int32_t mouse_x, int32_t mouse_y)
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
                pin_position[0] = device->position[0] + pin_desc->offset[0];
                pin_position[1] = device->position[1] + pin_desc->offset[1];

                if(mouse_x >= pin_position[0] - DEV_DEVICE_PIN_PIXEL_WIDTH && 
                    mouse_x <= pin_position[0] + DEV_DEVICE_PIN_PIXEL_WIDTH)
                {
                    if(mouse_y >= pin_position[1] - DEV_DEVICE_PIN_PIXEL_WIDTH && 
                        mouse_y <= pin_position[1] + DEV_DEVICE_PIN_PIXEL_WIDTH)
                    {
                        // printf("pin %d clicked\n", pin_index);
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

    uint32_t cur_selected_device_type = DEV_DEVICE_TYPE_LAST;
    uint32_t cur_edit_mode = M_EDIT_MODE_SELECT;
    uint32_t run_simulation = 0;

    struct m_selected_pin_t first_pin = {};
    struct m_selected_pin_t second_pin = {};

    struct dev_t *clock = NULL;
    struct dev_t *nmos = NULL;

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

        igSetNextWindowPos((ImVec2){0, 18}, 0, (ImVec2){});
        igSetNextWindowSize((ImVec2){m_window_width, 32}, 0);
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){2, 2});
        igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){2, 2});
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowMinSize, (ImVec2){});
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
            if(igButton("C", (ImVec2){28, 28}))
            {
                if(clock != NULL)
                {
                    dev_DeviceStep(clock);
                }
            }


            igSameLine(0, -1);
            if(run_simulation)
            {
                if(igImageButton("##stop_simulation", (void *)(uintptr_t)m_pause_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    run_simulation = 0;
                }
            }
            else
            {
                if(igImageButton("##begin_simulation", (void *)(uintptr_t)m_play_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    run_simulation = 1;
                    sim_BeginSimulation();
                }
            }

            igSameLine(0, -1);
            igSeparatorEx(ImGuiSeparatorFlags_Vertical, 2.0f);

            int32_t mouse_x = in_mouse_x / d_model_view_projection_matrix[0] + d_model_view_projection_matrix[12];
            int32_t mouse_y = in_mouse_y / d_model_view_projection_matrix[5] + d_model_view_projection_matrix[13];

            igSameLine(0, -1);
            if(igImageButton("##select_mode", (void *)(uintptr_t)m_cursor_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                cur_edit_mode = M_EDIT_MODE_SELECT;
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
                if(igImageButton("##button", (void *)(uintptr_t)dev_devices_texture, (ImVec2){24, 24}, uv0, uv1, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    cur_selected_device_type = device_type;
                    cur_edit_mode = M_EDIT_MODE_PLACE;
                }
                igPopID();
            }

            switch(cur_edit_mode)
            { 
                case M_EDIT_MODE_PLACE:
                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
                    {
                        if(cur_selected_device_type != DEV_DEVICE_TYPE_LAST)
                        {
                            struct dev_t *device = dev_CreateDevice(cur_selected_device_type);
                            device->position[0] = 20 * (mouse_x / 20);
                            device->position[1] = 20 * (mouse_y / 20);
                            if(device->type == DEV_DEVICE_TYPE_CLOCK)
                            {
                                clock = device;
                            }
                            else if(device->type == DEV_DEVICE_TYPE_NMOS)
                            {
                                nmos = device;
                            }
                        }
                    }
                break;
  
                case M_EDIT_MODE_SELECT:
                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
                    {
                        struct m_selected_pin_t pin = m_GetPinUnderMouse(mouse_x, mouse_y);

                        if(pin.device != NULL)
                        {
                            printf("picked pin %d of device %p\n", pin.pin, pin.device);
                            first_pin = pin;
                            cur_edit_mode = M_EDIT_MODE_DRAG_WIRE;
                        }
                    }
                break;

                case M_EDIT_MODE_DRAG_WIRE:
                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
                    {
                        second_pin = m_GetPinUnderMouse(mouse_x, mouse_y);
                        if(second_pin.device != NULL && (first_pin.device != second_pin.device || first_pin.pin != second_pin.pin))
                        {
                            struct dev_pin_t *pin0 = dev_GetDevicePin(first_pin.device, first_pin.pin);
                            struct dev_pin_t *pin1 = dev_GetDevicePin(second_pin.device, second_pin.pin);
                            struct wire_t *wire;
                            
                            if(pin0->wire == WIRE_INVALID_WIRE && pin1->wire == WIRE_INVALID_WIRE)
                            {
                                wire = w_CreateWire();
                                w_ConnectWire(wire, first_pin.device, first_pin.pin);
                                w_ConnectWire(wire, second_pin.device, second_pin.pin);
                            }
                            else if(pin0->wire == WIRE_INVALID_WIRE)
                            {
                                wire = w_GetWire(pin1->wire);
                                w_ConnectWire(wire, first_pin.device, first_pin.pin);
                            }
                            else if(pin1->wire == WIRE_INVALID_WIRE)
                            {
                                wire = w_GetWire(pin0->wire);
                                w_ConnectWire(wire, second_pin.device, second_pin.pin);
                            }

                            printf("wire %p between pins %d and %d of devices %p and %p\n", wire, first_pin.pin, second_pin.pin, first_pin.device, second_pin.device);
                        }

                        first_pin = (struct m_selected_pin_t){};
                        cur_edit_mode = M_EDIT_MODE_SELECT;
                    }
                break;
            }
        }
        igEnd();
        igPopStyleVar(3);
        ui_EndFrame();

        if(run_simulation)
        {
            sim_Step();
            struct dev_pin_t *clock_pin = dev_GetDevicePin(clock, 0);
            struct dev_pin_t *nmos_drain = dev_GetDevicePin(nmos, DEV_MOS_PIN_DRAIN);
            struct wire_t *drain_wire = w_GetWire(nmos_drain->wire);
            printf("clock is %d, drain is %d\n", clock_pin->value, drain_wire->value);
        }

        d_DrawDevices();

        SDL_GL_SwapWindow(m_window);
    }

    ui_Shutdown();
}



