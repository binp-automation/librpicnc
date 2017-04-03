#pragma once

#include <pigpio.h>

#include "generator.h"

typedef struct {
	Generator *gen;
} Device;

int dev_init(Device *dev, Generator *gen) {
	dev->gen = gen;
	return 0;
}

int dev_free(Device *dev) {
	// nothing
	return 0;
}

