#include "sim.h"
#include "list.h"
#include "pool.h"
#include "SDL2/SDL.h"
#include <stdio.h>
#include <stdlib.h>

/* NOTE: maybe it'd be a good idea to keep a list of devices to update per device type. During
the device step, the simulation would update all devices of one type before going to the next.
This should improve instruction cache usage somewhat. The problem with this idea is that filling
those lists will require a fair amount of scattered writes. */
struct list_t               sim_update_devices;
struct list_t               sim_update_wires[2];
struct list_t               sim_source_updates;
uint32_t                    sim_cur_step;

struct list_t               sim_wire_data;
struct list_t               sim_wire_pins;
struct list_t               sim_dev_data;
struct list_t               sim_dev_pins;
struct list_t               sim_clocks;
struct sim_wire_data_t      sim_invalid_wires[2] = {{.value = WIRE_VALUE_Z}, {.value = WIRE_VALUE_Z}};
// struct sim_wire_data_t      sim_invalid_input_wire = {.value = WIRE_VALUE_Z};
// struct sim_wire_data_t      sim_invalid_output_wire = {.value = WIRE_VALUE_Z};

extern struct pool_t        dev_devices;
extern struct pool_t        dev_inputs;
extern struct pool_t        dev_clocks;
extern struct pool_t        dev_7seg_disps;
extern struct dev_desc_t    dev_device_descs[];

extern uint8_t              w_wire_value_resolution[][WIRE_VALUE_LAST];
extern struct pool_t        w_wires;

extern void (*dev_DeviceFuncs[])(struct sim_dev_data_t *device);

uint64_t                    sim_counter_freq;
uint64_t                    sim_prev_counter;
uint64_t                    sim_substeps_per_count;
uint64_t                    sim_next_clock_count;

void sim_Init()
{
    sim_update_devices = list_Create(sizeof(struct sim_dev_data_t *), 16384);
    sim_update_wires[0] = list_Create(sizeof(struct sim_wire_data_t *), 16384);
    sim_update_wires[1] = list_Create(sizeof(struct sim_wire_data_t *), 16384);
    sim_source_updates = list_Create(sizeof(struct sim_dev_data_t *), 16384);

    sim_wire_data = list_Create(sizeof(struct sim_wire_data_t), 16384);
    sim_wire_pins = list_Create(sizeof(struct wire_pin_t), 16384);

    sim_dev_data = list_Create(sizeof(struct sim_dev_data_t), 16384);
    sim_dev_pins = list_Create(sizeof(struct sim_dev_pin_t), 16384);

    sim_clocks = list_Create(sizeof(struct sim_clock_data_t), 256);

    sim_counter_freq = SDL_GetPerformanceFrequency();
    sim_substeps_per_count = SIM_SUBSTEPS_PER_SECOND / sim_counter_freq;
}

void sim_Shutdown()
{

}

uint32_t sim_CmpXchg8(uint8_t *location, uint32_t compare, uint32_t new_value) 
{
    uint32_t exchanged = 0;
    __asm__ volatile
    (
        "mov rcx, %1\n"
        "mov ebx, %2\n"
        "mov eax, %3\n"
        "lock cmpxchg byte ptr [rcx], bl\n"
        "setz al\n"
        "movzx eax, al\n"
        "mov %0, eax\n" 
        : "=m" (exchanged)
        : "rm" (location), "rm" (new_value), "rm" (compare)
        : "eax", "ebx", "rcx"
    );

    return exchanged;
}

uint64_t sim_XInc64(uint64_t *location)
{
    uint64_t old_value;

    __asm__ volatile
    (
        "mov rcx, %1\n"
        "mov rax, 0x1\n"
        "lock xadd qword ptr [rcx], rax\n"
        "mov %0, rax\n"
        : "=m" (old_value)
        : "rm" (location)
        : "rax", "rcx"
    );

    return old_value;
}

void sim_QueueDevice(struct sim_dev_data_t *device)
{
    if(sim_CmpXchg8(&device->queued, 0, 1))
    {
        // uint64_t index = list_AddElement(&sim_update_devices, NULL);
        uint64_t index = sim_XInc64(&sim_update_devices.cursor);
        struct sim_dev_data_t **update_device = list_GetElement(&sim_update_devices, index);
        *update_device = device;
    }
}

