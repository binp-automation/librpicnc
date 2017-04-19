#pragma once

#include <pigpio.h>

#include "ringbuffer.h"
define_ringbuffer(RB, rb, int)


#define ACTION_NONE 0x00
// [ACTION_NONE]
#define ACTION_WAIT 0x01
// [ACTION_WAIT][delayUs]
#define ACTION_GPIO 0x02
// [ACTION_GPIO][gpioOn][gpioOff]
#define ACTION_SIZE 4


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

uint32_t gen_position(const Generator *gen);

void gen_run(Generator *gen, void (*get_action)(uint32_t*, void*), void *user_data);
void gen_stop(Generator *gen);
void gen_clear(Generator *gen);
