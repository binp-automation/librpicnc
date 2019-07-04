#pragma once

#include <unistd.h>

#include "command.h"


typedef struct {
	int idle;
	int done;
	Cmd cmd;
	uint32_t remain; // us
	uint32_t steps;
	uint8_t phase;
	uint8_t dir;
} _AxisState;

void _axis_state_init(_AxisState *st);

typedef struct {
	uint32_t on;
	uint32_t off;
} PinAction;

PinAction new_pin_action();

typedef struct Axis {
	// gpio pins
	int pin_step;
	int pin_dir;
	int pin_left;
	int pin_right;
	
	// location
	uint32_t length;
	int32_t position;
	
	// current state
	_AxisState state;
} Axis;

int axis_init(Axis *axis, int step, int dir, int left, int right);
int axis_free(Axis *axis);

void axis_set_cmd(Axis *axis, Cmd cmd);
PinAction axis_eval_cmd(Axis *axis);
PinAction axis_step(Axis *axis, Cmd (*get_cmd)(void*), void *userdata);
uint8_t axis_read_sensors(Axis *axis);
