#include <stdio.h>
#include <math.h>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb_image.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "main.h"
#include "draw.h"
#include "ui.h"
#include "in.h"
#include "sim.h"
#include "list.h"

 

SDL_Window *                        m_window;
SDL_GLContext *                     m_context;
uint32_t                            m_window_width = 1300;
uint32_t                            m_window_height = 700;
float                               m_zoom = 1000.0f;
float                               m_offset_x = 0.0f;
float                               m_offset_y = 0.0f;
GLuint                              m_cursor_texture;
GLuint                              m_play_texture;
GLuint                              m_step_texture;
GLuint                              m_pause_texture;
GLuint                              m_wire_texture;
GLuint                              m_move_texture;
GLuint                              m_rotate_texture;
GLuint                              m_fliph_texture;
GLuint                              m_flipv_texture;
// int32_t                             m_mouse_x;
// int32_t                             m_mouse_y;
// float                               m_mouse_pos[2];
ivec2_t                             m_mouse_pos;
ivec2_t                             m_snapped_mouse_pos;
// int32_t                             m_clicked_mouse_pos[2];
uint32_t                            m_draw_selection_box;
ivec2_t                             m_clicked_mouse_pos;
ivec2_t                             m_selection_box_max;
ivec2_t                             m_selection_box_min;
// int32_t                             m_selection_box_min[2];
// int32_t                             m_selection_box_max[2];
// float                               m_snapped_mouse_pos[2];
// int32_t                             m_place_device_x;
// int32_t                             m_place_device_y;
ivec2_t                             m_place_device_pos;
// int32_t                             m_snapped_mouse_x;
// int32_t                             m_snapped_mouse_y;
uint32_t                            m_run_simulation;
uint32_t                            m_single_step;
uint32_t                            m_run;
uint32_t                            m_cur_edit_func;
uint32_t                            m_wire_func_stage;
uint32_t                            m_selected_device_type;
// struct dev_t *                  m_selected_device;
ivec2_t                             m_selections_center = {};
struct m_picked_element_t           m_picked_elements[2] = {};
struct list_t                       m_selections;
struct list_t                       m_wire_segs;
// struct list_t                       m_wire_segs;
struct list_t                       m_elements_in_box;
// struct wire_seg_pos_t *         m_prev_wire_segment;
union m_wire_seg_t *                m_cur_wire_segment;
union m_wire_seg_t *                m_prev_wire_segment;
struct m_explorer_save_load_args_t  m_save_load_args;
struct m_explorer_state_t           m_explorer_state;
char                                m_work_dir[FILE_MAX_PATH_LEN];
char                                m_file_path[FILE_MAX_PATH_LEN];
char                                m_file_name[FILE_MAX_PATH_LEN];
char                                m_file_full_path[FILE_MAX_PATH_LEN];
uint32_t                            m_cur_state = M_STATE_EDIT;

void (*m_StateFuncs[M_STATE_LAST])() = {
    [M_STATE_EDIT]              = m_EditState,
    [M_STATE_SELECTION_BOX]     = m_SelectionBoxState,
    [M_STATE_EXPLORER]          = m_ExplorerState
};

#define M_OVERWRITE_MODAL_WINDOW_NAME "overwrite_modal"
uint32_t                            m_overwrite_modal_open = 0;

/* from elem.c */
// extern struct list_t                obj_objects_in_box;
extern struct pool_t                elem_elements[];
extern struct dbvt_t                elem_dbvt[];

/* from in.c */
extern float                        in_mouse_x;
extern float                        in_mouse_y;

/* from dev.c */
extern struct pool_t                dev_devices;
extern struct list_t                dev_pin_blocks;
// extern GLuint                       dev_devices_texture_small;
// extern uint32_t                     dev_devices_texture_small_width;
// extern uint32_t                     dev_devices_texture_small_height;
extern struct dev_desc_t            dev_device_descs[];

/* from wire.c */
extern struct list_t                w_wire_seg_pos;
extern struct pool_t                w_wire_segs;
extern struct pool_t                w_wire_juncs;
extern struct pool_t                w_wires;
extern uint64_t                     w_seg_junc_count;

/* from draw.c */
extern float                        d_model_view_projection_matrix[];
extern GLuint                       d_devices_texture;
extern uint32_t                     d_devices_texture_width;
extern uint32_t                     d_devices_texture_height;

struct m_editor_state_t m_CreateEditorState()
{
    return (struct m_editor_state_t){
        .selections         = list_Create(sizeof(struct elem_t *), 512),
        .elements_in_box    = list_Create(sizeof(struct elem_t *), 512),
        .cur_state          = M_STATE_EDIT,
    };
}

void m_DestroyEditorState(struct m_editor_state_t *editor_state)
{
    list_Destroy(&editor_state->selections);
    list_Destroy(&editor_state->elements_in_box);
}

void m_SelectElement(struct elem_t *element, uint32_t multiple)
{
    // vec2_t bounding_box_center;
    // vec2_t_add(&bounding_box_center, &element->node->max.xy, &element->node->min.xy);
    // vec2_t_scale(&bounding_box_center, &bounding_box_center, 0.5f);
    // ivec2_t element_position = cvt_vec2_t_ivec2_t(bounding_box_center);

    if(element->selection_index != INVALID_LIST_INDEX)
    {
        uint64_t index = element->selection_index;
        element->selection_index = INVALID_LIST_INDEX;   

        if(multiple)
        {
            list_RemoveElement(&m_selections, index);
            if(element->selection_index < m_selections.cursor)
            {
                struct elem_t *moved_element = *(struct elem_t **)list_GetElement(&m_selections, index);
                moved_element->selection_index = index;
                elem_UpdateElement(moved_element);
            }

            ivec2_t_sub(&m_selections_center, &m_selections_center, &element->position);
            return;
        }
    }

    if(!multiple)
    {
        m_ClearSelections();
    }

    element->selection_index = list_AddElement(&m_selections, &element);
    elem_UpdateElement(element);
    ivec2_t_add(&m_selections_center, &m_selections_center, &element->position);
}

void m_ClearSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        element->selection_index = INVALID_LIST_INDEX;
        elem_UpdateElement(element);
    }

    m_selections.cursor = 0;
    m_selections_center = (ivec2_t){};
}

void m_DeleteSelections()
{
    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        elem_DestroyElement(element);
    }

    m_selections.cursor = 0;
    m_selections_center = (ivec2_t){};
}

void m_TranslateSelections(ivec2_t *translation) 
{
    m_selections_center = (ivec2_t){};

    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        elem_Translate(element, translation);
        ivec2_t_add(&m_selections_center, &m_selections_center, &element->position);
    }
}

void m_RotateSelections(int32_t ccw_rotation)
{
    ivec2_t rotation_pivot = {.x = m_selections_center.x / (int32_t)m_selections.cursor, .y = m_selections_center.y / (int32_t)m_selections.cursor};

    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        elem_Rotate(element, &rotation_pivot, ccw_rotation);
        elem_UpdateElement(element);
    }
}

void m_FlipSelectionsH()
{
    ivec2_t flip_pivot = {.x = m_selections_center.x / (int32_t)m_selections.cursor, .y = m_selections_center.y / (int32_t)m_selections.cursor};

    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        elem_FlipHorizontally(element, &flip_pivot);
    }
}

void m_FlipSelectionsV()
{
    ivec2_t flip_pivot = {.x = m_selections_center.x / (int32_t)m_selections.cursor, .y = m_selections_center.y / (int32_t)m_selections.cursor};

    for(uint32_t index = 0; index < m_selections.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
        elem_FlipVertically(element, &flip_pivot);
    }
}

void m_GetTypedElementsUnderMouse(uint32_t type, struct list_t *elements)
{
    elem_GetTypedElementsInsideBox(type, &(vec2_t){m_mouse_pos.x, m_mouse_pos.y}, &(vec2_t){m_mouse_pos.x, m_mouse_pos.y}, elements);
}

struct m_picked_element_t m_GetPinUnderMouse()
{
    struct m_picked_element_t selected_pin = {};

    m_elements_in_box.cursor = 0;

    m_GetTypedElementsUnderMouse(ELEM_TYPE_DEVICE, &m_elements_in_box);

    for(uint32_t device_index = 0; device_index < m_elements_in_box.cursor; device_index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, device_index);
        struct dev_t *device = element->base_object;
        if(device != NULL)
        {
            struct dev_desc_t *desc = dev_device_descs + device->type;
            for(uint32_t pin_index = 0; pin_index < desc->pin_count; pin_index++)
            {
                struct dev_pin_desc_t *pin_desc = desc->pins + pin_index;
                ivec2_t pin_position;

                dev_GetDeviceLocalPinPosition(device, pin_index, &pin_position);
                ivec2_t_add(&pin_position, &pin_position, &device->position);

                if(m_mouse_pos.x >= pin_position.x - ELEM_DEVICE_PIN_PIXEL_WIDTH && 
                    m_mouse_pos.x <= pin_position.x + ELEM_DEVICE_PIN_PIXEL_WIDTH)
                {
                    if(m_mouse_pos.y >= pin_position.y - ELEM_DEVICE_PIN_PIXEL_WIDTH && 
                       m_mouse_pos.y <= pin_position.y + ELEM_DEVICE_PIN_PIXEL_WIDTH)
                    {
                        selected_pin.element = element;
                        selected_pin.index = pin_index;
                        device_index = dev_devices.cursor;
                        printf("pin index: %d\n", pin_index);
                        break;
                    }    
                }
            }
            
        }
    }

    return selected_pin;
}

