#pragma once


#include <stdlib.h>
#include <stdint.h>

#include <device.h>
#include <generator.h>
#include <command.h>
#include <task.h>


typedef struct {
	uint32_t mask_step_pos;
	uint32_t mask_step_neg;
	uint32_t mask_dir_pos;
	uint32_t mask_dir_neg;
	int32_t position;

	int sense;
	uint32_t pin_left;
	uint32_t pin_right;
	uint32_t length;
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
