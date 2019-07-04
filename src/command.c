#include <command.h>


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

Cmd cmd_move_vel(uint8_t dir, uint32_t steps, uint32_t period) {
	Cmd cmd;
	cmd.type = CMD_MOVE;
	cmd.move.type = CMD_MOVE_VEL;
	cmd.move.dir = dir;
	cmd.move.steps = steps;
	cmd.move.vel.period = period;
	return cmd;
}

Cmd cmd_move_acc(uint8_t dir, uint32_t steps, uint32_t begin_period, uint32_t end_period) {
	Cmd cmd;
	cmd.type = CMD_MOVE;
	cmd.move.type = CMD_MOVE_ACC;
	cmd.move.dir = dir;
	cmd.move.steps = steps;
	cmd.move.acc.begin_period = begin_period;
	cmd.move.acc.end_period = end_period;
	return cmd;
}

Cmd cmd_move_sin(uint8_t dir, uint32_t steps, uint32_t begin, uint32_t size, uint32_t period) {
	Cmd cmd;
	cmd.type = CMD_MOVE;
	cmd.move.type = CMD_MOVE_SIN;
	cmd.move.dir = dir;
	cmd.move.steps = steps;
	cmd.move.sin.begin = begin;
	cmd.move.sin.size = size;
	cmd.move.sin.period = period;
	return cmd;
}
