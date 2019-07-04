#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include <generator.h>
#include <command.h>
#include <axis.h>

#include <ringbuffer.h>
ringbuffer_declare(RBC, rbc, Cmd)

#define MAX_AXES 8
#define SYNC_MAP_SIZE MAX_AXES


typedef struct {
	Axis axes[MAX_AXES];
	int axis_count;
} Device;

int dev_init(Device *dev, int axis_count);
int dev_free(Device *dev);
int dev_run(Device *dev, Generator *gen, Cmd (*get_cmd)(int axis, void *userdata), void *userdata);
int dev_clear(Device *dev);
