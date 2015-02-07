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

.ifdef WP_COMPLAIN
CFLAGS += -DWP_COMPLAIN
.endif

SRCS=canonicalpath.c canname.c fwatch.c watchpaths.c
DEPS=deps.mk

all: fwatch canname

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $> -o $@

canname: canname.c canonicalpath.o
	$(CC) $(CFLAGS) $> -o $@

clean:
	rm -f ../obj/*

depend:
	: > $(DEPS)
	makedepend -f $(DEPS) -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

.if exists($(DEPS))
.include "$(DEPS)"
.endif
