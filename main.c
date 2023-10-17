#include <stdio.h>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb_image.h"
#include <stdint.h>
#include "main.h"
#include "draw.h"
#include "ui.h"
#include "in.h"
#include "sim.h"
#include "list.h"

 

SDL_Window *                    m_window;
SDL_GLContext *                 m_context;
uint32_t                        m_window_width = 800;
uint32_t                        m_window_height = 600;
float                           m_zoom = 0.6f;
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
// struct dev_t *                  m_selected_device;
int32_t                         m_selections_center[2] = {};
struct list_t                   m_selections;
struct list_t                   m_objects_in_box;
struct list_t                   m_wire_seg_pos;
struct list_t                   m_wire_segs;
// struct wire_seg_pos_t *         m_prev_wire_segment;
struct wire_seg_pos_t *         m_cur_wire_segment;
struct list_t                   m_objects[M_OBJECT_TYPE_LAST];

extern float                    in_mouse_x;
extern float                    in_mouse_y;
extern struct pool_t            dev_devices;
extern struct list_t            dev_pin_blocks;
extern GLuint                   dev_devices_texture;
extern uint32_t                 dev_devices_texture_width;
extern uint32_t                 dev_devices_texture_height;
extern struct dev_desc_t        dev_device_descs[];

extern struct list_t            w_wire_seg_pos;
extern struct pool_t            w_wires;

extern float                    d_model_view_projection_matrix[];

struct m_object_t *m_CreateObject(uint32_t type, void *base_object)
{
    uint64_t index = list_AddElement(&m_objects[type], NULL);
    struct m_object_t *object = list_GetElement(&m_objects[type], index);
    object->object = base_object;
    object->type = type;
    object->selection_index = INVALID_LIST_INDEX;
    m_UpdateObject(object);
    return object;
}

void m_DestroyObject(struct m_object_t *object)
{
    switch(object->type)
    {
        case M_OBJECT_TYPE_DEVICE:
        {
            struct dev_t *device = object->object;
            dev_DestroyDevice(device);
        }
        break;

        case M_OBJECT_TYPE_SEGMENT:
        {
            struct wire_seg_t *segment = object->object;
        }
        break;
    }
}

void m_UpdateObject(struct m_object_t *object)
{
    switch(object->type)
    {
        case M_OBJECT_TYPE_DEVICE:
        {
            struct dev_t *device = object->object;
            struct dev_desc_t *dev_desc = dev_device_descs + device->type;
            int32_t box_min[2];
            int32_t box_max[2];
            dev_GetDeviceLocalBoxPosition(device, box_min, box_max);

            for(uint32_t pin_index = 0; pin_index < dev_desc->pin_count; pin_index++)
            {
                int32_t pin_min[2];
                int32_t pin_max[2];

                dev_GetDeviceLocalPinPosition(device, pin_index, pin_min);

                pin_max[0] = pin_min[0] + M_DEVICE_PIN_PIXEL_WIDTH;
                pin_max[1] = pin_min[1] + M_DEVICE_PIN_PIXEL_WIDTH;

                pin_min[0] -= M_DEVICE_PIN_PIXEL_WIDTH;
                pin_min[1] -= M_DEVICE_PIN_PIXEL_WIDTH;

                if(pin_min[0] < box_min[0])
                {
                    box_min[0] = pin_min[0];
                }

                if(pin_min[1] < box_min[1])
                {
                    box_min[1] = pin_min[1];
                }

                if(pin_max[0] > box_max[0])
                {
                    box_max[0] = pin_max[0];
                }

                if(pin_max[1] > box_max[1])
                {
                    box_max[1] = pin_max[1];
                }
            }

            object->size[0] = (box_max[0] - box_min[0]) / 2;
            object->size[1] = (box_max[1] - box_min[1]) / 2;

            box_min[0] += device->position[0];
            box_min[1] += device->position[1];

            box_max[0] += device->position[0];
            box_max[1] += device->position[1];

            object->position[0] = (box_max[0] + box_min[0]) / 2;
            object->position[1] = (box_max[1] + box_min[1]) / 2;

            device->selection_index = object->selection_index;
        }
        break;

        case M_OBJECT_TYPE_SEGMENT:
        {
            struct wire_seg_t *segment = object->object;

            for(uint32_t index = 0; index < 2; index++)
            {
                if(segment->pos->ends[WIRE_SEG_END_INDEX][index] > segment->pos->ends[WIRE_SEG_START_INDEX][index])
                {
                    object->size[index] = segment->pos->ends[WIRE_SEG_END_INDEX][index] - segment->pos->ends[WIRE_SEG_START_INDEX][index]; 
                }
                else
                {
                    object->size[index] = segment->pos->ends[WIRE_SEG_START_INDEX][index] - segment->pos->ends[WIRE_SEG_END_INDEX][index]; 
                }

                object->size[index] = object->size[index] / 2 + M_WIRE_PIXEL_WIDTH;
            }

            object->position[0] = (segment->pos->ends[WIRE_SEG_START_INDEX][0] + segment->pos->ends[WIRE_SEG_END_INDEX][0]) / 2;
            object->position[1] = (segment->pos->ends[WIRE_SEG_START_INDEX][1] + segment->pos->ends[WIRE_SEG_END_INDEX][1]) / 2;
            segment->selection_index = object->selection_index;
        }
        break;
    }
}

