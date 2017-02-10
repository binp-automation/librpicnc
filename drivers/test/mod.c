#include <stdlib.h>
#include <stdio.h>

#include <export.h>


typedef struct CNC_DriverImpl {
	int axis_count;
} CNC_DriverImpl;

typedef struct CNC_AxisImpl {
	CNC_DriverImpl *driver;
	int32_t step_counter;
	void (*handler)(CNC_AxisEvent event, void *data);
	void *handler_data;
} CNC_AxisImpl;

CNC_DriverImpl *create_driver(void *params[]) {
	CNC_DriverImpl *driver = malloc(sizeof(CNC_DriverImpl));
	driver->axis_count = 0;
	return driver;
}

int destroy_driver(CNC_DriverImpl *driver) {
	if (driver->axis_count > 0) {
		fprintf(
			stderr, 
			"[%s] cannot destroy driver: there are %d binded axes\n", 
			__FUNCTION__, driver->axis_count
		);
		return 1;
	}
	free(driver);
	return 0;
}

CNC_AxisImpl *create_axis(CNC_DriverImpl *driver, void *params[]) {
	CNC_AxisImpl *axis = malloc(sizeof(CNC_AxisImpl));

	axis->driver = driver;
	axis->step_counter = 0;
	axis->handler = NULL;
	axis->handler_data = NULL;

	driver->axis_count += 1;

	return axis;
}

int destroy_axis(CNC_AxisImpl *axis) {
	axis->driver->axis_count -= 1;
	free(axis);
	return 0;
}

int32_t axis_get_counter(CNC_AxisImpl *axis) {
	return axis->step_counter;
}

void axis_set_counter(CNC_AxisImpl *axis, int32_t value) {
	axis->step_counter = value;
}

void axis_set_handler(CNC_AxisImpl *axis, void (*handler)(CNC_AxisEvent event, void *data), void *data) {
	axis->handler = handler;
	axis->handler_data = data;
}

int32_t axis_move(CNC_AxisImpl *axis, int32_t count, uint32_t period_us) {
	axis->step_counter += count;
	return count;
}

void axis_move_async(CNC_AxisImpl *axis, int32_t count, uint32_t period_us) {
	axis->step_counter += count;
}
