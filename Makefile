CF=-Wall
LF=-pthread -lpigpio

.PHONY: all

all: ramp

ramp: main.o
	gcc ${LF} $^ -o $@

main.o: main.c axis.h
	gcc ${CF} -c $< -o $@

