#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "dev.h"
#include "wire.h"

enum M_OBJECT_TYPES
{
    M_OBJECT_TYPE_DEVICE,
    M_OBJECT_TYPE_SEGMENT,
    M_OBJECT_TYPE_JUNCTION,
    M_OBJECT_TYPE_LAST
};

/* forward declaration */
struct m_object_t;

struct m_object_link_t
{
    struct m_object_t *         object;
    struct m_object_link_t *    next;
    struct m_object_link_t *    prev;
};

struct m_object_t
{
    void *                      object;
    // uint32_t                    index;
    uint32_t                    type;
    int32_t                     position[2];
    int32_t                     size[2];
    struct m_object_link_t *    first_link;
    struct m_object_link_t *    last_link;
    uint64_t                    selection_index;
};

struct m_picked_object_t
{
    struct m_object_t *     object;
    uint32_t                index;
};

union m_wire_seg_t
{
    struct wire_seg_pos_t     seg_pos;
    struct wire_seg_t *       segment;
};

#define M_SNAP_VALUE                20
#define M_REGION_SIZE               (M_SNAP_VALUE*20)
#define M_DEVICE_PIN_PIXEL_WIDTH    8
#define M_WIRE_PIXEL_WIDTH          4

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
};

// struct m_device_section_t
// {
//     uintptr_t count;
//     uintptr_t devices;
// };

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

// struct m_pin_record_t
// {
//     uintptr_t deserialized_index;
//     uintptr_t device;
//     uintptr_t wire;
// };

// struct m_wire_section_t
// {
//     uintptr_t count;
//     uintptr_t wires;
// };

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
    int32_t   ends[2][2];
    uintptr_t segments[2];
};

struct m_junction_record_t
{
    uintptr_t   deserialized_index;
    uint64_t    device : 48;
    uint64_t    pin : 16;
};

struct m_object_t *m_CreateObject(uint32_t type, void *base_object);

void m_DestroyObject(struct m_object_t *object);

void m_UpdateObject(struct m_object_t *object);

void m_SelectObject(struct m_object_t *object, uint32_t multiple);

void m_ClearSelections();

void m_DeleteSelections();

void m_TranslateSelections(int32_t dx, int32_t dy);

void m_RotateSelections(int32_t ccw_rotation);

struct wire_t *m_CreateWire(struct m_picked_object_t *first_contact, struct m_picked_object_t *second_contact, struct list_t *segments);

void m_SerializeCircuit(void **file_buffer, size_t *file_buffer_size);

void m_DeserializeCircuit(void *file_buffer, size_t file_buffer_size);

void m_SaveCircuit(const char *file_name);

#endif