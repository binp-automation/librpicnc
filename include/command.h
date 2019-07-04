#pragma once

#include <stdlib.h>
#include <stdint.h>

#define CMD_NONE       0x00

#define CMD_IDLE       0x01
#define CMD_WAIT       0x02
#define CMD_SYNC       0x03

#define CMD_MOVE       0x10

#define CMD_MOVE_VEL   0x11
#define CMD_MOVE_ACC   0x12
#define CMD_MOVE_SIN   0x13

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
	uint32_t period; // us
} CmdMoveVel;

typedef struct {
	uint32_t begin_period;
	uint32_t end_period;
} CmdMoveAcc;

typedef struct {
	uint32_t begin;
	uint32_t size;
	uint32_t period;
} CmdMoveSin;

typedef struct {
	uint8_t type;
	uint8_t dir;
	uint32_t steps;
	union {
		CmdMoveVel vel;
		CmdMoveAcc acc;
		CmdMoveSin sin;
	};
} CmdMove;

typedef struct {
	uint8_t type;
	union {
		CmdNone none;
		CmdWait wait;
		CmdSync sync;
		CmdMove move;
	};
} Cmd;


Cmd cmd_none();
Cmd cmd_idle();
Cmd cmd_wait(uint32_t duration);
Cmd cmd_sync(uint32_t id, uint32_t count);
Cmd cmd_move_vel(uint8_t dir, uint32_t steps, uint32_t period);
Cmd cmd_move_acc(uint8_t dir, uint32_t steps, uint32_t begin_period, uint32_t end_period);
Cmd cmd_move_sin(uint8_t dir, uint32_t steps, uint32_t begin, uint32_t size, uint32_t period);
