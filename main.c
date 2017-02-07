#include <stdlib.h>
#include <stdio.h>

#include "drivers/driver.h"

int	main(int argc, char *argv[]) {
	Driver drv;
	const char * const params[] = {NULL};
	load_driver(&drv, "./drivers/test", params);

	free_driver(&drv);

	return 0;
}