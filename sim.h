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

struct sim_wire_data_t
{
    uint32_t first_pin[2];
    uint16_t pin_count[2];
    // uint32_t first_input_pin;
    // uint32_t first_output_pin;
    // uint16_t input_pin_count;
    // uint16_t output_pin_count;
    uint16_t queued;
    uint16_t value;
};

struct sim_dev_data_t
{
    uint32_t first_pin;
    uint16_t queued;
    uint16_t type;
};

void sim_Init();

void sim_Shutdown();

uint32_t sim_CmpXchg16(uint16_t *location, uint32_t compare, uint32_t new_value);

void sim_QueueDevice(struct sim_dev_data_t *device);

void sim_QueueWire(struct sim_wire_data_t *wire);

void sim_BeginSimulation();

void sim_StopSimulation();

void sim_WireStep(struct sim_wire_data_t *wire);

void sim_DeviceStep();

void sim_Step();


#endif