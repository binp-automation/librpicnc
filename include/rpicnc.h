#pragma once


#include <stdlib.h>
#include <stdint.h>

#include <device.h>
#include <generator.h>
#include <command.h>
#include <task.h>


typedef struct {
	int pin_step;
	int pin_dir;
	int pin_left;
	int pin_right;

	int position;
	int length;
} AxisInfo;


int cnc_init(int axes_count, AxisInfo *axes_info);
int cnc_quit();
int cnc_clear();

// synchronous
int cnc_run_task(Task *task);
int cnc_read_sensors();
int cnc_axes_info(AxisInfo *axes_info);

// asynchronous
int cnc_push_task(Task *task);
int cnc_run_async();
int cnc_is_busy();

int cnc_wait();
int cnc_stop();
