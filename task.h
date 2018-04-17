#pragma once


#include "device.h"
#include "command.h"


#define TASK_NONE    0x00
#define TASK_SCAN    0x01
#define TASK_CALIB   0x02

#define TASK_CMDS    0x10
#define TASK_GCODE   0x11
#define TASK_CURVE   0x12

// task status
#define TS_NONE 0x00
#define TS_WAIT 0x01
#define TS_PROC 0x02
#define TS_DONE 0x03
#define TS_ERROR 0xF0

// stop codes
#define SC_DONE         0x01
#define SC_SENS_L       0x10
#define SC_SENS_R       0x11
#define SC_USER_STOP    0x20


typedef struct {

} TaskNone;

typedef struct {
	// in
	int axis;
	int t_ivel;
	int t_vel;
	int tt_acc;
	// out
	int length;
} TaskScan;

typedef struct {
	// in
	int axis;
	int it_ivel;
	// out
	float t_ivel;
	float t_vel;
	float tt_acc;
} TaskCalib;

typedef struct {
	// in
	int cmds_count[MAX_AXES];
	Cmd *(cmds[MAX_AXES]);
	// out
	int cmds_done[MAX_AXES];
} TaskCmds;

// TODO
typedef struct {
	// in
	// out
} TaskGCode;

// TODO
typedef struct {
	// in
	// out
} TaskCurve;

typedef struct {
	int type;
	union {
		TaskNone    none;
		TaskCalib   calib;
		TaskScan    scan;
		TaskCmds    cmds;
		TaskGCode   gcode;
		TaskCurve   curve;
	};
	// out
	int status;
	int stop_code;
} Task;
