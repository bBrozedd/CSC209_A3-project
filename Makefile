CFLAGS = -Wall -g

SRC_DIR = src
BUILD_DIR = build
TARGET = myprogram

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(BUILD_DIR) lr

lr: $(BUILD_DIR)/minimum_lr.o
	gcc $(CFLAGS) -o lr $^ -lm

$(BUILD_DIR)/minimum_lr.o: $(SRC_DIR)/minimum_lr.c
	gcc $(CFLAGS) -c $< -o $@ -lm

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) lr
