#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "ringbuffer.h"
#include "command.h"
#include "axis.h"

#define FREQ_TO_US(freq) \
	((uint32_t) (1e6/(freq)))

#define CMD_BUFSIZE   0x40
#define WAVE_BUFSIZE  0x40
#define WAVE_LEN      0x100

#define GEN_DELAY 10000 // us

typedef struct {
	int naxes;
	Axis axes[MAX_AXES];
	float accel;
	
	RB *cmdbuf;
	RB *wavebuf;
	Cmd cmd;
} Generator;


void _gen_free_rbs(Generator *gen) {
	if (gen->cmdbuf) { free(gen->cmdbuf); }
	if (gen->wavebuf) { free(gen->wavebuf); }
}

int gen_init(Generator *gen, int naxes, float accel) {
	gen->naxes = naxes;
	gen->accel = accel;

	gen->cmdbuf = rb_init(CMD_BUFSIZE, sizeof(Cmd));
	gen->wavebuf = rb_init(WAVE_BUFSIZE, sizeof(int));
	if (!gen->cmdbuf || !gen->wavebuf) {
		goto err_rbs;
	}
	
	gen->cmd = cmd_none();
	
	return 0;
	
err_rbs:
	_gen_free_rbs(gen);
	return 1;
}

int gen_free(Generator *gen) {
	_gen_free_rbs(gen);
	return 0;
}

int _gen_make_delay(Generator *gen, uint32_t delay) {
	gpioPulse_t pulses[2];
	
	pulses[0].usDelay = delay;
	pulses[0].gpioOn = 0;
	pulses[0].gpioOff = 0;
	pulses[1].usDelay = 0;
	pulses[1].gpioOn = 0;
	pulses[1].gpioOff = 0;
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(2, pulses);
	return gpioWaveCreate();
}

int _gen_push_motion(Generator *gen, float speed) {
	return -1;
}

void _gen_pop_waves(Generator *gen) {
	int wave = -1;
	if (gpioWaveTxBusy()) {
		wave = gpioWaveTxAt();
	}
	while(!rb_empty(gen->wavebuf) && (*((int*) rb_tail(gen->wavebuf))) != wave) {
		int w;
		rb_pop(gen->wavebuf, (uint8_t*) &w);
		gpioWaveDelete(w);
	}
}

void _gen_push_waves(Generator *gen) {
	while (!rb_full(gen->wavebuf)) {
		int wave = -1;
		switch (gen->cmd.type) {
			case CMD_NONE: break;
			case CMD_WAIT: {
				printf("wait %d\n", gen->cmd.wait.duration);
				wave = _gen_make_delay(gen, gen->cmd.wait.duration);
				gen->cmd = cmd_none();
			} break;
			case CMD_MOVE: {
				printf("move\n");
				/*
				int i, naxes = gen->naxes;
				for (i = 0; i < naxes; ++i) {
					gen->axes[i].steps = gen->cmd.move.steps[i];
				}
				_gen_push_waves(gen, gen->cmd.move.speed);
				*/
			} break;
			default: {
				fprintf(stderr, "invalid cmd type: %d\n", gen->cmd.type);
			} break;
		}
		
		printf("wave %d\n", wave);
		if (wave < 0) { break; }
		
		rb_push(gen->wavebuf, (uint8_t*) &wave);
		gpioWaveTxSend(wave, PI_WAVE_MODE_ONE_SHOT_SYNC);
		printf("done\n");
	}
}

void _gen_pop_cmds(Generator *gen) {
	if (gen->cmd.type == CMD_NONE) {
		rb_pop(gen->cmdbuf, (uint8_t*) &gen->cmd);
	}
}

int gen_run(Generator *gen, void(*push_cmds_cb)(RB *cmdbuf, void *userdata), void *userdata) {
	for (;;) {
		if (!rb_full(gen->cmdbuf)) {
			push_cmds_cb(gen->cmdbuf, userdata);
		}
		if (!rb_empty(gen->cmdbuf)) {
			_gen_pop_cmds(gen);
		} else {
			break;
		}
		
		if (!rb_empty(gen->wavebuf)) {
			_gen_pop_waves(gen);
		}
		if (!rb_full(gen->wavebuf)) {
			_gen_push_waves(gen);
		}
		
		gpioDelay(GEN_DELAY);
	}
	
	return 0;
}
