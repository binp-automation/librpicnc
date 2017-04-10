BUILD_DIR=build

HEADERS=pigpio/pigpio.h utility.h ringbuffer.h command.h generator.h axis.h device.h midi/still_alive.h
BIN=$(BUILD_DIR)/cnc

CF=-g -Wall -Ipigpio
LF=-lm -pthread -lpigpio -L$(BUILD_DIR)/pigpio/

.PHONY: all clean dirs cnc pigpio midi

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

midi: midi/still_alive.h

midi/still_alive.h: midi/transform.py
	cd midi; python3 transform.py > still_alive.h

clean: 
	cd pigpio; make clean
	rm -rf $(BUILD_DIR)