void m_SelectObject(struct m_object_t *object, uint32_t multiple)
{
    if(object->selection_index != INVALID_LIST_INDEX)
    {
        uint64_t index = object->selection_index;
        object->selection_index = INVALID_LIST_INDEX;   

        if(multiple)
        {
            list_RemoveElement(&m_selections, index);
            if(object->selection_index < m_selections.cursor)
            {
                struct m_object_t *moved_object = *(struct m_object_t **)list_GetElement(&m_selections, index);
                moved_object->selection_index = index;
                m_UpdateObject(moved_object);
            }
            return;
        }
    }

    if(!multiple)
    {
        m_ClearSelections();
    }

    object->selection_index = list_AddElement(&m_selections, &object);
    m_UpdateObject(object);
    
    // m_selections_center[0] += object->position[0];
    // m_selections_center[1] += object->position[1];
}

void m_ClearSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
        object->selection_index = INVALID_LIST_INDEX;
        m_UpdateObject(object);
    }

    m_selections.cursor = 0;
    m_selections_center[0] = 0;
    m_selections_center[1] = 0;
}

void m_DeleteSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
        m_DestroyObject(object);
    }

    m_selections.cursor = 0;
}

void m_TranslateSelections(int32_t dx, int32_t dy)
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
        switch(object->type)
        {
            case M_OBJECT_TYPE_DEVICE:
            {
                struct dev_t *device = object->object;
                device->position[0] += dx;
                device->position[1] += dy;
            }
            break;

            case M_OBJECT_TYPE_SEGMENT:
            {
                struct wire_seg_t *segment = object->object;
                segment->pos->ends[WIRE_SEG_START_INDEX][0] += dx;
                segment->pos->ends[WIRE_SEG_START_INDEX][1] += dy;
                segment->pos->ends[WIRE_SEG_END_INDEX][0] += dx;
                segment->pos->ends[WIRE_SEG_END_INDEX][1] += dy;
            }
            break;
        }

        m_UpdateObject(object);
    }
}

void m_RotateSelections(int32_t ccw_rotation)
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
        switch(object->type)
        {
            case M_OBJECT_TYPE_DEVICE:
            {
                struct dev_t *device = object->object;
                if(ccw_rotation)
                {
                    device->rotation = (device->rotation + 1) % 4;
                }
                else
                {
                    if(device->rotation == 0)
                    {
                        device->rotation = 4;
                    }
                    device->rotation--;
                }
            }
            break;

            case M_OBJECT_TYPE_SEGMENT:

            break;
        }
    }
}

void m_GetTypedObjectsInsideBox(uint32_t type, int32_t *box_min, int32_t *box_max)
{
    struct list_t *object_list = &m_objects[type];
    m_objects_in_box.cursor = 0;

    for(uint32_t index = 0; index < object_list->cursor; index++)
    {
        struct m_object_t *object = list_GetElement(object_list, index);
        if(box_max[0] >= object->position[0] - object->size[0] && box_min[0] <= object->position[0] + object->size[0])
        {
            if(box_max[1] >= object->position[1] - object->size[1] && box_min[1] <= object->position[1] + object->size[1])
            {
                list_AddElement(&m_objects_in_box, &object);
            }
        }
    }
}

void m_GetTypedObjectsUnderMouse(uint32_t type)
{
    m_GetTypedObjectsInsideBox(type, (int32_t []){m_mouse_x, m_mouse_y}, (int32_t []){m_mouse_x, m_mouse_y});
}

struct m_object_t *m_GetObjectUnderMouse()
{
    
}

struct m_picked_object_t m_GetPinUnderMouse()
{
    struct m_picked_object_t selected_pin = {};

