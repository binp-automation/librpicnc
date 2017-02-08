CFLAGS=-Wall


.PHONY: all dirs gpio test

all: dirs test # gpio


test:
	cd $@; make all OBJ_DIR=../$(OBJ_DIR)/$@ BIN_DIR=../$(BIN_DIR)/$@ INC_DIR=../$(INC_DIR)

gpio:
	cd $@; make all OBJ_DIR=../$(OBJ_DIR)/$@ BIN_DIR=../$(BIN_DIR)/$@ INC_DIR=../$(INC_DIR)


dirs: $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@
