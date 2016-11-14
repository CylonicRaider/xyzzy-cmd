
KLIBC = /usr/lib/klibc
CC = gcc
LD = gcc
# The -iwithprefix apparently magically adds the GCC include directory
# into the path.
CFLAGS = -O2 -flto -Wall -D__KLIBC__ -D_BITSIZE=64 -iwithprefix include \
    -nostdinc -I$(KLIBC)/include/bits64 -I$(KLIBC)/include/arch/x86_64 \
    -I$(KLIBC)/include
LDFLAGS = -fwhole-program -nostdlib -static -L$(KLIBC)/lib \
    $(KLIBC)/lib/crt0.o -lc

.PHONY: clean

frobnicate: frobnicate.c Makefile
	$(LD) -DFROBNICATE_STANDALONE -o $@ $< $(CFLAGS) $(LDFLAGS)

frobnicate.o: frobnicate.c frobnicate.h Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

strings.h strings.c: strings.ftm frobnicate
	./frobstrings.py -o strings.c -h strings.h strings.ftm || \
	rm -f strings.h strings.c

clean:
	rm -rf strings.[ch] *.o frobnicate
