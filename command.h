#pragma once

#include <stdlib.h>

#define CMD_NONE       0x00

#define CMD_IDLE       0x01
#define CMD_WAIT       0x02
#define CMD_SYNC       0x03

#define CMD_MOVE       0x11
#define CMD_ACCL       0x12
#define CMD_SINE       0x13

typedef struct {
	
} CmdNone;

typedef struct {
	
} CmdIdle;

typedef struct {
	uint32_t duration; // us
} CmdWait;

typedef struct {
	uint32_t id;
	uint32_t count;
} CmdSync;

typedef struct {
	uint8_t dir;
	uint32_t steps;
	uint32_t period; // us
} CmdMove;

typedef struct {
	uint8_t dir;
	uint32_t steps;
	uint32_t begin_period;
	uint32_t end_period;
} CmdAccl;

typedef struct {
	uint8_t dir;
	uint32_t steps;
	uint32_t begin;
	uint32_t size;
	uint32_t period;
} CmdSine;

typedef struct {
	int type;
	union {
		CmdNone none;
		CmdWait wait;
		CmdSync sync;
		CmdMove move;
		CmdAccl accl;
		CmdSine sine;
	};
} Cmd;

Cmd cmd_none() {
	Cmd cmd;
	cmd.type = CMD_NONE;
	return cmd;
}

Cmd cmd_idle() {
	Cmd cmd;
	cmd.type = CMD_IDLE;
	return cmd;
}

Cmd cmd_wait(uint32_t duration) {
	Cmd cmd;
	cmd.type = CMD_WAIT;
	cmd.wait.duration = duration;
	return cmd;
}

Cmd cmd_sync(uint32_t id, uint32_t count) {
	Cmd cmd;
	cmd.type = CMD_SYNC;
	cmd.sync.id = id;
	cmd.sync.count = count;
	return cmd;
}

Cmd cmd_move(uint8_t dir, uint32_t steps, uint32_t period) {
	Cmd cmd;
	cmd.type = CMD_MOVE;
	cmd.move.dir = dir;
	cmd.move.steps = steps;
	cmd.move.period = period;
	return cmd;
}

Cmd cmd_accl(uint8_t dir, uint32_t steps, uint32_t begin_period, uint32_t end_period) {
	Cmd cmd;
	cmd.type = CMD_ACCL;
	cmd.accl.dir = dir;
	cmd.accl.steps = steps;
	cmd.accl.begin_period = begin_period;
	cmd.accl.end_period = end_period;
	return cmd;
}

Cmd cmd_sine(uint8_t dir, uint32_t steps, uint32_t begin, uint32_t size, uint32_t period) {
	Cmd cmd;
	cmd.type = CMD_SINE;
	cmd.sine.dir = dir;
	cmd.sine.steps = steps;
	cmd.sine.begin = begin;
	cmd.sine.size = size;
	cmd.sine.period = period;
	return cmd;
}
