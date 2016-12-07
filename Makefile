
KLIBC = /usr/lib/klibc
CC = gcc
LD = gcc
STRIP = strip
# The -iwithprefix apparently magically adds the GCC include directory
# into the path.
CFLAGS = -O2 -g -std=c99 -flto -Wall -Werror -D__KLIBC__ -D_BITSIZE=64 \
    -iwithprefix include -nostdinc -I$(KLIBC)/include/bits64 \
    -I$(KLIBC)/include/arch/x86_64 -I$(KLIBC)/include -Isrc \
    -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LDFLAGS = -fwhole-program -nostdlib -static -L$(KLIBC)/lib \
    $(KLIBC)/lib/crt0.o -lc -Wl,--gc-sections

.PHONY: clean
# Implicit rules create a cyclic dependency between %.frs and %.frs.c.
.SUFFIXES:
.SECONDARY:

xyzzy: build/xyzzy-full
	$(STRIP) -s -R '.note*' -R '.comment*' -o $@ $<

frobnicate build/xyzzy-full:
	$(LD) -o $@ $^ $(CFLAGS) $(LDFLAGS)

frobnicate: build/frobnicate.o build/frobnicate-main.o
build/xyzzy-full: build/strings.frs.o build/frobnicate.o build/status.o \
    build/note.o build/userhash.o build/comm.o build/ioutils.o \
    build/xyzzy.o

build:
	mkdir -p $@

build/%.o: src/%.c | build
	$(CC) -c -o $@ $< $(CFLAGS)

src/%.frs.h src/%.frs.c: src/%.frs frobnicate script/frobstrings.py
	script/frobstrings.py -o src/$*.frs.c -h src/$*.frs.h src/$*.frs || \
	{ rm -f src/$*.frs.h src/$*.frs.c; false; }

clean:
	rm -rf build src/*.frs.[ch] .deps.mk frobnicate xyzzy

# Be less noisy.
.deps.mk: $(filter-out %.frs.c %.frs.h,$(wildcard src/*.[ch])) src/*.frs \
    script/makedeps.py
	@script/makedeps.py $^ > $@

-include .deps.mk
