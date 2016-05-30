# Makefile for 'cs50ds' library
#   Builds and compiles all the data structures into a library.
#
# Ihab Basri and Issa Sylla, May 6 2016


OBJS = web.o file.o word.o index.o



LIB = common.a

CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make

#compiling
$(LIB): $(OBJS)
	ar cr $@ $^

web.o: web.h

index.o: index.h

file.o: file.h

word.o: word.h

clean:
	rm -f *~
	rm -f *.o
	rm -f $(LIB)

	
#end