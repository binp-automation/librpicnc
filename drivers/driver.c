#include "driver.h"

#include "export.h"

#include <dlfcn.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int load_driver(Driver *driver, const char *path, const char * const params[]) {
	int len = strlen(path) + 7 + 1;
	char *mod_path = malloc(sizeof(char)*(len + 1));
	snprintf(mod_path, len, "%s/mod.so", path);
	void *mod = dlopen(mod_path, RTLD_NOW | RTLD_LOCAL);
	free(mod_path);
	if (mod == NULL) {
		fprintf(stderr, "[driver] cannot load shared object '%s/mod.so'\n", path);
		return 1;
	}

	void *sym = dlsym(mod, "create_driver");
	
	if (sym == NULL) {
		fprintf(stderr, "[driver] cannot find symbol '%s'\n", "create_driver");
		dlclose(mod);
		return 2;
	}	

	DriverImpl *drvi = ((typeof(&create_driver)) sym)(params);
	if (drvi == NULL) {
		fprintf(stderr, "[driver] error create driver\n");
		dlclose(mod);
		return 3;
	}

	driver->impl = drvi;
	driver->module = mod;

	return 0;
}

int free_driver(Driver *driver) {
	void *sym = dlsym(driver->module, "destroy_driver");
	
	if (sym == NULL) {
		fprintf(stderr, "[driver] cannot find symbol '%s'\n", "destroy_driver");
		return 2;
	}

	((typeof(&destroy_driver)) sym)(driver->impl);

	dlclose(driver->module);

	driver->impl = NULL;
	driver->module = NULL;

	return 0;
}

int driver_create_axis(Driver *self, Axis *axis, const char * const params[]);
int driver_destroy_axis(Driver *self, Axis *axis);

uint32_t axis_get_counter(const Axis *self);
void axis_set_counter(Axis *self, uint32_t value);

void axis_set_handler(Axis *self, void (*handler)(AxisEvent event, void *data), void *data);

uint32_t axis_move(Axis *self, uint8_t dir, uint32_t count, uint32_t period_us);
void axis_move_async(Axis *self, uint8_t dir, uint32_t count, uint32_t period_us);

