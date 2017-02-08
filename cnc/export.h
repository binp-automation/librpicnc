#pragma once

#include "driver.h"

CNC_DriverImpl *create_driver(void *params[]);
int destroy_driver(CNC_DriverImpl *driver);

CNC_AxisImpl *create_axis(CNC_DriverImpl *driver, void *params[]);
int destroy_axis(CNC_AxisImpl *axis);

uint32_t get_counter(CNC_AxisImpl *axis);
void set_counter(CNC_AxisImpl *axis, uint32_t value);

void set_handler(CNC_AxisImpl *axis, void (*handler)(CNC_AxisEvent event, void *data), void *data);

uint32_t move(CNC_AxisImpl *axis, uint8_t dir, uint32_t count, uint32_t period_us);
void move_async(CNC_AxisImpl *axis, uint8_t dir, uint32_t count, uint32_t period_us);