    m_GetTypedObjectsUnderMouse(M_OBJECT_TYPE_DEVICE);

    for(uint32_t device_index = 0; device_index < m_objects_in_box.cursor; device_index++)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_objects_in_box, device_index);
        struct dev_t *device = object->object;
        if(device != NULL)
        {
            struct dev_desc_t *desc = dev_device_descs + device->type;
            for(uint32_t pin_index = 0; pin_index < desc->pin_count; pin_index++)
            {
                struct dev_pin_desc_t *pin_desc = desc->pins + pin_index;
                int32_t pin_position[2];

                dev_GetDeviceLocalPinPosition(device, pin_index, pin_position);

                pin_position[0] += device->position[0];
                pin_position[1] += device->position[1];

                if(m_mouse_x >= pin_position[0] - M_DEVICE_PIN_PIXEL_WIDTH && 
                    m_mouse_x <= pin_position[0] + M_DEVICE_PIN_PIXEL_WIDTH)
                {
                    if(m_mouse_y >= pin_position[1] - M_DEVICE_PIN_PIXEL_WIDTH && 
                        m_mouse_y <= pin_position[1] + M_DEVICE_PIN_PIXEL_WIDTH)
                    {
                        selected_pin.object = object;
                        selected_pin.index = pin_index;
                        device_index = dev_devices.cursor;
                        break;
                    }    
                }
            }
            
        }
    }

    return selected_pin;
}

struct m_picked_object_t m_GetSegmentUnderMouse()
{
    struct m_picked_object_t selected_segment = {};

    m_GetTypedObjectsUnderMouse(M_OBJECT_TYPE_SEGMENT);

    if(m_objects_in_box.cursor != 0)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_objects_in_box, 0);
        selected_segment.object = object;
    }

    return selected_segment;
}

struct dev_t *m_GetDeviceUnderMouse()
{
    m_GetTypedObjectsUnderMouse(M_OBJECT_TYPE_DEVICE);

    if(m_objects_in_box.cursor != 0)
    {
        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_objects_in_box, 0);
        return object->object;
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


// void m_SelectDevice(struct dev_t *device, uint32_t multiple)
// {
//     if(device->selection_index != INVALID_LIST_INDEX)
//     {
//         uint64_t index = device->selection_index;
//         device->selection_index = INVALID_LIST_INDEX;   

//         if(multiple)
//         {
//             list_RemoveElement(&m_selections, index);
//             if(device->selection_index < m_selections.cursor)
//             {
//                 struct dev_t *moved_device = *(struct dev_t **)list_GetElement(&m_selections, index);
//                 moved_device->selection_index = index;
//             }

//             return;
//         }
//     }

//     if(!multiple)
//     {
//         m_ClearSelections();
//     }

//     device->selection_index = list_AddElement(&m_selections, &device);
// }

void m_ClearWireSegments()
{
    m_wire_seg_pos.cursor = 0;
    m_wire_func_stage = 0;
    m_cur_wire_segment = NULL;
}

void m_ClearSelectedDeviceType()
{
    m_selected_device_type = DEV_DEVICE_TYPE_LAST;
}

void m_SetEditFunc(uint32_t func)
{
    m_cur_edit_func = func;
    switch(m_cur_edit_func)
    {
        case M_EDIT_FUNC_SELECT:
        case M_EDIT_FUNC_MOVE:
            m_ClearSelectedDeviceType();
            m_ClearWireSegments();
            ui_SetCursor(SDL_SYSTEM_CURSOR_ARROW);
        break;

        case M_EDIT_FUNC_PLACE:
            m_ClearWireSegments();
            m_ClearSelections();
            ui_SetCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;

        case M_EDIT_FUNC_WIRE:
            m_ClearSelectedDeviceType();
            m_ClearSelections();
            ui_SetCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;
    }
}

struct wire_t *m_CreateWire(struct m_picked_object_t *first_contact, struct m_picked_object_t *second_contact, struct list_t *segments)
{
    struct wire_t *wire = NULL;
    struct wire_junc_t *start_junction = NULL;
    struct wire_junc_t *end_junction = NULL;

    union m_wire_seg_t *first_segment = list_GetElement(segments, 0);
    union m_wire_seg_t *last_segment = list_GetElement(segments, segments->cursor - 1);

