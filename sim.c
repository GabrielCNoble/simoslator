#include "sim.h"
#include "list.h"
#include "pool.h"
#include "SDL2/SDL.h"
#include <stdio.h>

struct list_t   sim_update_devices;
struct list_t   sim_update_wires;
uint32_t        sim_cur_step;

void sim_Init()
{
    sim_update_devices = list_Create(sizeof(struct dev_t *), 0xffff);
    sim_update_wires = list_Create(sizeof(struct wire_t *), 0xffff);
}

void sim_Shutdown()
{

}

uint32_t sim_CmpXchg32(uint32_t *location, uint32_t compare, uint32_t new_value) 
{
    uint32_t exchanged = 0;
    __asm__ volatile
    (
        "mov rcx, %1\n"
        "mov ebx, %2\n"
        "mov eax, %3\n"
        "lock cmpxchg dword ptr [rcx], ebx\n"
        "setz al\n"
        "movzx eax, al\n"
        "mov %0, eax\n" 
        : "=m" (exchanged)
        : "rm" (location), "rm" (new_value), "rm" (compare)
        : "eax", "ebx", "ecx"
    );

    return exchanged;
}

void sim_QueueDevice(struct dev_t *device)
{
    if(sim_CmpXchg32(&device->queued, 0, 1))
    {
        uint64_t index = list_AddElement(&sim_update_devices, NULL);
        struct dev_t **update_device = list_GetElement(&sim_update_devices, index);
        *update_device = device;
    }
}

void sim_QueueWire(struct wire_t *wire)
{
    if(sim_CmpXchg32(&wire->queued, 0, 1))
    {
        uint64_t index = list_AddElement(&sim_update_wires, NULL);
        struct wire_t **update_wire = list_GetElement(&sim_update_wires, index);
        *update_wire = wire;
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
                    struct dev_t *device = *(struct dev_t **)list_GetElement(&sim_update_devices, update_index);
                    dev_DeviceStep(device);
                    device->queued = 0;
                }

                sim_update_devices.cursor = 0;
            }
            break;

            case SIM_STEP_WIRE:
            {
                for(uint32_t update_index = 0; update_index < sim_update_wires.cursor; update_index++)
                {
                    struct wire_t *wire = *(struct wire_t **)list_GetElement(&sim_update_wires, update_index);
                    w_WireStep(wire);
                    wire->queued = 0;
                }

                sim_update_wires.cursor = 0;
            }
            break;
        }
    }
}
