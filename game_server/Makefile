# Makefile for 'server' data type
#
# Topaz, May 2016

#path of libraries
L = ../lib
C = ../common

PROG = gameserver
OBJS = server.o 
LLIBS = $L/cs50ds.a
CLIBS = $C/common.a

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C -lm -std=gnu99 -lncurses
MAKE = make

#compiling
$(PROG): $(OBJS) $(LLIBS) $(CLIBS)
	$(CC) $(CFLAGS) $^  -o $@

server.o:

# build the libraries
$L/cs50ds.a: 
	cd $L; $(MAKE)

$C/common.a: 
	cd $C; $(MAKE)

#cleaning
clean: 
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
	cd $L; $(MAKE) clean
	cd $C; $(MAKE) clean
