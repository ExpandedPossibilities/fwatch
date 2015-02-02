CFLAGS=-std=c99
SRCS=$(wildcard *.c)

all: fwatch

watchpaths.o: watchpaths.c watchpaths.h
	$(CC) -c $(CFLAGS) $< -o $@

canonicalpath.o: canonicalpath.c canonicalpath.h
	$(CC) -c $(CFLAGS) $< -o $@

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $^ -o $@

canonicalpath_test: canonicalpath_test.c canonicalpath.o
	$(CC) $(CFLAGS) $^ -o $@

test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch fwatchtest canonicalpath_test *.o
	rm -fr *.dSYM

depend:
	makedepend -f $(MAKEFILE_LIST) -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

# DO NOT DELETE

canonicalpath.o: canonicalpath.h
canonicalpath_test.o: canonicalpath.h
fwatch.o: watchpaths.h reallocarray.h
watchpaths.o: watchpaths.h canonicalpath.h reallocarray.h
