# Makefile for 'hashtable' module
#
# Ihab Basri, April 24, 2016

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb
PROG = hashtabletest
OBJS = hashtabletest.o hashtable.o
LIBS =

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

hashtabletest.o: hashtable.h
hashtable.o: hashtable.h

clean:
	rm -f *~
	rm -f *.o
	rm -f $(PROG)