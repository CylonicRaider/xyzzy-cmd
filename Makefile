
KLIBC = /usr/lib/klibc
CC = gcc
LD = gcc
STRIP = strip
# The -iwithprefix apparently magically adds the GCC include directory
# into the path.
CFLAGS = -O2 -flto -Wall -D__KLIBC__ -D_BITSIZE=64 -iwithprefix include \
    -nostdinc -I$(KLIBC)/include/bits64 -I$(KLIBC)/include/arch/x86_64 \
    -I$(KLIBC)/include -Isrc -Iautosrc
LDFLAGS = -fwhole-program -nostdlib -static -L$(KLIBC)/lib \
    $(KLIBC)/lib/crt0.o -lc

.PHONY: clean
# Implicit rules create a cyclic dependency between %.frs and %.frs.c.
.SUFFIXES:

xyzzy: build/xyzzy-full
	$(STRIP) -sw -R '.note*' -R '.comment*' -o $@ $<

frobnicate build/xyzzy-full:
	$(LD) -o $@ $^ $(CFLAGS) $(LDFLAGS)

frobnicate: build/frobnicate.o build/frobnicate-main.o
build/xyzzy-full: build/xyzzy.o build/comm.o build/frobnicate.o \
    build/strings.frs.o

build/xyzzy.o: src/frobnicate.h autosrc/strings.frs.h
build/comm.o: src/frobnicate.h
build/frobnicate.o: src/die.h src/frobnicate.h
build/frobnicate-main.o: src/frobnicate.h
build/strings.frs.o: autosrc/strings.frs.h

build autosrc:
	mkdir -p $@

build/%.o: src/%.c | build
	$(CC) -c -o $@ $< $(CFLAGS)
build/%.o: autosrc/%.c | build
	$(CC) -c -o $@ $< $(CFLAGS)

autosrc/%.frs.h autosrc/%.frs.c: src/%.frs frobnicate script/frobstrings.py \
    | autosrc
	script/frobstrings.py -o autosrc/$*.frs.c -h autosrc/$*.frs.h \
	src/$*.frs || { rm -f autosrc/$*.frs.h autosrc/$*.frs.c; false; }

clean:
	rm -rf build autosrc frobnicate xyzzy
