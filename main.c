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
			cmd = cmd_move(500, center);
			cookie->chan[0].pos = center;
			cookie->chan[0].idx = 0;
		}
	} else {
		if (cookie->chan[1].idx < 0) {
			int center = cookie->chan[1].len/2;
			cmd = cmd_move(500, center);
			cookie->chan[1].pos = center;
			cookie->chan[1].idx = 0;
		}
	}
	return cmd;
}

/*
Cmd get_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	printf("get_cmd %d\n", ax);

	Cmd cmd;
	int i = cookie->i, j = cookie->j;
	int x = cookie->x, y = cookie->y;
	float speed = 500;

	if (ax == 0) {
		if(i < 10) {
			cmd = cmd_move(speed, x*(2*(i%2) - 1));
		} else {
			cmd = cmd_none();
		}
		++i;
	} else {
		if(j == 0) {
			cmd = cmd_wait(0.5*y/speed*1e6);
		} else if(j > 0 && j < 11) {
			cmd = cmd_move(speed, y*(2*(j%2) - 1));
		} else {
			cmd = cmd_none();
		}
		++j;
	}

	cookie->i = i;
	cookie->j = j;
	return cmd;
}
*/

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
	
	// axis_scan(axis_y, &gen);
	axis_y->length = 2000;
	printf("length: %d\n", axis_y->length);
	// axis_scan(axis_x, &gen);
	axis_x->length = 4000;
	printf("length: %d\n", axis_x->length);
	cookie.chan[0].len = axis_x->length;
	cookie.chan[1].len = axis_y->length;

	dev_run(&dev, &gen, center_cmd, (void*) &cookie);

	// dev_run(&dev, &gen, get_cmd, (void*) &cookie);

	axis_free(axis_x);
	axis_free(axis_y);
	dev_free(&dev);
	gen_free(&gen);

	gpioTerminate();
	
	printf("stop\n");

	return 0;
}
