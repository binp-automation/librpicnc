CFLAGS=-Wall $(shell pkg-config --cflags gtk+-2.0)
LFLAGS=$(shell pkg-config --libs gtk+-2.0) -lwiringPi

all: motor

motor: main.o gpio.o
	gcc $(LFLAGS) $^ -o $@

main.o: main.c gui.h gpio.h
	gcc -c $(CFLAGS) $< -o $@

gpio.o: gpio.c gpio.h
	gcc -c $(CFLAGS) $< -o $@