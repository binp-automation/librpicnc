#include <axis.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#include <command.h>
#include <generator.h>


#define sqr(x) ((x)*(x))

static uint32_t isqrt(uint32_t x){
	uint32_t op  = x;
	uint32_t res = 0;
	// The second-to-top bit is set:
	//use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	uint32_t one = 1uL << 30;
	// "one" starts at the highest power of four <= than the argument.
	while (one > op) {
		one >>= 2;
	}
	while (one != 0) {
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
	return res;
}

static uint64_t isqrt64(uint64_t x){
	uint64_t op  = x;
	uint64_t res = 0;
	// The second-to-top bit is set: 
	// use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	uint64_t one = (uint64_t)1 << 62; 
	// "one" starts at the highest power of four <= than the argument.
	while (one > op) {
		one >>= 2;
	}
	while (one != 0) {
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
	return res;
}


#define MAX_DELAY 1000000 // us
#define REDIR_DELAY 100 // us


void _axis_state_init(_AxisState *st) {
	st->idle = 0;
	st->done = 1;
	st->cmd = cmd_none();
	st->remain = 0;
}

PinAction new_pin_action() {
	PinAction pa;
	pa.on = 0;
	pa.off = 0;
	return pa;
}

int axis_init(
	Axis *axis,
	uint32_t mask_step_pos, uint32_t mask_step_neg,
	uint32_t mask_dir_pos, uint32_t mask_dir_neg,
	int sense, uint32_t left, uint32_t right
) {
	axis->mask_step_pos  = mask_step_pos;
	axis->mask_step_neg  = mask_step_neg;
	axis->mask_dir_pos   = mask_dir_pos;
	axis->mask_dir_neg   = mask_dir_neg;

	axis->sense = sense;
	axis->pin_left  = left;
	axis->pin_right = right;
	
	axis->position = 0;
	axis->length = 0;
	
	_axis_state_init(&axis->state);

	uint32_t i = 0;
	for (i = 0; i < 32; ++i) {
		if (
			((1<<i) & axis->mask_step_pos) ||
			((1<<i) & axis->mask_step_neg) ||
			((1<<i) & axis->mask_dir_pos) ||
			((1<<i) & axis->mask_dir_neg)
		) {
			gpioSetMode(i, PI_OUTPUT);
		}
	}
	if (sense) {
		gpioSetMode(axis->pin_left,  PI_INPUT);
		gpioSetMode(axis->pin_right, PI_INPUT);
	}
	
	return 0;
}

int axis_free(Axis *axis) {
	if (axis) {
		uint32_t i = 0;
		for (i = 0; i < 32; ++i) {
			if (
				((1<<i) & axis->mask_step_pos) ||
				((1<<i) & axis->mask_step_neg) ||
				((1<<i) & axis->mask_dir_pos) ||
				((1<<i) & axis->mask_dir_neg)
			) {
				gpioSetMode(i, PI_INPUT);
			}
		}
		if (axis->sense) {
			gpioSetMode(axis->pin_left,  PI_INPUT);
			gpioSetMode(axis->pin_right, PI_INPUT);
		}
		return 0;
	}
	return 1;
}

void axis_set_cmd(Axis *axis, Cmd cmd) {
	_AxisState *st = &axis->state;
	st->cmd = cmd;
	st->idle = 0;
	st->done = 0;
	st->remain = 0;
	if (cmd.type == CMD_NONE) {
		st->done = 1;
	} else if (cmd.type == CMD_IDLE) {
		st->idle = 1;
	} else if (cmd.type == CMD_WAIT) {
		st->remain = cmd.wait.duration;
		st->done = 1;
	} else if (cmd.type == CMD_SYNC) {
		st->idle = 1;
	} else if (cmd.type == CMD_MOVE) {
		st->remain = 0;
		if (cmd.move.steps != 0) {
			st->steps = cmd.move.steps;
			st->dir = 0;
		} else {
			st->done = 1;
		}
	} else {
		st->done = 1;
	}
}

PinAction axis_eval_cmd(Axis *axis) {
	_AxisState *st = &axis->state;
	PinAction pa = new_pin_action();
	if (!st->idle && !st->done) {
		uint32_t delay = 0;
		if (st->cmd.type == CMD_MOVE) {
			if (st->cmd.move.type == CMD_MOVE_VEL) {
				delay = st->cmd.move.vel.period;
			} else if (st->cmd.move.type == CMD_MOVE_ACC) {
				uint64_t tb = st->cmd.move.acc.begin_period;
				uint64_t te = st->cmd.move.acc.end_period;
				uint64_t i = st->steps;
				uint64_t n = st->cmd.move.steps;
				if (te == 0) {
					delay = (tb*isqrt64(2*n))/isqrt64(2*i + 1);
				} else if (tb == 0) {
					delay = (te*isqrt64(2*n))/isqrt64(2*n - 2*i - 1);
				} else {
					delay = (tb*te*isqrt64(2*n))/isqrt64(tb*tb*(2*n - 2*i - 1) + te*te*(2*i + 1));
				}
			} else {
				st->done = 1;
			}
		} else {
			st->done = 1;
		}

		if (delay > MAX_DELAY) {
			delay = MAX_DELAY;
		}

		if (!st->done) {
			if (!st->dir) {
				if (st->cmd.move.dir) {
					pa.on = axis->mask_dir_pos;
					pa.off = axis->mask_dir_neg;
				} else {
					pa.off = axis->mask_dir_pos;
					pa.on = axis->mask_dir_neg;
				}
				st->remain = REDIR_DELAY;
				st->dir = 1;
			} else {
				if (st->steps > 0) {
					if (st->phase == 0) {
						pa.on = axis->mask_step_pos;
						pa.off = axis->mask_step_neg;
						st->remain = delay/2;
						st->phase = 1;
					} else {
						pa.off = axis->mask_step_pos;
						pa.on = axis->mask_step_neg;
						st->remain = delay/2 + (delay % 2);
						st->phase = 0;
						st->steps -= 1;
					}
				}
			}
			if (st->steps <= 0) {
				st->done = 1;
			}
		}
	}
	//printf("pa: %08x\n", pa);
	return pa;
}

PinAction axis_step(Axis *axis, Cmd (*get_cmd)(void*), void *userdata) {
	// _AxisState *st = &axis->state;
	if (axis->state.idle || axis->state.done) {
		axis_set_cmd(axis, get_cmd(userdata));
	}
	return axis_eval_cmd(axis);
}


uint8_t axis_read_sensors(Axis *axis) {
	return ((!!gpioRead(axis->pin_left))<<0) | ((!!gpioRead(axis->pin_right))<<1);
}
