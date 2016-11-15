
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
# Implicit rules create a cyclic dependency between %.frs and %.frs.c.
.SUFFIXES:

frobnicate: frobnicate.c Makefile
	$(LD) -DFROBNICATE_STANDALONE -o $@ $< $(CFLAGS) $(LDFLAGS)

frobnicate.o: frobnicate.c frobnicate.h Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

%.frs.h %.frs.c: %.frs frobnicate
	./frobstrings.py -o $*.frs.c -h $*.frs.h $*.frs || \
	rm -f $*.frs.h $*.frs.c

clean:
	rm -rf *.frs.[ch] *.o frobnicate