void sim_QueueWire(struct sim_wire_data_t *wire)
{
    if(wire != &sim_invalid_wires[DEV_PIN_TYPE_IN] && wire != &sim_invalid_wires[DEV_PIN_TYPE_OUT] && sim_CmpXchg8(&wire->queued, 0, 1))
    {
        struct list_t *wire_update_list = &sim_update_wires[wire->source_count == 0];
        uint64_t index = sim_XInc64(&wire_update_list->cursor);
        struct sim_wire_data_t **update_wire = list_GetElement(wire_update_list, index);
        *update_wire = wire;
    }
}

void sim_QueueSource(struct sim_dev_data_t *device)
{
    uint64_t index = sim_XInc64(&sim_source_updates.cursor);
    struct sim_dev_data_t **update_source = list_GetElement(&sim_source_updates, index);
    *update_source = device;
}

void sim_BeginSimulation()
{
    sim_wire_data.cursor = 0;
    sim_wire_pins.cursor = 0;
    sim_dev_data.cursor = 0;
    sim_dev_pins.cursor = 0;
    sim_update_devices.cursor = 0;
    sim_update_wires[0].cursor = 0;
    sim_update_wires[1].cursor = 0;
    sim_source_updates.cursor = 0;
    sim_clocks.cursor = 0;

    for(uint32_t index = 0; index < w_wires.cursor; index++)
    {
        struct wire_t *wire = w_GetWire(index);

        if(wire != NULL)
        {
            uint32_t cur_input = sim_wire_pins.cursor;
            uint32_t next_source_move = cur_input;
            // uint32_t source_count = 0;
            uint32_t cur_output = cur_input + wire->input_count;
            uint32_t pin_count = wire->input_count + wire->output_count;

            wire->sim_data = list_AddElement(&sim_wire_data, NULL);
            struct sim_wire_data_t *wire_data = list_GetElement(&sim_wire_data, wire->sim_data);
            // wire_data->mos_source = 0;
            wire_data->source_count = 0;
            wire_data->value = WIRE_VALUE_Z;
            wire_data->queued = 0;
            wire_data->first_pin[DEV_PIN_TYPE_IN] = cur_input;
            wire_data->first_pin[DEV_PIN_TYPE_OUT] = cur_output;
            wire_data->pin_count[DEV_PIN_TYPE_IN] = wire->input_count;
            wire_data->pin_count[DEV_PIN_TYPE_OUT] = wire->output_count;
            

            for(uint32_t index = 0; index < pin_count; index++)
            {
                list_AddElement(&sim_wire_pins, NULL);
            }

            struct wire_junc_t *junction = wire->first_junction;
            while(junction != NULL)
            {
                if(junction->pin.device != DEV_INVALID_DEVICE)
                {
                    struct wire_pin_t *sim_pin;
                    struct dev_t *device = dev_GetDevice(junction->pin.device);
                    struct dev_desc_t *desc = dev_device_descs + device->type;

                    if(desc->pins[junction->pin.pin].type == DEV_PIN_TYPE_IN)
                    {
                        sim_pin = list_GetElement(&sim_wire_pins, cur_input);
                        cur_input++;

                        if((device->type == DEV_DEVICE_TYPE_NMOS || device->type == DEV_DEVICE_TYPE_PMOS) && junction->pin.pin == DEV_MOS_PIN_SOURCE)
                        {
                            wire_data->source_count++;
                        }
                        else
                        {
                            if(wire_data->source_count > 0)
                            {
                                struct wire_pin_t *move_pin = list_GetElement(&sim_wire_pins, next_source_move);
                                *sim_pin = *move_pin;
                                sim_pin = move_pin;
                            }
                            next_source_move++;
                        }
                    }
                    else
                    {
                        sim_pin = list_GetElement(&sim_wire_pins, cur_output);
                        cur_output++;
                    }

                    sim_pin->device = junction->pin.device;
                    sim_pin->pin = junction->pin.pin;
                }

                junction = junction->wire_next;
            }
        }
    }

    for(uint32_t index = 0; index < dev_devices.cursor; index++)
    {
        struct dev_t *device = dev_GetDevice(index);
        if(device != NULL)
        {
            struct dev_desc_t *desc = dev_device_descs + device->type;

            device->sim_data = list_AddElement(&sim_dev_data, NULL);
            struct sim_dev_data_t *dev_data = list_GetElement(&sim_dev_data, device->sim_data);
            dev_data->queued = 0;
            dev_data->type = device->type;
            dev_data->first_pin = sim_dev_pins.cursor;
            dev_data->device = device;

            for(uint32_t pin_index = 0; pin_index < desc->pin_count; pin_index++)
            {
                struct dev_pin_t *pin = dev_GetDevicePin(device, pin_index);
                uint64_t sim_pin_index = list_AddElement(&sim_dev_pins, NULL);
                struct sim_dev_pin_t *sim_pin = list_GetElement(&sim_dev_pins, sim_pin_index);
                sim_pin->wire = pin->junction;
                sim_pin->value = WIRE_VALUE_U;
            }
        }
    }

    for(uint32_t wire_pin_index = 0; wire_pin_index < sim_wire_pins.cursor; wire_pin_index++)
    {
        struct wire_pin_t *sim_wire_pin = list_GetElement(&sim_wire_pins, wire_pin_index);
        struct dev_t *device = dev_GetDevice(sim_wire_pin->device);
        sim_wire_pin->device = device->sim_data;

        struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, device->sim_data);
        struct sim_dev_pin_t *sim_dev_pin = list_GetElement(&sim_dev_pins, dev_sim_data->first_pin + sim_wire_pin->pin);
        // struct wire_t *wire = w_GetWire(sim_dev_pin->wire);
        struct wire_junc_t *junction = w_GetWireJunction(sim_dev_pin->wire);
        sim_dev_pin->wire = junction->base.wire->sim_data;
    }

    /* allocate enough update slots upfront */
    for(uint32_t index = 0; index < sim_dev_data.cursor; index++)
    {
        list_AddElement(&sim_update_devices, NULL);
    }
    sim_update_devices.cursor = 0;
    for(uint32_t index = 0; index < sim_wire_data.cursor; index++)
    {
        list_AddElement(&sim_update_wires[0], NULL);
        list_AddElement(&sim_update_wires[1], NULL);
    }
    sim_update_wires[0].cursor = 0;
    sim_update_wires[1].cursor = 0;
    for(uint32_t index = 0; index < sim_dev_data.cursor; index++)
    {
        list_AddElement(&sim_source_updates, NULL);
    }
    sim_source_updates.cursor = 0;

    /* queue all devices to jumpstart the simulation */
    for(uint32_t index = 0; index < sim_dev_data.cursor; index++)
    {
        struct sim_dev_data_t *dev_data = list_GetElement(&sim_dev_data, index);
        sim_QueueDevice(dev_data);
    }

    for(uint32_t index = 0; index < dev_inputs.cursor; index++)
    {
        struct dev_input_t *input = pool_GetValidElement(&dev_inputs, index);

        if(input != NULL)
        {
            struct sim_dev_data_t *sim_data = list_GetElement(&sim_dev_data, input->device->sim_data);
            struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, sim_data->first_pin);
            pin->value = input->init_value;
        }
    }

    sim_next_clock_count = 0xffffffffffffffff;
    sim_prev_counter = SDL_GetPerformanceCounter();

    for(uint32_t index = 0; index < dev_clocks.cursor; index++)
    {
        struct dev_clock_t *clock = pool_GetValidElement(&dev_clocks, index);
        if(clock != NULL)
        {
            uint64_t index = list_AddElement(&sim_clocks, NULL);
            struct sim_clock_data_t *sim_clock = list_GetElement(&sim_clocks, index);
            sim_clock->half_period = (SIM_SUBSTEPS_PER_SECOND / clock->frequency) >> 1;
            sim_clock->substeps_left = sim_clock->half_period;
            sim_clock->device = clock->device->sim_data;

            sim_next_clock_count = (sim_clock->substeps_left < sim_next_clock_count) ? sim_clock->substeps_left : sim_next_clock_count;

            struct sim_dev_data_t *sim_data = list_GetElement(&sim_dev_data, clock->device->sim_data);
            struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, sim_data->first_pin);
            pin->value = WIRE_VALUE_0S;
        }
    }
}

