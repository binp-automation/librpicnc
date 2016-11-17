#pragma once

void gpio_init();
void gpio_quit();

void gpio_start();
void gpio_stop();

void gpio_set_dir(int dir);

void gpio_set_clock(double freq);