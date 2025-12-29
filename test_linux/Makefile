CC = gcc
# Added -I../components/comms to CFLAGS
CFLAGS = -Wall -I../components/config -I../components/engine -I../components/storage -I../components/hal -I../components/comms -g
LDFLAGS = -lcjson -lcurl -lpthread -Wl,--wrap=hal_get_zone_state

# Updated SRCS to include the new comms components
SRCS = main_mock.c \
       mongoose.c \
       ../components/engine/engine.c \
       ../components/engine/monitor.c \
       ../components/engine/user_mgr.c \
       ../components/comms/dispatcher.c \
       ../components/comms/smtp.c \
       ../components/comms/noonlight.c \
       ../components/storage/storage_mgr.c \
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