BD=build

HEADERS=ringbuffer.h command.h generator.h axis.h axis_task.h device.h rpicnc.h task.h
SRCS=command.c generator.c axis.c axis_task.c device.c rpicnc.c
OBJS=$(SRCS:%=$(BD)/%.o)
LIB=$(BD)/rpicnc.so
EXAMPLES=saw.c turnover.c

CF=-g -Wall -I. -Iinclude -fPIC
LF=-lm -pthread -L$(BD)/pigpio/
PIGPIO_OBJS=$(BD)/pigpio/command.o $(BD)/pigpio/pigpio.o

.PHONY: all clean dirs pigpio rpicnc examples

all: dirs pigpio rpicnc examples

clean: 
	cd pigpio; make clean
	rm -rf $(BD)

dirs:
	mkdir -p $(BD)
	mkdir -p $(BD)/pigpio
	mkdir -p $(BD)/examples

pigpio:
	cd pigpio; make lib
	cp pigpio/libpigpio.so $(BD)/pigpio/
	cp pigpio/command.o pigpio/pigpio.o $(BD)/pigpio/

$(PIGPIO_OBJS): %: pigpio



rpicnc: $(LIB)

$(LIB): $(OBJS) $(PIGPIO_OBJS)
	gcc ${LF} $^ -shared -o $@

$(SRCS:%=$(BD)/%.o): $(BD)/%.o: src/% $(HEADERS:%=include/%)
	gcc ${CF} -c $< -o $@


examples: $(EXAMPLES:%.c=$(BD)/examples/%)

$(EXAMPLES:%=$(BD)/examples/%.o): $(BD)/examples/%.o: examples/% $(HEADERS:%=include/%)
	gcc ${CF} -c -I. $< -o $@

$(EXAMPLES:%.c=$(BD)/examples/%): %: %.c.o $(OBJS) $(PIGPIO_OBJS)
	gcc ${LF} $^ -o $@
