CC = gcc
CFLAGS = -Wall -I../components/config -I../components/engine -I../components/storage -I../components/hal -g
# IMPORTANT: -lcurl must be here
LDFLAGS = -lcjson -lcurl

SRCS = main_mock.c \
       ../components/engine/engine.c \
       ../components/engine/user_mgr.c \
       ../components/engine/notifier.c \
       ../components/storage/storage.c \
       ../components/hal/hal_linux.c

OBJS = $(SRCS:.c=.o)
TARGET = sentinel_test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Deep cleaning..."
	rm -f $(TARGET)
	find .. -name "*.o" -type f -delete

run: all
	./$(TARGET)