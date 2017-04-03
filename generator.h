#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <pigpio.h>

#include "ringbuffer.h"
define_ringbuffer(RB, rb, int)

#define FREQ_TO_US(freq) \
	((uint32_t) (1e6/(freq)))

#define WAVE_BUFSIZE  0x4

#define GEN_DELAY 100000 // us

typedef struct {
	RB *wavebuf;
	int counter;
	int run;
} Generator;


int gen_init(Generator *gen) {
	gen->wavebuf = rb_init(WAVE_BUFSIZE);
	if (!gen->wavebuf) {
		goto err_rbs;
	}
	gen->counter = 0;
	gen->run = 0;
	return 0;
	
err_rbs:
	return 1;
}

int gen_free(Generator *gen) {
	if (gen->wavebuf) {
		rb_free(gen->wavebuf);
		gen->wavebuf = NULL;
		return 0;
	}

	return 1;
}

void _gen_pop_waves(Generator *gen) {
	int cur_wave = -1;
	if (gpioWaveTxBusy()) {
		cur_wave = gpioWaveTxAt();
		printf("wave tx busy on wave %d\n", cur_wave);
	} else {
		printf("wave tx NOT busy\n");
	}
	while(!rb_empty(gen->wavebuf) && (*((int*) rb_tail(gen->wavebuf))) != cur_wave) {
		int wave;
		rb_pop(gen->wavebuf, &wave);
		rawWaveInfo_t info = rawWaveInfo(wave);
		gen->counter += (info.topCB - info.botCB) - 1;
		printf("pop wave %d\n", wave);
		gpioWaveDelete(wave);
	}
}

void _gen_push_wave(Generator *gen, int wave) {
	if (wave < 0) {
		return;
	}

	assert(!rb_full(gen->wavebuf));
	
	printf("push wave %d\n", wave);

	if (!rb_empty(gen->wavebuf)) {
		int prev_wave = *(int*) rb_head(gen->wavebuf);
		printf("rb non-empty, last wave: %d\n", prev_wave);
		rawCbs_t *src_cbp = rawWaveCBAdr(rawWaveInfo(prev_wave).topCB - 1);
		rawCbs_t *dst_cbp = rawWaveCBAdr(rawWaveInfo(wave).botCB);
		src_cbp->next = dst_cbp->next;
	} else {
		printf("rb empty\n");
		int n = gpioWaveTxSend(wave, PI_WAVE_MODE_ONE_SHOT);
		printf("tx send, ret: %d\n", n);
	}
	rb_push(gen->wavebuf, &wave);
}

int gen_run(Generator *gen, int (*get_wave_cb)(void*), void *user_data) {
	gen->run = 1;

	while (gen->run) {
		while (!rb_full(gen->wavebuf)) {
			int wave = get_wave_cb(user_data);
			if (wave < 0) {
				break;
			}
			_gen_push_wave(gen, wave);
		}
		
		if (!rb_empty(gen->wavebuf)) {
			_gen_pop_waves(gen);
		} else {
			break;
		}

		gpioDelay(GEN_DELAY);
	}
	gen->run = 0;

	return 0;
}

int gen_stop(Generator *gen) {
	gen->run = 0;
	if (gpioWaveTxBusy()) {
		_gen_pop_waves(gen);
		gen->counter += gpioWaveTxCbPos();
		gpioWaveTxStop();
	}
	return 0;
}