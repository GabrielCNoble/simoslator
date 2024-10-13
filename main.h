#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "dev.h"
#include "wire.h"
#include "elem.h"
#include "file.h"

// enum M_OBJECT_TYPES
// {
//     M_OBJECT_TYPE_DEVICE,
//     M_OBJECT_TYPE_SEGMENT,
//     M_OBJECT_TYPE_JUNCTION,
//     M_OBJECT_TYPE_LAST
// };

// /* forward declaration */
// struct m_object_t;

// struct m_object_link_t
// {
//     struct m_object_t *         object;
//     struct m_object_link_t *    next;
//     struct m_object_link_t *    prev;
// };

// struct m_object_t
// {
//     POOL_ELEMENT;
//     void *                      object;
//     // uint32_t                    index;
//     uint32_t                    type;
//     int32_t                     position[2];
//     int32_t                     size[2];
//     struct m_object_link_t *    first_link;
//     struct m_object_link_t *    last_link;
//     uint64_t                    selection_index;
// };

enum M_EXPLORER_MODES
{
    M_EXPLORER_MODE_LOAD,
    M_EXPLORER_MODE_SAVE,
    M_EXPLORER_MODE_LAST,
};

struct m_filtered_dir_ent_t
{
    struct file_dir_ent_t * entry;
    uint32_t                selected;
    uint32_t                match_start;
    uint32_t                match_size;
};

enum M_EXPLORER_SORT_DIRECTIONS
{
    M_EXPLORER_SORT_DIRECTION_NONE,
    M_EXPLORER_SORT_DIRECTION_ASC,
    M_EXPLORER_SORT_DIRECTION_DESC
};

/* forward declaration */
struct m_explorer_state_t;

typedef uint32_t (m_explorer_callback_t)(struct m_explorer_state_t *explorer_state, void *args);

struct m_explorer_save_load_args_t
{
    char *  file_name;
    char *  file_path;
    char    full_path[FILE_MAX_PATH_LEN];
};

struct m_explorer_state_t
{
    struct file_dir_t           current_dir;
    char                        current_path[FILE_MAX_PATH_LEN];
    char                        search_bar[FILE_MAX_PATH_LEN];
    char                        file_name[FILE_MAX_PATH_LEN];
    struct list_t               filtered_entries;
    uint32_t                    mode;
    void *                      data;
    m_explorer_callback_t *     SaveCallback;
    m_explorer_callback_t *     LoadCallback;
    uint32_t                    sort_dir;
};


struct m_picked_element_t
{
    struct elem_t *         element;
    uint32_t                index;
};

union m_wire_seg_t
{
    struct wire_seg_pos_t     seg_pos;
    struct wire_seg_t *       segment;
};

#define M_SNAP_VALUE                10
#define M_REGION_SIZE               (M_SNAP_VALUE*20)
// #define M_DEVICE_PIN_PIXEL_WIDTH    8
// #define M_WIRE_PIXEL_WIDTH          4


struct m_editor_state_t
{
    struct list_t   selections;
    struct list_t   elements_in_box;
    ivec2_t         device_place_pos;
    ivec2_t         selection_center;
    uint32_t        cur_state;
};

enum M_MODES
{
    M_STATE_EDIT = 0,
    M_STATE_SELECTION_BOX,
    M_STATE_EXPLORER,
    M_STATE_LAST,
};

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

#define M_FILE_HEADER_MAGIC "FROOGGER"

struct m_file_header_t
{
    char        magic[(sizeof(M_FILE_HEADER_MAGIC) + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1)];
    uintptr_t   version;
    uintptr_t   devices;
    uintptr_t   device_count;
    uintptr_t   wires;
    uintptr_t   wire_count;
    uintptr_t   segments;
    uintptr_t   segment_count;
    uintptr_t   junctions;
    uintptr_t   junction_count;
    uintptr_t   seg_juncs;
    uintptr_t   seg_junc_count;
};

struct m_device_record_t
{
    uintptr_t   deserialized_index;
    uintptr_t   extra;
    int32_t     position[2];
    uint16_t    type;
    uint8_t     flip;
    uint8_t     angle;
    uint32_t    pad;
};

struct m_wire_record_t
{
    uintptr_t deserialized_index;
    uintptr_t junctions;
    uintptr_t junction_count;
    uintptr_t segments;
    uintptr_t segment_count;
};

struct m_segment_record_t
{
    uintptr_t deserialized_index;
    uintptr_t segments[2];
    ivec2_t   ends[2];
};

struct m_junction_record_t 
{
    uintptr_t   deserialized_index;
    uintptr_t   first_segment;
    uintptr_t   segment_count;
    uint64_t    device : 48;
    uint64_t    pin : 16;
};

struct m_seg_junc_record_t
{
    uint64_t segment;
    uint64_t tip_index;
};

// struct m_object_t *m_CreateObject(uint32_t type, void *base_object);

// void m_DestroyObject(struct m_object_t *object);

// void m_UpdateObject(struct m_object_t *object);

struct m_editor_state_t m_CreateEditorState();

void m_DestroyEditorState(struct m_editor_state_t *editor_state);

void m_SelectElement(struct elem_t *element, uint32_t multiple);

void m_ClearSelections();

void m_DeleteSelections();

void m_TranslateSelections(ivec2_t *translation);

void m_RotateSelections(int32_t ccw_rotation);

void m_FlipSelectionsH();

void m_FlipSelectionsV();

struct wire_t *m_CreateWire(struct m_picked_element_t *first_contact, struct m_picked_element_t *second_contact, struct list_t *segments);

void m_SerializeCircuit(void **file_buffer, size_t *file_buffer_size);

void m_DeserializeCircuit(void *file_buffer, size_t file_buffer_size);

void m_SaveCircuit(const char *file_name);

void m_ClearCircuit();

void m_SelectionBoxState();

void m_EditState();

void m_OpenExplorer(struct m_explorer_state_t *explorer, const char *path, uint32_t mode, m_explorer_callback_t *SaveCallback, m_explorer_callback_t *LoadCallback, void *data);

void m_CloseExplorer(struct m_explorer_state_t *explorer);

void m_Back(struct m_explorer_state_t *explorer);

void m_ChangeDir(struct m_explorer_state_t *explorer, const char *path);

void m_FilterEntries(struct m_explorer_state_t *explorer);

void m_SortEntries(struct m_explorer_state_t *explorer);

void m_ExplorerState();

#endif