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
		//printf("left\n");
		cookie->counter = gen_position(gen);
		gen_stop(gen);
	} else if(axis->pin_right == gpio && level) {
		//printf("right\n");
		gen_stop(gen);
	}
}

Cmd _axis_get_cmd(void *userdata) {
	_AxisCookie *cookie = (_AxisCookie*) userdata;
	//printf("current_cmd: %d\n", cookie->current_cmd);
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

int _gen_eval(_AxisCookie *cookie) {
	gen_run(cookie->gen, _axis_get_wave, (void*) cookie);
	gen_clear(cookie->gen);
	_axis_state_init(&cookie->axis->state);
	gpioDelay(100000);
	return 0;
}

int _axis_move(_AxisCookie *cookie, uint8_t dir, uint32_t dist, float vel) {
	cookie->cmd_list[0] = cmd_move(dir, dist, 1e6/vel);
	cookie->cmd_list[1] = cmd_idle();
	cookie->current_cmd = 0;
	_gen_eval(cookie);
	return 0;
}

int _axis_move_acc(
	_AxisCookie *cookie, uint8_t dir, uint32_t dist, 
	float vel_ini, float vel_max, float acc_max
) {
	float dt = (vel_max - vel_ini)/acc_max;
	uint32_t acc_dist = (vel_ini + 0.5*acc_max*dt)*dt;
	if (2*acc_dist < dist) {
		cookie->cmd_list[0] = cmd_accl(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max);
		cookie->cmd_list[1] = cmd_move(dir, dist - 2*acc_dist, 1e6/vel_max);
		cookie->cmd_list[2] = cmd_accl(dir, acc_dist, 1e6/vel_max, vel_ini < 1.0 ? 0 : 1e6/vel_ini);
		cookie->cmd_list[3] = cmd_idle();
	} else {
		acc_dist = dist/2;
		float vel_max_red = sqrt(vel_ini*vel_ini + 2.0*acc_max*acc_dist);
		cookie->cmd_list[0] = cmd_accl(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max_red);
		cookie->cmd_list[1] = cmd_accl(dir, acc_dist + (dist%2), 1e6/vel_max_red, vel_ini < 1.0 ? 0 : 1e6/vel_ini);
		cookie->cmd_list[2] = cmd_idle();
	}
	cookie->current_cmd = 0;
	_gen_eval(cookie);
	return 0;
}

int _axis_move_acc_end(
	_AxisCookie *cookie, uint8_t dir, uint32_t vel_dist, 
	float vel_ini, float vel_max, float acc_max
) {
	float dt = (vel_max - vel_ini)/acc_max;
	uint32_t acc_dist = (vel_ini + 0.5*acc_max*dt)*dt;
	cookie->cmd_list[0] = cmd_accl(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max);
	cookie->cmd_list[1] = cmd_move(dir, vel_dist, 1e6/vel_max);
	cookie->cmd_list[2] = cmd_idle();
	cookie->current_cmd = 0;
	_gen_eval(cookie);
	return 0;
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

	if (!gpioRead(axis->pin_right)) {
		_axis_move_acc_end(&cookie, 1, 0xffffffff, vel_ini, vel_max, acc_max);
	}

	cookie.counter = 0;
	if(!gpioRead(axis->pin_left)) {
		gen->counter = 0;
		_axis_move_acc_end(&cookie, 0, 0xffffffff, vel_ini, vel_max, acc_max);
		axis->length = ((gen->counter/pulse_count)*(pulse_count - 2) + cookie.counter)/4;
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

	/*
	// test initial velocity
	if (*vel_ini < 1.0) {
		*vel_ini = 1.0;
	}
	int test_dist = 1000;
	int depth = 0;
	float vl = *vel_ini, vr = -1.0;
	while (1) {
		// move left if we aren't already here
		//printf("move to 0\n");
		if (!gpioRead(axis->pin_left)) {
			_axis_move(&cookie, 0, 0xffffffff, vel_stable);
		}

		if (depth > max_depth) {
			break;
		}

		// move to test_dist
		//printf("move to %d\n", test_dist);
		_axis_move(&cookie, 1, test_dist, vel_stable);

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
		//printf("test velocity %f\n", v);
		_axis_move(&cookie, 0, 2*test_dist, v);

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
	*/

	*vel_ini = vel_stable;
	if (*vel_max < vel_stable) {
		*vel_max = vel_stable;
	}

	// investigate av curve
	float acc_stable = *acc_max;
	float vel_acc_stable = *vel_max;

	int i = 0;
	for (i = 0; i < 10; ++i) {
		int test_dist = 1000;
		float acc = *acc_max*(i+1);
		int depth = 0;
		float vl = *vel_max, vr = -1.0;

		float vel_max_stable = -1.0;
		while (1) {
			
			// move left if we aren't already here
			//printf("move to 0\n");
			if (!gpioRead(axis->pin_left)) {
				_axis_move_acc_end(&cookie, 0, 0xffffffff, *vel_ini, vel_acc_stable, acc_stable);
			}

			if (depth > max_depth) {
				break;
			}

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

			//printf("v: %f, vel_ini: %f, acc: %f\n", v, *vel_ini, acc);
			float dt = (v - *vel_ini)/acc;
			uint32_t acc_dist = (*vel_ini + 0.5*acc*dt)*dt;
			//printf("dt: %f, dist_acc: %d\n", dt, acc_dist);

			// move to test_dist
			//printf("move to %d + %d\n", dist_acc, test_dist);
			_axis_move_acc(&cookie, 1, test_dist + acc_dist, *vel_ini, vel_acc_stable, acc_stable);
			if (gpioRead(axis->pin_left)) {
				printf("[error] cannot move to destination\n");
				break;
			}

			// perform test
			//printf("test velocity %f\n", v);
			_axis_move_acc_end(&cookie, 0, 2*test_dist, *vel_ini, v, acc);

			// if we had moved
			if (gpioRead(axis->pin_left)) {
				// acceptable speed
				vl = v;
				vel_max_stable = stable_mul*v;
			} else {
				// speed is too high
				vr = v;
			}
		}

		if (vel_max_stable > 0.0) {
			vel_acc_stable = vel_max_stable;
			acc_stable = stable_mul*acc;
		}

		printf("acc: %f, vel: %f\n", acc_stable, vel_acc_stable);
	}

	*vel_max = vel_acc_stable;
	*acc_max = acc_stable;

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}

uint8_t axis_read_sensors(Axis *axis) {
	return ((!!gpioRead(axis->pin_left))<<0) | ((!!gpioRead(axis->pin_right))<<1);
}
