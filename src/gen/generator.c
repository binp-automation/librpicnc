#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <pigpio.h>

#include "ringbuffer.h"
define_ringbuffer(RB, rb, int)

#define ACTION_NONE 0x00
// [ACTION_NONE]
#define ACTION_WAIT 0x01
// [ACTION_WAIT][delayUs]
#define ACTION_GPIO 0x02
// [ACTION_GPIO][gpioOn][gpioOff]
#define _ACTION_SIZE 4


typedef struct {
	RB *wavebuf;

	gpioPulse_t *pulses;
	uint32_t pulse_count;
	int reuse_pulses;
	uint32_t reuse_count;

	uint32_t delay;

	int current;
	int run;
	uint32_t counter;
} Generator;


Generator *gen_init(uint32_t buffer_size, uint32_t wave_size, uint32_t delay);
void gen_free(Generator *gen);

static void _gen_pop_waves(Generator *gen);
static void _gen_push_wave(Generator *gen, int wave);
static int _gen_make_wave(Generator *gen, void (*get_action)(uint32_t*, void*), void *userdata);

uint32_t gen_position(const Generator *gen);

void gen_run(Generator *gen, void (*get_action)(uint32_t*, void*), void *user_data);
void gen_stop(Generator *gen);
void gen_clear(Generator *gen);


Generator *gen_init(uint32_t buffer_size, uint32_t wave_size, uint32_t delay) {
	if (gpioInitialise() < 0) {
		goto err_gpio;
	}

	Generator *gen = (Generator*) malloc(sizeof(Generator));
	if (!gen) {
		goto err_alloc;
	}

	gen->pulse_count = wave_size;
	gen->pulses = (gpioPulse_t*) malloc(wave_size*sizeof(gpioPulse_t));
	if (!gen->pulses) {
		goto err_pulses;
	}

	gen->wavebuf = rb_init(buffer_size);
	if (!gen->wavebuf) {
		goto err_rb;
	}
	gen->delay = delay;
	gen->current = -1;
	gen->run = 0;
	gen->counter = 0;
	return gen;
	
err_rb:
	free(gen->pulses);
err_pulses:
	free(gen);
err_alloc:
	gpioTerminate();
err_gpio:
	return NULL;
}

void gen_free(Generator *gen) {
	gen_clear(gen);
	free(gen->pulses);
	rb_free(gen->wavebuf);
	free(gen);
	gpioTerminate();
}

static void _gen_pop_waves(Generator *gen) {
	while(!rb_empty(gen->wavebuf) && (*((int*) rb_tail(gen->wavebuf))) != gen->current) {
		int wave;
		rb_pop(gen->wavebuf, &wave);
		rawWaveInfo_t info = rawWaveInfo(wave);
		gen->counter += (info.topCB - info.botCB);
		// printf("pop wave %d, %d\n", wave, (info.topCB - info.botCB));
		gpioWaveDelete(wave);
	}
}

static void _gen_push_wave(Generator *gen, int wave) {
	if (wave < 0) {
		return;
	}

	assert(!rb_full(gen->wavebuf));
	
	// printf("push wave %d\n", wave);

	if (!rb_empty(gen->wavebuf)) {
		int prev_wave = *(int*) rb_head(gen->wavebuf);
		// printf("rb non-empty, last wave: %d\n", prev_wave);
		rawCbs_t *src_cbp = rawWaveCBAdr(rawWaveInfo(prev_wave).topCB - 1);
		rawCbs_t *dst_cbp = rawWaveCBAdr(rawWaveInfo(wave).botCB);
		src_cbp->next = dst_cbp->next;
	} else {
		// printf("rb empty\n");
		int n = 
		gpioWaveTxSend(wave, PI_WAVE_MODE_ONE_SHOT);
		// printf("tx send, ret: %d\n", n);
		gen->current = wave;
	}
	rb_push(gen->wavebuf, &wave);
}

static int _gen_make_wave(Generator *gen, void (*get_action)(uint32_t*, void*), void *userdata) {
	gpioPulse_t *pulses = gen->pulses;
	int pulse_count = gen->pulse_count;
	uint32_t action[_ACTION_SIZE];

	if (!gen->reuse_pulses) {
		int i;
		for (i = 0; i < pulse_count - 2; ++i) {
			get_action(action, userdata);
			if (action[0] == ACTION_NONE) {
				break;
			} else if (action[0] == ACTION_WAIT) {
				pulses[i].usDelay = action[1];
				pulses[i].gpioOn = 0;
				pulses[i].gpioOff = 0;
			} else if (action[0] == ACTION_GPIO) {
				pulses[i].usDelay = 0;
				pulses[i].gpioOn = action[1];
				pulses[i].gpioOff = action[2];
			} else {
				break;
			}
		}
		pulse_count = i + 2;

		// dummy last pulses (never executed)
		for (i = pulse_count - 2; i < pulse_count; ++i) {
			pulses[i].usDelay = 1;
			pulses[i].gpioOn = 0;
			pulses[i].gpioOff = 0;
		}
	} else {
		pulse_count = gen->reuse_count;
	}

	if (pulse_count <= 2) {
		return -1;
	}

	gpioWaveAddNew();

	gpioWaveAddGeneric(pulse_count, pulses);
	int wave = gpioWaveCreate();

	if (wave < 0) {
		gen->reuse_pulses = 1;
		gen->reuse_count = pulse_count;
	} else {
		gen->reuse_pulses = 0;
	}

	return wave;
}

void gen_run(Generator *gen, void (*get_action)(uint32_t*, void*), void *user_data) {
	gen->run = 1;

	while (gen->run) {
		while (!rb_full(gen->wavebuf)) {
			int wave = _gen_make_wave(gen, get_action, user_data);
			if (wave < 0) {
				break;
			}
			_gen_push_wave(gen, wave);
		}

		gpioDelay(gen->delay);
		printf("delay\n");

		if (gpioWaveTxBusy()) {
			gen->current = gpioWaveTxAt();
			int pos = gen_position(gen);
			// printf("busy %d at %d\n", gen->current, pos);

		} else if(gen->run) {
			// printf("idle and run\n");
			gen->current = -1;
			gen->run = 0;
		}

		if (!rb_empty(gen->wavebuf)) {
			_gen_pop_waves(gen);
		}
	}
	gen->run = 0;
}

uint32_t gen_position(const Generator *gen) {
	uint32_t pos = 0;
	if (gpioWaveTxBusy()) {
		pos = gpioWaveTxCbPos();
	}
	return gen->counter + pos;
}

void gen_stop(Generator *gen) {
	gpioWaveTxStop();
	gen->run = 0;
}

void gen_clear(Generator *gen) {
	gen->current = -1;
	_gen_pop_waves(gen);
	gen->counter = 0;
}
