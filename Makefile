CC ?= cc
CFLAGS += -Wall -Wextra -O2 -std=c99

all: r-go-led x11idle

r-go-led: r-go-led.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o r-go-led r-go-led.c $(LDLIBS)

x11idle: x11idle.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o x11idle x11idle.c $(LDLIBS) -lX11 -lXss

clean:
	rm -f r-go-led x11idle
