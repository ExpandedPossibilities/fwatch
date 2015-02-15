#Subdir build template from:
#http://make.mad-scientist.net/papers/multi-architecture-builds/

OBJDIR:=obj

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
	rm -rf $(OBJDIR)/*

else
#----- End Boilerplate

SRCDIR=..
vpath %c $(SRCDIR)
vpath %h $(SRCDIR)

TEST_S=$(wildcard $(SRCDIR)/tests/*.c)
SRCS=$(wildcard $(SRCDIR)/*.c) $(TEST_S)
TEST_E=$(subst ../,,$(TEST_S:.c=))
DEPS=deps.mk

CFLAGS=-std=c99

ifdef ANALYZE
CFLAGS += --analyze
endif

ifdef RELEASE
CFLAGS += -O2 -pipe
else
CFLAGS += -g -pipe -Wall -pedantic
endif

ifdef FW_DEBUG
CFLAGS += -DFW_DEBUG
endif

ifdef CP_DEBUG
CFLAGS += -DCP_DEBUG
endif

ifdef WP_DEBUG
CFLAGS += -DWP_DEBUG
endif

ifdef WP_COMPLAIN
CFLAGS += -DWP_COMPLAIN
endif

all: fwatch canname

fwatch:  watchpaths.o canonicalpath.o

canname: canonicalpath.o

tests/t_findslashes: canonicalpath.o

tests/runtests: $(SRCDIR)/tests/runtests
	cp $< $@
	chmod 755 $@

tests:
	mkdir -p $@

tests/cannames: $(SRCDIR)/tests/genpaths.pl tests
	$< > $@

test: all tests tests/runtests tests/cannames $(TEST_E)
	tests/runtests `pwd`

depend:
	: > $(DEPS)
	makedepend -f $(DEPS) -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null

-include $(DEPS)

#----- Begin Boilerplate
endif
