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
#include "task.h"

#include "main.h"

#define DEBUG


static int initialized = 0;

static Device device;
static Generator generator;

typedef struct {
	Task **buffer;
	int pos;
	int end;
	int length;
} TaskQueue;

static TaskQueue task_queue;
#define TQLEN 0x100

void _task_queue_init(TaskQueue *tq, int len) {
	tq->buffer = malloc(sizeof(Task*)*len);
	tq->length = len;
	tq->pos = 0;
	tq->end = 0;
}

void _task_queue_free(TaskQueue *tq) {
	free(tq->buffer);
}

int cnc_init(int axes_count, AxisInfo *axes_info) {
	printf("[ cnc ] %s\n", __func__);

	if (initialized) {
		printf("[error] cnc already initialized\n");
		return 1;
	}

	if (gpioInitialise() < 0) {
		printf("[error] cannot initialize cnc\n");
		return 2;
	}

	gen_init(&generator, 0x10);
	dev_init(&device, axes_count);
	_task_queue_init(&task_queue, TQLEN);

#ifdef DEBUG
	printf("axes_count: %d\n", axes_count);
#endif /* DEBUG */

	int i;
	for (i = 0; i < axes_count; ++i) {
		AxisInfo *ai = &axes_info[i];
		axis_init(
			&device.axes[i],
			ai->pin_step, ai->pin_dir,
			ai->pin_left, ai->pin_right
		);
#ifdef DEBUG
		printf("axis[%d]: s:%d. d:%d, l:%d, r:%d\n", i, ai->pin_step, ai->pin_dir, ai->pin_left, ai->pin_right);
#endif /* DEBUG */
	}

	initialized = 1;

	return 0;
}

int cnc_quit() {
	printf("[ cnc ] %s\n", __func__);
	if (!initialized) {
		printf("[error] cnc was not initialized\n");
		return 1;
	}

	int i;
	for (i = 0; i < device.axis_count; ++i) {
		axis_free(&device.axes[i]);
	}

	dev_free(&device);
	gen_free(&generator);
	_task_queue_free(&task_queue);

	gpioTerminate();

	initialized = 0;
	return 0;
}

int cnc_clear() {
	gen_clear(&generator);
	dev_clear(&device);
	return 0;
}

typedef struct {
	Cmd *cmds;
	int pos;
	int len;
} _CmdsChannel;

typedef struct {
	_CmdsChannel chs[MAX_AXES];
} _CmdsCookie;

Cmd _next_cmd(int axis, void *userdata) {
	_CmdsCookie *cookie = (_CmdsCookie*) userdata;
	Cmd cmd = cmd_idle();
	_CmdsChannel *ch = &cookie->chs[axis];
	if (ch->pos < ch->len) {
		cmd = ch->cmds[ch->pos];
		ch->pos += 1;
	}
	return cmd;
}

int cnc_run_task(Task *task) {
	if (task->type == TASK_NONE) {
		// pass
	} else if (task->type == TASK_SCAN) {
		Axis *axis = &device.axes[task->scan.axis];
		axis_scan(axis, &generator, task->scan.t_ivel);
		printf("length: %d\n", axis->length);
		task->scan.length = axis->length;
	} else if (task->type == TASK_CMDS) {
		_CmdsCookie cookie;
		int i;
		for (i = 0; i < device.axis_count; ++i) {
			cookie.chs[i].cmds = task->cmds.cmds[i];
			cookie.chs[i].len = task->cmds.cmds_count[i];
			cookie.chs[i].pos = 0;
		}
		dev_run(&device, &generator, _next_cmd, (void*) &cookie);
		cnc_clear();
	} else {
		// unknown task type
		return 1;
	}
	return 0;
}


// asynchronous

int cnc_push_task(Task *task) {
	printf("[ cnc ] %s\n", __func__);

	TaskQueue *tq = &task_queue;
	if (tq->end >= tq->length) {
		printf("[error] task_queue full\n");
		return 1;
	}

	tq->buffer[tq->end] = task;
	tq->end += 1;

	return 0;
}

int cnc_run_async() {
	// TODO
	return 1;
}

int cnc_is_busy() {
	// TODO
	return 1;
}

int cnc_stop() {
	// TODO
	return 1;
}


int main() {
	return 0;
}
