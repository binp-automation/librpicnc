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

#include "midi/still_alive.h"

#define SPEED 8000
#define ACCEL 20000

typedef struct {
	int64_t time;
	int idx;
	int pos;
	int len;
	int dir;
} Chan;

typedef struct {
	Chan chan[2];
} Cookie;

#define TICKS_TO_US(ticks) \
	((1000000*((int64_t) ticks))/512)

#define NOTE_TO_FREQ(note) \
	(pow(2, ((note) - 57)/12.0)*440.0)

#define BORDER 0.2

Cmd cnt_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	Cmd cmd = cmd_none();
	if (ax == 0 || ax == 1) {
		Chan *c = &cookie->chan[ax];
		if (c->idx < 0) {
			int center = c->len/2;
			cmd = cmd_move(center, SPEED, ACCEL);
			c->pos = center;
			c->idx = 0;
		}
	}
	printf("axis %d:\n", ax);
	print_cmd(cmd);
	return cmd;
}

Cmd get_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	Cmd cmd = cmd_none();
	if (ax == 0 || ax == 1) {
		Chan *c = &cookie->chan[ax];
		const int *chan = ((ax == 0) ? chan1 : chan2);
		int i = c->idx;
		if (chan[3*i + 0] != 0 || chan[3*i + 1] != 0 || chan[3*i + 2] != 0) {
			printf("axis %d:\n", ax);
			const int *note = chan + 3*i;
			printf("c->time: %lld\n", c->time);
			int64_t time = TICKS_TO_US(note[0]);
			printf("time: %lld\n", time);
			if (c->time < time) {
				int delay = time - c->time;
				cmd = cmd_wait(delay);
				c->time += delay;
				printf("delay: %d\n", delay);
			} else {
				float freq = NOTE_TO_FREQ(note[2]);
				int delay = TICKS_TO_US(note[1]) - (c->time - time);
				int steps = floor(1e-6*delay*freq);

				int cmd_delay = axis_count_cmd_delay(cmd_move(steps, freq, ACCEL));
				if (cmd_delay > delay) {
					steps -= ceil(1e-6*(cmd_delay - delay)*freq) + 1;
					cmd_delay = axis_count_cmd_delay(cmd_move(steps, freq, ACCEL));
				}
				if (cmd_delay > delay) {
					fprintf(stderr, "cmd_delay(%d) > delay(%d)\n", cmd_delay, delay);
				}
				delay = cmd_delay;

				if (c->dir == 0) {
					if (c->pos <= c->len*BORDER) {
						c->dir = 1;
					}
				} else {
					if (c->pos >= c->len*(1.0 - BORDER)) {
						c->dir = 0;
					}
				}
				if (c->dir == 0) {
					steps = -steps;
				}

				cmd = cmd_move(steps, freq, ACCEL);

				c->time += delay;
				c->idx += 1;
				c->pos += steps;
				printf("delay: %d\n", delay);
			}
		}
	}
	print_cmd(cmd);
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
		cookie.chan[i].dir = 1;
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

	dev_run(&dev, &gen, cnt_cmd, (void*) &cookie);
	dev_clear(&dev);

	dev_run(&dev, &gen, get_cmd, (void*) &cookie);
	dev_clear(&dev);

	axis_free(axis_x);
	axis_free(axis_y);
	dev_free(&dev);
	gen_free(&gen);

	gpioTerminate();
	
	printf("stop\n");

	return 0;
}
