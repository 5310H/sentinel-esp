CC = gcc
CFLAGS = -g -Wall -I../components/config -I../components/engine -I../components/storage -I../components/hal -I../components/comms -g
LDFLAGS = -lcjson -lcurl -lpthread -lssl -lcrypto -DMG_ENABLE_OPENSSL=1 

OBJ = ../components/storage/storage_mgr.o \
      ../components/comms/dispatcher.o \
      ../components/comms/noonlight.o \
      ../components/comms/smtp.o \
      ../components/engine/user_mgr.o \
      ../components/engine/engine.o \
      ../components/engine/monitor.o \
      mongoose.o \
      main_mock.o

all: sentinel_test

sentinel_test: $(OBJ)
	$(CC) $(OBJ) -o sentinel_test $(LDFLAGS) \
	-Wl,--wrap=hal_get_zone_state \
	-Wl,--wrap=hal_set_relay \
	-Wl,--wrap=hal_set_siren \
	-Wl,--wrap=hal_set_strobe \
	-Wl,--wrap=hal_get_relay_state

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) sentinel_test history.csv
