BUILD_DIR=build

HEADERS=pigpio/pigpio.h utility.h ringbuffer.h command.h generator.h axis.h device.h main.h
BIN=$(BUILD_DIR)/cnc
LIB=$(BUILD_DIR)/cnc.so

CF=-g -Wall -Ipigpio -fPIC
LF=-lm -pthread -lpigpio -L$(BUILD_DIR)/pigpio/

.PHONY: all clean dirs cnc pigpio

all: dirs pigpio cnc

cnc: $(BIN) $(LIB)

$(BIN): $(BUILD_DIR)/main.o
	gcc ${LF} $^ -o $@

$(LIB): $(BUILD_DIR)/main.o
	gcc ${LF} $^ -shared -o $@

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
