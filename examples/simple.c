#include <stdio.h>
#include <assert.h>

#include <rpicnc.h>

int main(int argc, char *argv[]) {
	printf("start\n");

	AxisInfo axis_info = {
		.mask_step_pos=(1<<16),
		.mask_step_neg=0,

		.mask_dir_pos=0,
		.mask_dir_neg=(1<<12),

		.sense=0,
		.pin_left=0,
		.pin_right=0,

		.position=0,
		.length=0,
	};

	assert(cnc_init(1, &axis_info) == 0);

	float min_vel = 500; // steps per second
	float max_vel = 1000; // steps per second
	uint32_t acc_steps = 500;
	uint32_t max_vel_steps = 500;

	uint32_t min_vel_period_us = (uint32_t) (1e6/min_vel);
	uint32_t max_vel_period_us = (uint32_t) (1e6/max_vel);

	Cmd cmds[] = {
		// Forward
		cmd_move_acc(1, acc_steps, min_vel_period_us, max_vel_period_us),
		cmd_move_vel(1, max_vel_steps, max_vel_period_us),
		cmd_move_acc(1, acc_steps, max_vel_period_us, min_vel_period_us),
		// Backward
		cmd_move_acc(0, acc_steps, min_vel_period_us, max_vel_period_us),
                cmd_move_vel(0, max_vel_steps, max_vel_period_us),
                cmd_move_acc(0, acc_steps, max_vel_period_us, min_vel_period_us),
	};

	Task task = {
		.type=TASK_CMDS,
		.cmds={
			.cmds_count={sizeof(cmds)/sizeof(cmds[0])},
			.cmds={cmds},
		},
	};

	for(;;) {
		assert(cnc_run_task(&task) == 0);
	}

	return 0;
}

