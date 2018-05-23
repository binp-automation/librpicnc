#pragma once

#include "axis.h"


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
	cookie->cmd_list[0] = cmd_move_vel(dir, dist, 1e6/vel);
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
		cookie->cmd_list[0] = cmd_move_acc(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max);
		cookie->cmd_list[1] = cmd_move_vel(dir, dist - 2*acc_dist, 1e6/vel_max);
		cookie->cmd_list[2] = cmd_move_acc(dir, acc_dist, 1e6/vel_max, vel_ini < 1.0 ? 0 : 1e6/vel_ini);
		cookie->cmd_list[3] = cmd_idle();
	} else {
		acc_dist = dist/2;
		float vel_max_red = sqrt(vel_ini*vel_ini + 2.0*acc_max*acc_dist);
		cookie->cmd_list[0] = cmd_move_acc(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max_red);
		cookie->cmd_list[1] = cmd_move_acc(dir, acc_dist + (dist%2), 1e6/vel_max_red, vel_ini < 1.0 ? 0 : 1e6/vel_ini);
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
	cookie->cmd_list[0] = cmd_move_acc(dir, acc_dist, vel_ini < 1.0 ? 0 : 1e6/vel_ini, 1e6/vel_max);
	cookie->cmd_list[1] = cmd_move_vel(dir, vel_dist, 1e6/vel_max);
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
		axis->length = ((gen->counter*(pulse_count - 2))/pulse_count + cookie.counter)/4 - 2100; // Why 2100?
	}
	axis->position = 0;

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

	float stable_mul = 0.75;
	float acc_vel_factor = 0.95;
	int max_depth = 3;
	int attemts = 3;

	float vel_stable = *vel_ini;

	// test initial velocity
	if (*vel_ini < 1.0) {
		*vel_ini = 1.0;
	}
	int test_dist = 100;
	int depth = 0;
	float vl = *vel_ini, vr = -1.0;
	while (depth <= max_depth) {
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

		int stable = 1;
		int j;
		for (j = 0; j < attemts; ++j) {
			// move left if we aren't already here
			if (!gpioRead(axis->pin_left)) {
				_axis_move(&cookie, 0, 0xffffffff, vel_stable);
			}

			// move to test_dist
			_axis_move(&cookie, 1, test_dist, vel_stable);

			// test speed
			_axis_move(&cookie, 0, 2*test_dist, v);

			if (!gpioRead(axis->pin_left)) {
				// speed is too high
				vr = v;
				stable = 0;
				break;
			}
		}

		// if motion is stable
		if (stable) {
			vl = v;
			vel_stable = stable_mul*v;
		}
	}

	*vel_ini = vel_stable;
	if (*vel_max < vel_stable) {
		*vel_max = vel_stable;
	}

	// investigate av curve
	float acc_stable = *acc_max;
	float vel_acc_stable = *vel_max;
	float vel_acc_max = -1.0;

	int acc_depth = 0;
	float al = *acc_max/2, ar = -1.0;
	while (acc_depth <= max_depth) {
		// perform binary search
		float acc;
		if (ar < 0.0) {
			// while we don't know vr
			acc = 2*al;
		} else {
			// search between vl and vr
			acc = 0.5*(al + ar);
			acc_depth += 1;
		}

		int test_dist = 1000;
		int depth = 0;
		float vl = *vel_max, vr = -1.0;

		float vel_max_stable = -1.0;
		while (depth <= max_depth) {

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

			float dt = (v - *vel_ini)/acc;
			uint32_t acc_dist = (*vel_ini + 0.5*acc*dt)*dt;
			
			int stable = 1;
			int j;

			for (j = 0; j < attemts; ++j) {
				// move left if we aren't already here
				if (!gpioRead(axis->pin_left)) {
					_axis_move_acc_end(&cookie, 0, 0xffffffff, *vel_ini, vel_acc_stable, acc_stable);
				}

				// move to test_dist
				_axis_move_acc(&cookie, 1, test_dist + acc_dist, *vel_ini, vel_acc_stable, acc_stable);
				if (gpioRead(axis->pin_left)) {
					printf("[error] cannot move to destination\n");
					break;
				}

				// perform test
				_axis_move_acc_end(&cookie, 0, 2*test_dist, *vel_ini, v, acc);

				if (!gpioRead(axis->pin_left)) {
					// speed is too high
					vr = v;
					stable = 0;
					break;
				}
			}

			// if we had moved
			if (stable) {
				// acceptable speed
				vl = v;
				vel_max_stable = stable_mul*v;
			}
		}

		if (vel_max_stable > 0.0) {
			if (vel_acc_max < 0.0 || vel_acc_max < vel_max_stable) {
				vel_acc_max = vel_max_stable;
			}
			if (vel_max_stable < acc_vel_factor*vel_acc_max) {
				ar = acc;
			} else {
				al = acc;
				vel_acc_stable = vel_max_stable;
				acc_stable = stable_mul*acc;
			}
		} else {
			ar = acc;
		}

		printf("acc: %f, vel: %f\n", acc_stable, vel_acc_stable);
	}

	*vel_max = vel_acc_stable;
	*acc_max = acc_stable;

	// move to 0
	if (!gpioRead(axis->pin_left)) {
		_axis_move_acc_end(&cookie, 0, 0xffffffff, *vel_ini, *vel_max, *acc_max);
	}
	axis->position = 0;

	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}
