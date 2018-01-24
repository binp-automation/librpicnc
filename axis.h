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


#define sqr(x) ((x)*(x))

uint32_t isqrt(uint32_t x){
	uint32_t op  = x;
	uint32_t res = 0;
	uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	// "one" starts at the highest power of four <= than the argument.
	while (one > op) {
		one >>= 2;
	}
	while (one != 0) {
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
	return res;
}


#define MAX_DELAY 1000000 // us
#define POST_DELAY 1000 // us


typedef struct {
	int idle;
	int done;
	Cmd cmd;
	uint32_t remain; // us
	uint32_t steps;
	uint8_t phase;
	uint8_t dir; 
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
		if (cmd.move.steps != 0) {
			st->steps = cmd.move.steps;
			st->dir = cmd.move.dir;
		} else {
			st->done = 1;
		}
	} else if (cmd.type == CMD_ACCL) {
		st->remain = 0;
		if (cmd.accl.steps != 0) {
			st->steps = cmd.accl.steps;
			st->dir = cmd.accl.dir;
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
		uint32_t delay = 0;
		if (st->cmd.type == CMD_NONE) {
			st->done = 1;
		} else if (st->cmd.type == CMD_WAIT) {
			st->done = 1;
		} else if (st->cmd.type == CMD_MOVE) {
			delay = st->cmd.move.period;
		} else if (st->cmd.type == CMD_ACCL) {
			uint32_t tb = st->cmd.accl.begin_period;
			uint32_t te = st->cmd.accl.end_period;
			uint32_t i = st->steps;
			uint32_t n = st->cmd.accl.steps;
			if (te == 0) {
				delay = (tb*isqrt(2*n))/isqrt(2*i + 1);
			} else if (tb == 0) {
				delay = (te*isqrt(2*n))/isqrt(2*n - 2*i - 1);
			} else {
				delay = (tb*te*isqrt(2*n))/isqrt(tb*tb*(2*n - 2*i - 1) + te*te*(2*i + 1));
			}
		} else {
			st->done = 1;
		}

		if (!st->done) {
			if (st->dir) {
				pa.on = 1 << axis->pin_dir;
				axis->direction = 1;
			} else {
				pa.off = 1 << axis->pin_dir;
				axis->direction = 0;
			}
			if (st->steps > 0) {
				if (st->phase == 0) {
					pa.on = 1 << axis->pin_step;
					st->remain = delay/2;
					st->phase = 1;
				} else {
					pa.off = 1 << axis->pin_step;
					st->remain = (delay - 1)/2 + 1;
					st->phase = 0;
					st->steps -= 1;
				}
			}
			if (st->steps <= 0) {
				st->done = 1;
			}
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

int axis_scan(Axis *axis, Generator *gen, uint32_t period) {
	int pulse_count = 0x100;

	_AxisScanCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = pulse_count;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);

	gpioSetAlertFuncEx(axis->pin_left, _axis_scan_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_scan_alert, (void*) &cookie);

	if (!gpioRead(axis->pin_right)) {
		axis_set_cmd(axis, cmd_move(1, 0xffffffff, period));
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
	}

	gpioDelay(1000);

	cookie.counter = 0;
	if(!gpioRead(axis->pin_left)) {
		gen->counter = 0;
		axis_set_cmd(axis, cmd_move(0, 0xffffffff, period));
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
