CC      ?= cc
CFLAGS  ?= -std=gnu99 -Wall -Wextra -O2 -g
SRC     := src/ringbuf.c src/protocol.c src/sensor.c src/gateway.c
HEADERS := src/ringbuf.h src/protocol.h src/sensor.h
TARGET  := gateway
SIM     := gateway_sim
TESTBIN := test_core

.PHONY: all sim test clean

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -DUSE_MQTT $(SRC) -o $@ -lmosquitto

sim: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(SIM)

test: tests/test_ringbuf.c tests/test_protocol.c src/ringbuf.c src/protocol.c $(HEADERS)
	$(CC) $(CFLAGS) -o $(TESTBIN) \
	    tests/test_ringbuf.c tests/test_protocol.c src/ringbuf.c src/protocol.c
	./$(TESTBIN)

clean:
	rm -f $(TARGET) $(SIM) $(TESTBIN) *.o