    if(first_contact->object->type == second_contact->object->type)
    {
        if(first_contact->object->type == M_OBJECT_TYPE_DEVICE)
        {
            /* no wire connected to neither of the devices */
            wire = w_AllocWire();
            start_junction = w_AllocWireJunction(wire);
            end_junction = w_AllocWireJunction(wire);
            w_ConnectPin(start_junction, first_contact->object->object, first_contact->index);            
            w_ConnectPin(end_junction, second_contact->object->object, second_contact->index);  
        }
        else
        {
            struct wire_elem_t *wire_a = first_contact->object->object;
            struct wire_elem_t *wire_b = second_contact->object->object;

            if(wire_a->wire != wire_b->wire)
            {
                wire = w_MergeWires(wire_a->wire, wire_b->wire);
            }
            else
            {
                wire = wire_a->wire;
            }

            if(first_contact->object->type == M_OBJECT_TYPE_SEGMENT)
            {
                start_junction = w_AddJunction(first_contact->object->object, first_segment->seg_pos.ends[WIRE_SEG_START_INDEX]);
                m_CreateObject(M_OBJECT_TYPE_SEGMENT, start_junction->last_segment);
            }
            else
            {
                start_junction = first_contact->object->object;
            }

            if(second_contact->object->type == M_OBJECT_TYPE_SEGMENT)
            {
                end_junction = w_AddJunction(second_contact->object->object, last_segment->seg_pos.ends[WIRE_SEG_END_INDEX]);
                m_CreateObject(M_OBJECT_TYPE_SEGMENT, end_junction->last_segment);
            }
            else
            {
                end_junction = second_contact->object->object;
            }
        }
    }
    else
    {
        struct wire_elem_t *wire_elem;
        struct dev_t *device;

        if(first_contact->object->type == M_OBJECT_TYPE_DEVICE)
        {
            device = first_contact->object->object;
            wire_elem = second_contact->object->object;
            start_junction = w_AllocWireJunction(wire_elem->wire);
            w_ConnectPin(start_junction, device, first_contact->index);

            if(second_contact->object->type == M_OBJECT_TYPE_SEGMENT)
            {
                end_junction = w_AddJunction(second_contact->object->object, last_segment->seg_pos.ends[WIRE_SEG_END_INDEX]);
                m_CreateObject(M_OBJECT_TYPE_SEGMENT, end_junction->last_segment);
            }
            else
            {
                end_junction = second_contact->object->object;
            }
        }
        else
        {
            device = second_contact->object->object;
            wire_elem = first_contact->object->object;
            end_junction = w_AllocWireJunction(wire_elem->wire);
            w_ConnectPin(end_junction, device, second_contact->index);

            if(first_contact->object->type == M_OBJECT_TYPE_SEGMENT)
            {
                start_junction = w_AddJunction(first_contact->object->object, first_segment->seg_pos.ends[WIRE_SEG_START_INDEX]);
                m_CreateObject(M_OBJECT_TYPE_SEGMENT, start_junction->last_segment);
            }
            else
            {
                start_junction = first_contact->object->object;
            }
        }

        wire = wire_elem->wire;
    }

    struct wire_seg_t *prev_segment = NULL;

    for(uint32_t index = 0; index < segments->cursor; index++)
    {
        union m_wire_seg_t *segment_pos = list_GetElement(segments, index);
        struct wire_seg_t *segment = w_AllocWireSegment(wire);
        segment->pos->ends[WIRE_SEG_START_INDEX][0] = segment_pos->seg_pos.ends[WIRE_SEG_START_INDEX][0];
        segment->pos->ends[WIRE_SEG_START_INDEX][1] = segment_pos->seg_pos.ends[WIRE_SEG_START_INDEX][1];
        segment->pos->ends[WIRE_SEG_END_INDEX][0] = segment_pos->seg_pos.ends[WIRE_SEG_END_INDEX][0];
        segment->pos->ends[WIRE_SEG_END_INDEX][1] = segment_pos->seg_pos.ends[WIRE_SEG_END_INDEX][1];

        segment->segments[WIRE_SEG_START_INDEX] = prev_segment;
        if(prev_segment != NULL)
        {
            prev_segment->segments[WIRE_SEG_END_INDEX] = segment;
        }
        prev_segment = segment;
        segment_pos->segment = segment;

        m_CreateObject(M_OBJECT_TYPE_SEGMENT, segment);
    }

    w_LinkSegmentToJunction(first_segment->segment, start_junction, WIRE_SEG_START_INDEX);
    w_LinkSegmentToJunction(last_segment->segment, end_junction, WIRE_SEG_END_INDEX);

    return wire;
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

