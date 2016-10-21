#include "gpio.h"
#include "gui.h"

int main(int argc, char *argv[]) {
    gpio_init();
    gui_main(argc, argv);
    gpio_quit();
    return 0;
}