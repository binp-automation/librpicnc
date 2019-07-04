#include <stdio.h>
#include <assert.h>

#include <rpicnc.h>

int main(int argc, char *argv[]) {
	printf("start\n");

	AxisInfo axis_info = {
		.pin_step=12,
		.pin_dir=16,
		.pin_left=20,
		.pin_right=21,

		.position=1000000,
		.length=2000000,
	};
	assert(cnc_init(1, &axis_info) == 0);

	Cmd cmds[] = {cmd_move_vel(0, 100, 10000)};
	Task task = {
		.type=TASK_CMDS,
		.cmds={
			.cmds_count={sizeof(cmds)/sizeof(cmds[0])},
			.cmds={cmds},
		},
	};
	printf("%d\n", task.cmds.cmds_count[0]);
	assert(cnc_run_task(&task) == 0);

	return 0;
}
