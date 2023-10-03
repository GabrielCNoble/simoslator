#include "sim.h"
#include "list.h"
#include "pool.h"
#include "SDL2/SDL.h"
#include <stdio.h>

/* NOTE: maybe it'd be a good idea to keep a list of devices to update per device type. During
the device step, the simulation would update all devices of one type before going to the next.
This should improve instruction cache usage somewhat. The problem with this idea is that filling
those lists will require a fair amount of scattered writes. */
struct list_t               sim_update_devices;

struct list_t               sim_update_wires;
uint32_t                    sim_cur_step;

struct list_t               sim_wire_data;
struct list_t               sim_wire_pins;
struct list_t               sim_dev_data;
struct list_t               sim_dev_pins;

extern struct pool_t        dev_devices;
extern struct pool_t        dev_inputs;
extern struct dev_desc_t    dev_device_descs[];

extern uint8_t              w_wire_value_resolution[][WIRE_VALUE_LAST];
extern struct pool_t        w_wires;

extern void (*dev_DeviceFuncs[])(struct sim_dev_data_t *device);

void sim_Init()
{
    sim_update_devices = list_Create(sizeof(struct sim_dev_data_t *), 16384);
    sim_update_wires = list_Create(sizeof(struct sim_wire_data_t *), 16384);

    sim_wire_data = list_Create(sizeof(struct sim_wire_data_t), 16384);
    sim_wire_pins = list_Create(sizeof(struct wire_pin_t), 16384);

    sim_dev_data = list_Create(sizeof(struct sim_dev_data_t), 16384);
    sim_dev_pins = list_Create(sizeof(struct dev_pin_t), 16384);
}

void sim_Shutdown()
{

}

uint32_t sim_CmpXchg16(uint16_t *location, uint32_t compare, uint32_t new_value) 
{
    uint32_t exchanged = 0;
    __asm__ volatile
    (
        "mov rcx, %1\n"
        "mov ebx, %2\n"
        "mov eax, %3\n"
        "lock cmpxchg word ptr [rcx], bx\n"
        "setz al\n"
        "movzx eax, al\n"
        "mov %0, eax\n" 
        : "=m" (exchanged)
        : "rm" (location), "rm" (new_value), "rm" (compare)
        : "eax", "ebx", "rcx"
    );

    return exchanged;
}

void sim_QueueDevice(struct sim_dev_data_t *device)
{
    if(sim_CmpXchg16(&device->queued, 0, 1))
    {
        uint64_t index = list_AddElement(&sim_update_devices, NULL);
        struct sim_dev_data_t **update_device = list_GetElement(&sim_update_devices, index);
        *update_device = device;
    }
}

void sim_QueueWire(struct sim_wire_data_t *wire)
{
    if(sim_CmpXchg16(&wire->queued, 0, 1))
    {
        uint64_t index = list_AddElement(&sim_update_wires, NULL);
        struct sim_wire_data_t **update_wire = list_GetElement(&sim_update_wires, index);
        *update_wire = wire;
    }
}

