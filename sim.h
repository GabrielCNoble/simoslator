#ifndef SIM_H
#define SIM_H

#include <stdint.h>
#include "dev.h"
#include "wire.h"

enum SIM_STEPS
{
    SIM_STEP_DEVICE,
    SIM_STEP_WIRE,
    SIM_STEP_LAST
};

void sim_Init();

void sim_Shutdown();

uint32_t sim_CmpXchg32(uint32_t *location, uint32_t compare, uint32_t new_value);

void sim_QueueDevice(struct dev_t *device);

void sim_QueueWire(struct wire_t *wire);

void sim_Step();


#endif