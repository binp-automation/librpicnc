#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "axis.h"

volatile int done = 0;

void stop_handler(int signo) {
	_stop_all();
	exit(1);
}

int main(int argc, char *argv[]) {
	int i;
	
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}
	
	float freq = 4000; // Hz
	float accel = 0.02; // step*sec
	Axis axis_x, axis_y;
	axis_init(&axis_x,  5, 23, 12, 26, accel);
	axis_init(&axis_y, 18, 17, 19, 16, accel);

	gpioSetSignalFunc(SIGTERM, stop_handler);
	gpioSetSignalFunc(SIGINT, stop_handler);

	printf("scan ...\n");
	int scan;
	scan = axis_scan(&axis_x, freq);
	if (scan < 0) {
		fprintf(stderr, "scan failed\n");
	} else {
		printf("scan x done %d\n", scan);
	}
	axis_x.length *= 0.75;
	scan = axis_scan(&axis_y, freq);
	if (scan < 0) {
		fprintf(stderr, "scan failed\n");
	} else {
		printf("scan y done %d\n", scan);
	}
	axis_y.length *= 0.75;
	
	
	printf("move ...\n");
	int move;
	move = axis_move_abs(&axis_x, freq, axis_x.length/2);
	printf("move x %d %d\n", move, axis_x.position);
	move = axis_move_abs(&axis_y, freq, axis_y.length/2);
	printf("move y %d %d\n", move, axis_y.position);
	
	
	printf("play ...\n");
	Axis *axis = &axis_y;
	axis->accel = 0.01;
	float dur = 0.5; // sec
	float base_freq = 500; // Hz
	int dir = 0;
	int buffer = 0.2*axis->length;
	int tones[] = {0, 2, 4, 5, 7, 9, 11, 12, 12, 11, 9, 7, 5, 4, 2, 0};
	for (i = 0; i < sizeof(tones)/sizeof(tones[0]); ++i) {
		float freq = base_freq*pow(2, ((float) tones[i])/12);
		int32_t steps = freq*dur;
		int lf = axis->position - steps > buffer;
		int rf = axis->position + steps < axis->length - buffer;
		if (!lf && !rf) {
			fprintf(stderr, "cannot move\n");
			break;
		}
		if ((dir && !rf) || (!dir && !lf)) {
			dir = !dir;
		}
		if(!dir) {
			steps = -steps;
		}
		axis_move_rel(&axis_y, freq, steps);
	}
	
	// while(!done) {
	//	gpioDelay(100000);
	//}

	printf("stop\n");

	return 0;
}
