#include <stdlib.h>

#include <export.h>


typedef struct CNC_DriverImpl {

} CNC_DriverImpl;

CNC_DriverImpl *create_driver(void *params[]) {
	return malloc(sizeof(CNC_DriverImpl));
}

int destroy_driver(CNC_DriverImpl *driver) {
	free(driver);
	return 0;
}
