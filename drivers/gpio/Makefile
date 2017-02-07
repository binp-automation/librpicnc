CFLAGS=-Wall -fPIC
LFLAGS=-lwiringPi

.PHONY: all dirs

all: dirs $(BUILD_DIR)/mod.so $(BUILD_DIR)/mod.conf

$(BUILD_DIR)/mod.so: $(OBJ_DIR)/mod.o
	gcc -shared $(LFLAGS) $^ -o $@

$(OBJ_DIR)/mod.o: $(SRC_DIR)/mod.c
	gcc -c $(CFLAGS) $< -o $@

dirs: $(BUILD_DIR) $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(BUILD_DIR)/mod.conf: $(SRC_DIR)/mod.conf
	cp -u $< $@