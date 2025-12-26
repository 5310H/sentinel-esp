CC = gcc
CFLAGS = -Wall -I../components/config -I../components/engine -I../components/storage -I../components/hal -g
LDFLAGS = -lcjson -lcurl -lpthread -Wl,--wrap=hal_get_zone_state

# Added ../components/engine/monitor.c to the list
SRCS = main_mock.c \
       mongoose.c \
       ../components/engine/engine.c \
       ../components/engine/monitor.c \
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
	rm -f $(TARGET)
	find .. -name "*.o" -type f -delete
	rm -f *.o