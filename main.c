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
	uint8_t num;
	int32_t pos;
	float ivel;
	float vel;
	float acc;
} Chan;

typedef struct {
	Chan chan[2];
} Cookie;

Cmd move_cmd(int ax, void *userdata) {
	Cookie *cookie = (Cookie*) userdata;
	Cmd cmd = cmd_none();
	if (ax == 0 || ax == 1) {
		Chan *ch = &cookie->chan[ax];
		if (ch->pos != 0) {
			uint8_t d = ch->pos > 0;
			uint32_t p = (d ? 1 : -1)*ch->pos;
			uint32_t it = ch->ivel > 1.0 ? 1e6/ch->ivel : 0;
			uint32_t t = 1e6/ch->vel;
			uint32_t ap = (sqr(ch->vel) - sqr(ch->ivel))/(2*ch->acc);
			if (2*ap > p) {
				float v = sqrt(ch->acc*p + sqr(ch->ivel));
				t = 1e6/v;
				ap = p/2;
				p = p - 2*ap;
			}
			if (ch->num == 0) {
				cmd = cmd_accl(d, ap, it, t);
				ch->num += 1;
			} else if (ch->num == 1) {
				cmd = cmd_move(d, p, t);
				ch->num += 1;
			} else if (ch->num == 2) {
				cmd = cmd_accl(d, ap, t, it);
				ch->num += 1;
			}

		}
	}
	return cmd;
}

Generator gen;
Device dev;

int cnc_init() {
	printf("start\n");
	if (gpioInitialise() < 0) {
		printf("error gpio init\n");
		return 1;
	}

	gen_init(&gen, 0x10);
	dev_init(&dev, 2);

	Axis *axis_x = &dev.axes[0];
	Axis *axis_y = &dev.axes[1];
	axis_init(axis_x,  6, 13, 19, 16);
	axis_init(axis_y, 27, 22, 12,  5);

	return 0;
}

int cnc_quit() {
	Axis *axis_x = &dev.axes[0];
	Axis *axis_y = &dev.axes[1];
	axis_free(axis_x);
	axis_free(axis_y);

	dev_free(&dev);
	gen_free(&gen);

	gpioTerminate();
	
	printf("stop\n");

	return 0;
}

int cnc_scan_x() {
	Axis *axis_x = &dev.axes[0];
	axis_scan(axis_x, &gen, 1000);
	printf("length: %d\n", axis_x->length);
	return axis_x->length;
}

int cnc_scan_y() {
	Axis *axis_y = &dev.axes[1];
	axis_scan(axis_y, &gen, 1000);
	printf("length: %d\n", axis_y->length);
	return axis_y->length;
}

int cnc_move(
	int32_t px, int32_t py, 
	float ivx, float ivy, 
	float vx, float vy, 
	float ax, float ay
) {
	printf("%d, %d, %f, %f, %f, %f\n", px, py, vx, vy, ax, ay);

	Cookie cookie;
	cookie.chan[0].num = 0;
	cookie.chan[1].num = 0;
	cookie.chan[0].pos = px;
	cookie.chan[1].pos = py;
	cookie.chan[0].ivel = ivx;
	cookie.chan[1].ivel = ivy;
	cookie.chan[0].vel = vx;
	cookie.chan[1].vel = vy;
	cookie.chan[0].acc = ax;
	cookie.chan[1].acc = ay;

	dev_run(&dev, &gen, move_cmd, (void*) &cookie);
	gen_clear(&gen);
	dev_clear(&dev);

	return 0;
}

int main() {

	cnc_init();

	cnc_move(1000, 1000, 0, 0, 1000, 1000, 5000, 5000);

	cnc_quit();

	return 0;
}
