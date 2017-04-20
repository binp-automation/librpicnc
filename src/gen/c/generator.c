#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "generator.h"


static void _gen_pop_waves(Generator *gen, int count_flag);
static void _gen_push_wave(Generator *gen, int wave);
static int _gen_make_wave(Generator *gen, void (*get_action)(uint32_t*, void*), void *userdata);
static uint32_t _gen_wave_offset(const Generator *gen);


Generator *gen_init(uint32_t buffer_size, uint32_t wave_size, uint32_t delay) {
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
	return NULL;
}

void gen_free(Generator *gen) {
	free(gen->pulses);
	rb_free(gen->wavebuf);
	free(gen);
}

static void _gen_pop_waves(Generator *gen, int count_flag) {
	while(!rb_empty(gen->wavebuf) && (*((int*) rb_tail(gen->wavebuf))) != gen->current) {
		int wave;
		rb_pop(gen->wavebuf, &wave);
		if (count_flag) {
			rawWaveInfo_t info = rawWaveInfo(wave);
			gen->counter += (info.topCB - info.botCB);
		}
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
	uint32_t action[ACTION_SIZE];

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
	gpioSetMode(18, PI_OUTPUT); // TODO: REMOVE
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
			int pos = _gen_wave_offset(gen);
			printf("busy %d at %d\n", gen->current, pos);
		} else {
			if(gen->run) { // DMA reached waveform end
				gen->current = -1;
				_gen_pop_waves(gen, 1);
				gen->run = 0;
			} else { // stopped manually
				_gen_pop_waves(gen, 1);
				gen->current = -1;
				_gen_pop_waves(gen, 0);
			}
		}

		if (!rb_empty(gen->wavebuf)) {
			_gen_pop_waves(gen, 1);
		}
	}
	gen->run = 0;
}

void gen_stop(Generator *gen) {
	if (gpioWaveTxBusy()) {
		gen->current = gpioWaveTxAt();
		gpioWaveTxStop();
	} else {
		gen->current = -1;
	}
	gen->run = 0;
}

static uint32_t _gen_wave_offset(const Generator *gen) {
	if (gpioWaveTxBusy()) {
		return gpioWaveTxCbPos();
	} else {
		return 0;
	}
}

uint32_t gen_position(const Generator *gen) {
	return gen->counter + _gen_wave_offset(gen);
}

void gen_clear_position(Generator *gen) {
	gen->counter = 0;
}