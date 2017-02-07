#pragma once

#include "driver.h"

DriverImpl *create_driver(const char * const params[]);
void destroy_driver(DriverImpl *driver);

AxisImpl *create_axis(DriverImpl *driver, const char * const params[]);
void destroy_axis(DriverImpl *driver, AxisImpl *axis);

uint32_t get_counter(const AxisImpl *axis);
void set_counter(AxisImpl *axis, uint32_t value);

void set_handler(AxisImpl *axis, void (*handler)(AxisEvent event, void *data), void *data);

uint32_t move(AxisImpl *axis, uint8_t dir, uint32_t count, uint32_t period_us);
void move_async(AxisImpl *axis, uint8_t dir, uint32_t count, uint32_t period_us);
