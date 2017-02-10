CFLAGS=-Wall -fPIC
LFLAGS=

.PHONY: all dirs

all: dirs $(BIN_DIR)/mod.so

$(BIN_DIR)/mod.so: $(OBJ_DIR)/mod.o
	gcc -shared $(LFLAGS) $^ -o $@

$(OBJ_DIR)/mod.o: mod.c $(INC_DIR)/export.h
	gcc -c $(CFLAGS) -I $(INC_DIR) $< -o $@

dirs: $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@