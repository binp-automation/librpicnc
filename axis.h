#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "generator.h"

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

int axis_init(Axis *axis, int step, int dir, int left, int right) {
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

int axis_set_dir(Axis *axis, int dir) {
	if (dir == 0) {
		if (!gpioRead(axis->pin_left)) {
			gpioWrite(axis->pin_dir, 0);
			return 0;
		} else {
			return -1;
		}
	} else {
		if (!gpioRead(axis->pin_right)) {
			gpioWrite(axis->pin_dir, 1);
			return 0;
		} else {
			return -1;
		}
	}
}

typedef struct {
	Axis *axis;
	Generator *gen;
	int counter;
} _AxisScanUserData;

void _axis_scan_cb(int gpio, int level, uint32_t tick, void *_userdata) {
	_AxisScanUserData *userdata = (_AxisScanUserData*) _userdata;
	Axis *axis = userdata->axis;
	Generator *gen = userdata->gen;
	if(axis->pin_left == gpio && level) {
		printf("left\n");
		userdata->counter = gen_position(gen);
		gen_stop(gen);
	} else if(axis->pin_right == gpio && level) {
		printf("right\n");
		gen_stop(gen);
	}
}

#define _AXIS_WAVE_SIZE 0x100
gpioPulse_t _axis_pulses[_AXIS_WAVE_SIZE + 2];

int _axis_gen_wave(void *_userdata) {
	_AxisScanUserData *userdata = (_AxisScanUserData*) _userdata;
	Axis *axis = userdata->axis;
	gpioPulse_t *pulses = _axis_pulses;

	int i;
	int mask = (1<<axis->pin_step);
	int delay = 1000;
	for (i = 0; i < _AXIS_WAVE_SIZE; ++i) {
		pulses[i].usDelay = delay;
		pulses[i].gpioOn =  mask*((i+1)%2);
		pulses[i].gpioOff = mask*(i%2);
	}

	// dummy last pulses (never executed)
	for (i = 0; i < 2; ++i) {
		pulses[_AXIS_WAVE_SIZE + i].usDelay = 1;
		pulses[_AXIS_WAVE_SIZE + i].gpioOn = 0;
		pulses[_AXIS_WAVE_SIZE + i].gpioOff = 0;
	}
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(_AXIS_WAVE_SIZE + 2, pulses);
	int wave = gpioWaveCreate();

	return wave;
}

int axis_scan(Axis *axis, Generator *gen) {
	_AxisScanUserData userdata;
	userdata.axis = axis;
	userdata.gen = gen;

	gpioSetAlertFuncEx(axis->pin_left, _axis_scan_cb, (void*) &userdata);
	gpioSetAlertFuncEx(axis->pin_right, _axis_scan_cb, (void*) &userdata);

	if(axis_set_dir(axis, 1) == 0) {
		gen_run(gen, _axis_gen_wave, (void*) &userdata);
		gen_clear(gen);
	}

	userdata.counter = 0;
	if(axis_set_dir(axis, 0) == 0) {
		gen->counter = 0;
		gen_run(gen, _axis_gen_wave, (void*) &userdata);
		axis->length = gen->counter + userdata.counter;
		gen_clear(gen);
	}

	printf("counter: %d\n", userdata.counter);

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	return 0;
}