#include <stdio.h>
#include <assert.h>

#include <rpicnc.h>

int main(int argc, char *argv[]) {
	printf("start\n");

	AxisInfo axis_info = {
		.mask_step_pos=(1<<12)|(1<<21),
		.mask_step_neg=0,

		.mask_dir_pos=(1<<20),
		.mask_dir_neg=(1<<16),

		.sense=0,
		.pin_left=0,
		.pin_right=0,

		.position=0,
		.length=0,
	};

	assert(cnc_init(1, &axis_info) == 0);

	uint32_t delay = 1000*1000; // microseconds
	int factor = 1; // step division factor

	// continuous movement
	float speed_vel = 1200*factor; // speed, Hz (steps per second)
	uint32_t steps_vel = 200*30*factor; // number of steps

	float speed_ini = 400*factor; // initial speed, Hz
	uint32_t steps_acc = 20*30*factor; // number of steps during acceleration

	// speed conversion from Hz to period in microseconds 
	uint32_t period_vel = (uint32_t) (1e6/speed_vel);
	uint32_t period_ini = (uint32_t) (1e6/speed_ini);
	uint32_t period_mid = (uint32_t) (1e6/(0.5*speed_vel + 0.5*speed_ini));

	Cmd cmds[] = {
		// make some room for acceleration
		cmd_move_acc(0, (steps_acc + 0)/2, period_ini, period_mid),
		cmd_move_acc(0, (steps_acc + 1)/2, period_mid, period_ini),

		cmd_wait(delay),

		// accelerate and move forward
		cmd_move_acc(1, steps_acc, period_ini, period_vel),
		cmd_move_vel(1, steps_vel, period_vel),
		cmd_move_acc(1, steps_acc, period_vel, period_ini),

		cmd_wait(delay),
		
		// accelerate and move backward
		cmd_move_acc(0, steps_acc, period_ini, period_vel),
		cmd_move_vel(0, steps_vel, period_vel),
		cmd_move_acc(0, steps_acc, period_vel, period_ini),

		cmd_wait(delay),

		// return to initial position
		cmd_move_acc(1, (steps_acc + 1)/2, period_ini, period_mid),
		cmd_move_acc(1, (steps_acc + 0)/2, period_mid, period_ini),
	};

	Task task = {
		.type=TASK_CMDS,
		.cmds={
			.cmds_count={sizeof(cmds)/sizeof(cmds[0])},
			.cmds={cmds},
		},
	};

	//for(;;) {
	assert(cnc_run_task(&task) == 0);
	//}

	return 0;
}
