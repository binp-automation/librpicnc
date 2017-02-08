CFLAGS=$(shell pkg-config --cflags gtk+-2.0) -Wall
LFLAGS=$(shell pkg-config --libs gtk+-2.0) -ldl

BIN_DIR=build
OBJ_DIR=build/obj

.PHONY: all dirs drivers

all: dirs drivers $(BIN_DIR)/cnc-gui


$(BIN_DIR)/cnc-gui: $(OBJ_DIR)/main.o $(OBJ_DIR)/cnc/driver.o
	gcc $(LFLAGS) $^ -o $@

$(OBJ_DIR)/main.o: main.c
	gcc -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/cnc/driver.o: cnc/driver.c cnc/driver.h cnc/export.h
	gcc -c $(CFLAGS) $< -o $@


drivers:
	cd $@; make all OBJ_DIR=../$(OBJ_DIR)/$@ BIN_DIR=../$(BIN_DIR)/$@ INC_DIR=../cnc


dirs: $(BIN_DIR) $(OBJ_DIR) $(OBJ_DIR)/cnc

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/cnc:
	mkdir -p $@