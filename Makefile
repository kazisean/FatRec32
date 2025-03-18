CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Wextra  -Wno-unused
LDFLAGS=-lcrypto

.PHONY: all
all: fatrec32

fatrec32: fatrec32.o
	$(CC) $(LDFLAGS) -o $@ $^

fatrec32.o: fatrec32.c 
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o fatrec32