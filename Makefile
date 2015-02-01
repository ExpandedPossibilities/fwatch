CFLAGS=-g 


all: fwatch #fwatchtest

#
#watchpaths.o: watchpaths.c watchpaths.h
#	$(CC) -c $(CFLAGS) $ -o $@
#
#canonicalpath.o: canonicalpath.c canonicalpath.h
#	$(CC) -c $(CFLAGS) $< -o $@

fwatch:  watchpaths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $> -o $@

canonicalpath_test: canonicalpath_test.c canonicalpath.o
	$(CC) $(CFLAGS) $> -o $@

test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch fwatchtest canonicalpath_test *.o
	rm -fr *.dSYM
