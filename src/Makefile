#------------------------------------------------------------------------------

prefix=/usr/local
bindir=${prefix}/bin
DESTDIR=
binname=xandra
OBJ=server.o unicode.o xandra.o

CC=gcc
# release
CFLAGS=-std=gnu11 -O3
# development
#CFLAGS=-pedantic\
       -Wextra\
       -Wall\
       -Waggregate-return\
       -Wcast-align\
       -Wcast-qual\
       -Wconversion\
       -Wfloat-equal\
       -Wlogical-op\
       -Wpointer-arith\
       -Wredundant-decls\
       -Wshadow\
       -Wstrict-overflow=5\
       -Wswitch-default\
       -Wswitch-enum\
       -Wundef\
       -Wwrite-strings\
       -Wbad-function-cast\
       -Wmissing-prototypes\
       -Wnested-externs\
       -Wstrict-prototypes\
       -std=gnu11\
       -O0\
       -fdiagnostics-color=always\
       -ggdb\
       -DDEBUG
LDFLAGS=
LDLIBS=-lxdo

#------------------------------------------------------------------------------

.PHONY: all clean distclean install uninstall

all: $(binname)

$(binname): $(OBJ)

server.o: server.c server.h unicode.h

unicode.o: unicode.c unicode.h

xandra.o: xandra.c server.h unicode.h

clean:
	rm -f $(binname) $(OBJ)

distclean: clean

install: $(binname)
	install -d $(DESTDIR)$(bindir)
	install $(binname) $(DESTDIR)$(bindir)/$(binname)

uninstall:
	rm $(DESTDIR)$(bindir)/$(binname)
