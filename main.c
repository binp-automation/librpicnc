#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include "command.h"
#include "generator.h"
#include "axis.h"
#include "device.h"

typedef struct {
	int i;
	int j;
} Cookie;

Cmd get_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	printf("get_cmd %d\n", ax);

	Cmd cmd;
	int i = cookie->i, j = cookie->j;

	if (ax == 0) {
		if(i < 10) {
			cmd = cmd_move(500, 400*(2*(i%2) - 1));
			++i;
		} else {
			cmd = cmd_none();
		}
	} else {
		if(j < 10) {
			cmd = cmd_move(250, -200*(2*(j%2) - 1));
			++j;
		} else {
			cmd = cmd_none();
		}
	}

	cookie->i = i;
	cookie->j = j;
	return cmd;
}

int main(int argc, char *argv[]) {
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}

	Cookie cookie;
	cookie.i = 0;
	cookie.j = 0;
	
	Generator gen;
	gen_init(&gen, 0x10);

	Device dev;
	dev_init(&dev, 2);

	Axis *axis_x = &dev.axes[0];
	Axis *axis_y = &dev.axes[1];
	axis_init(axis_x,  5, 23, 12, 26);
	axis_init(axis_y, 18, 17, 19, 16);
	
	//axis_scan(axis_y, &gen);
	//printf("length: %d\n", axis_y->length);

	dev_run(&dev, &gen, get_cmd, (void*) &cookie);

	axis_free(axis_x);
	axis_free(axis_y);
	dev_free(&dev);
	gen_free(&gen);

	gpioTerminate();
	
	printf("stop\n");

	return 0;
}
