TARGET=hopscotch

SRC_DIR = ./src
OBJ_DIR = ./obj
LIB_DIR = ./lib
BIN_DIR = ./bin
INC_DIR = ./include

CC = g++

CFLAGS += \
	-g \
	-Wall \
	-std=c++11 \
	-fsanitize=address \
#	-O2 \

LIBS += \
	-lcityhash \
	-lpthread \
	-laio \

DEFS += \
	-DCITYHASH \
	-DLINUX_AIO \
	-DUNIFORM \
#	-DHOTSPOT \

OBJ_SRC += \
	$(SRC_DIR)/util.c \
	$(SRC_DIR)/keygen.c \
	$(SRC_DIR)/queue.c \
	$(SRC_DIR)/cond_lock.c \
	$(SRC_DIR)/request.c \
	$(SRC_DIR)/handler.c \
	$(SRC_DIR)/request.c \
	$(SRC_DIR)/device.c \
	$(SRC_DIR)/poller.c \
	$(SRC_DIR)/aio.c \

TARGET_OBJ =\
		$(patsubst %.c,%.o,$(OBJ_SRC))\

all: client server

client: $(SRC_DIR)/client.cc $(LIB_DIR)/libdfhash.a
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS) $(DEFS) -I$(INC_DIR) 

server: $(SRC_DIR)/server.cc $(OBJ_DIR)/$(TARGET).o $(LIB_DIR)/libdfhash.a
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS) $(DEFS) -I$(INC_DIR)

$(OBJ_DIR)/$(TARGET).o: $(SRC_DIR)/$(TARGET).c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $^ $(CFLAGS) $(LIBS) $(DEFS) -I$(INC_DIR)
	@mv *.o $(OBJ_DIR)/

$(LIB_DIR)/libdfhash.a: $(TARGET_OBJ)
	@mkdir -p $(LIB_DIR)
	@mv $(SRC_DIR)/*.o $(LIB_DIR)
	$(AR) r $@ $(LIB_DIR)/*

.c.o:
	$(CC) $(CFLAGS) $(LIBS) $(DEFS) -c $< -o $@ -I$(INC_DIR)

clean:
	@rm -vf $(BIN_DIR)/*
	@rm -vf $(OBJ_DIR)/*.o
	@rm -vf $(LIB_DIR)/*.o
	@rm -vf $(LIB_DIR)/libdfhash.a