struct m_picked_element_t m_GetSegmentUnderMouse()
{
    struct m_picked_element_t selected_segment = {};
    // m_elements_in_box.cursor = 0;

    // m_GetTypedElementsUnderMouse(ELEM_TYPE_SEGMENT, &m_elements_in_box);

    // if(m_elements_in_box.cursor != 0)
    // {
    //     struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, 0);
    //     selected_segment.element = element;
    // }

    return selected_segment;
}

struct dev_t *m_GetDeviceUnderMouse()
{
    m_elements_in_box.cursor = 0;

    m_GetTypedElementsUnderMouse(ELEM_TYPE_DEVICE, &m_elements_in_box);

    if(m_elements_in_box.cursor != 0)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, 0);
        return element->base_object;
    }

    return NULL;
}

struct dev_input_t *m_GetInputUnderMouse()
{
    struct dev_input_t *input = NULL;
    struct dev_t *device = m_GetDeviceUnderMouse();
    if(device != NULL && device->type == DEV_DEVICE_INPUT)
    {
        input = device->data;
    }

    return input;
}

void m_ClearWireSegments()
{
    m_wire_segs.cursor = 0;
    m_wire_func_stage = 0;
    m_cur_wire_segment = NULL;
    m_prev_wire_segment = NULL;
}