    m_selections = list_Create(sizeof(struct m_object_t *), 512);
    m_objects_in_box = list_Create(sizeof(struct m_object_t *), 512);
    m_wire_seg_pos = list_Create(sizeof(union m_wire_seg_t), 512);
    m_objects[M_OBJECT_TYPE_DEVICE] = list_Create(sizeof(struct m_object_t), 16384);
    m_objects[M_OBJECT_TYPE_SEGMENT] = list_Create(sizeof(struct m_object_t), 16384);
    // m_wire_segs = list_Create(sizeof(struct wire_seg_t), 512);

    d_Init();
    ui_Init();
    dev_Init();
    w_Init();
    sim_Init();

    glClearColor(0.8, 0.8, 0.8, 1);
    glClearDepth(1);

    m_selected_device_type = DEV_DEVICE_TYPE_LAST;
    m_cur_edit_func = M_EDIT_FUNC_SELECT;
    struct m_picked_object_t picked_objects[2] = {};
    // struct m_selected_pin_t pins[2] = {};

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

        m_mouse_x = in_mouse_x / d_model_view_projection_matrix[0] + d_model_view_projection_matrix[12];
        m_mouse_y = in_mouse_y / d_model_view_projection_matrix[5] + d_model_view_projection_matrix[13];

        if(m_mouse_x > 0)
        {
            m_snapped_mouse_x = M_SNAP_VALUE * ((m_mouse_x / M_SNAP_VALUE) + ((m_mouse_x % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        }
        else
        {
            m_snapped_mouse_x = M_SNAP_VALUE * ((m_mouse_x / M_SNAP_VALUE) - ((abs(m_mouse_x) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        }

        if(m_mouse_y > 0)
        {
            m_snapped_mouse_y = M_SNAP_VALUE * ((m_mouse_y / M_SNAP_VALUE) + ((m_mouse_y % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        }
        else
        {
            m_snapped_mouse_y = M_SNAP_VALUE * ((m_mouse_y / M_SNAP_VALUE) - ((abs(m_mouse_y) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        }

        if(igIsKeyPressed_Bool(ImGuiKey_Delete, 0))
        {
            m_DeleteSelections();
        }

        if(igIsKeyPressed_Bool(ImGuiKey_G, 0))
        {
            m_SetEditFunc(M_EDIT_FUNC_SELECT);
        }
        else if(igIsKeyPressed_Bool(ImGuiKey_W, 0))
        {
            m_SetEditFunc(M_EDIT_FUNC_WIRE);
        }
        else if(igIsKeyPressed_Bool(ImGuiKey_T, 0))
        {
            if(m_selections.cursor > 0)
            {
                struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, 0);
                if(object->type == M_OBJECT_TYPE_SEGMENT)
                {
                    printf("%d\n", w_TryReachSegment(object->object));
                }
            }
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
                    m_SetEditFunc(M_EDIT_FUNC_SELECT);
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
                    struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
                    struct dev_t *device = object->object;
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
                    struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
                    struct dev_t *device = object->object;
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
                    struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
                    struct dev_t *device = object->object;

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
                    struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_selections, index);
                    struct dev_t *device = object->object;

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
                ImVec2 uv0 = (ImVec2){(float)desc->offset[0] / (float)dev_devices_texture_width,
                                      (float)desc->offset[1] / (float)dev_devices_texture_height};
                
                ImVec2 uv1 = (ImVec2){(float)(desc->offset[0] + desc->width) / (float)dev_devices_texture_width,
                                      (float)(desc->offset[1] + desc->height) / (float)dev_devices_texture_height};
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
                            m_CreateObject(M_OBJECT_TYPE_DEVICE, device);
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
                                uint32_t multiple = igIsKeyDown_Nil(ImGuiKey_LeftShift);
                                m_GetTypedObjectsInsideBox(M_OBJECT_TYPE_DEVICE, (int32_t []){m_mouse_x, m_mouse_y}, (int32_t []){m_mouse_x, m_mouse_y});
                                if(m_objects_in_box.cursor != 0)
                                {
                                    struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_objects_in_box, 0);
                                    m_SelectObject(object, multiple);
                                }
                                else
                                {
                                    m_GetTypedObjectsInsideBox(M_OBJECT_TYPE_SEGMENT, (int32_t []){m_mouse_x, m_mouse_y}, (int32_t []){m_mouse_x, m_mouse_y});
                                    if(m_objects_in_box.cursor != 0)
                                    {
                                        struct m_object_t *object = *(struct m_object_t **)list_GetElement(&m_objects_in_box, 0);
                                        m_SelectObject(object, multiple);
                                    }
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
                            int32_t dx = m_snapped_mouse_x - m_place_device_x;
                            int32_t dy = m_snapped_mouse_y - m_place_device_y;
                            
                            if(dx != 0)
                            {
                                m_place_device_x = m_snapped_mouse_x;
                            }

                            if(dy != 0)
                            {
                                m_place_device_y = m_snapped_mouse_y;
                            }

                            if(igIsMouseClicked_Bool(ImGuiMouseButton_Right, 0))
                            {
                                dx = 0;
                                dy = 0;
                            }

                            m_TranslateSelections(dx, dy);
                        }
                    }

                    if(igIsKeyPressed_Bool(ImGuiKey_Escape, 0))
                    {
                        m_ClearSelections();
                    }
                break;

                case M_EDIT_FUNC_WIRE:

                    if(m_cur_wire_segment != NULL)
                    {
                        int32_t dx = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] - m_snapped_mouse_x;
                        int32_t dy = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] - m_snapped_mouse_y;
                        if(abs(dx) > abs(dy))
                        {
                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] = m_snapped_mouse_x;
                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1];
                        }
                        else
                        {
                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0];
                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1] = m_snapped_mouse_y;
                        }
                    }

                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
                    {
                        if(m_cur_wire_segment == NULL || m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] != m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] ||
                                                         m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] != m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1])
                        {
                            struct wire_seg_pos_t *prev_segment = m_cur_wire_segment;
                            uint64_t segment_index = list_AddElement(&m_wire_seg_pos, NULL);
                            m_cur_wire_segment = list_GetElement(&m_wire_seg_pos, segment_index);

                            if(prev_segment != NULL)
                            {
                                m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] = prev_segment->ends[WIRE_SEG_END_INDEX][0];
                                m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] = prev_segment->ends[WIRE_SEG_END_INDEX][1];
                            }
                            else
                            {
                                m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] = m_snapped_mouse_x;
                                m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] = m_snapped_mouse_y;
                            }

                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] = m_snapped_mouse_x;
                            m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1] = m_snapped_mouse_y;
                        }

                        picked_objects[m_wire_func_stage] = m_GetPinUnderMouse();

                        if(picked_objects[m_wire_func_stage].object == NULL)
                        {
                            picked_objects[m_wire_func_stage] = m_GetSegmentUnderMouse();
                        }

                        if(picked_objects[m_wire_func_stage].object != NULL)
                        {
                            // struct dev_t *device = picked_objects[m_wire_func_stage].object->object;
                            // printf("clicked on pin %d of device %d\n", picked_objects[m_wire_func_stage].index, device->element_index);
                            printf("clicked on object %p of type %d\n", picked_objects[m_wire_func_stage].object, picked_objects[m_wire_func_stage].object->type);
                            switch(picked_objects[m_wire_func_stage].object->type)
                            {
                                case M_OBJECT_TYPE_DEVICE:
                                {
                                    struct dev_t *device = picked_objects[m_wire_func_stage].object->object;
                                    dev_GetDeviceLocalPinPosition(device, picked_objects[m_wire_func_stage].index, m_cur_wire_segment->ends[WIRE_SEG_START_INDEX]);
                                    m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] += device->position[0];
                                    m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] += device->position[1];
                                    m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0];
                                    m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1];
                                }
                                break;

                                case M_OBJECT_TYPE_SEGMENT:
                                {
                                    struct wire_seg_t *segment = picked_objects[m_wire_func_stage].object->object;
                                }
                                break;
                            }

                            m_wire_func_stage++;
                        }

                        if(m_wire_func_stage == 2)
                        {
                            /* get rid of the last segment created */
                            m_wire_seg_pos.cursor--;
                            if(picked_objects[0].object != picked_objects[1].object || picked_objects[0].index != picked_objects[1].index)
                            {
                                struct wire_t *wire = m_CreateWire(&picked_objects[0], &picked_objects[1], &m_wire_seg_pos);
                                printf("wire %p (%d) between pins %d and %d of devices %p and %p\n", wire, wire->element_index, picked_objects[0].index, 
                                                                                                                                picked_objects[1].index, 
                                                                                                                                picked_objects[0].object, 
                                                                                                                                picked_objects[1].object);
                            }

                            m_ClearWireSegments();
                        }

                    }

                    if(igIsKeyPressed_Bool(ImGuiKey_Escape, 0))
                    {
                        m_ClearWireSegments();
                    }
                break;
            }
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



