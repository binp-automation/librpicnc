#include <stdlib.h>
#include <stdio.h>

#include "cnc/driver.h"

typedef struct {
	CNC_Axis axis;
} AxisView;

AxisView *axis_view_create() {
	return NULL;
}

int	main(int argc, char *argv[]) {
	CNC_Driver *drv = cnc_create_driver("./drivers/test", NULL);
	if (drv == NULL) {
		return 1;
	}

	void *params[] = {"size", (void*) 1000, NULL};
	CNC_Axis *ax = cnc_driver_create_axis(drv, params);
	if (ax == NULL) {
		return 2;
	}

	cnc_destroy_axis(ax);

	cnc_destroy_driver(drv);

	return 0;
}