
STRIP = strip

# We recommend building against the klibc, by using its "klcc" script as CC.
CFLAGS = -Iinc -std=c99 -Wall -Werror -O2 -g -fno-asynchronous-unwind-tables
LDFLAGS = -static
STRIPFLAGS = -s -R '.note*' -R '.comment*'

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
