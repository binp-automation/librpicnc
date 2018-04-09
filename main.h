#include "device.h"
#include "generator.h"
#include "command.h"
#include "task.h"


typedef struct {
	int step;
	int dir;
	int left;
	int right;
} AxisInfo;


int cnc_init(int axes_count, AxisInfo *axes_info);
int cnc_quit();

// synchronous
int cnc_run_task(Task task);

// asynchronous
int cnc_run_task_async(Task task);
int cnc_is_busy();
int cnc_stop();
