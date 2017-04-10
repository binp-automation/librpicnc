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

typedef struct {

} _AxisCmdNone;

typedef struct {

} _AxisCmdWait;


#define MAX_DELAY 1000000 // us
#define POST_DELAY 1000 // us

typedef struct {
	int stage;
	int phase;
	float speed;
	uint32_t steps;
	uint32_t ramp;
	uint32_t ramp_steps;
} _AxisCmdMove;

typedef struct {
	int idle;
	int done;
	Cmd cmd;
	uint32_t remain; // us
	union {
		_AxisCmdNone none;
		_AxisCmdWait wait;
		_AxisCmdMove move;
	};
} _AxisState;

void _axis_state_init(_AxisState *st) {
	st->idle = 1;
	st->done = 1;
	st->cmd = cmd_none();
	st->remain = 0;
}

typedef struct {
	uint32_t on;
	uint32_t off;
} PinAction;

PinAction new_pin_action() {
	PinAction pa;
	pa.on = 0;
	pa.off = 0;
	return pa;
}

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
	_AxisState state;
} Axis;

int axis_init(Axis *axis, int step, int dir, int left, int right) {
	axis->pin_step  = step;
	axis->pin_dir   = dir;
	axis->pin_left  = left;
	axis->pin_right = right;
	
	axis->position = 0;
	axis->length = 0;
	
	_axis_state_init(&axis->state);

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

void axis_set_cmd(Axis *axis, Cmd cmd) {
	_AxisState *st = &axis->state;
	st->cmd = cmd;
	st->idle = 0;
	st->done = 0;
	if (cmd.type == CMD_NONE) {
		st->idle = 1;
	} else if (cmd.type == CMD_WAIT) {
		st->remain = cmd.wait.duration;
		st->done = 1;
	} else if (cmd.type == CMD_MOVE) {
		st->remain = 0;
		st->move.stage = 0;
		if (cmd.move.steps != 0) {
			st->move.steps = cmd.move.steps > 0 ? cmd.move.steps : -cmd.move.steps;
			st->move.phase = 0;
			st->move.speed = 0.0;
			st->move.ramp = floor((st->cmd.move.speed*st->cmd.move.speed)/(2*st->cmd.move.accel));
			if (2*st->move.ramp > st->move.steps) {
				st->move.ramp = (st->move.steps - 1)/2 + 1;
			}
			st->move.ramp_steps = 0;
		} else {
			st->done = 1;
		}
	} else {
		st->done = 1;
	}
}

PinAction axis_eval_cmd(Axis *axis) {
	_AxisState *st = &axis->state;
	PinAction pa = new_pin_action();
	if (!st->done) {
		if (st->cmd.type == CMD_NONE) {
			st->done = 1;
		} else if (st->cmd.type == CMD_WAIT) {
			st->done = 1;
		} else if (st->cmd.type == CMD_MOVE) {
			if (st->move.stage == 0) { // set direction
				if (st->cmd.move.steps > 0) {
					pa.on = 1 << axis->pin_dir;
					axis->direction = 1;
				} else {
					pa.off = 1 << axis->pin_dir;
					axis->direction = 0;
				}
				st->remain = 0;
				st->move.stage += 1;
			} else {
				uint32_t delay = MAX_DELAY;
				if (st->move.stage == 1) { // accelerate
					float speed = sqrt(2*st->cmd.move.accel*(st->move.ramp_steps + 0.5));
					delay = FREQ_TO_US(speed);
					if (delay > MAX_DELAY) {
						delay = MAX_DELAY;
					}
					st->move.ramp_steps += 1;
					if (st->move.ramp_steps > st->move.ramp) {
						if (st->move.steps < st->move.ramp) {
							st->move.stage += 2;
						} else {
							st->move.stage += 1;
						}
					}
				} else if (st->move.stage == 2) { // move
					delay = FREQ_TO_US(st->cmd.move.speed);
					if (st->move.ramp >= st->move.steps) {
						st->move.stage += 1;
					}
				} else if (st->move.stage == 3) { // decelerate
					float speed = sqrt(2*st->cmd.move.accel*(st->move.steps - 0.5));
					delay = FREQ_TO_US(speed);
					if (delay > MAX_DELAY) {
						delay = MAX_DELAY;
					}
				} else if (st->move.stage == 4) {
					st->remain = POST_DELAY;
					st->done = 1;
				}
				if (st->move.steps > 0) {
					if (st->move.phase == 0) {
						pa.on = 1 << axis->pin_step;
						st->remain = delay/2;
						st->move.phase = 1;
					} else {
						pa.off = 1 << axis->pin_step;
						st->remain = (delay - 1)/2 + 1;
						st->move.phase = 0;
						st->move.steps -= 1;
					}
				}
				if (st->move.steps <= 0) {
					st->move.stage = 4;
				}
			}
		} else {
			st->done = 1;
		}
	}
	return pa;
}

PinAction axis_step(Axis *axis, Cmd (*get_cmd)(void*), void *userdata) {
	// _AxisState *st = &axis->state;
	if (axis->state.idle || axis->state.done) {
		axis_set_cmd(axis, get_cmd(userdata));
	}
	return axis_eval_cmd(axis);
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
	for (i = 0; i < pulse_count - 2; ++i) {
		PinAction pa = axis_eval_cmd(axis);
		pulses[i].usDelay = axis->state.remain;
		pulses[i].gpioOn =  pa.on;
		pulses[i].gpioOff = pa.off;
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

int axis_scan(Axis *axis, Generator *gen, float speed, float accel) {
	int pulse_count = 0x100;

	_AxisScanCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = pulse_count;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);

	gpioSetAlertFuncEx(axis->pin_left, _axis_scan_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_scan_alert, (void*) &cookie);

	if (!gpioRead(axis->pin_right)) {
		axis_set_cmd(axis, cmd_move(0x7fffffff, speed, accel));
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
	}

	cookie.counter = 0;
	if(!gpioRead(axis->pin_left)) {
		gen->counter = 0;
		axis_set_cmd(axis, cmd_move(-0x80000000, speed, accel));
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		axis->length = ((gen->counter/pulse_count)*(pulse_count - 2) + cookie.counter)/4;
		gen_clear(gen);
		_axis_state_init(&axis->state);
	}

	printf("counter: %d\n", cookie.counter);

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}
