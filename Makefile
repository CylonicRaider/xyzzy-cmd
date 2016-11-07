
CC = klcc
CFLAGS = -O2 -flto -static -Wall
LDFLAGS = -fwhole-program

.PHONY: clean

frobnicate: frobnicate.c
	$(CC) -DFROBNICATE_STANDALONE -o $@ $< $(CFLAGS) $(LDFLAGS)

frobnicate.o: frobnicate.c frobnicate.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o frobnicate
