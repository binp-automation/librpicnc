#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#define MODE_NONE 0x00
#define MODE_SCAN 0x11
#define MODE_MOVE 0x21

#define STAGE_NONE 0x00 
#define STAGE_SCAN_LR 0x11
#define STAGE_SCAN_RL 0x12
#define STAGE_MOVE_EN 0x21
#define STAGE_MOVE_BR 0x22

#define FREQ_TO_US(freq) \
	((uint32_t) (1e6/(freq)))

typedef struct Axis {
	int pin_step;
	int pin_dir;
	int pin_left;
	int pin_right;
	
	uint32_t length;
	int32_t position;
	
	float accel;
	
	int _mode;
	int _stage;
	
	uint32_t _last_tick;
	uint32_t _tick_count;
} Axis;

void _stop_all() {
	gpioWaveTxStop();
	gpioWaveClear();
}

int _wave_gen_step(int pin, float freq) {
	int delay = FREQ_TO_US(freq);
	
	gpioPulse_t pulses[2];
	pulses[0].usDelay = delay/2;
	pulses[0].gpioOn = 1<<pin;
	pulses[0].gpioOff = 0;
	pulses[1].usDelay = (delay - 1)/2 + 1;
	pulses[1].gpioOn = 0;
	pulses[1].gpioOff = 1<<pin;
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(2, pulses);
	return gpioWaveCreate();
}

int _wave_gen_ramp(int pin, float begin_freq, float end_freq, float accel) {
	int i;
	int steps = fabs(end_freq - begin_freq)*accel;
	
	gpioPulse_t *pulses = (gpioPulse_t *) malloc(sizeof(gpioPulse_t)*2*steps);
	
	for (i = 0; i < 2*steps; ++i) {
		int delay = FREQ_TO_US(begin_freq + (end_freq - begin_freq)*(0.5*i + 0.25)/steps);
		if (i % 2 == 0) {
			pulses[i].usDelay = delay/2;
			pulses[i].gpioOn = 1<<pin;
			pulses[i].gpioOff = 0;
		} else {
			pulses[i].usDelay = (delay - 1)/2 + 1;
			pulses[i].gpioOn = 0;
			pulses[i].gpioOff = 1<<pin;
		}
	}
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(2*steps, pulses);
	int wave = gpioWaveCreate();
	
	free(pulses);
	
	return wave;
}

