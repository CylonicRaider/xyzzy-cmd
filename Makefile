
CC = gcc
STRIP = strip

# Choose libc
#KLIBC = /usr/lib/klibc
ifneq ($(strip $(KLIBC)),)
    # The -iwithprefix apparently magically adds the GCC include directory
    # into the path.
    CFLAGS = -O2 -g -std=c99 -flto -Wall -Werror -D__KLIBC__ -D_BITSIZE=64 \
        -iwithprefix include -nostdinc -I$(KLIBC)/include/bits64 \
        -I$(KLIBC)/include/arch/x86_64 -I$(KLIBC)/include -Iinc \
        -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables
    LDFLAGS = -fwhole-program -nostdlib -static -L$(KLIBC)/lib \
        $(KLIBC)/lib/crt0.o -lc -Wl,--gc-sections
    STRIPFLAGS = -s -R '.note*' -R '.comment*'
else
    CFLAGS = -O2 -g -std=c99 -Wall -Werror -Iinc
    LDFLAGS =
    STRIPFLAGS =
endif

# Kbuild-like messages
ifneq ($(strip $(V)),)
    I = @\#
    Q =
else
    I = @
    Q = @
endif

.PHONY: clean
# Implicit rules create a cyclic dependency between %.frs and %.frs.c.
.SUFFIXES:
.SECONDARY:

xyzzy: build/xyzzy-full
	$(I)echo "  STRIP   $@"
	$(Q)$(STRIP) $(STRIPFLAGS) -o $@ $<

frobnicate build/xyzzy-full:
	$(I)echo "  CCLD    $@"
	$(Q)$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

build:
	$(I)echo "  MKDIR   $@"
	$(Q)mkdir -p $@

build/%.o: src/%.c | build
	$(I)echo "  CC      $@"
	$(Q)$(CC) -c -o $@ $< $(CFLAGS)

inc/%.frs.h src/%.frs.c: src/%.frs frobnicate script/frobstrings.py
	$(I)echo "  FROB    $<"
	$(Q)script/frobstrings.py -o src/$*.frs.c -h inc/$*.frs.h src/$*.frs || \
	{ rm -f inc/$*.frs.h src/$*.frs.c; false; }

clean:
	$(I)echo "  CLEAN   ."
	$(Q)rm -rf build src/*.frs.c inc/*.frs.h .deps.mk frobnicate xyzzy

# Be less noisy.
.deps.mk: $(filter-out %.frs.c %.frs.h,$(wildcard src/*.c inc/*.h)) \
    src/*.frs script/makedeps.py
	@INCLUDE=inc script/makedeps.py $^ \
	frobnicate:build/frobnicate-main.o build/xyzzy-full:build/xyzzy.o \
	> $@

-include .deps.mk
