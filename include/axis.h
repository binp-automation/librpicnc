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
	// gpio masks
	uint32_t mask_step_pos;
	uint32_t mask_step_neg;
	uint32_t mask_dir_pos;
	uint32_t mask_dir_neg;

	int sense;
	uint32_t pin_left;
	uint32_t pin_right;
	
	// location
	int32_t position;
	uint32_t length;
	
	// current state
	_AxisState state;
} Axis;

int axis_init(
	Axis *axis,
	uint32_t mask_step_pos, uint32_t mask_step_neg,
	uint32_t mask_dir_pos, uint32_t mask_dir_neg,
	int sense, uint32_t left, uint32_t right
);
int axis_free(Axis *axis);

void axis_set_cmd(Axis *axis, Cmd cmd);
PinAction axis_eval_cmd(Axis *axis);
PinAction axis_step(Axis *axis, Cmd (*get_cmd)(void*), void *userdata);
uint8_t axis_read_sensors(Axis *axis);
