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
	
	Axis axis_x, axis_y;
	axis_init(&axis_x, 19, 23, 12, 26);
	axis_init(&axis_y, 18, 17,  5, 16);

	gpioSetSignalFunc(SIGTERM, stop_handler);
	gpioSetSignalFunc(SIGINT, stop_handler);

	printf("scan ...\n");
	int scan;
	/*
	scan = axis_scan(&axis_x);
	if (scan < 0) {
		fprintf(stderr, "scan failed\n");
	} else {
		printf("scan x done %d\n", scan);
	}
	*/
	scan = axis_scan(&axis_y);
	if (scan < 0) {
		fprintf(stderr, "scan failed\n");
	} else {
		printf("scan y done %d\n", scan);
	}
	
	gpioDelay(10000);
	
	axis_y.ramp = 100;
	
	printf("move ...\n");
	int move;
	/*
	move = axis_move(&axis_x, axis_x.length/2);
	printf("move x %d\n", move);
	*/
	move = axis_move(&axis_y, axis_y.length/2);
	printf("move y %d\n", move);

	// while(!done) {
	//	gpioDelay(100000);
	//}

	printf("stop\n");

	return 0;
}
