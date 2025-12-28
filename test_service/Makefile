# Compiler and Flags
CC = gcc
# Added -g for debug symbols and -O0 to disable optimization for easier debugging
CFLAGS = -Wall -Wextra -D_GNU_SOURCE -I.
DEBUG_FLAGS = -g -O0

# Directories
COMM_DIR = comm
TEST_DIR = test_service

# Source Files
SOURCES = $(TEST_DIR)/sentinel_service_test.c $(COMM_DIR)/noonlight.c
OBJECTS = $(SOURCES:.c=.o)

# Target Binary
TARGET = sentinel_service_test

# Default Rule: Build the standard test binary
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Debug Rule: Build with symbols for GDB/Valgrind
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "Debug build complete. Use 'gdb ./$(TARGET)' to start debugging."

# Rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the test
run: all
	./$(TARGET)

# Cleanup
clean:
	rm -f $(COMM_DIR)/*.o $(TEST_DIR)/*.o $(TARGET)

.PHONY: all clean run debug
