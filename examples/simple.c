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

	float sv = 500;
	//uint32_t cv = 1000;

	float sa = 200;
	uint32_t ca = 200;

	uint32_t pv = (uint32_t) (1e6/sv);
	uint32_t pa = (uint32_t) (1e6/sa);

	Cmd cmds[] = {
		cmd_move_acc(1, ca, pa, pv),
		//cmd_move_vel(1, cv, pv),
		cmd_move_acc(1, ca, pv, pa),
		cmd_move_acc(0, ca, pa, pv),
		//cmd_move_vel(0, cv, pv),
		cmd_move_acc(0, ca, pv, pa),
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
