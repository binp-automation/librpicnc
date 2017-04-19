#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

#define GPIO_MODE_INPUT  PI_INPUT
#define GPIO_MODE_OUTPUT PI_OUTPUT

int gpio_init() {
	return gpioInitialise();
}

int gpio_term() {
	return gpioTerminate();
}

int gpio_set_mode(int pin, int mode) {
	return gpioSetMode(pin, mode);
}

int gpio_set_alert_func(int pin, ) {
	gpioSetAlertFuncEx(axis->pin_left, NULL, NULL);
}

uint32_t _axis_get_ramp_delay(const CmdMove move, int pos) {
	float speed = sqrt(2*move.accel*(pos + 0.5));
	uint32_t delay = FREQ_TO_US(speed);
	if (delay > MAX_DELAY) {
		delay = MAX_DELAY;
	}
	return delay;
}

int axis_scan(Axis *axis, Generator *gen, float speed, float accel) {
	int pulse_count = 0x100;

	_AxisScanCookie cookie;
	cookie.axis = axis;
	cookie.gen = gen;
	cookie.pulse_count = pulse_count;
	cookie.pulses = (gpioPulse_t*) malloc(sizeof(gpioPulse_t)*cookie.pulse_count);

	gpioSetAlertFuncEx(axis->pin_left, _axis_scan_alert, (void*) &cookie);
	gpioSetAlertFuncEx(axis->pin_right, _axis_scan_alert, (void*) &cookie);

	if (!gpioRead(axis->pin_right)) {
		axis_set_cmd(axis, cmd_move(0x7fffffff, speed, accel));
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		gen_clear(gen);
		_axis_state_init(&axis->state);
		gpioDelay(SCAN_DELAY);
	}

	cookie.counter = 0;
	if(!gpioRead(axis->pin_left)) {
		gen->counter = 0;
		axis_set_cmd(axis, cmd_move(-0x80000000, speed, accel));
		gen_run(gen, _axis_get_wave, (void*) &cookie);
		axis->length = ((gen->counter/pulse_count)*(pulse_count - 2) + cookie.counter)/4;
		gen_clear(gen);
		_axis_state_init(&axis->state);
		gpioDelay(SCAN_DELAY);
	}

	printf("counter: %d\n", cookie.counter);

	
	gpioSetAlertFuncEx(axis->pin_right, NULL, NULL);

	free(cookie.pulses);

	return 0;
}
