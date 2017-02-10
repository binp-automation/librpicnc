#include "driver.h"

#include "export.h"

#include <dlfcn.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

CNC_Driver *cnc_create_driver(const char *path, void *params[]) {
	int len = strlen(path) + 7 + 1;
	char *mod_path = malloc(sizeof(char)*(len + 1));
	snprintf(mod_path, len, "%s/mod.so", path);
	void *mod = dlopen(mod_path, RTLD_NOW | RTLD_LOCAL);
	free(mod_path);
	if (mod == NULL) {
		fprintf(stderr, "[%s] cannot load shared object '%s/mod.so'\n", __FUNCTION__, path);
		goto err_dlib;
	}

	void *sym = dlsym(mod, "create_driver");
	
	if (sym == NULL) {
		fprintf(stderr, "[%s] cannot find symbol '%s'\n", __FUNCTION__, "create_driver");
		goto err_driver;
	}	

	CNC_DriverImpl *drvi = ((typeof(&create_driver)) sym)(params);
	if (drvi == NULL) {
		fprintf(stderr, "[%s] error create driver\n", __FUNCTION__);
		goto err_driver;
	}

	CNC_Driver *driver = malloc(sizeof(CNC_Driver));

	driver->impl = drvi;
	driver->module = mod;

	return driver;

err_driver:
	dlclose(mod);
err_dlib:
	return NULL;
}

int cnc_destroy_driver(CNC_Driver *driver) {
	void *sym = dlsym(driver->module, "destroy_driver");
	
	if (sym == NULL) {
		fprintf(stderr, "[%s] cannot find symbol '%s'\n", __FUNCTION__, "destroy_driver");
		return 1;
	}

	if (((typeof(&destroy_driver)) sym)(driver->impl) != 0) {
		fprintf(stderr, "[%s] error destroy driver\n", __FUNCTION__);
		return 1;
	}

	dlclose(driver->module);

	free(driver);

	return 0;
}

#define FN_COUNT 6

CNC_Axis *cnc_driver_create_axis(CNC_Driver *driver, void *params[]) {
	CNC_Axis *axis = malloc(sizeof(CNC_Axis));

	axis->fnptrs = malloc(sizeof(void*)*FN_COUNT);

	const char *const fns_sym[FN_COUNT] = {
		"destroy_axis",
		"axis_get_counter",
		"axis_set_counter",
		"axis_set_handler",
		"axis_move",
		"axis_move_async",
	};


	int i;
	for (i = 0; i < FN_COUNT; ++i) {
		void *sym = dlsym(driver->module, fns_sym[i]);
		if (sym == NULL) {
			fprintf(stderr, "[%s] cannot find symbol '%s'\n", __FUNCTION__, fns_sym[i]);
			goto err_fns;
		}
		axis->fnptrs[i] = sym;
	}

	void *sym = dlsym(driver->module, "create_axis");
	
	if (sym == NULL) {
		fprintf(stderr, "[%s] cannot find symbol '%s'\n", __FUNCTION__, "create_axis");
		goto err_create;
	}

	CNC_AxisImpl *axi = ((typeof(&create_axis)) sym)(driver->impl, params);
	if (axi == NULL) {
		fprintf(stderr, "[%s] error create axis\n", __FUNCTION__);
		goto err_create;
	}
	axis->impl = axi;

	return axis;

err_create:
err_fns:
	free(axis->fnptrs);
	free(axis);
	return NULL;
}

int cnc_destroy_axis(CNC_Axis *axis) {
	if (((typeof(&destroy_axis)) axis->fnptrs[0])(axis->impl) != 0) {
		fprintf(stderr, "[%s] error destroy axis\n", __FUNCTION__);
		return 1;
	}

	free(axis->fnptrs);
	free(axis);

	return 0;
}

int32_t cnc_axis_get_counter(CNC_Axis *self) {
	return ((typeof(&axis_get_counter)) self->fnptrs[1])(self->impl);
}

void cnc_axis_set_counter(CNC_Axis *self, int32_t value) {
	((typeof(&axis_set_counter)) self->fnptrs[2])(self->impl, value);
}

void cnc_axis_set_handler(CNC_Axis *self, void (*handler)(CNC_AxisEvent event, void *data), void *data) {
	((typeof(&axis_set_handler)) self->fnptrs[3])(self->impl, handler, data);
}

int32_t cnc_axis_move(CNC_Axis *self, int32_t count, uint32_t period_us) {
	return ((typeof(&axis_move)) self->fnptrs[4])(self->impl, count, period_us);
}

void cnc_axis_move_async(CNC_Axis *self, int32_t count, uint32_t period_us) {
	((typeof(&axis_move_async)) self->fnptrs[5])(self->impl, count, period_us);
}