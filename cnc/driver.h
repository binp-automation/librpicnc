#pragma once

#include <stdint.h>

typedef struct CNC_Driver CNC_Driver;
typedef struct CNC_DriverImpl CNC_DriverImpl;

typedef struct CNC_Axis CNC_Axis;
typedef struct CNC_AxisImpl CNC_AxisImpl;
typedef struct CNC_AxisEvent CNC_AxisEvent;


struct CNC_Driver {
	CNC_DriverImpl *impl;
	void *module;
};

struct CNC_Axis {
	CNC_AxisImpl *impl;
	void **fnptrs;
};

CNC_Driver *cnc_create_driver(const char *path, void *params[]);
int cnc_destroy_driver(CNC_Driver *driver);

CNC_Axis *cnc_driver_create_axis(CNC_Driver *driver, void *params[]);

int cnc_destroy_axis(CNC_Axis *self);

int32_t cnc_axis_get_counter(CNC_Axis *self);
void cnc_axis_set_counter(CNC_Axis *self, int32_t value);

void cnc_axis_set_handler(CNC_Axis *self, void (*handler)(CNC_AxisEvent event, void *data), void *data);

int32_t cnc_axis_move(CNC_Axis *self, int32_t count, uint32_t period_us);
void cnc_axis_move_async(CNC_Axis *self, int32_t count, uint32_t period_us);

static const uint8_t CNC_EVENT_SENS = 0x10;
static const uint8_t CNC_EVENT_MOVE = 0x20;

struct CNC_AxisEvent {
	uint8_t type;
	void *value;
};