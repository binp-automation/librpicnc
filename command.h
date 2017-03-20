#pragma once

#include <stdlib.h>

#define CMD_NONE 0x00
#define CMD_WAIT 0x01
#define CMD_MOVE 0x02

#define MAX_AXES 8

typedef struct {
	
} CmdNone;

typedef struct {
	uint32_t duration;
} CmdWait;

typedef struct {
	float speed;
	int32_t steps[MAX_AXES];
} CmdMove;

typedef struct {
	int type;
	union {
		CmdNone none;
		CmdWait wait;
		CmdMove move;
	};
} Cmd;

Cmd cmd_none() {
	Cmd cmd;
	cmd.type = CMD_NONE;
	return cmd;
}

Cmd cmd_wait(uint32_t duration) {
	Cmd cmd;
	cmd.type = CMD_WAIT;
	cmd.wait.duration = duration;
	return cmd;
}

Cmd cmd_move(float speed, int axes, const int32_t steps[]) {
	int i;
	Cmd cmd;
	cmd.type = CMD_MOVE;
	cmd.move.speed = speed;
	
	if (axes > MAX_AXES) { axes = MAX_AXES; }
	for (i = 0; i < axes; ++i) {
		cmd.move.steps[i] = steps[i];
	}
	for (i = axes; i < MAX_AXES; ++i) {
		cmd.move.steps[i] = 0;
	}
	
	return cmd;
}
