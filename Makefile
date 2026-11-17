CXX      := clang++
CPPFLAGS := -Isrc
CXXFLAGS := -Wall -Wextra -O3
DEPFLAGS := -MMD -MP
LDFLAGS  :=
LDLIBS   :=

SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/parsolver

SRCS := $(shell find $(SRC_DIR) -name '*.cc')
OBJS := $(SRCS:$(SRC_DIR)/%.cc=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) compile_commands.json

.PHONY: all clean

-include $(DEPS)
