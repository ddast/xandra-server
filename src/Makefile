binname=xandra

CC=gcc
CFLAGS=-Wall\
       -Wextra\
       -Wpedantic\
       -std=gnu11\
       -fdiagnostics-color=always\
       -MMD -MP
LDFLAGS=-lxdo
LDLIBS=

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

$(binname): $(OBJ)

-include $(DEP)

.PHONY: clean
clean:
	rm -f $(binname) $(OBJ) $(DEP)
