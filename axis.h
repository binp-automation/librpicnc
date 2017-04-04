#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "utility.h"
#include "command.h"
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
	int direction;
	
	// current state
	int idle;
	gpioPulse_t pulse;
	Cmd cmd;
	int dir;
	int dir_set;
	int32_t steps;
	int     state;
	uint32_t remain;
} Axis;

int axis_init(Axis *axis, int step, int dir, int left, int right) {
	axis->pin_step  = step;
	axis->pin_dir   = dir;
	axis->pin_left  = left;
	axis->pin_right = right;
	
	axis->position = 0;
	axis->length = 0;
	
	axis->idle = 0;
	axis->cmd = cmd_none();
	axis->steps = 0;
	axis->state = 0;
	axis->remain = 0;

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

int axis_set_direction(Axis *axis, int dir) {
	if (dir == 0) {
		if (!gpioRead(axis->pin_left)) {
			gpioWrite(axis->pin_dir, 0);
			axis->direction = 0;
			return 0;
		} else {
			return -1;
		}
	} else {
		if (!gpioRead(axis->pin_right)) {
			gpioWrite(axis->pin_dir, 1);
			axis->direction = 1;
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
	gpioPulse_t *pulses;
	int pulse_count;
} _AxisScanCookie;

void _axis_scan_alert(int gpio, int level, uint32_t tick, void *userdata) {
	_AxisScanCookie *cookie = (_AxisScanCookie*) userdata;
	Axis *axis = cookie->axis;
	Generator *gen = cookie->gen;
	if(axis->pin_left == gpio && level) {
		printf("left\n");
		cookie->counter = gen_position(gen);
		gen_stop(gen);
	} else if(axis->pin_right == gpio && level) {
		printf("right\n");
		gen_stop(gen);
	}
}

int _axis_get_wave(void *userdata) {
	_AxisScanCookie *cookie = (_AxisScanCookie*) userdata;
	Axis *axis = cookie->axis;
	gpioPulse_t *pulses = cookie->pulses;
	int pulse_count = cookie->pulse_count;

	int i;
	int mask = (1<<axis->pin_step);
	int delay = 1000;
	for (i = 0; i < pulse_count - 2; ++i) {
		pulses[i].usDelay = delay;
		pulses[i].gpioOn =  mask*((i+1)%2);
		pulses[i].gpioOff = mask*(i%2);
	}

	// dummy last pulses (never executed)
	for (i = pulse_count - 2; i < pulse_count; ++i) {
		pulses[i].usDelay = 1;
		pulses[i].gpioOn = 0;
		pulses[i].gpioOff = 0;
	}
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(pulse_count, pulses);
	int wave = gpioWaveCreate();

	printf("wid: %d\n", wave);

	return wave;
}

int axis_scan(Axis *axis, Generator *gen) {
	_AxisScanCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = 0x100;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);

	gpioSetAlertFuncEx(axis->pin_left, _axis_scan_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_scan_alert, (void*) &cookie);

	if(axis_set_direction(axis, 1) == 0) {
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
	}

	cookie.counter = 0;
	if(axis_set_direction(axis, 0) == 0) {
		gen->counter = 0;
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		axis->length = gen->counter + cookie.counter;
		gen_clear(gen);
	}

	printf("counter: %d\n", cookie.counter);

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}

int _axis_next_pulse(Axis *axis, Cmd (*get_cmd)(void*), void *userdata) {
	if (axis->cmd.type == CMD_NONE) {
		axis->cmd = get_cmd(userdata);
		axis->idle = 0;
		if (axis->cmd.type == CMD_NONE) {
			axis->idle = 1;
		} else if (axis->cmd.type == CMD_WAIT) {
			// nothing
		} else if (axis->cmd.type == CMD_MOVE) {
			CmdMove cmd = axis->cmd.move;
			if (cmd.steps == 0) {
				axis->cmd = cmd_none();
			} else {
				axis->dir = cmd.steps > 0 ? 1 : 0;
				axis->steps = cmd.steps > 0 ? cmd.steps : -cmd.steps;
				axis->dir_set = 0;
				axis->direction = axis->dir;
			}
		}
	}

	// get next pulse
	if (axis->cmd.type == CMD_NONE) {
		// nothing
	} else if (axis->cmd.type == CMD_WAIT) {
		axis->pulse.gpioOn = 0;
		axis->pulse.gpioOff = 0;
		axis->remain = axis->cmd.wait.duration;
		axis->cmd = cmd_none();
	} else if (axis->cmd.type == CMD_MOVE) {
		if (!axis->dir_set) {
			axis->remain = 0;
			if (axis->dir == 0) {
				axis->pulse.gpioOn = 0;
				axis->pulse.gpioOff = 1<<axis->pin_dir;
			} else {
				axis->pulse.gpioOn = 1<<axis->pin_dir;
				axis->pulse.gpioOff = 0;
			}
			axis->dir_set = 1;
		} else {
			if (axis->steps > 0) {
				int delay = FREQ_TO_US(axis->cmd.move.speed);
				if (axis->state == 0) {
					axis->state = 1;
					axis->remain = (delay - 1)/2 + 1;
					axis->pulse.gpioOn = 1<<axis->pin_step;
					axis->pulse.gpioOff = 0;
				} else {
					axis->state = 0;
					axis->remain = delay/2;
					axis->pulse.gpioOn = 0;
					axis->pulse.gpioOff = 1<<axis->pin_step;
					axis->steps -= 1;
				}
			} 
			if (axis->steps == 0) {
				axis->cmd = cmd_none();
			}
		}
	}

	return 0;
}