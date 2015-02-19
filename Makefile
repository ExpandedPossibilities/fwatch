CFLAGS=-std=c99

.ifdef RELEASE
CFLAGS +=-O2 -pipe
.else
CFLAGS +=-g -pipe -Wall -pedantic
.endif

.ifdef FW_DEBUG
CFLAGS += -DFW_DEBUG
.endif

.ifdef CP_DEBUG
CFLAGS += -DCP_DEBUG
.endif

.ifdef WP_DEBUG
CFLAGS += -DWP_DEBUG
.endif

.ifdef WP_COMPLAIN
CFLAGS += -DWP_COMPLAIN
.endif

DEPS=deps.mk

bins: fwatch canname

testbins: tests/runtests tests/cannames tests/t_canonicalpath tests/t_findslashes

all: bins testbins

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $> -o $@

canname: canname.c canonicalpath.o
	$(CC) $(CFLAGS) $> -o $@
tests:
	mkdir tests

#tests/runtests: tests
#tests/cannames: tests
#tests/t_canonicalpath: tests
#tests/t_findslashes: tests

tests/runtests: ../tests/runtests
	cp ../tests/runtests $@
	chmod 755 $@

tests/cannames: ../tests/genpaths.pl
	../tests/genpaths.pl > $@

tests/t_canonicalpath: ../tests/t_canonicalpath.c
	 $(CC) $(CFLAGS) $> -o $@

tests/t_findslashes: ../tests/t_findslashes.c canonicalpath.o
	 $(CC) $(CFLAGS) $> -o $@

test: tests all
	tests/runtests `pwd`

clean:
	rm -rf ../obj/*

depend:
	: > $(DEPS)
	makedepend -f $(DEPS) -Y -- $(CFLAGS) -- *.c tests/*.c 2>/dev/null

.if exists($(DEPS))
.include "$(DEPS)"
.endif
