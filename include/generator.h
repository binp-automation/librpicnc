#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <pigpio.h>

#include <ringbuffer.h>
ringbuffer_declare(RB, rb, int)

#define GEN_DELAY 10000 // us

typedef struct {
	RB *wavebuf;
	int current;
	int counter;
	int run;
} Generator;

int gen_init(Generator *gen, int bufsize);
int gen_free(Generator *gen);

int gen_run(Generator *gen, int (*get_wave)(void*), void *user_data);
int gen_position(Generator *gen);
int gen_stop(Generator *gen);
int gen_clear(Generator *gen);
