#Subdir build template from:
#http://make.mad-scientist.net/papers/multi-architecture-builds/

OBJDIR:=obj
SRCDIR:=src

ifndef REALMAKE

.SUFFIXES:

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(abspath $(MAKEFILE_LIST)) \
		  REALMAKE=1 $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

GNUmakefile :: ;
Makefile :: ;
makefile :: ;
%.mk :: ;

% :: $(OBJDIR) ; :

.PHONY: clean
clean:
	rm -rf $(OBJDIR)

else
#----- End Boilerplate

SRCDIR=../src
vpath %c $(SRCDIR)
vpath %h $(SRCDIR)

CFLAGS=-std=c99
SRCS=$(wildcard $(SRCDIR)/*.c)
DEPS=../deps.mk

all: fwatch

fwatch:  watchpaths.o canonicalpath.o

canonicalpath_test: canonicalpath.o


test_canp: canonicalpath_test
	./canonicalpath_test fred/234//../w..//

clean:
	rm -f fwatch fwatchtest canonicalpath_test *.o *.bak
	rm -fr *.dSYM

depend:
	: > $(DEPS)
	makedepend -f $(DEPS) -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

include $(DEPS)

#----- Begin Boilerplate
endif
