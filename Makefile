CFLAGS=-g 


all: fwatch #fwatchtest

watch_paths.o: watch_paths.c watch_paths.h
	$(CC) -c $(CFLAGS) $< -o $@

canonicalpath.o: canonicalpath.c canonicalpath.h
	$(CC) -c $(CFLAGS) $< -o $@

fwatch:  watch_paths.o canonicalpath.o fwatch.c
	$(CC) $(CFLAGS) $^ -o $@

canonicalpath_test: canonicalpath_test.c canonicalpath.o
	$(CC) $(CFLAGS) $^ -o $@

test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch fwatchtest canonicalpath_test *.o
	rm -fr *.dSYM
