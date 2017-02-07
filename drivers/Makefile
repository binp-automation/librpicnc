CFLAGS=-Wall


.PHONY: all dirs gpio test

all: dirs $(OBJ_DIR)/driver.o test # gpio


$(OBJ_DIR)/driver.o: driver.c driver.h export.h
	gcc -c $(CFLAGS) $< -o $@


test:
	cd $@; make all OBJ_DIR=../$(OBJ_DIR)/$@ BIN_DIR=../$(BIN_DIR)/$@

gpio:
	cd $@; make all OBJ_DIR=../$(OBJ_DIR)/$@ BIN_DIR=../$(BIN_DIR)/$@


dirs: $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@
