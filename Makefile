TARGET=hopscotch

SRC_DIR = ./src
OBJ_DIR = ./obj
LIB_DIR = ./lib
BIN_DIR = ./bin

CC = g++

CFLAGS += \
	-g \
	-Wall \
	-std=c++11 \
	-fsanitize=address \
	-lcityhash \
	-DCITYHASH \
	-DUNIFORM \
#	-O2 \
#	-DHOTSPOT \

all: client server

client: $(SRC_DIR)/client.cc $(LIB_DIR)/libdfhash.a
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

server: $(SRC_DIR)/server.cc $(OBJ_DIR)/$(TARGET).o $(LIB_DIR)/libdfhash.a
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

$(OBJ_DIR)/$(TARGET).o: $(SRC_DIR)/$(TARGET).c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $^ $(CFLAGS)
	@mv *.o $(OBJ_DIR)/

$(LIB_DIR)/libdfhash.a:
	@mkdir -p $(LIB_DIR)
	$(CC) -c $(SRC_DIR)/util.c $(CFLAGS)
	$(CC) -c $(SRC_DIR)/keygen.c $(CFLAGS)
	@mv *.o $(LIB_DIR)
	$(AR) r $@ $(LIB_DIR)/*

clean:
	@rm -vf $(BIN_DIR)/*
	@rm -vf $(OBJ_DIR)/*.o
	@rm -vf $(LIB_DIR)/*.o
	@rm -vf $(LIB_DIR)/libdfhash.a
