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
	-lpthread \
	-DCITYHASH \
	-DUNIFORM \
#	-O2 \
#	-DHOTSPOT \

OBJ_SRC += \
	$(SRC_DIR)/util.c \
	$(SRC_DIR)/stopwatch.c \
	$(SRC_DIR)/keygen.c \
	$(SRC_DIR)/queue.c \
	$(SRC_DIR)/cond_lock.c \
	$(SRC_DIR)/request.c \
	$(SRC_DIR)/handler.c \
	$(SRC_DIR)/request.c \

TARGET_OBJ =\
		$(patsubst %.c,%.o,$(OBJ_SRC))\

all: client server

client: $(SRC_DIR)/client.cc $(LIB_DIR)/libdfhash.a
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

server: $(SRC_DIR)/server.cc $(OBJ_DIR)/$(TARGET).o $(LIB_DIR)/libdfhash.a
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

$(OBJ_DIR)/$(TARGET).o: $(SRC_DIR)/$(TARGET).c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $^ $(CFLAGS)
	@mv *.o $(OBJ_DIR)/

$(LIB_DIR)/libdfhash.a: $(TARGET_OBJ)
	@mkdir -p $(LIB_DIR)
	@mv $(SRC_DIR)/*.o $(LIB_DIR)
	$(AR) r $@ $(LIB_DIR)/*

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -vf $(BIN_DIR)/*
	@rm -vf $(OBJ_DIR)/*.o
	@rm -vf $(LIB_DIR)/*.o
	@rm -vf $(LIB_DIR)/libdfhash.a
