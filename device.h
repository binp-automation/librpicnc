#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "generator.h"
#include "command.h"

#include "ringbuffer.h"
define_ringbuffer(RBC, rbc, Cmd)

#define MAX_AXES 8

typedef struct {
	Axis axes[MAX_AXES];
	int axis_count;
} Device;

int dev_init(Device *dev, int axis_count) {
	dev->axis_count = axis_count;
	return 0;
}

int dev_free(Device *dev) {
	return 0;
}

typedef struct {
	Device *dev;
	Generator *gen;
	Cmd (*get_cmd)(int, void*);
	void *userdata;
	gpioPulse_t *pulses;
	int pulse_count;
	int reuse_pulses;
	int reuse_count;
} _DevRunCookie;

void _dev_run_alert(int gpio, int level, uint32_t tick, void *userdata) {
	_DevRunCookie *cookie = (_DevRunCookie*) userdata;
	Device *dev = cookie->dev;
	Generator *gen = cookie->gen;
	int i;
	for (i = 0; i < dev->axis_count; ++i) {
		if(dev->axes[i].pin_left == gpio && level) {
			printf("axis %d left\n", i);
			gen_stop(gen);
		} else if(dev->axes[i].pin_right == gpio && level) {
			printf("axis %d right\n", i);
			gen_stop(gen);
		}
	}
}

typedef struct {
	int axis;
	Cmd (*get_cmd)(int, void*);
	void *userdata;
} _AxisGetCmdCookie;

Cmd _axis_get_cmd(void *userdata) {
	_AxisGetCmdCookie *cookie = (_AxisGetCmdCookie*) userdata;
	Cmd cmd = cookie->get_cmd(cookie->axis, cookie->userdata);
	// printf("axis %d cmd %d\n", cookie->axis, cmd.type);
	return cmd;
}

int _dev_run_get_wave(void *userdata) {
	_DevRunCookie *cookie = (_DevRunCookie*) userdata;
	Device *dev = cookie->dev;

	gpioPulse_t *pulses = cookie->pulses;
	int pulse_count = cookie->pulse_count;

	_AxisGetCmdCookie axis_cookie;
	axis_cookie.get_cmd = cookie->get_cmd;
	axis_cookie.userdata = cookie->userdata;

	int i, ax;
	for (ax = 0; ax < dev->axis_count; ++ax) {
		if (dev->axes[ax].state.idle) {
			axis_cookie.axis = ax;
			axis_set_cmd(&dev->axes[ax], _axis_get_cmd((void*) &axis_cookie));
		}
	}

	if (!cookie->reuse_pulses) {
		int total = 0;
		for (i = 0; i < pulse_count - 2; ++i) {
			int nax = -1, remain = 0;
			for (ax = 0; ax < dev->axis_count; ++ax) {
				if (!dev->axes[ax].state.idle && (nax < 0 || dev->axes[ax].state.remain < remain)) {
					nax = ax;
					remain = dev->axes[ax].state.remain;
				}
			}
			if (nax < 0) {
				break;
			}
			// printf("nax: %d, remain: %d, steps: %d\n", nax, remain, dev->axes[nax].state.move.steps);
			if (remain > 0) {
				total += remain;
				for (ax = 0; ax < dev->axis_count; ++ax) {
					dev->axes[ax].state.remain -= remain;
				}
				pulses[i].usDelay = remain;
				pulses[i].gpioOn = 0;
				pulses[i].gpioOff = 0;
			} else {
				axis_cookie.axis = nax;
				PinAction pa = axis_step(&dev->axes[nax], _axis_get_cmd, (void*) &axis_cookie);
				pulses[i].gpioOn = pa.on;
				pulses[i].gpioOff = pa.off;
				pulses[i].usDelay = 0;
			}
			// printf("pulse: on: %d, off: %d, delay: %d\n", pulses[i].gpioOn, pulses[i].gpioOff, pulses[i].usDelay);
		}
		pulse_count = i + 2;
		// printf("total: %d\n", total);
		// printf("pulses: %d\n", pulse_count);

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

	// printf("wid: %d\n", wave);

	return wave;
}

int dev_run(Device *dev, Generator *gen, Cmd (*get_cmd)(int axis, void *userdata), void *userdata) {
	_DevRunCookie cookie;
	cookie.dev = dev;
	cookie.gen = gen;

	cookie.get_cmd = get_cmd;
	cookie.userdata = userdata;

	cookie.pulse_count = 0x100;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);

	cookie.reuse_pulses = 0;
	cookie.reuse_count = 0;

	int i;

	for (i = 0; i < dev->axis_count; ++i) {
		gpioSetAlertFuncEx(dev->axes[i].pin_left, _dev_run_alert, (void*) &cookie);
		gpioSetAlertFuncEx(dev->axes[i].pin_right, _dev_run_alert, (void*) &cookie);
	}

	gen_run(gen, _dev_run_get_wave, (void*) &cookie);

	for (i = 0; i < dev->axis_count; ++i) {
		gpioSetAlertFuncEx(dev->axes[i].pin_left, NULL, NULL);
		gpioSetAlertFuncEx(dev->axes[i].pin_right, NULL, NULL);
	}

	free(cookie.pulses);

	return 0;
}

int dev_clear(Device *dev) {
	int i;
	for (i = 0; i < dev->axis_count; ++i) {
		_axis_state_init(&dev->axes[i].state);
	}
	return 0;
}