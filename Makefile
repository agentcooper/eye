CPP = g++
CPPFLAGS = -g -Wall -Wextra -Wno-unused-parameter -Wnon-virtual-dtor -pedantic -Wimplicit-fallthrough -std=c++20
ifeq ($(DEBUG), 1)
    CPPFLAGS += -DDEBUG
endif
ifeq ($(TRACE), 1)
    CPPFLAGS += -DTRACE
endif
SRC_DIR = src
BUILD_DIR = build
TARGET = eye

OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Compiler

all: $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) -MMD `llvm-config --cxxflags` $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CPP) `llvm-config --ldflags --system-libs --libs all` $(CPPFLAGS) -o $@ $^

-include $(wildcard $(BUILD_DIR)/*.d)

# Runtime

$(BUILD_DIR)/runtime.o: runtime/runtime.cpp runtime/GarbageCollector.hpp runtime/debug.hpp
	$(CPP) $(CPPFLAGS) -c runtime/runtime.cpp -o $(BUILD_DIR)/runtime.o

.PHONY: program.o
program.o: $(TARGET) $(SOURCE_FILE)
	./$(TARGET) emit $(SOURCE_FILE)

program: program.o $(BUILD_DIR)/runtime.o
	@$(CPP) $(CPPFLAGS) program.o $(BUILD_DIR)/runtime.o -o program

# Other

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	rm -rf program.o program

.PHONY: snapshot
snapshot: $(TARGET)
	./scripts/snapshot
