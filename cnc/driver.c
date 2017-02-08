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

	((typeof(&destroy_driver)) sym)(driver->impl);

	dlclose(driver->module);

	driver->impl = NULL;
	driver->module = NULL;

	return 0;
}