void sim_StopSimulation()
{
    // for(uint32_t index = 0; index < w_wires.cursor; index++)
    // {
    //     struct wire_t *wire = pool_GetValidElement(&w_wires, index);
    //     if(wire != NULL)
    //     {
    //         wire->value = WIRE_VALUE_Z;
    //     }
    // }
}

struct sim_dev_data_t *sim_GetDevSimData(uint64_t data)
{
    return list_GetElement(&sim_dev_data, data);
}

struct sim_dev_pin_t *sim_GetDevSimPin(struct sim_dev_data_t *data, uint16_t pin)
{
    if(data != NULL)
    {
        return list_GetElement(&sim_dev_pins, data->first_pin + pin);
    }

    return NULL;
}

struct sim_wire_data_t *sim_GetWireSimData(uint64_t wire, uint32_t pin_type)
{
    if(wire == WIRE_INVALID_WIRE)
    {
        return sim_invalid_wires + pin_type;
    }

    return list_GetElement(&sim_wire_data, wire);
}

void sim_WireStep(struct sim_wire_data_t *wire)
{
    uint8_t wire_value = WIRE_VALUE_Z;

    for(uint32_t pin_index = 0; pin_index < wire->pin_count[DEV_PIN_TYPE_OUT]; pin_index++)
    {
        struct wire_pin_t *wire_pin = list_GetElement(&sim_wire_pins, wire->first_pin[DEV_PIN_TYPE_OUT] + pin_index);
        struct sim_dev_data_t *dev_data = list_GetElement(&sim_dev_data, wire_pin->device);
        struct sim_dev_pin_t *device_pin = list_GetElement(&sim_dev_pins, dev_data->first_pin + wire_pin->pin);
        wire_value = w_wire_value_resolution[wire_value][device_pin->value];
    }

    if(wire_value != wire->value)
    {
        uint32_t pin_count = wire->pin_count[DEV_PIN_TYPE_IN] - wire->source_count;
        // uint32_t pin_count = wire->pin_count[DEV_PIN_TYPE_IN];

        for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
        {
            struct wire_pin_t *wire_pin = list_GetElement(&sim_wire_pins, wire->first_pin[DEV_PIN_TYPE_IN] + pin_index);
            struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, wire_pin->device);
            sim_QueueDevice(dev_sim_data);
        }

        uint32_t first_pin = wire->first_pin[DEV_PIN_TYPE_IN] + pin_count;

        if(wire->source_count > 0 && wire_value == WIRE_VALUE_ERR)
        {
            wire_value = WIRE_VALUE_IND;
        }

        for(uint32_t pin_index = 0; pin_index < wire->source_count; pin_index++)
        {
            struct wire_pin_t *wire_pin = list_GetElement(&sim_wire_pins, wire->first_pin[DEV_PIN_TYPE_IN] + pin_index);
            struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, wire_pin->device);
            sim_QueueSource(dev_sim_data);
        }
    }

    wire->value = wire_value;
}