int _set_dir_safe(Axis *axis, int dir) {
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

int _wave_wait() {
	while (gpioWaveTxBusy()) {
		gpioDelay(10000);
	}
	return 0;
}

void _axis_alert_cb(int gpio, int level, uint32_t tick, void *userdata) {
	Axis *axis = (Axis*) userdata;
	if (axis != NULL) {
		if(axis->pin_left == gpio && level) {
			printf("left\n");
			gpioWaveTxStop();
		} else if(axis->pin_right == gpio && level) {
			printf("right\n");
			gpioWaveTxStop();
		}
		if (axis->_mode == MODE_SCAN) {
			if(axis->pin_left == gpio && level) {
				if(axis->_stage == STAGE_SCAN_RL) {
					axis->_tick_count = tick - axis->_last_tick;
				}
			} else if(axis->pin_right == gpio && level) {
				if(axis->_stage == STAGE_SCAN_LR) {
					axis->_tick_count = tick - axis->_last_tick;
				}
			}
		} else if (axis->_mode == MODE_MOVE) {
			if(axis->pin_left == gpio && level) {
				axis->_stage = STAGE_MOVE_BR;
				axis->_tick_count = tick - axis->_last_tick;
			} else if(axis->pin_right == gpio && level) {
				axis->_stage = STAGE_MOVE_BR;
				axis->_tick_count = tick - axis->_last_tick;
			}
		} else {
			_stop_all();
		}
	} else {
		_stop_all();
	}
}

int axis_init(Axis *axis, int step, int dir, int left, int right, float accel) {
	axis->pin_step  = step;
	axis->pin_dir   = dir;
	axis->pin_left  = left;
	axis->pin_right = right;
	
	axis->position = 0;
	axis->length = 0;
	
	axis->accel = accel;
	
	axis->_mode = MODE_NONE;
	axis->_stage = STAGE_NONE;

	gpioSetMode(axis->pin_step, PI_OUTPUT);
	gpioSetMode(axis->pin_dir,  PI_OUTPUT);
	gpioSetMode(axis->pin_left,  PI_INPUT);
	gpioSetMode(axis->pin_right, PI_INPUT);
	
	gpioSetAlertFuncEx(axis->pin_left, _axis_alert_cb, (void*) axis);
	gpioSetAlertFuncEx(axis->pin_right, _axis_alert_cb, (void*) axis);
	
	return 0;
}

uint32_t _axis_move_rel(Axis *axis, float freq, uint32_t steps, int dir) {
	int i, steps_done = 0;
	
	axis->_mode = MODE_MOVE;
	axis->_stage = STAGE_MOVE_EN;
	
	int waves[3];
	waves[0] = _wave_gen_ramp(axis->pin_step, 0, freq, axis->accel);
	waves[1] = _wave_gen_step(axis->pin_step, freq);
	waves[2] = _wave_gen_ramp(axis->pin_step, freq, 0, axis->accel);
	for (i = 0; i < sizeof(waves)/sizeof(waves[0]); ++i) {
		if (waves[i] < 0) {
			fprintf(stderr, "error create waves\n");
			goto err_waves;
		}
	}

	char comp[4] = {
		0xff & (steps >>  0), 0xff & (steps >>  8), 
		0xff & (steps >> 16), 0xff & (steps >> 24)
	};
	char chain[] = {
		waves[0],
		255, 0,
			255, 0,
				waves[1],
			255, 1, 255, 255,
		255, 1, comp[2], comp[3],
		255, 0,
			waves[1],
		255, 1, comp[0], comp[1],
		waves[2]
	};
	
	if (_set_dir_safe(axis, dir) != 0) {
		goto err_chain;
	}
	
	axis->_last_tick = gpioTick();
	int cs = gpioWaveChain(chain, sizeof(chain));
	if (cs != 0) {
		fprintf(stderr, "error transmit chain: %d", cs);
		goto err_chain;
	}
	_wave_wait();
	goto ok;
	
	err_chain:
	err_waves:
	for (i = 0; i < sizeof(waves)/sizeof(waves[0]); ++i) {
		if (waves[i] >= 0) {
			gpioWaveDelete(waves[i]);
		}
	}
	return 0;
	
	ok:
	steps_done = steps;
	if (axis->_stage == STAGE_MOVE_BR) {
		steps_done = axis->_tick_count/FREQ_TO_US(freq);
	}
	
	if(dir) {
		axis->position += steps_done;
	} else {
		axis->position -= steps_done;
	}
	
	return steps_done;
}

int32_t axis_move_rel(Axis *axis, float freq, int32_t steps) {
	if (steps > 0) {
		return _axis_move_rel(axis, freq, steps, 1);
	} else {
		return -_axis_move_rel(axis, freq, -steps, 0);
	}
}

int32_t axis_move_abs(Axis *axis, float freq, int32_t pos) {
	return axis_move_rel(axis, freq, pos - axis->position);
}

uint32_t axis_scan(Axis *axis, float freq) {
	const uint32_t max_steps = 0xFFFFFFFF;
	
	axis->_mode = MODE_SCAN;
	
	axis->_stage = STAGE_SCAN_RL;
	_axis_move_rel(axis, freq, max_steps, 0);
	
	axis->_stage = STAGE_SCAN_LR;
	uint32_t lr = _axis_move_rel(axis, freq, max_steps, 1);
	printf("lr steps: %d\n", lr);
	
	axis->_stage = STAGE_SCAN_RL;
	uint32_t rl = _axis_move_rel(axis, freq, max_steps, 0);
	printf("rl steps: %d\n", rl);
	
	// axis->_mode = MODE_NONE;
	
	uint32_t min_len = rl < lr ? rl : lr;
	axis->length = min_len;
	axis->position = 0;
	
	return min_len;
}
