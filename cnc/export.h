#pragma once

#include "driver.h"

CNC_DriverImpl *create_driver(void *params[]);
int destroy_driver(CNC_DriverImpl *driver);

CNC_AxisImpl *create_axis(CNC_DriverImpl *driver, void *params[]);
int destroy_axis(CNC_AxisImpl *axis);

int32_t axis_get_counter(CNC_AxisImpl *axis);
void axis_set_counter(CNC_AxisImpl *axis, int32_t value);

void axis_set_handler(CNC_AxisImpl *axis, void (*handler)(CNC_AxisEvent event, void *data), void *data);

int32_t axis_move(CNC_AxisImpl *axis, int32_t count, uint32_t period_us);
void axis_move_async(CNC_AxisImpl *axis, int32_t count, uint32_t period_us);
