#include <wiringPi.h>

#define EN  22 // BCM GPIO 22
#define DIR 17 // BCM GPIO 17
#define PUL 18 // BCM GPIO 18

#define PWM_FREQ 19.2e6
#define PWM_RANGE 1024

static int gpio_clk = (int) (PWM_FREQ/(500*PWM_RANGE));
static int gpio_dir = 0;

void gpio_update_dir() {
	digitalWrite(DIR, gpio_dir ? HIGH : LOW);
}

void gpio_set_dir(int dir) {
	gpio_dir = dir;
	gpio_update_dir();
}

void gpio_update_clock() {
	pwmSetClock(gpio_clk);
	pwmWrite(PUL, PWM_RANGE/2);
}

void gpio_set_clock(double freq) {
	gpio_clk = (int) (PWM_FREQ/(freq*PWM_RANGE));
	if(gpio_clk < 2) { gpio_clk = 2; }
	if(gpio_clk > 4095) { gpio_clk = 4095; }
	gpio_update_clock();
}

void gpio_init() {
	wiringPiSetupGpio();

	pinMode(EN,  OUTPUT);
	pinMode(DIR, OUTPUT);
	pinMode(PUL, OUTPUT);

	digitalWrite(EN,  HIGH);
	digitalWrite(DIR, LOW);
	digitalWrite(PUL, LOW);
}

void gpio_quit() {
	pinMode(EN,  OUTPUT);
	pinMode(DIR, OUTPUT);
	pinMode(PUL, OUTPUT);

	digitalWrite(EN,  LOW);
	digitalWrite(DIR, LOW);
	digitalWrite(PUL, LOW);
}

void gpio_start() {
	digitalWrite(EN,  LOW);
	gpio_update_dir();

	pinMode(PUL, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	gpio_update_clock();
}

void gpio_stop() {
	digitalWrite(EN,  HIGH);

	pinMode(PUL, OUTPUT);
	digitalWrite(PUL, LOW);
}