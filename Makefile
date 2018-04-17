BUILD_DIR=build

HEADERS=pigpio/pigpio.h ringbuffer.h command.h generator.h axis.h device.h main.h
BIN=$(BUILD_DIR)/cnc
LIB=$(BUILD_DIR)/cnc.so

CF=-g -Wall -Ipigpio -fPIC
LF=-lm -pthread -L$(BUILD_DIR)/pigpio/
PIGPIO_OBJS=$(BUILD_DIR)/pigpio/command.o $(BUILD_DIR)/pigpio/pigpio.o

.PHONY: all clean dirs cnc pigpio

all: dirs pigpio cnc

cnc: $(BIN) $(LIB)

$(BIN): $(BUILD_DIR)/main.o $(PIGPIO_OBJS)
	gcc ${LF} $^ -o $@

$(LIB): $(BUILD_DIR)/main.o $(PIGPIO_OBJS)
	gcc ${LF} $^ -shared -o $@

$(BUILD_DIR)/main.o: main.c $(HEADERS)
	gcc ${CF} -c $< -o $@

dirs:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/pigpio

pigpio:
	cd pigpio; make lib
	cp pigpio/libpigpio.so ./$(BUILD_DIR)/pigpio/
	cp pigpio/command.o pigpio/pigpio.o ./$(BUILD_DIR)/pigpio/

clean: 
	cd pigpio; make clean
	rm -rf $(BUILD_DIR)
