CFLAGS=-std=c99

.ifdef RELEASE
CFLAGS +=-O2 -pipe
.else
CFLAGS +=-g -pipe -Wall -pedantic
.endif

.ifdef FW_DEBUG
CFLAGS += -DFW_DEBUG
.endif

.ifdef WP_DEBUG
CFLAGS += -DWP_DEBUG
.endif

SRCS=canonicalpath.c canonicalpath_test.c fwatch.c watchpaths.c
DEPS=deps.mk

all: fwatch

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $> -o $@

canonicalpath_test: canonicalpath_test.c canonicalpath.o
	$(CC) $(CFLAGS) $> -o $@

clean:
	rm -f ../obj/*

depend:
	: > $(DEPS)
	makedepend -f $(DEPS) -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

.if exists($(DEPS))
.include "$(DEPS)"
.endif
