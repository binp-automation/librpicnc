#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "generator.h"


int gen_wave(void *data) {
	int i;
	gpioPulse_t pulses[3];

	pulses[0].usDelay = (*(int*) data)*(1 + (rand() % 3));
	pulses[0].gpioOn = 0;
	pulses[0].gpioOff = 0;

	// dummy last pulses (never executed)
	for (i = 0; i < 2; ++i) {
		pulses[1 + i].usDelay = 1;
		pulses[1 + i].gpioOn = 0;
		pulses[1 + i].gpioOff = 0;
	}
	
	gpioWaveAddNew();

	gpioWaveAddGeneric(sizeof(pulses)/sizeof(pulses[0]), pulses);
	int wave = gpioWaveCreate();
	return wave;
}

int main(int argc, char *argv[]) {
	srand(time(NULL));

	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}
	
	Generator gen;
	gen_init(&gen);
	
	int delay = 300000; // us
	gen_run(&gen, gen_wave, &delay);
	
	gen_free(&gen);
	
	printf("stop\n");

	return 0;
}
