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

#define SPEED 4000
#define ACCEL 20000

typedef struct {
	int time;
	int idx;
	int pos;
	int len;
} Chan;

typedef struct {
	Chan chan[2];
} Cookie;

Cmd center_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	Cmd cmd = cmd_none();
	if (ax == 0) {
		if (cookie->chan[0].idx < 0) {
			int center = cookie->chan[0].len/2;
			cmd = cmd_move(center, SPEED, ACCEL);
			cookie->chan[0].pos = center;
			cookie->chan[0].idx = 0;
		}
	} else {
		if (cookie->chan[1].idx < 0) {
			int center = cookie->chan[1].len/2;
			cmd = cmd_move(center, SPEED, ACCEL);
			cookie->chan[1].pos = center;
			cookie->chan[1].idx = 0;
		}
	}
	return cmd;
}

int main(int argc, char *argv[]) {
	int i;
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}

	Cookie cookie;
	for (i = 0; i < 2; ++i) {
		cookie.chan[i].time = 0;
		cookie.chan[i].idx = -1;
		cookie.chan[i].pos = 0;
		cookie.chan[i].len = 0;
	}
	
	Generator gen;
	gen_init(&gen, 0x10);

	Device dev;
	dev_init(&dev, 2);

	Axis *axis_x = &dev.axes[0];
	Axis *axis_y = &dev.axes[1];
	axis_init(axis_x,  5, 23, 12, 26);
	axis_init(axis_y, 18, 17, 19, 16);
	
	axis_scan(axis_y, &gen, SPEED, ACCEL);
	printf("length: %d\n", axis_y->length);
	axis_scan(axis_x, &gen, SPEED, ACCEL);
	printf("length: %d\n", axis_x->length);
	cookie.chan[0].len = axis_x->length;
	cookie.chan[1].len = axis_y->length;

	dev_run(&dev, &gen, center_cmd, (void*) &cookie);
	dev_clear(&dev);

	// dev_run(&dev, &gen, get_cmd, (void*) &cookie);

	axis_free(axis_x);
	axis_free(axis_y);
	dev_free(&dev);
	gen_free(&gen);

	gpioTerminate();
	
	printf("stop\n");

	return 0;
}
