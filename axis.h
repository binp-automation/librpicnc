#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

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
	float speed;
	float phase;
	int32_t steps;
} Axis;

int axis_init(Axis *axis, int step, int dir, int left, int right, float accel) {
	axis->pin_step  = step;
	axis->pin_dir   = dir;
	axis->pin_left  = left;
	axis->pin_right = right;
	
	axis->position = 0;
	axis->length = 0;
	
	axis->speed = 0;
	axis->phase = 0;
	axis->steps = 0;

	gpioSetMode(axis->pin_step, PI_OUTPUT);
	gpioSetMode(axis->pin_dir,  PI_OUTPUT);
	gpioSetMode(axis->pin_left,  PI_INPUT);
	gpioSetMode(axis->pin_right, PI_INPUT);
	
	return 0;
}

int axis_free(Axis *axis) {
	if (axis) {
		gpioSetMode(axis->pin_step, PI_INPUT);
		gpioSetMode(axis->pin_dir,  PI_INPUT);
		gpioSetMode(axis->pin_left,  PI_INPUT);
		gpioSetMode(axis->pin_right, PI_INPUT);
		return 0;
	}
	return 1;
}
