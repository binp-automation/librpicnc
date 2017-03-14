#include <stdio.h>
#include <stdlib.h>
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
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}
	
	float freq = 4000; // Hz
	float accel = 0.02; // step*sec
	Axis axis_x, axis_y;
	axis_init(&axis_x, 19, 23, 12, 26, accel);
	axis_init(&axis_y, 18, 17,  5, 16, accel);

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
	
	// while(!done) {
	//	gpioDelay(100000);
	//}

	printf("stop\n");

	return 0;
}
