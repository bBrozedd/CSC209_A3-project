CFLAGS = -Wall -g -Wextra -MMD -MP
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

server: $(SRC_DIR)/server.c
	gcc $(CFLAGS) -o server $<

worker: $(SRC_DIR)/worker.c
	gcc $(CFLAGS) -o worker $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

-include $(OBJS:.o=.d)
