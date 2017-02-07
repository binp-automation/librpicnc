#include "../export.h"

#include <stdlib.h>

typedef struct DriverImpl {

} DriverImpl;

DriverImpl *create_driver(const char * const params[]) {
	return malloc(sizeof(DriverImpl));
}

void destroy_driver(DriverImpl *driver) {
	free(driver);
}
