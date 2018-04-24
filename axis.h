#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "command.h"
#include "generator.h"


#define sqr(x) ((x)*(x))

uint32_t isqrt(uint32_t x){
	uint32_t op  = x;
	uint32_t res = 0;
	// The second-to-top bit is set:
	//use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	uint32_t one = 1uL << 30;
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

uint64_t isqrt64(uint64_t x){
	uint64_t op  = x;
	uint64_t res = 0;
	// The second-to-top bit is set: 
	// use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	uint64_t one = (uint64_t)1 << 62; 
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
	st->idle = 0;
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
	st->remain = 0;
	if (cmd.type == CMD_NONE) {
		st->done = 1;
	} else if (cmd.type == CMD_IDLE) {
		st->idle = 1;
	} else if (cmd.type == CMD_WAIT) {
		st->remain = cmd.wait.duration;
		st->done = 1;
	} else if (cmd.type == CMD_SYNC) {
		st->idle = 1;
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
	if (!st->idle && !st->done) {
		uint32_t delay = 0;
		if (st->cmd.type == CMD_MOVE) {
			delay = st->cmd.move.period;
		} else if (st->cmd.type == CMD_ACCL) {
			uint64_t tb = st->cmd.accl.begin_period;
			uint64_t te = st->cmd.accl.end_period;
			uint64_t i = st->steps;
			uint64_t n = st->cmd.accl.steps;
			if (te == 0) {
				delay = (tb*isqrt64(2*n))/isqrt64(2*i + 1);
			} else if (tb == 0) {
				delay = (te*isqrt64(2*n))/isqrt64(2*n - 2*i - 1);
			} else {
				delay = (tb*te*isqrt64(2*n))/isqrt64(tb*tb*(2*n - 2*i - 1) + te*te*(2*i + 1));
			}
		} else {
			st->done = 1;
		}

		if (delay > MAX_DELAY) {
			delay = MAX_DELAY;
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
					st->remain = delay/2 + (delay % 2);
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

	Cmd *cmd_list;
	int current_cmd;
	int reuse_pulses;
	int reuse_count;
} _AxisCookie;

void _axis_alert(int gpio, int level, uint32_t tick, void *userdata) {
	_AxisCookie *cookie = (_AxisCookie*) userdata;
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

Cmd _axis_get_cmd(void *userdata) {
	_AxisCookie *cookie = (_AxisCookie*) userdata;
	printf("current_cmd: %d\n", cookie->current_cmd);
	return cookie->cmd_list[cookie->current_cmd++];
}

int _axis_get_wave(void *userdata) {
	_AxisCookie *cookie = (_AxisCookie*) userdata;
	Axis *axis = cookie->axis;

	gpioPulse_t *pulses = cookie->pulses;
	int pulse_count = cookie->pulse_count;

	int i;
	if (!cookie->reuse_pulses) {
		int total = 0;
		for (i = 0; i < pulse_count - 2; ++i) {
			if (axis->state.idle) {
				break;
			}
			PinAction pa = axis_step(axis, _axis_get_cmd, userdata);
			pulses[i].gpioOn = pa.on;
			pulses[i].gpioOff = pa.off;
			pulses[i].usDelay = axis->state.remain;
			total += axis->state.remain;
		}
		pulse_count = i + 2;

		// dummy last pulses (never executed)
		for (i = pulse_count - 2; i < pulse_count; ++i) {
			pulses[i].usDelay = 1;
			pulses[i].gpioOn = 0;
			pulses[i].gpioOff = 0;
		}
	} else {
		pulse_count = cookie->reuse_count;
	}

	if (pulse_count <= 2) {
		return -1;
	}

	gpioWaveAddNew();

	gpioWaveAddGeneric(pulse_count, pulses);
	int wave = gpioWaveCreate();

	if (wave < 0) {
		cookie->reuse_pulses = 1;
		cookie->reuse_count = pulse_count;
	} else {
		cookie->reuse_pulses = 0;
	}

	return wave;
}

int axis_scan(Axis *axis, Generator *gen, float vel_ini, float vel_max, float acc_max) {
	int pulse_count = 0x100;
	Cmd cmdbuf[0x10];

	_AxisCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = pulse_count;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);
	cookie.reuse_pulses = 0;
	cookie.cmd_list = cmdbuf;

	gpioSetAlertFuncEx(axis->pin_left, _axis_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_alert, (void*) &cookie);

	if (vel_max < vel_ini) {
		vel_max = vel_ini;
	}

	float dt = (vel_max - vel_ini)/acc_max;
	uint32_t dist_acc = (vel_ini + 0.5*acc_max*dt)*dt;

	if (!gpioRead(axis->pin_right)) {
		cmdbuf[0] = cmd_accl(1, dist_acc, 1e6/vel_ini, 1e6/vel_max);
		cmdbuf[1] = cmd_move(1, 0xffffffff, 1e6/vel_max);
		cmdbuf[2] = cmd_idle();
		cookie.current_cmd = 0;
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
	}

	gpioDelay(1000);

	cookie.counter = 0;
	if(!gpioRead(axis->pin_left)) {
		gen->counter = 0;
		cmdbuf[0] = cmd_accl(0, dist_acc, 1e6/vel_ini, 1e6/vel_max);
		cmdbuf[1] = cmd_move(0, 0xffffffff, 1e6/vel_max);
		cmdbuf[2] = cmd_idle();
		cookie.current_cmd = 0;
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

int axis_calib(Axis *axis, Generator *gen, float *vel_ini, float *vel_max, float *acc_max) {
	int pulse_count = 0x100;
	Cmd cmdbuf[0x10];

	_AxisCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = pulse_count;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);
	cookie.reuse_pulses = 0;
	cookie.cmd_list = cmdbuf;

	gpioSetAlertFuncEx(axis->pin_left, _axis_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_alert, (void*) &cookie);

	float vel_stable = *vel_ini;

	float stable_mul = 0.5;
	int max_depth = 4;

	// test initial velocity
	int test_dist = 500;
	float vl = *vel_ini, vr = -1.0;
	int depth = 0;
	while (1) {
		// move left if we aren't already here
		printf("move to 0\n");
		if (!gpioRead(axis->pin_left)) {
			cmdbuf[0] = cmd_move(0, 0xffffffff, 1e6/vel_stable);
			cmdbuf[1] = cmd_idle();
			cookie.current_cmd = 0;
			gen_run(gen, _axis_get_wave, (void*) &cookie);
			gen_clear(gen);
			_axis_state_init(&axis->state);
			gpioDelay(1000);
		}

		if (depth > max_depth) {
			break;
		}

		// move to test_dist
		printf("move to %d\n", test_dist);
		cmdbuf[0] = cmd_move(1, test_dist, 1e6/vel_stable);
		cmdbuf[1] = cmd_idle();
		cookie.current_cmd = 0;
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
		gpioDelay(1000);

		// perform binary search
		float v;
		if (vr < 0.0) {
			// while we don't know vr
			v = 2*vl;
		} else {
			// search between vl and vr
			v = 0.5*(vl + vr);
			depth += 1;
		}
		printf("test velocity %f\n", v);
		cmdbuf[0] = cmd_move(0, 2*test_dist, 1e6/v);
		cmdbuf[1] = cmd_idle();
		cookie.current_cmd = 0;
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
		gpioDelay(1000);

		// if we had moved
		if (gpioRead(axis->pin_left)) {
			// acceptable speed
			vl = v;
			vel_stable = stable_mul*v;
		} else {
			// speed is too high
			vr = v;
		}
	}

	*vel_ini = vel_stable;

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}

uint8_t axis_read_sensors(Axis *axis) {
	return ((!!gpioRead(axis->pin_left))<<0) | ((!!gpioRead(axis->pin_right))<<1);
}