void sim_BeginSimulation()
{
    sim_wire_data.cursor = 0;
    sim_wire_pins.cursor = 0;

    sim_dev_data.cursor = 0;
    sim_dev_pins.cursor = 0;

    for(uint32_t index = 0; index < w_wires.cursor; index++)
    {
        struct wire_t *wire = w_GetWire(index);

        if(wire != NULL)
        {
            wire->sim_data = list_AddElement(&sim_wire_data, NULL);
            struct sim_wire_data_t *wire_data = list_GetElement(&sim_wire_data, wire->sim_data);
            wire_data->value = WIRE_VALUE_Z;
            wire_data->queued = 0;

            for(uint32_t pin_type = 0; pin_type < 2; pin_type++)
            {
                wire_data->pin_count[pin_type] = wire->wire_pins[pin_type].pin_count;

                struct wire_pin_block_t *pin_block = wire->wire_pins[pin_type].first_block;
                uint32_t total_pin_count = wire->wire_pins[pin_type].pin_count;
                wire_data->first_pin[pin_type] = sim_wire_pins.cursor;

                while(pin_block != NULL)
                {
                    uint32_t pin_count = WIRE_PIN_BLOCK_PIN_COUNT;

                    if(pin_count > total_pin_count)
                    {
                        pin_count = total_pin_count;
                    }
                    total_pin_count -= pin_count;

                    for(uint32_t pin_index = 0; pin_index < pin_count; pin_index++)
                    {
                        struct wire_pin_t *pin = pin_block->pins + pin_index;
                        uint64_t sim_pin_index = list_AddElement(&sim_wire_pins, NULL);
                        struct wire_pin_t *sim_pin = list_GetElement(&sim_wire_pins, sim_pin_index);
                        sim_pin->device = pin->device;
                        sim_pin->pin = pin->pin;
                    }

                    pin_block = pin_block->next;
                }
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

            for(uint32_t pin_index = 0; pin_index < desc->pin_count; pin_index++)
            {
                struct dev_pin_t *pin = dev_GetDevicePin(device, pin_index);
                uint64_t sim_pin_index = list_AddElement(&sim_dev_pins, NULL);
                struct dev_pin_t *sim_pin = list_GetElement(&sim_dev_pins, sim_pin_index);
                sim_pin->wire = pin->wire;
                sim_pin->value = WIRE_VALUE_U;
            }
            sim_QueueDevice(dev_data);
        }
    }

    for(uint32_t wire_pin_index = 0; wire_pin_index < sim_wire_pins.cursor; wire_pin_index++)
    {
        struct wire_pin_t *sim_wire_pin = list_GetElement(&sim_wire_pins, wire_pin_index);
        struct dev_t *device = dev_GetDevice(sim_wire_pin->device);
        sim_wire_pin->device = device->sim_data;

        struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, device->sim_data);
        struct dev_pin_t *sim_dev_pin = list_GetElement(&sim_dev_pins, dev_sim_data->first_pin + sim_wire_pin->pin);
        struct wire_t *wire = w_GetWire(sim_dev_pin->wire);
        sim_dev_pin->wire = wire->sim_data;
    }

    for(uint32_t index = 0; index < dev_inputs.cursor; index++)
    {
        struct dev_input_t *input = pool_GetValidElement(&dev_inputs, index);

        if(input != NULL)
        {
            struct sim_dev_data_t *sim_data = list_GetElement(&sim_dev_data, input->device->sim_data);
            struct dev_pin_t *pin = list_GetElement(&sim_dev_pins, sim_data->first_pin);
            pin->value = input->init_value;
            
            sim_QueueDevice(sim_data);
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

void sim_WireStep(struct sim_wire_data_t *wire)
{
    uint8_t wire_value = WIRE_VALUE_Z;

    for(uint32_t pin_index = 0; pin_index < wire->pin_count[DEV_PIN_TYPE_OUT]; pin_index++)
    {
        // struct wire_pin_block_t *wire_pin_block = list_GetElement(&w_wire_pin_blocks[DEV_PIN_TYPE_OUT], wire->wire_inputs.first_block + block_index);
        // uint32_t wire_pin_index = 0;
        struct wire_pin_t *wire_pin = list_GetElement(&sim_wire_pins, wire->first_pin[DEV_PIN_TYPE_OUT] + pin_index);
        struct sim_dev_data_t *dev_data = list_GetElement(&sim_dev_data, wire_pin->device);
        struct dev_pin_t *device_pin = list_GetElement(&sim_dev_pins, dev_data->first_pin + wire_pin->pin);
        // struct dev_t *device = dev_GetDevice(wire_pin->device);
        // struct sim_dev_data_t 
        // struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin->pin);
        wire_value = w_wire_value_resolution[wire_value][device_pin->value];

        // while(wire_pin_block->pins[wire_pin_index].device != DEV_INVALID_DEVICE)
        // {
        //     struct dev_t *device = pool_GetElement(&dev_devices, wire_pin_block->pins[wire_pin_index].device);
        //     // uint32_t device_pin_index = wire_pin_block->pins[wire_pin_index].pin % DEV_PIN_BLOCK_PIN_COUNT;
        //     // uint32_t device_pin_block_index = wire_pin_block->pins[wire_pin_index].pin / DEV_PIN_BLOCK_PIN_COUNT;
        //     // struct dev_pin_block_t *device_pin_block = list_GetElement(&dev_pin_blocks, device->first_pin_block + device_pin_block_index);
        //     // wire_value = w_wire_value_resolution[wire_value][device_pin_block->pins[device_pin_index].value];
        //     struct dev_pin_t *device_pin = dev_GetDevicePin(device, wire_pin_block->pins[wire_pin_index].pin);
        //     wire_value = w_wire_value_resolution[wire_value][device_pin->value];
        //     wire_pin_index++;
        // }
    }

    if(wire_value != wire->value)
    {
        for(uint32_t pin_index = 0; pin_index < wire->pin_count[DEV_PIN_TYPE_IN]; pin_index++)
        {
            struct wire_pin_t *wire_pin = list_GetElement(&sim_wire_pins, wire->first_pin[DEV_PIN_TYPE_IN] + pin_index);
            struct sim_dev_data_t *dev_sim_data = list_GetElement(&sim_dev_data, wire_pin->device);
            sim_QueueDevice(dev_sim_data);
            // struct dev_t *device = dev_GetDevice()
            // struct wire_pin_block_t *wire_pin_block = list_GetElement(&w_wire_pin_blocks[DEV_PIN_TYPE_IN], wire->wire_outputs.first_block + block_index);
            // uint32_t device_index = 0;

            // while(wire_pin_block->pins[device_index].device != DEV_INVALID_DEVICE)
            // {
            //     struct dev_t *device = pool_GetElement(&dev_devices, wire_pin_block->pins[device_index].device);
            //     sim_QueueDevice(device);
            //     device_index++;
            // }
        }
    }

    wire->value = wire_value;
}

void sim_DeviceStep(struct sim_dev_data_t *device)
{
    if(dev_DeviceFuncs[device->type] != NULL)
    {
        dev_DeviceFuncs[device->type](device);
    }
}

void sim_Step()
{
    sim_cur_step = SIM_STEP_DEVICE;
    
    while(sim_update_devices.cursor || sim_update_wires.cursor)
    {
        switch(sim_cur_step)
        {
            case SIM_STEP_DEVICE:
            {
                for(uint32_t update_index = 0; update_index < sim_update_devices.cursor; update_index++)
                {
                    struct sim_dev_data_t *device = *(struct sim_dev_data_t **)list_GetElement(&sim_update_devices, update_index);
                    sim_DeviceStep(device);
                    device->queued = 0;
                }

                sim_update_devices.cursor = 0;
                sim_cur_step = SIM_STEP_WIRE;
            }
            break;

            case SIM_STEP_WIRE:
            {
                for(uint32_t update_index = 0; update_index < sim_update_wires.cursor; update_index++)
                {
                    struct sim_wire_data_t *wire = *(struct sim_wire_data_t **)list_GetElement(&sim_update_wires, update_index);
                    sim_WireStep(wire);
                    wire->queued = 0;
                }

                sim_update_wires.cursor = 0;
                sim_cur_step = SIM_STEP_DEVICE;
            }
            break;
        }
    }
}
