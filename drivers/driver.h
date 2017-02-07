#pragma once

#include <stdint.h>

static const uint8_t SENS_HI  = 0x01;
static const uint8_t SENS_LO  = 0x02;
static const uint8_t SENS_MID = 0x03;

static const uint8_t MOVE_DONE = 0x10;

typedef struct AxisEvent {
	uint8_t type;
	uint32_t value;
} AxisEvent;

typedef struct AxisImpl AxisImpl;

typedef struct Axis {
	AxisImpl *impl;
	void *fn_get_counter;
	void *fn_set_counter;
	void *fn_set_handler;
	void *fn_move;
	void *fn_move_async;
} Axis;

uint32_t axis_get_counter(const Axis *self);
void axis_set_counter(Axis *self, uint32_t value);

void axis_set_handler(Axis *self, void (*handler)(AxisEvent event, void *data), void *data);

uint32_t axis_move(Axis *self, uint8_t dir, uint32_t count, uint32_t period_us);
void axis_move_async(Axis *self, uint8_t dir, uint32_t count, uint32_t period_us);


typedef struct DriverImpl DriverImpl;

typedef struct Driver {
	DriverImpl *impl;
	void *module; // dlopen shared object handle
} Driver;

int driver_create_axis(Driver *self, Axis *axis, const char * const params[]);
int driver_destroy_axis(Driver *self, Axis *axis);

int load_driver(Driver *driver, const char *path, const char * const params[]);
int free_driver(Driver *driver);