void sim_IndWireStep(struct sim_wire_data_t *wire)
{

}

void sim_DeviceStep(struct sim_dev_data_t *device)
{
    if(dev_DeviceFuncs[device->type] != NULL)
    {
        dev_DeviceFuncs[device->type](device);
    }
}

void sim_Step(uint32_t single_substep)
{
    sim_cur_step = SIM_STEP_DEVICE;
    uint64_t cur_count = SDL_GetPerformanceCounter();
    uint64_t substep_count = (cur_count - sim_prev_counter) / sim_substeps_per_count;
    sim_prev_counter = cur_count;

    if(substep_count > SIM_MAX_SUBSTEPS_PER_FRAME)
    {
        substep_count = SIM_MAX_SUBSTEPS_PER_FRAME;
    }

    if(single_substep)
    {
        substep_count = 1;
    }
    
    while(substep_count)
    {
        uint64_t elasped_substeps = 1;

        // if(sim_source_updates.cursor > 0)
        // {
        //     printf("source propagation step\n");
        //     for(uint32_t source_index = 0; source_index < sim_source_updates.cursor; source_index++)
        //     {
        //         struct sim_dev_data_t *device = *(struct sim_dev_data_t **)list_GetElement(&sim_source_updates, source_index);
        //         sim_DeviceStep(device);
        //         device->device->selection_index = 0;
        //     }
        //     sim_source_updates.cursor = 0;
        // }
        // else
        // {
        //     printf("device update step\n");

        //     if(sim_update_devices.cursor == 0)
        //     {
        //         for(uint32_t index = 0; index < dev_devices.cursor; index++)
        //         {
        //             struct dev_t *device = dev_GetDevice(index);
        //             if(device != NULL)
        //             {
        //                 device->selection_index = INVALID_LIST_INDEX;
        //             }
        //         }
        //     }

        for(uint32_t update_index = 0; update_index < sim_update_devices.cursor; update_index++)
        {
            struct sim_dev_data_t *device = *(struct sim_dev_data_t **)list_GetElement(&sim_update_devices, update_index);
            sim_DeviceStep(device);
            device->queued = 0;
        }
        sim_update_devices.cursor = 0;
        // }

        // for(uint32_t update_index = 0; update_index < sim_update_devices.cursor; update_index++)
        // {
        //     struct sim_dev_data_t *device = *(struct sim_dev_data_t **)list_GetElement(&sim_update_devices, update_index);
        //     sim_DeviceStep(device);
        //     device->queued = 0;
        // }
        // sim_update_devices.cursor = 0;

        struct list_t *wire_update_list;

        do
        {
            for(uint32_t source_index = 0; source_index < sim_source_updates.cursor; source_index++)
            {
                struct sim_dev_data_t *device = *(struct sim_dev_data_t **)list_GetElement(&sim_source_updates, source_index);
                sim_DeviceStep(device);
            }
            sim_source_updates.cursor = 0;

            wire_update_list = &sim_update_wires[sim_update_wires[0].cursor == 0];

            for(uint32_t update_index = 0; update_index < wire_update_list->cursor; update_index++)
            {
                struct sim_wire_data_t *wire = *(struct sim_wire_data_t **)list_GetElement(wire_update_list, update_index);
                sim_WireStep(wire);
                wire->queued = 0;
            }
            wire_update_list->cursor = 0;
        }
        while(sim_source_updates.cursor > 0);

        if(!(sim_update_devices.cursor || wire_update_list->cursor))
        {
            elasped_substeps = sim_next_clock_count;
            if(elasped_substeps > substep_count)
            {
                elasped_substeps = substep_count;
            }
        }

        sim_next_clock_count = 0xffffffffffffffff;

        for(uint32_t clock_index = 0; clock_index < sim_clocks.cursor; clock_index++)
        {
            struct sim_clock_data_t *clock = list_GetElement(&sim_clocks, clock_index);
            clock->substeps_left -= elasped_substeps;
            if(clock->substeps_left == 0)
            {
                clock->substeps_left = clock->half_period;
                struct sim_dev_data_t *device = list_GetElement(&sim_dev_data, clock->device);
                struct sim_dev_pin_t *pin = list_GetElement(&sim_dev_pins, device->first_pin);
                pin->value = !pin->value;
                sim_QueueDevice(device);
            }

            sim_next_clock_count = (clock->substeps_left < sim_next_clock_count) ? clock->substeps_left : sim_next_clock_count;
        }

        substep_count -= elasped_substeps;
    }

    for(uint32_t index = 0; index < dev_7seg_disps.cursor; index++)
    {
        struct dev_7seg_disp_t *display = pool_GetValidElement(&dev_7seg_disps, index);

        if(display != NULL)
        {
            dev_Update7SegDisplay(display);
        }
    }
}
