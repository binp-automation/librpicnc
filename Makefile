CF=-Wall
LF=-pthread -lpigpio

BUILD_DIR=build

BIN=$(BUILD_DIR)/cnc

.PHONY: all clean dirs cnc pigpio

all: dirs pigpio cnc

cnc: $(BIN)

$(BIN): $(BUILD_DIR)/main.o
	gcc ${LF} $^ -o $@

$(BUILD_DIR)/main.o: main.c axis.h pigpio/pigpio.h
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
