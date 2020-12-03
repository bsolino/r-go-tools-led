CC ?= cc
CFLAGS += -Wall -Wextra -O2 -std=c99

all: r-go-led

r-go-led: r-go-led.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o r-go-led r-go-led.c $(LDLIBS)

clean:
	rm -f r-go-led
