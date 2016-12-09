
KLIBC = /usr/lib/klibc
CC = gcc
LD = gcc
STRIP = strip

ifneq ($(strip $(GLIBC)),)
CFLAGS = -Wall -Werror -Iinc
LDFLAGS =
else
# The -iwithprefix apparently magically adds the GCC include directory
# into the path.
CFLAGS = -O2 -g -std=c99 -flto -Wall -Werror -D__KLIBC__ -D_BITSIZE=64 \
    -iwithprefix include -nostdinc -I$(KLIBC)/include/bits64 \
    -I$(KLIBC)/include/arch/x86_64 -I$(KLIBC)/include -Iinc \
    -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
LDFLAGS = -fwhole-program -nostdlib -static -L$(KLIBC)/lib \
    $(KLIBC)/lib/crt0.o -lc -Wl,--gc-sections
endif

.PHONY: clean
# Implicit rules create a cyclic dependency between %.frs and %.frs.c.
.SUFFIXES:
.SECONDARY:

xyzzy: build/xyzzy-full
	$(STRIP) -s -R '.note*' -R '.comment*' -o $@ $<

frobnicate build/xyzzy-full:
	$(LD) -o $@ $^ $(CFLAGS) $(LDFLAGS)

build:
	mkdir -p $@

build/%.o: src/%.c | build
	$(CC) -c -o $@ $< $(CFLAGS)

inc/%.frs.h src/%.frs.c: src/%.frs frobnicate script/frobstrings.py
	script/frobstrings.py -o src/$*.frs.c -h inc/$*.frs.h src/$*.frs || \
	{ rm -f inc/$*.frs.h src/$*.frs.c; false; }

clean:
	rm -rf build src/*.frs.c inc/*.frs.h .deps.mk frobnicate xyzzy

# Be less noisy.
.deps.mk: $(filter-out %.frs.c %.frs.h,$(wildcard src/*.c inc/*.h)) \
    src/*.frs script/makedeps.py
	@INCLUDE=inc script/makedeps.py $^ \
	frobnicate:build/frobnicate-main.o build/xyzzy-full:build/xyzzy.o \
	> $@

-include .deps.mk
