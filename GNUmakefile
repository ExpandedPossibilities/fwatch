CFLAGS=-std=c99
SRCS=$(wildcard *.c)

all: fwatch

fwatch:  watchpaths.o canonicalpath.o

canonicalpath_test: canonicalpath.o


test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch fwatchtest canonicalpath_test *.o *.bak
	rm -fr *.dSYM

depend:
	: > make.deps
	makedepend -f make.deps -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

include make.deps
