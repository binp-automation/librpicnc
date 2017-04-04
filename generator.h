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

#define GEN_DELAY 10000 // us

typedef struct {
	RB *wavebuf;
	int current;
	int counter;
	int run;
} Generator;

int gen_clear(Generator *gen);

int gen_init(Generator *gen, int bufsize) {
	gen->wavebuf = rb_init(bufsize);
	if (!gen->wavebuf) {
		goto err_rbs;
	}
	gen->current = -1;
	gen->counter = 0;
	gen->run = 0;
	return 0;
	
err_rbs:
	return 1;
}

int gen_free(Generator *gen) {
	gen_clear(gen);
	rb_free(gen->wavebuf);
	gen->wavebuf = NULL;
	return 0;
}

void _gen_pop_waves(Generator *gen) {
	while(!rb_empty(gen->wavebuf) && (*((int*) rb_tail(gen->wavebuf))) != gen->current) {
		int wave;
		rb_pop(gen->wavebuf, &wave);
		rawWaveInfo_t info = rawWaveInfo(wave);
		gen->counter += (info.topCB - info.botCB);
		printf("pop wave %d, %d\n", wave, (info.topCB - info.botCB));
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
		gen->current = wave;
	}
	rb_push(gen->wavebuf, &wave);
}

int gen_run(Generator *gen, int (*get_wave)(void*), void *user_data) {
	gen->run = 1;

	while (gen->run) {
		while (!rb_full(gen->wavebuf)) {
			int wave = get_wave(user_data);
			if (wave < 0) {
				break;
			}
			_gen_push_wave(gen, wave);
		}

		gpioDelay(GEN_DELAY);

		if (gpioWaveTxBusy()) {
			gen->current = gpioWaveTxAt();
		} else if(gen->run) {
			gen->current = -1;
			gen->run = 0;
		}

		if (!rb_empty(gen->wavebuf)) {
			_gen_pop_waves(gen);
		}
	}
	gen->run = 0;

	return 0;
}

int gen_position(Generator *gen) {
	if (gpioWaveTxBusy()) {
		return gpioWaveTxCbPos();
	}
	return 0;
}

int gen_stop(Generator *gen) {
	gpioWaveTxStop();
	gen->run = 0;
	return 0;
}

int gen_clear(Generator *gen) {
	gen->current = -1;
	_gen_pop_waves(gen);
	return 0;
}