void m_ClearSelectedDeviceType()
{
    m_selected_device_type = DEV_DEVICE_LAST;
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

struct wire_t *m_CreateWire(struct m_picked_element_t *first_element, struct m_picked_element_t *second_element, struct list_t *segments)
{
    struct wire_t *wire = NULL;
    // struct wire_junc_t *start_junction = NULL;
    // struct wire_junc_t *end_junction = NULL;

    union m_wire_seg_t *first_segment = list_GetElement(segments, 0);
    union m_wire_seg_t *last_segment = list_GetElement(segments, segments->cursor - 1);

    struct wire_seg_t *prev_segment = NULL;
    uint32_t junction_alloc_bitmask = WIRE_SEG_ALLOC_JUNC_BOTH;
    // ivec2_t prev_head_pos = first_segment->position;
    for(uint32_t index = 1; index < segments->cursor; index++)
    {
        union m_wire_seg_t *segment_pos = list_GetElement(segments, index);
        struct wire_seg_t *segment = w_AllocSegment(NULL, junction_alloc_bitmask);

        segment->ends[WIRE_SEG_TAIL_INDEX].junction->position = segment_pos->position;
        if(prev_segment != NULL)
        {
            w_LinkJunctionAndSegment(segment, prev_segment->ends[WIRE_SEG_TAIL_INDEX].junction, WIRE_SEG_HEAD_INDEX);
        }
        else
        {
            segment->ends[WIRE_SEG_HEAD_INDEX].junction->position = first_segment->position;
            first_segment = segment_pos;
        }

        prev_segment = segment;
        segment_pos->segment = segment;
        junction_alloc_bitmask = WIRE_SEG_ALLOC_JUNC_TAIL;
    }

    for(uint32_t index = 1; index < segments->cursor; index++)
    {
        union m_wire_seg_t *segment = list_GetElement(segments, index);
        elem_UpdateElement(segment->segment->element);
    }

    // if(first_contact->element->type == ELEM_TYPE_DEVICE)
    // {
    //     struct dev_t *device = first_contact->element->base_object;
    //     struct dev_pin_t *pin = dev_GetDevicePin(device, first_contact->index);
    //     struct wire_junc_t *junction = w_GetWireJunction(pin->junction);
    //     if(junction != NULL)
    //     {
    //         /* FIXME: this can happen if the user tries to connect a wire to a pin where there's already something
    //         connected to by clicking next to it instead of on it. Because the "hitbox" of the wire is smaller than
    //         the pin one, the click will miss the wire and land on the pin instead, and the new wire segments won't 
    //         be connected to the existing wire. 
            
    //         Ideally, wire tips should probably have a larger hitbox than device pins. */
    //         struct wire_seg_t *segment = junction->first_segment;
    //         w_ConnectSegments(first_segment->segment, segment, segment->junctions[WIRE_SEG_TAIL_INDEX].junction == junction);    
    //     }
    //     else
    //     {
    //         w_ConnectPinToSegment(first_segment->segment, WIRE_SEG_HEAD_INDEX, device, first_contact->index);
    //     }
    // }
    // else if(first_contact->element->type == ELEM_TYPE_SEGMENT)
    // {
    //     w_ConnectSegments(first_segment->segment, first_contact->element->base_object, WIRE_SEG_TAIL_INDEX);
    // }

    // if(second_contact->element->type == ELEM_TYPE_DEVICE)
    // {
    //     struct dev_t *device = second_contact->element->base_object;
    //     struct dev_pin_t *pin = dev_GetDevicePin(device, second_contact->index);
    //     struct wire_junc_t *junction = w_GetWireJunction(pin->junction);
    //     if(junction != NULL)
    //     {
    //         /* FIXME: this can happen if the user tries to connect a wire to a pin where there's already something
    //         connected to by clicking next to it instead of on it. Because the "hitbox" of the wire is smaller than
    //         the pin one, the click will miss the wire and land on the pin instead, and the new wire segments won't 
    //         be connected to the existing wire. 
            
    //         Ideally, wire tips should probably have a larger hitbox than device pins. */
    //         struct wire_seg_t *segment = junction->first_segment;
    //         w_ConnectSegments(last_segment->segment, segment, segment->junctions[WIRE_SEG_END_INDEX].junction == junction);    
    //     }
    //     else
    //     {
    //         w_ConnectPinToSegment(last_segment->segment, WIRE_SEG_END_INDEX, second_contact->element->base_object, second_contact->index);
    //     }
    // }
    // else if(second_contact->element->type == ELEM_TYPE_SEGMENT)
    // {
    //     w_ConnectSegments(last_segment->segment, second_contact->element->base_object, WIRE_SEG_END_INDEX);
    // }

    wire = first_segment->segment->base.wire;

    if(wire == NULL)
    {
        wire = w_AllocWire();
        w_MoveSegmentsToWire(wire, first_segment->segment);
    }

    return wire;
}

void m_SerializeCircuit(void **file_buffer, size_t *file_buffer_size)
{
    // size_t buffer_size = sizeof(struct m_file_header_t);
    // size_t device_count = dev_devices.cursor - (dev_devices.free_indices_top + 1);
    // size_t wire_count = w_wires.cursor - (w_wires.free_indices_top + 1);
    // size_t segment_count = w_wire_segs.cursor - (w_wire_segs.free_indices_top + 1);
    // size_t junction_count = w_wire_juncs.cursor - (w_wire_juncs.free_indices_top + 1);
    // buffer_size += sizeof(struct m_device_record_t) * device_count;
    // buffer_size += sizeof(struct m_wire_record_t) * wire_count;
    // buffer_size += sizeof(struct m_segment_record_t) * segment_count;
    // buffer_size += sizeof(struct m_junction_record_t) * junction_count;
    // buffer_size += sizeof(struct m_seg_junc_record_t) * w_seg_junc_count;

    // *file_buffer_size = buffer_size;
    // uint8_t *buffer = calloc(1, buffer_size);
    // *file_buffer = buffer;
    // uintptr_t out = (uintptr_t)buffer;

    // struct m_file_header_t *header = (struct m_file_header_t *)out;
    // out += sizeof(struct m_file_header_t);
    // strcpy(header->magic, M_FILE_HEADER_MAGIC);

    // header->devices = out;
    // out += sizeof(struct m_device_record_t) * device_count;
    // header->wires = out;
    // out += sizeof(struct m_wire_record_t) * wire_count;
    // header->segments = out;
    // out += sizeof(struct m_segment_record_t) * segment_count;
    // header->junctions = out;
    // out += sizeof(struct m_junction_record_t) * junction_count;
    // header->seg_juncs = out;
    // out += sizeof(struct m_seg_junc_record_t) * w_seg_junc_count;
    // // header->seg_juncs = out;
    

    // struct m_device_record_t *device_records = (struct m_device_record_t *)header->devices;
    // for(uint32_t device_index = 0; device_index < dev_devices.cursor; device_index++)
    // {
    //     struct dev_t *device = dev_GetDevice(device_index);
    //     if(device != NULL)
    //     {
    //         device->serialized_index = header->device_count;
    //         struct m_device_record_t *record = device_records + header->device_count;
    //         header->device_count++;

    //         record->position[0] = device->position.x;
    //         record->position[1] = device->position.y;
    //         // record->flip = device->flip;
    //         // record->angle = device->rotation;
    //         record->type = device->type;
    //         record->extra = 0;
    //     }
    // }

    // struct m_wire_record_t *wire_records = (struct m_wire_record_t *)header->wires;
    // struct m_segment_record_t *segment_records = (struct m_segment_record_t *)header->segments;
    // struct m_junction_record_t *junction_records = (struct m_junction_record_t *)header->junctions;
    // struct m_seg_junc_record_t *seg_junc_records = (struct m_seg_junc_record_t *)header->seg_juncs;

    // for(uint32_t wire_index = 0; wire_index < w_wires.cursor; wire_index++)
    // {
    //     struct wire_t *wire = w_GetWire(wire_index);
    //     if(wire != NULL)
    //     {
    //         struct m_wire_record_t *wire_record = wire_records + header->wire_count;
    //         header->wire_count++;

    //         wire_record->segments = header->segment_count;
    //         wire_record->junctions = header->junction_count;
    //         // wire_record->segment_count = header->segment_count;
    //         // wire_record->junction_count = header->junction_count;

    //         struct wire_seg_t *segment = wire->first_segment;
    //         while(segment != NULL)
    //         {    
    //             segment->serialized_index = header->segment_count;
    //             struct m_segment_record_t *segment_record = segment_records + header->segment_count;
    //             header->segment_count++;

    //             for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    //             {
    //                 segment_record->ends[tip_index].x = segment->ends[tip_index].x;
    //                 segment_record->ends[tip_index].y = segment->ends[tip_index].y;    
    //             }

    //             segment = segment->wire_next;
    //         }

    //         segment = wire->first_segment;
    //         while(segment != NULL)
    //         {   
    //             struct m_segment_record_t *segment_record = segment_records + segment->serialized_index;

    //             for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    //             {
    //                 if(segment->segments[tip_index] != NULL)
    //                 {
    //                     struct wire_seg_t *linked_segment = segment->segments[tip_index];
    //                     segment_record->segments[tip_index] = linked_segment->serialized_index;
    //                 }
    //                 else
    //                 {
    //                     segment_record->segments[tip_index] = WIRE_INVALID_WIRE;
    //                 }
    //             }

    //             segment = segment->wire_next;
    //         }

    //         struct wire_junc_t *junction = wire->first_junction;
    //         while(junction != NULL)
    //         {
    //             struct m_junction_record_t *junction_record = junction_records + header->junction_count;
    //             header->junction_count++;

    //             if(junction->pin.device != DEV_INVALID_DEVICE)
    //             {
    //                 struct dev_t *device = dev_GetDevice(junction->pin.device);
    //                 junction_record->device = device->serialized_index;
    //                 junction_record->pin = junction->pin.pin;
    //             }
    //             else
    //             {
    //                 junction_record->device = DEV_INVALID_DEVICE;
    //                 junction_record->pin = DEV_INVALID_PIN;
    //             }

    //             junction_record->first_segment = header->seg_junc_count;

    //             struct wire_seg_t *segment = junction->first_segment;
    //             while(segment)
    //             {
    //                 struct  m_seg_junc_record_t *seg_junc_record = seg_junc_records + header->seg_junc_count;
    //                 header->seg_junc_count++;
    //                 junction_record->segment_count++;

    //                 seg_junc_record->tip_index = segment->junctions[WIRE_SEG_END_INDEX].junction == junction;
    //                 seg_junc_record->segment = segment->serialized_index;
    //                 segment = segment->junctions[seg_junc_record->tip_index].next;
    //             }

    //             junction = junction->wire_next;
    //         }

    //         wire_record->segment_count = header->segment_count - wire_record->segments;
    //         wire_record->junction_count = header->junction_count - wire_record->junctions;
    //     }
    // }

    // header->devices -= (uintptr_t)buffer;
    // header->wires -= (uintptr_t)buffer;
    // header->segments -= (uintptr_t)buffer;
    // header->junctions -= (uintptr_t)buffer;
    // header->seg_juncs -= (uintptr_t)buffer;
}

void m_DeserializeCircuit(void *file_buffer, size_t file_buffer_size)
{
    // struct m_file_header_t *file_header = file_buffer;
    // if(!strcmp(file_header->magic, M_FILE_HEADER_MAGIC))
    // {
    //     file_header->devices += (uintptr_t)file_buffer;
    //     file_header->wires += (uintptr_t)file_buffer;
    //     file_header->segments += (uintptr_t)file_buffer;
    //     file_header->junctions += (uintptr_t)file_buffer;
    //     file_header->seg_juncs += (uintptr_t)file_buffer;
        
    //     struct m_device_record_t *device_records = (struct m_device_record_t *)file_header->devices;
    //     struct m_wire_record_t *wire_records = (struct m_wire_record_t *)file_header->wires;
    //     struct m_segment_record_t *segment_records = (struct m_segment_record_t *)file_header->segments;
    //     struct m_junction_record_t *junction_records = (struct m_junction_record_t *)file_header->junctions;
    //     struct m_seg_junc_record_t *seg_junc_records = (struct m_seg_junc_record_t *)file_header->seg_juncs;

    //     for(uint32_t index = 0; index < file_header->device_count; index++)
    //     {
    //         struct m_device_record_t *record = device_records + index;
    //         struct dev_t *device = dev_CreateDevice(record->type);
    //         device->position.x = record->position[0];
    //         device->position.y = record->position[1];
    //         // device->flip = record->flip;
    //         // device->rotation = record->angle;
    //         record->deserialized_index = device->element_index;
    //         // dev_UpdateDeviceRotation(device);
    //         elem_UpdateElement(device->element);
    //     }

    //     for(uint32_t index = 0; index < file_header->wire_count; index++)
    //     {
    //         struct m_wire_record_t *wire_record = wire_records + index;
    //         struct wire_t *wire = w_AllocWire();
    //         wire_record->deserialized_index = wire->element_index;
            
    //         for(uint32_t segment_index = 0; segment_index < wire_record->segment_count; segment_index++)
    //         {
    //             struct m_segment_record_t *segment_record = segment_records + wire_record->segments + segment_index;
    //             struct wire_seg_t *segment = w_AllocWireSegment(wire);
    //             segment_record->deserialized_index = segment->base.element_index;
    //             for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    //             {
    //                 segment->ends[tip_index].x = segment_record->ends[tip_index].x;
    //                 segment->ends[tip_index].y = segment_record->ends[tip_index].y;
    //             }
    //             // struct obj_t *object = obj_CreateObject(OBJECT_TYPE_SEGMENT, segment);
    //             elem_UpdateElement(segment->element);
    //         }

    //         for(uint32_t segment_index = 0; segment_index < wire_record->segment_count; segment_index++)
    //         {
    //             struct m_segment_record_t *segment_record = segment_records + wire_record->segments + segment_index;
    //             struct wire_seg_t *segment = pool_GetElement(&w_wire_segs, segment_record->deserialized_index);

    //             for(uint32_t tip_index = WIRE_SEG_START_INDEX; tip_index <= WIRE_SEG_END_INDEX; tip_index++)
    //             {
    //                 if(segment_record->segments[tip_index] != WIRE_INVALID_WIRE)
    //                 {
    //                     struct m_segment_record_t *linked_segment_record = segment_records + segment_record->segments[tip_index];
    //                     struct wire_seg_t *linked_segment = pool_GetElement(&w_wire_segs, linked_segment_record->deserialized_index);
    //                     segment->segments[tip_index] = linked_segment;
    //                 }
    //             }
    //         }

    //         for(uint32_t junction_index = 0; junction_index < wire_record->junction_count; junction_index++)
    //         {
    //             struct m_junction_record_t *junction_record = junction_records + wire_record->junctions + junction_index;
    //             struct m_seg_junc_record_t *seg_junc_record = seg_junc_records + junction_record->first_segment;
    //             struct wire_junc_t *junction = w_AllocWireJunction(wire);

    //             for(uint32_t segment_index = 0; segment_index < junction_record->segment_count; segment_index++)
    //             {
    //                 struct m_seg_junc_record_t *seg_junc = seg_junc_record + segment_index;
    //                 struct wire_seg_t *segment = pool_GetElement(&w_wire_segs, seg_junc->segment);
    //                 w_LinkSegmentToJunction(segment, junction, seg_junc->tip_index);
    //             }

    //             if(junction_record->device != DEV_INVALID_DEVICE)
    //             {
    //                 struct m_device_record_t *device_record = device_records + junction_record->device;
    //                 struct dev_t *device = dev_GetDevice(device_record->deserialized_index);
    //                 w_ConnectPinToJunction(junction, device, junction_record->pin);
    //             }
    //         }
    //     }

        
    // }
}

void m_SaveCircuit(const char *file_name)
{
    void *file_buffer;
    size_t file_buffer_size;
    m_SerializeCircuit(&file_buffer, &file_buffer_size);
    FILE *file = fopen(file_name, "wb");
    fwrite(file_buffer, file_buffer_size, 1, file);
    fclose(file);
    free(file_buffer);
}

void m_LoadCircuit(const char *file_name)
{
    FILE *file = fopen(file_name, "rb");
    if(file != NULL)
    {
        fseek(file, 0, SEEK_END);
        size_t file_buffer_size = ftell(file);
        rewind(file);

        void *file_buffer = calloc(1, file_buffer_size);
        fread(file_buffer, file_buffer_size, 1, file);
        fclose(file);
        m_DeserializeCircuit(file_buffer, file_buffer_size);
        free(file_buffer);
    }
}

void m_ClearCircuit()
{
    m_ClearSelections();
    m_ClearSelectedDeviceType();
    dev_ClearDevices();
    w_ClearWires();
    pool_Reset(&elem_elements[ELEM_TYPE_DEVICE]);
    pool_Reset(&elem_elements[ELEM_TYPE_SEGMENT]);
}

void m_UpdateFileNameAndPath()
{
    strncpy(m_file_name, m_save_load_args.file_name, sizeof(m_file_name) - 1);
    strncpy(m_file_path, m_save_load_args.file_path, sizeof(m_file_path) - 1);
    strncpy(m_file_full_path, m_save_load_args.full_path, sizeof(m_file_full_path) - 1);
}

uint32_t m_SaveCircuitExplorerCallback(struct m_explorer_state_t *explorer, void *args)
{
    struct m_explorer_save_load_args_t *save_args = (struct m_explorer_save_load_args_t *)args;

    if(!strstr(save_args->full_path, ".mos"))
    {
        strcat(save_args->full_path, ".mos");
    }

    if(file_Exists(save_args->full_path))
    {
        m_overwrite_modal_open = 1;
        return 0;
    }

    m_UpdateFileNameAndPath();
    m_SaveCircuit(save_args->full_path);

    return 1;
}

uint32_t m_LoadCircuitExplorerCallback(struct m_explorer_state_t *explorer, void *args)
{
    struct m_explorer_save_load_args_t *load_args = (struct m_explorer_save_load_args_t *)args;
    m_UpdateFileNameAndPath();
    m_LoadCircuit(load_args->full_path);
    return 1;
}

void m_SnapCoords(ivec2_t *coords, ivec2_t *snapped_coords)
{
    // for(uint32_t index = 0; index < 2; index++)
    // {
    //     snapped_coords[index] = coords[index] / M_SNAP_VALUE;

    //     if(coords[index] > 0.0f)
    //     {
    //         if(fmodf(coords[index], M_SNAP_VALUE) > M_SNAP_VALUE / 2.0f)
    //         {
    //             snapped_coords[index] = ceilf(snapped_coords[index]);
    //         }
    //         else
    //         {
    //             snapped_coords[index] = floorf(snapped_coords[index]);
    //         }
    //     }
    //     else
    //     {
    //         if(fmodf(coords[index], M_SNAP_VALUE) > -M_SNAP_VALUE / 2.0f)
    //         {
    //             snapped_coords[index] = ceilf(snapped_coords[index]);
    //         }
    //         else
    //         {
    //             snapped_coords[index] = floorf(snapped_coords[index]);
    //         }
    //     }

    //     snapped_coords[index] *= M_SNAP_VALUE;
    // }
    if(coords->x > 0)
    {
        snapped_coords->x = M_SNAP_VALUE * ((coords->x / M_SNAP_VALUE) + ((coords->x % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
    }
    else
    {
        snapped_coords->x = M_SNAP_VALUE * ((coords->x / M_SNAP_VALUE) - ((abs(coords->x) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
    }

    if(coords->y > 0)
    {
        snapped_coords->y = M_SNAP_VALUE * ((coords->y / M_SNAP_VALUE) + ((coords->y % M_SNAP_VALUE) > M_SNAP_VALUE / 2));
    }
    else
    {
        snapped_coords->y = M_SNAP_VALUE * ((coords->y / M_SNAP_VALUE) - ((abs(coords->y) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
    }
}

void m_SimulationState()
{

}

void m_SelectionBoxState()
{
    if(igIsMouseReleased_Nil(ImGuiMouseButton_Left))
    {
        m_draw_selection_box = 0;
        m_cur_state = M_STATE_EDIT;
        return;
    }

    m_draw_selection_box = 1;

    for(uint32_t index = 0; index < 2; index++)
    {
        if(m_clicked_mouse_pos.comps[index] > m_mouse_pos.comps[index])
        {
            m_selection_box_min.comps[index] = m_mouse_pos.comps[index];
            m_selection_box_max.comps[index] = m_clicked_mouse_pos.comps[index];
        }
        else
        {
            m_selection_box_max.comps[index] = m_mouse_pos.comps[index];
            m_selection_box_min.comps[index] = m_clicked_mouse_pos.comps[index];
        }
    }

    m_ClearSelections();

    m_elements_in_box.cursor = 0;
    elem_GetTypedElementsInsideBox(ELEM_TYPE_SEGMENT, &(vec2_t){m_selection_box_min.x, m_selection_box_min.y}, 
                                                      &(vec2_t){m_selection_box_max.x, m_selection_box_max.y}, 
                                                      &m_elements_in_box);

    for(uint32_t index = 0; index < m_elements_in_box.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, index);
        m_SelectElement(element, 1);
    }

    m_elements_in_box.cursor = 0;
    elem_GetTypedElementsInsideBox(ELEM_TYPE_DEVICE, &(vec2_t){m_selection_box_min.x, m_selection_box_min.y}, 
                                                      &(vec2_t){m_selection_box_max.x, m_selection_box_max.y}, 
                                                      &m_elements_in_box);

    for(uint32_t index = 0; index < m_elements_in_box.cursor; index++)
    {
        struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, index);
        m_SelectElement(element, 1);
    }
}

void m_EditState()
{
    if(igIsPopupOpen_Str(M_OVERWRITE_MODAL_WINDOW_NAME, 0))
    {
        return;
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

    if(igIsKeyDown_Nil(ImGuiKey_LeftArrow))
    {
        m_offset_x -= 5.0f;
    }
    if(igIsKeyDown_Nil(ImGuiKey_RightArrow))
    {
        m_offset_x += 5.0f;
    }
    if(igIsKeyDown_Nil(ImGuiKey_UpArrow))
    {
        m_offset_y += 5.0f;
    }
    if(igIsKeyDown_Nil(ImGuiKey_DownArrow))
    {
        m_offset_y -= 5.0f;
    }

    switch(m_cur_edit_func)
    { 
        case M_EDIT_FUNC_PLACE:
            // m_place_device_x = m_snapped_mouse_pos.x;
            // m_place_device_y = m_snapped_mouse_pos.y;
            m_place_device_pos = m_snapped_mouse_pos;
            if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable() && !m_run_simulation)
            {
                if(m_selected_device_type != DEV_DEVICE_LAST)
                {
                    struct dev_t *device = dev_CreateDevice(m_selected_device_type);
                    // device->position.x = m_place_device_x;
                    // device->position.y = m_place_device_y;
                    device->position = m_place_device_pos;
                    elem_UpdateElement(device->element);
                }
            }
        break;

        case M_EDIT_FUNC_SELECT:

            if(ui_IsMouseAvailable())
            {
                if(!m_run_simulation && !m_single_step)
                {
                    if(igIsMouseDown_Nil(ImGuiMouseButton_Left))
                    {
                        if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0))
                        {
                            // m_clicked_mouse_pos.x = m_mouse_pos.x;
                            // m_clicked_mouse_pos.y = m_mouse_pos.y;
                            m_clicked_mouse_pos = m_mouse_pos;
                        }

                        int32_t box_width = abs(m_mouse_pos.x - m_clicked_mouse_pos.x);
                        int32_t box_height = abs(m_mouse_pos.y - m_clicked_mouse_pos.y);

                        if(abs(m_mouse_pos.x - m_clicked_mouse_pos.x) == 0 || abs(m_mouse_pos.y - m_clicked_mouse_pos.y) == 0)
                        {
                            m_selection_box_min.x = m_clicked_mouse_pos.x;
                            m_selection_box_min.y = m_clicked_mouse_pos.y;
                            m_selection_box_max.x = m_mouse_pos.x;
                            m_selection_box_max.y = m_mouse_pos.y;
                        }
                        else
                        {
                            m_cur_state = M_STATE_SELECTION_BOX;

                            // m_draw_selection_box = 1;

                            // for(uint32_t index = 0; index < 2; index++)
                            // {
                            //     if(m_clicked_mouse_pos[index] > m_mouse_pos[index])
                            //     {
                            //         m_selection_box_min[index] = m_mouse_pos[index];
                            //         m_selection_box_max[index] = m_clicked_mouse_pos[index];
                            //     }
                            //     else
                            //     {
                            //         m_selection_box_max[index] = m_mouse_pos[index];
                            //         m_selection_box_min[index] = m_clicked_mouse_pos[index];
                            //     }
                            // }
                        }
                    }

                    if(igIsMouseReleased_Nil(ImGuiMouseButton_Left))
                    {
                        uint32_t multiple = igIsKeyDown_Nil(ImGuiKey_LeftShift) || m_draw_selection_box;

                        m_elements_in_box.cursor = 0;
                        elem_GetTypedElementsInsideBox(ELEM_TYPE_SEGMENT, &(vec2_t){m_selection_box_min.x, m_selection_box_min.y}, 
                                                                          &(vec2_t){m_selection_box_max.x, m_selection_box_max.y}, 
                                                                          &m_elements_in_box);
                        if(m_elements_in_box.cursor != 0)
                        {
                            struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, 0);
                            m_SelectElement(element, multiple);
                        }
                        else
                        {
                            m_elements_in_box.cursor = 0;
                            elem_GetTypedElementsInsideBox(ELEM_TYPE_DEVICE, &(vec2_t){m_selection_box_min.x, m_selection_box_min.y}, 
                                                                             &(vec2_t){m_selection_box_max.x, m_selection_box_max.y}, 
                                                                             &m_elements_in_box);

                            if(m_elements_in_box.cursor != 0)
                            {
                                struct elem_t *element = *(struct elem_t **)list_GetElement(&m_elements_in_box, 0);
                                m_SelectElement(element, multiple);
                            }
                        }
                    }
                }
                else if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0))
                {
                    struct dev_input_t *input = m_GetInputUnderMouse();
                    dev_ToggleInput(input);
                }
                // if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0))
                // {
                //     if(!m_run_simulation)
                //     {
                //         uint32_t multiple = igIsKeyDown_Nil(ImGuiKey_LeftShift);
                //         obj_GetTypedObjectsInsideBox(OBJECT_TYPE_DEVICE, (int32_t []){m_mouse_pos[0], m_mouse_pos[1]}, (int32_t []){m_mouse_pos[0], m_mouse_pos[1]});
                //         if(obj_objects_in_box.cursor != 0)
                //         {
                //             struct obj_t *object = *(struct obj_t **)list_GetElement(&obj_objects_in_box, 0);
                //             m_SelectObject(object, multiple);
                //         }
                //         else
                //         {
                //             obj_GetTypedObjectsInsideBox(OBJECT_TYPE_SEGMENT, (int32_t []){m_mouse_pos[0], m_mouse_pos[1]}, (int32_t []){m_mouse_pos[0], m_mouse_pos[1]});
                //             if(obj_objects_in_box.cursor != 0)
                //             {
                //                 struct obj_t *object = *(struct obj_t **)list_GetElement(&obj_objects_in_box, 0);
                //                 m_SelectObject(object, multiple);
                //             }
                //         }
                //     }
                //     else
                //     {
                //         struct dev_input_t *input = m_GetInputUnderMouse();
                //         dev_ToggleInput(input);
                //     }
                // }
                if(!m_run_simulation && igIsMouseDown_Nil(ImGuiMouseButton_Right))
                {
                    // int32_t dx = m_snapped_mouse_pos.x - m_place_device_pos.x;
                    // int32_t dy = m_snapped_mouse_pos.y - m_place_device_pos.y;
                    ivec2_t translation;
                    ivec2_t_sub(&translation, &m_snapped_mouse_pos, &m_place_device_pos);
                    
                    if(translation.x != 0)
                    {
                        m_place_device_pos.x = m_snapped_mouse_pos.x;
                    }

                    if(translation.y != 0)
                    {
                        m_place_device_pos.y = m_snapped_mouse_pos.y;
                    }

                    if(igIsMouseClicked_Bool(ImGuiMouseButton_Right, 0))
                    {
                        translation.x = 0;
                        translation.y = 0;
                    }

                    m_TranslateSelections(&translation);
                }
            }

            if(igIsKeyPressed_Bool(ImGuiKey_Escape, 0))
            {
                m_ClearSelections();
            }
        break;

        case M_EDIT_FUNC_WIRE:
        {
            ivec2_t next_segment_pos = m_snapped_mouse_pos;

            if(m_cur_wire_segment != NULL)
            {
                int32_t dx = m_cur_wire_segment->position.x - m_snapped_mouse_pos.x;
                int32_t dy = m_cur_wire_segment->position.y - m_snapped_mouse_pos.y;
                if(abs(dx) > abs(dy))
                {
                    // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].x = m_snapped_mouse_pos.x;
                    // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].y = m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].y;
                    next_segment_pos.y = m_cur_wire_segment->position.y;
                }
                else
                {
                    next_segment_pos.x = m_cur_wire_segment->position.x;
                    // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].x = m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].x;
                    // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].y = m_snapped_mouse_pos.y;
                }
            }

            if(igIsMouseClicked_Bool(ImGuiMouseButton_Left, 0) && ui_IsMouseAvailable())
            {
                // struct wire_seg_pos_t *prev_segment = NULL;
                // if(m_cur_wire_segment == NULL || m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] != m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] ||
                //                                  m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] != m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1])
                // {
                //     prev_segment = m_cur_wire_segment;
                //     uint64_t segment_index = list_AddElement(&m_wire_seg_pos, NULL);
                //     m_cur_wire_segment = list_GetElement(&m_wire_seg_pos, segment_index);

                //     if(prev_segment != NULL)
                //     {
                //         m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] = prev_segment->ends[WIRE_SEG_END_INDEX][0];
                //         m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] = prev_segment->ends[WIRE_SEG_END_INDEX][1];
                //     }
                //     else
                //     {
                //         m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0] = m_snapped_mouse_pos[0];
                //         m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1] = m_snapped_mouse_pos[1];
                //     }

                //     m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][0] = m_snapped_mouse_pos[0];
                //     m_cur_wire_segment->ends[WIRE_SEG_END_INDEX][1] = m_snapped_mouse_pos[1];
                // }

                // union m_wire_seg_t next_segment = {};
                

                m_picked_elements[m_wire_func_stage] = m_GetSegmentUnderMouse();

                // if(m_picked_elements[m_wire_func_stage].element == NULL)
                // {
                //     m_picked_elements[m_wire_func_stage] = m_GetPinUnderMouse();                    
                // }

                // if(m_picked_elements[m_wire_func_stage].element != NULL)
                // {
                //     // struct dev_t *device = picked_objects[m_wire_func_stage].object->object;
                //     // printf("clicked on pin %d of device %d\n", picked_objects[m_wire_func_stage].index, device->element_index);
                //     printf("clicked on object %p of type %d\n", m_picked_elements[m_wire_func_stage].element, m_picked_elements[m_wire_func_stage].element->type);
                //     switch(m_picked_elements[m_wire_func_stage].element->type)
                //     {
                //         case ELEM_TYPE_DEVICE:
                //         {
                //             // vec2_t pin_position[2];
                //             struct dev_t *device = m_picked_elements[m_wire_func_stage].element->base_object;
                //             dev_GetDeviceLocalPinPosition(device, m_picked_elements[m_wire_func_stage].index, &next_segment_pos);

                //             // union m_wire_seg_t *segment = m_AppendSegment(pin_position);
                //             // next_segment_pos[0] += device->position.x;
                //             // next_segment_pos[1] += device->position.y;
                //             ivec2_t_add(&next_segment_pos, &next_segment_pos, &device->position);
                //             m_SnapCoords(&next_segment_pos, &next_segment_pos);

                //             // next_segment.seg_pos.ends[WIRE_SEG_END_INDEX][0] = next_segment.seg_pos.ends[WIRE_SEG_START_INDEX][0];
                //             // next_segment.seg_pos.ends[WIRE_SEG_END_INDEX][1] = next_segment.seg_pos.ends[WIRE_SEG_START_INDEX][1];

                //             // if(prev_segment != NULL)
                //             // {
                //             //     prev_segment->ends[WIRE_SEG_END_INDEX][0] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][0];
                //             //     prev_segment->ends[WIRE_SEG_END_INDEX][1] = m_cur_wire_segment->ends[WIRE_SEG_START_INDEX][1];
                //             // }
                //         }
                //         break;

                //         case ELEM_TYPE_SEGMENT:
                //         {
                //             struct wire_seg_t *segment = m_picked_elements[m_wire_func_stage].element->base_object;
                //             next_segment_pos.x = m_snapped_mouse_pos.x;
                //             next_segment_pos.y = m_snapped_mouse_pos.y;
                //         }
                //         break;
                //     }

                //     m_wire_func_stage++;
                // }
                // else
                // {
                    // m_AppendSegment(m_snapped_mouse_pos);
                    // next_segment_pos.x = m_snapped_mouse_pos.x;
                    // next_segment_pos.y = m_snapped_mouse_pos.y;
                // }

                // if(m_wire_func_stage == 2)
                // {
                //     /* get rid of the last segment created */
                //     // m_wire_seg_pos.cursor--;

                //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].x = next_segment_pos.x;
                //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].y = next_segment_pos.y;

                //     if(m_picked_elements[0].element != m_picked_elements[1].element || m_picked_elements[0].index != m_picked_elements[1].index)
                //     {
                //         struct wire_t *wire = m_CreateWire(&m_picked_elements[0], &m_picked_elements[1], &m_wire_seg_pos);
                //         printf("wire %p (%d) between pins %d and %d of devices %p and %p\n", wire, wire->element_index, m_picked_elements[0].index, 
                //                                                                                                         m_picked_elements[1].index, 
                //                                                                                                         m_picked_elements[0].element, 
                //                                                                                                         m_picked_elements[1].element);
                //     }

                //     m_ClearWireSegments();
                // }
                // else

                uint32_t prev_seg_count = m_wire_segs.cursor;

                {
                    if(m_cur_wire_segment == NULL || !ivec2_t_equal(&m_cur_wire_segment->position, &next_segment_pos))
                    {
                        union m_wire_seg_t *prev_segment = m_cur_wire_segment;
                        uint64_t segment_index = list_AddElement(&m_wire_segs, NULL);
                        m_cur_wire_segment = list_GetElement(&m_wire_segs, segment_index);

                        // if(prev_segment != NULL)
                        // {
                        //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].x = prev_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].x;
                        //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].y = prev_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].y;
                        // }
                        // else
                        // {
                        //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].x = next_segment_pos.x;
                        //     m_cur_wire_segment->seg_pos.ends[WIRE_SEG_HEAD_INDEX].y = next_segment_pos.y;
                        // }

                        // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].x = next_segment_pos.x;
                        // m_cur_wire_segment->seg_pos.ends[WIRE_SEG_TAIL_INDEX].y = next_segment_pos.y;
                        m_cur_wire_segment->position = next_segment_pos;
                    }
                }

                if(igIsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if(m_wire_segs.cursor > prev_seg_count)
                    {
                        m_wire_segs.cursor--;
                    }

                    m_CreateWire(NULL, NULL, &m_wire_segs);
                    m_ClearWireSegments();
                }
            }

            if(igIsKeyPressed_Bool(ImGuiKey_Escape, 0))
            {
                m_ClearWireSegments();
            }
        }
        break;
    }
}

void m_OpenExplorer(struct m_explorer_state_t *explorer, const char *path, uint32_t mode, m_explorer_callback_t *SaveCallback, m_explorer_callback_t *LoadCallback, void *data)
{
    m_ChangeDir(explorer, path);
    explorer->SaveCallback = SaveCallback;
    explorer->LoadCallback = LoadCallback;
    explorer->data = data;
    explorer->mode = mode;
    m_FilterEntries(explorer);
    m_SortEntries(explorer);
    m_cur_state = M_STATE_EXPLORER;
}

void m_CloseExplorer(struct m_explorer_state_t *explorer)
{
    // explorer->open = 0;
}

void m_Back(struct m_explorer_state_t *explorer)
{

}

void m_ChangeDir(struct m_explorer_state_t *explorer, const char *path)
{
    if(file_OpenDir(path, &explorer->current_dir))
    {
        strcpy(explorer->current_path, explorer->current_dir.path);
        m_explorer_state.search_bar[0] = '\0';
        m_FilterEntries(explorer);
        m_SortEntries(explorer);
    }
}

void m_FilterEntries(struct m_explorer_state_t *explorer)
{
    explorer->filtered_entries.cursor = 0;
    if(explorer->search_bar[0] != '\0')
    {
        uint32_t match_size = strlen(explorer->search_bar);
        for(uint32_t entry_index = 0; entry_index < explorer->current_dir.entries.cursor; entry_index++)
        {
            struct file_dir_ent_t *entry = list_GetElement(&explorer->current_dir.entries, entry_index);
            char *match_start = strstr(entry->name, explorer->search_bar);
            if(match_start != NULL)
            {
                uint64_t index = list_AddElement(&explorer->filtered_entries, NULL);
                struct m_filtered_dir_ent_t *filtered_entry = list_GetElement(&explorer->filtered_entries, index);
                filtered_entry->entry = entry;
                filtered_entry->match_start = match_start - entry->name;
                filtered_entry->match_size = match_size;
                filtered_entry->selected = 0;
            }
        }
    }
    else
    {
        for(uint32_t entry_index = 0; entry_index < explorer->current_dir.entries.cursor; entry_index++)
        {
            struct file_dir_ent_t *entry = list_GetElement(&explorer->current_dir.entries, entry_index);
            uint64_t index = list_AddElement(&explorer->filtered_entries, NULL);
            struct m_filtered_dir_ent_t *filtered_entry = list_GetElement(&explorer->filtered_entries, index);
            filtered_entry->entry = entry;
            filtered_entry->selected = 0;
        }
    }
}

int32_t m_CompareEntriesAsc(const void *a, const void *b)
{
    struct m_filtered_dir_ent_t *dir_a = (struct m_filtered_dir_ent_t *)a;
    struct m_filtered_dir_ent_t *dir_b = (struct m_filtered_dir_ent_t *)b;
    return strcmp(dir_a->entry->name, dir_b->entry->name);
}

int32_t m_CompareEntriesDesc(const void *a, const void *b)
{
    struct m_filtered_dir_ent_t *dir_a = (struct m_filtered_dir_ent_t *)a;
    struct m_filtered_dir_ent_t *dir_b = (struct m_filtered_dir_ent_t *)b;
    return strcmp(dir_b->entry->name, dir_a->entry->name);
}

void m_SortEntries(struct m_explorer_state_t *explorer)
{
    switch(explorer->sort_dir)
    {
        case M_EXPLORER_SORT_DIRECTION_ASC:
            list_Qsort(&explorer->filtered_entries, m_CompareEntriesAsc);
        break;

        case M_EXPLORER_SORT_DIRECTION_DESC:
            list_Qsort(&explorer->filtered_entries, m_CompareEntriesDesc);
        break;
    }
}

void m_ExplorerSave()
{
    if(m_explorer_state.file_name[0] != '\0')
    {
        m_save_load_args.file_name = m_explorer_state.file_name;
        m_save_load_args.file_path = m_explorer_state.current_path;
        strncpy(m_save_load_args.full_path, m_save_load_args.file_path, sizeof(m_save_load_args.full_path) - 1);
        strncat(m_save_load_args.full_path, "/", sizeof(m_save_load_args.full_path) - 1);
        strncat(m_save_load_args.full_path, m_save_load_args.file_name, sizeof(m_save_load_args.full_path) - 1);
        if(m_explorer_state.SaveCallback(&m_explorer_state, &m_save_load_args))
        {
            m_cur_state = M_STATE_EDIT;
        }
    }
}

void m_ExplorerLoad()
{
    if(m_explorer_state.file_name[0] != '\0')
    {
        m_save_load_args.file_name = m_explorer_state.file_name;
        m_save_load_args.file_path = m_explorer_state.current_path;
        strncpy(m_save_load_args.full_path, m_save_load_args.file_path, sizeof(m_save_load_args.full_path) - 1);
        strncat(m_save_load_args.full_path, "/", sizeof(m_save_load_args.full_path) - 1);
        strncat(m_save_load_args.full_path, m_save_load_args.file_name, sizeof(m_save_load_args.full_path) - 1);
        if(m_explorer_state.LoadCallback(&m_explorer_state, &m_save_load_args))
        {
            m_cur_state = M_STATE_EDIT;
        }
    }
}

void m_ExplorerState()
{
    static char matched_entry[FILE_MAX_PATH_LEN];
    igSetNextWindowPos((ImVec2){0, 18}, 0, (ImVec2){});
    igSetNextWindowSize((ImVec2){m_window_width, m_window_height - 18}, 0);

    if(igBegin("##explorer", NULL, ImGuiWindowFlags_NoDecoration))
    {
        
        if(igIsKeyDown_Nil(ImGuiKey_Escape) && igIsWindowHovered(0))
        {
            m_explorer_state.search_bar[0] = '\0';
            m_explorer_state.file_name[0] = '\0';
            m_FilterEntries(&m_explorer_state);
            m_SortEntries(&m_explorer_state);
        }

        ImVec2 window_size;
        igGetWindowSize(&window_size);
        igSetNextItemWidth(window_size.x - 60);
        if(igInputText("##current_dir", m_explorer_state.current_path, sizeof(m_explorer_state.current_path), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL))
        {
            m_ChangeDir(&m_explorer_state, m_explorer_state.current_path);
        }
        igSameLine(0.0f, 4.0f);
        if(igButton("Cancel", (ImVec2){48, 0}))
        {
            m_cur_state = M_STATE_EDIT;
        }
        
        igSetNextItemWidth(window_size.x - 320);
        if(igInputText("##file_name", m_explorer_state.file_name, sizeof(m_explorer_state.file_name), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL))
        {
            // ed_ChangeDir(explorer, explorer->current_path);
        }
        igSameLine(0.0f, 4.0f);
        ImVec2 cursor_pos;
        igGetCursorPos(&cursor_pos);
        igSetNextItemWidth(window_size.x - cursor_pos.x - 56);
        if(igInputText("##search_bar", m_explorer_state.search_bar, sizeof(m_explorer_state.search_bar), 0, NULL, NULL))
        {
            m_FilterEntries(&m_explorer_state);
            m_SortEntries(&m_explorer_state);
        }
        igSameLine(0.0f, 4.0f);

        switch(m_explorer_state.mode)
        {
            case M_EXPLORER_MODE_LOAD:
                if(igButton("Open", (ImVec2){48, 0}))
                {
                    m_ExplorerLoad();
                }
            break;

            case M_EXPLORER_MODE_SAVE:
                if(igButton("Save", (ImVec2){48, 0}))
                {
                    m_ExplorerSave();
                }
            break;
        }
        

        igGetCursorPos(&cursor_pos);

        if(igBeginTable("##entries_table", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, (ImVec2){0, m_window_height - cursor_pos.y - 24}, 0.0f))
        {
            igTableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, 0);
            igTableSetupColumn("Type", 0, 0.0f, 0);
            igTableSetupScrollFreeze(2, 1);
            igTableHeadersRow();
            ImGuiTableSortSpecs *sort_specs = igTableGetSortSpecs();

            if(sort_specs->SpecsDirty)
            {
                switch(sort_specs->Specs[0].SortDirection)
                {
                    case ImGuiSortDirection_Ascending:
                        m_explorer_state.sort_dir = M_EXPLORER_SORT_DIRECTION_ASC;
                    break;

                    case ImGuiSortDirection_Descending:
                        m_explorer_state.sort_dir = M_EXPLORER_SORT_DIRECTION_DESC;
                    break;
                }

                m_SortEntries(&m_explorer_state);

                sort_specs->SpecsDirty = 0;
            }
            
            for(uint32_t index = 0; index < m_explorer_state.filtered_entries.cursor; index++)
            {
                struct m_filtered_dir_ent_t *entry = list_GetElement(&m_explorer_state.filtered_entries, index);
                igTableNextRow(0, 0.0f);

                igTableNextColumn();
                if(m_explorer_state.search_bar[0] != '\0')
                {
                    strncpy(matched_entry, entry->entry->name, entry->match_start);
                    matched_entry[entry->match_start] = '\0';
                    igTextColored((ImVec4){1, 1, 1, 1}, matched_entry);
                    igSameLine(0, 0);
                    strncpy(matched_entry, entry->entry->name + entry->match_start, entry->match_size);
                    matched_entry[entry->match_size] = '\0';
                    igTextColored((ImVec4){1, 1 , 0, 1}, matched_entry);
                    igSameLine(0, 0);
                    strncpy(matched_entry, entry->entry->name + entry->match_start + entry->match_size, FILE_MAX_PATH_LEN);
                    igTextColored((ImVec4){1, 1, 1, 1}, matched_entry);
                    igSameLine(0, 0);
                }
                else
                {
                    igText(entry->entry->name);
                }
                
                igTableNextColumn();
                igPushID_Int(index);

                const char *label;
                
                switch(entry->entry->type)
                {
                    case FILE_DIR_ENT_TYPE_FILE:
                        label = "File";
                    break;

                    case FILE_DIR_ENT_TYPE_DIR:
                        label = "Dir";
                    break;
                }

                if(igSelectable_Bool(label, entry->selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, (ImVec2){0, 0}))
                {
                    if(igIsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if(entry->entry->type == FILE_DIR_ENT_TYPE_DIR)
                        {
                            strcpy(matched_entry, m_explorer_state.current_dir.path);
                            strcat(matched_entry, "/");
                            strcat(matched_entry, entry->entry->name);
                            m_ChangeDir(&m_explorer_state, matched_entry);
                        }
                        else if(m_explorer_state.file_name[0] != '\0')
                        {
                            switch(m_explorer_state.mode)
                            {
                                case M_EXPLORER_MODE_LOAD:
                                    m_ExplorerLoad();
                                break;

                                case M_EXPLORER_MODE_SAVE:
                                    m_ExplorerSave();
                                break;
                            }
                        }
                    }
                    else
                    {
                        if(!entry->selected)
                        {
                            for(uint32_t index = 0; index < m_explorer_state.filtered_entries.cursor; index++)
                            {
                                struct m_filtered_dir_ent_t *selected_entry = list_GetElement(&m_explorer_state.filtered_entries, index);
                                selected_entry->selected = 0;
                            }
                        }
                        else
                        {
                            
                        }

                        strcpy(m_explorer_state.file_name, entry->entry->name);
                        entry->selected = 1;
                    }
                }
                
                igPopID();
            }
            // printf("%d\n", igTableGetColumnFlags(0));

            igEndTable();
        }
    }
    igEnd();
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("oh, shit...\n");
        return -1;
    }

    getcwd(m_work_dir, sizeof(m_work_dir));
    strcpy(m_file_path, m_work_dir);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow("simoslator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_window_width, m_window_height, SDL_WINDOW_OPENGL);
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
    m_cursor_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/play.png", &width, &height, &channels, STBI_rgb_alpha);
    m_play_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/play_pause.png", &width, &height, &channels, STBI_rgb_alpha);
    m_step_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/pause.png", &width, &height, &channels, STBI_rgb_alpha);
    m_pause_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/connected.png", &width, &height, &channels, STBI_rgb_alpha);
    m_wire_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/rotate.png", &width, &height, &channels, STBI_rgb_alpha);
    m_rotate_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/fliph.png", &width, &height, &channels, STBI_rgb_alpha);
    m_fliph_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    pixels = stbi_load("res/flipv.png", &width, &height, &channels, STBI_rgb_alpha);
    m_flipv_texture = d_CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST, 4, GL_RGBA, pixels);
    free(pixels);

    m_selections = list_Create(sizeof(struct elem_t *), 512);
    m_wire_segs = list_Create(sizeof(union m_wire_seg_t), 512);
    m_elements_in_box = list_Create(sizeof(struct elem_t *), 512);
    elem_elements[ELEM_TYPE_DEVICE] = pool_CreateTyped(struct elem_t, 16384);
    elem_dbvt[ELEM_TYPE_DEVICE] = dbvt_Create();
    elem_elements[ELEM_TYPE_SEGMENT] = pool_CreateTyped(struct elem_t, 16384);
    elem_dbvt[ELEM_TYPE_SEGMENT] = dbvt_Create();

    m_explorer_state.filtered_entries = list_Create(sizeof(struct m_filtered_dir_ent_t), 128);
    m_explorer_state.search_bar[0] = '\0';
    m_explorer_state.current_dir.path[0] = '\0';

    d_Init();
    ui_Init();
    dev_Init();
    w_Init();
    sim_Init();

    glClearColor(0.8, 0.8, 0.8, 1);
    glClearDepth(1);

    m_selected_device_type = DEV_DEVICE_LAST;
    m_cur_edit_func = M_EDIT_FUNC_SELECT;
    
    // struct m_selected_pin_t pins[2] = {};
    m_run = 1;
    while(!in_ReadInput() && m_run)
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        ui_BeginFrame();
        if(igBeginMainMenuBar())
        {
            if(igBeginMenu("File", 1))
            {
                if(igMenuItem_Bool("New", NULL, 0, 1))
                {
                    m_ClearCircuit();
                }

                if(igMenuItem_Bool("Save", NULL, 0, 1))
                {
                    strncpy(m_explorer_state.file_name, m_file_name, sizeof(m_explorer_state.file_name) - 1);
                    m_OpenExplorer(&m_explorer_state, m_file_path, M_EXPLORER_MODE_SAVE, m_SaveCircuitExplorerCallback, m_LoadCircuitExplorerCallback, NULL);
                }

                if(igMenuItem_Bool("Load", NULL, 0, 1))
                {
                    m_explorer_state.file_name[0] = '\0';
                    m_OpenExplorer(&m_explorer_state, m_file_path, M_EXPLORER_MODE_LOAD, m_SaveCircuitExplorerCallback, m_LoadCircuitExplorerCallback, NULL);
                }

                if(igMenuItem_Bool("Quit", NULL, 0, 1))
                {
                    m_run = 0;
                }

                igEndMenu();
            }
            igEndMainMenuBar();
        }

        if(igIsKeyDown_Nil(ImGuiKey_LeftCtrl))
        {
            if(igIsKeyPressed_Bool(ImGuiKey_S, 0))
            {
                if(file_Exists(m_file_full_path))
                {
                    m_overwrite_modal_open = 1;
                }
                else
                {
                    m_OpenExplorer(&m_explorer_state, m_file_path, M_EXPLORER_MODE_SAVE, m_SaveCircuitExplorerCallback, m_LoadCircuitExplorerCallback, NULL);
                }
            }
        }

        if(m_overwrite_modal_open)
        {
            if(!igIsPopupOpen_Str(M_OVERWRITE_MODAL_WINDOW_NAME, 0));
            {
                igOpenPopup_Str(M_OVERWRITE_MODAL_WINDOW_NAME, 0);
            }

            igSetNextWindowSize((ImVec2){200, 100}, 0);
            igSetNextWindowPos((ImVec2){m_window_width / 2, m_window_height / 2}, 0, (ImVec2){0.5, 0.5});
            if(igBeginPopupModal(M_OVERWRITE_MODAL_WINDOW_NAME, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                igTextWrapped("File already exists. Do you want to overwrite it?");
                if(igButton("Yes", (ImVec2){80, 0}))
                {
                    m_UpdateFileNameAndPath();
                    m_SaveCircuit(m_file_full_path);
                    m_cur_state = M_STATE_EDIT;
                    m_overwrite_modal_open = 0;
                }
                igSameLine(0, -1);
                if(igButton("No", (ImVec2){80, 0}))
                {
                    m_cur_state = M_STATE_EDIT;
                    m_overwrite_modal_open = 0;
                }

                if(m_overwrite_modal_open == 0)
                {
                    igCloseCurrentPopup();   
                }

                igEndPopup();
            }
        }

        m_mouse_pos.x = (in_mouse_x - d_model_view_projection_matrix[12]) / d_model_view_projection_matrix[0];
        m_mouse_pos.y = (in_mouse_y - d_model_view_projection_matrix[13]) / d_model_view_projection_matrix[5];

        m_SnapCoords(&m_mouse_pos, &m_snapped_mouse_pos);

        // if(m_mouse_pos[0] > 0)
        // {
        //     m_snapped_mouse_x = M_SNAP_VALUE * ((m_mouse_pos[0] / M_SNAP_VALUE) + ((m_mouse_pos[0] % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        // }
        // else
        // {
        //     m_snapped_mouse_x = M_SNAP_VALUE * ((m_mouse_pos[0] / M_SNAP_VALUE) - ((abs(m_mouse_pos[0]) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        // }

        // if(m_mouse_pos[1] > 0)
        // {
        //     m_snapped_mouse_y = M_SNAP_VALUE * ((m_mouse_pos[1] / M_SNAP_VALUE) + ((m_mouse_pos[1] % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        // }
        // else
        // {
        //     m_snapped_mouse_y = M_SNAP_VALUE * ((m_mouse_pos[1] / M_SNAP_VALUE) - ((abs(m_mouse_pos[1]) % M_SNAP_VALUE) > M_SNAP_VALUE / 2));    
        // }


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
                if(m_window_height + m_zoom > 150.0f)
                {
                    m_zoom -= 150.0f;
                }
                else
                {
                    m_zoom = 150.0f - m_window_height;
                }
            }

            igSameLine(0, -1);

            if(igButton("-", (ImVec2){28, 28}))
            {
                if(m_zoom < 5000.0f)
                {
                    m_zoom += 150.0f;
                }
            }

            igSameLine(0, -1);
            if(m_run_simulation)
            {
                if(igImageButton("##stop_simulation", (void *)(uintptr_t)m_pause_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    m_run_simulation = 0;
                    m_single_step = 0;
                    sim_StopSimulation();
                }
            }
            else
            {
                if(igImageButton("##begin_simulation", (void *)(uintptr_t)m_play_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
                {
                    m_run_simulation = 1;
                    if(!m_single_step)
                    {
                        sim_BeginSimulation();
                    }
                    m_SetEditFunc(M_EDIT_FUNC_SELECT);
                    m_single_step = 0;
                }
            }

            igSameLine(0, -1);
            if(igImageButton("##step_simulation", (void *)(uintptr_t)m_step_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                if(!m_single_step && !m_run_simulation)
                {
                    sim_BeginSimulation();
                }
                m_SetEditFunc(M_EDIT_FUNC_SELECT);
                m_run_simulation = 1;
                m_single_step = 1;
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
                // for(uint32_t index = 0; index < m_selections.cursor; index++)
                // {
                //     struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
                //     struct dev_t *device = element->base_object;
                //     dev_RotateDevice(device, 1);
                // }

                m_RotateSelections(1);
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
                // for(uint32_t index = 0; index < m_selections.cursor; index++)
                // {
                //     struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
                //     struct dev_t *device = element->base_object;
                //     dev_RotateDevice(device, 0);
                // }
                m_RotateSelections(0);
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
            if(igImageButton("##flip_v", (void *)(uintptr_t)m_fliph_texture, (ImVec2){24, 24}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                // for(uint32_t index = 0; index < m_selections.cursor; index++)
                // {
                //     struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
                //     struct dev_t *device = element->base_object;

                //     // device->flip ^= DEV_DEVICE_FLIP_Y;
                //     // dev_UpdateDeviceRotation(device);
                //     dev_FlipDeviceV(device);
                // }
                m_FlipSelectionsV();
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
            if(igImageButton("##flip_h", (void *)(uintptr_t)m_flipv_texture, (ImVec2){24, 24}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 1}))
            {
                // for(uint32_t index = 0; index < m_selections.cursor; index++)
                // {
                //     struct elem_t *element = *(struct elem_t **)list_GetElement(&m_selections, index);
                //     struct dev_t *device = element->base_object;

                //     dev_FlipDeviceH(device);
                // }
                m_FlipSelectionsH();
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

            for(uint32_t device_type = 0; device_type < DEV_DEVICE_LAST; device_type++)
            {
                ImVec2 first_cursor_pos;
                ImVec2 second_cursor_pos;
                ImVec2 button_size;
                struct dev_desc_t *desc = dev_device_descs + device_type;
                ImVec2 uv0 = (ImVec2){((float)desc->tex_coords.x) / (float)d_devices_texture_width,
                                      ((float)desc->tex_coords.y) / (float)d_devices_texture_height};
                
                ImVec2 uv1 = (ImVec2){((float)(desc->tex_coords.x + desc->size.x)) / (float)d_devices_texture_width,
                                      ((float)(desc->tex_coords.y + desc->size.y)) / (float)d_devices_texture_height};
                igSameLine(0, -1);
                igPushID_Int(device_type);
                igGetCursorScreenPos(&first_cursor_pos);
                igPushStyleColor_Vec4(ImGuiCol_Button, (m_selected_device_type == device_type) ? button_active_color : button_color);
                if(igImageButton("##button", (void *)(uintptr_t)d_devices_texture, (ImVec2){24, 24}, (ImVec2){}, (ImVec2){}, (ImVec4){1, 1, 1, 1}, (ImVec4){1, 1, 1, 0}))
                {
                    m_selected_device_type = device_type;
                    m_SetEditFunc(M_EDIT_FUNC_PLACE);
                }
                igGetItemRectSize(&button_size);                
            
                ImVec2 image_size;
                
                if(desc->size.x > desc->size.y)
                {
                    float ratio = 24.0f / (float)desc->size.x;
                    image_size.x = 24.0f;
                    image_size.y = (float)desc->size.y * ratio;
                }
                else
                {
                    float ratio = 24.0f / (float)desc->size.y;
                    image_size.x = (float)desc->size.x * ratio;
                    image_size.y = 24.0f;
                }

                first_cursor_pos.x += (button_size.x - image_size.x) / 2.0f;
                first_cursor_pos.y += (button_size.y - image_size.y) / 2.0f;
                ImGuiWindow *window = igGetCurrentWindow();
                ImVec2 image_min = first_cursor_pos;
                ImVec2 image_max = (ImVec2){first_cursor_pos.x + image_size.x, first_cursor_pos.y + image_size.y};
                ImDrawList_AddImage(window->DrawList, (void *)(uintptr_t)d_devices_texture, image_min, image_max, uv0, uv1, igGetColorU32_Vec4((ImVec4){1, 1, 1, 1}));
                
                igPopStyleColor(1);
                igPopID();
            }

            // m_draw_selection_box = 0;
            m_StateFuncs[m_cur_state]();
        }
        igEnd();
        igPopStyleVar(3);
        ui_EndFrame();

        if(m_run_simulation)
        {
            sim_Step(m_single_step);

            if(m_single_step)
            {
                m_run_simulation = 0;
            }
        }

        d_Draw();

        SDL_GL_SwapWindow(m_window);
    }

    ui_Shutdown();
}



