#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "generator.h"

volatile int done = 0;

void stop_handler(int signo) {
	done = 1;
}

void push_cmds(RB *rb, void *data) {
	Cmd cmd = cmd_wait(1000000);
	while (!rb_full(rb) && (*(int*)data)-- > 0) {
		rb_push(rb, (uint8_t*) &cmd);
	}
} 

int main(int argc, char *argv[]) {
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}
	
	Generator gen;
	gen_init(&gen, 2, 0);
	
	int cnt = 10;
	gen_run(&gen, push_cmds, &cnt);
	
	gen_free(&gen);
	
	printf("stop\n");

	return 0;
}
