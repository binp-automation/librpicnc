BUILD_DIR=build

HEADERS=pigpio/pigpio.h utility.h ringbuffer.h command.h generator.h axis.h device.h
BIN=$(BUILD_DIR)/cnc

CF=-g -Wall -Ipigpio
LF=-lm -pthread -lpigpio -L$(BUILD_DIR)/pigpio/

.PHONY: all clean dirs cnc pigpio

all: dirs pigpio cnc

cnc: $(BIN)

$(BIN): $(BUILD_DIR)/main.o
	gcc ${LF} $^ -o $@

$(BUILD_DIR)/main.o: main.c $(HEADERS)
	gcc ${CF} -c $< -o $@

dirs:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/pigpio

pigpio:
	cd pigpio; make lib
	cp pigpio/libpigpio*.so ./$(BUILD_DIR)/pigpio/

clean: 
	cd pigpio; make clean
	rm -rf $(BUILD_DIR)
