#pragma once

#include <axis.h>
#include <generator.h>

#include <pigpio.h>


typedef struct {
	Axis *axis;
	Generator *gen;
	int counter;
	gpioPulse_t *pulses;
	int pulse_count;

	Cmd *cmd_list;
	int current_cmd;
	int reuse_pulses;
	int reuse_count;
} _AxisCookie;

int axis_scan(Axis *axis, Generator *gen, float vel_ini, float vel_max, float acc_max);
int axis_calib(Axis *axis, Generator *gen, float *vel_ini, float *vel_max, float *acc_max);
