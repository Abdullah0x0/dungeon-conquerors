CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -lSDL2 -pthread -lm

SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include
ASSETS_DIR = assets

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = dungeon_conquerors

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: all
	./$(TARGET) 