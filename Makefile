CC ?= cc
CFLAGS += -Wall -Wextra -O2 -std=c99

all: r-go-led standard-leds x11idle

r-go-led: r-go-led.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o r-go-led r-go-led.c $(LDLIBS)

standard-leds: standard-leds.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o standard-leds standard-leds.c $(LDLIBS)

x11idle: x11idle.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o x11idle x11idle.c $(LDLIBS) -lX11 -lXss

clean:
	rm -f r-go-led standard-leds x11idle
