#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "generator.h"
#include "axis.h"


int main(int argc, char *argv[]) {
	srand(time(NULL));

	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}
	
	Axis axis_y;
	axis_init(&axis_y, 18, 17, 19, 16);
	Generator gen;
	gen_init(&gen, 0x10);
	
	axis_scan(&axis_y, &gen);
	
	gen_free(&gen);
	axis_free(&axis_y);

	printf("length: %d\n", axis_y.length);
	
	printf("stop\n");

	return 0;
}
