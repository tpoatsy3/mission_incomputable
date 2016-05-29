# Makefile for 'cs50ds' library
#   Builds and compiles all the data structures into a library.
#
# Ihab Basri and Issa Sylla, May 1 2016


OBJS = memory/memory.o bag/bag.o list/list.o counters/counters.o hashtable/hashtable.o 
LIB = cs50ds.a

CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make

$(LIB): $(OBJS)
	ar cr $@ $^

bag/bag.o: bag/bag.c bag/bag.h
	cd bag; $(MAKE) bag.o

list/list.o: list/list.c list/list.h
	cd list; $(MAKE) list.o
	
counters/counters.o: counters/counters.c counters/counters.h
	cd counters; $(MAKE) counters.o
	
hashtable/hashtable.o: hashtable/hashtable.c hashtable/hashtable.h
	cd hashtable; $(MAKE) hashtable.o

memory/memory.o: memory/memory.c memory/memory.h
	cd memory; $(MAKE) memory.o

clean:
	rm -f *~
	rm -f *.o
	rm -f $(LIB)
	cd memory; $(MAKE) $@
	cd bag; $(MAKE) $@
	cd list; $(MAKE) $@
	cd counters; $(MAKE) $@
	cd hashtable; $(MAKE) $@
	
#end
