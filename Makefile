
PORT = 58800
CFLAGS = -Wall -g -Wextra -MMD -MP -DPORT=$(PORT)
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
TARGETS = test_model server worker

OBJS = $(TEST_MODEL_OBJS)
TEST_MODEL_SRCS = $(SRC_DIR)/model.c $(SRC_DIR)/test_model.c
TEST_MODEL_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_MODEL_SRCS))

.PHONY: all clean

all: $(BUILD_DIR) $(TARGETS)

test_model: $(TEST_MODEL_OBJS)
	gcc $^ -o $@ $(LDFLAGS)

server: $(BUILD_DIR)/server.o
	gcc $^ -o $@ $(LDFLAGS)

worker: $(BUILD_DIR)/worker.o $(BUILD_DIR)/model.o
	gcc $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

-include $(OBJS:.o=.d)
