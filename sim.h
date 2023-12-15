#ifndef SIM_H
#define SIM_H

#include <stdint.h>
#include "dev.h"
#include "wire.h"

/* 1 ns per step */
#define SIM_SUBSTEPS_PER_SECOND 1000000000
#define SIM_MAX_SUBSTEPS_PER_FRAME 10

enum SIM_STEPS
{
    SIM_STEP_DEVICE,
    SIM_STEP_WIRE,
    SIM_STEP_LAST
};

struct sim_wire_data_t 
{
    uint32_t    first_pin[2];
    uint16_t    pin_count[2];
    uint16_t    source_count;
    // uint16_t    queued;
    uint8_t     queued;
    uint8_t     value:          7;
    // uint8_t     mos_source:     1;
    // uint8_t     immediate : 1;
    // uint16_t value;
}; 

struct sim_dev_data_t
{
    struct dev_t *  device;
    uint32_t        first_pin;
    uint16_t        type;
    uint8_t         queued;
    uint8_t         pad;
}; 

struct sim_dev_pin_t
{
    uint64_t wire:          48;
    uint64_t value:         4;
    uint64_t mos_source:    1;
};

struct sim_clock_data_t
{
    uint64_t        half_period;
    uint64_t        substeps_left;
    uint64_t        device;
};

void sim_Init();

void sim_Shutdown();

uint32_t sim_CmpXchg8(uint8_t *location, uint32_t compare, uint32_t new_value);

uint64_t sim_XInc64(uint64_t *location);

void sim_QueueDevice(struct sim_dev_data_t *device);

void sim_QueueWire(struct sim_wire_data_t *wire);

void sim_QueueSource(struct sim_dev_data_t *device);

void sim_BeginSimulation();

void sim_StopSimulation();

struct sim_wire_data_t *sim_GetWireSimData(uint64_t wire, uint32_t pin_type);

void sim_WireStep(struct sim_wire_data_t *wire);

void sim_IndWireStep(struct sim_wire_data_t *wire);

void sim_DeviceStep();

void sim_Step(uint32_t single_substep);


#endif