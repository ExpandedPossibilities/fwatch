CFLAGS=-std=c99

.ifdef RELEASE
CFLAGS+=-O2 -pipe
.else
CFLAGS+=-g -pipe -Wall -pedantic
.endif

.ifdef DEBUG
CFLAGS+= -DDEBUG
.endif

.ifdef DEBUG_WATCH_PATHS
CFLAGS+= -DDEBUG_WATCH_PATHS
.endif

#echo *.c
SRCS=canonicalpath.c canonicalpath_test.c fwatch.c watchpaths.c

all: fwatch

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $> -o $@

canonicalpath_test: canonicalpath_test.c canonicalpath.o
	$(CC) $(CFLAGS) $> -o $@

test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch canonicalpath_test *.o

depend:
	makedepend -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null


# DO NOT DELETE

canonicalpath.o: canonicalpath.h
canonicalpath_test.o: canonicalpath.h
fwatch.o: watchpaths.h reallocarray.h
watchpaths.o: watchpaths.h canonicalpath.h reallocarray.h
