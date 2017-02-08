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
	void *params[] = {"size", (void*) 1000, NULL};
	CNC_Driver *drv = cnc_create_driver("./drivers/test", params);

	if (drv != NULL) {
		cnc_destroy_driver(drv);
	}
	return 0;
}