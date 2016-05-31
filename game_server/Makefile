# Makefile for 'server' data type
#
# Topaz, May 2016

L = ./lib
C = ./common

PROG = server
OBJS = server.o 
LLIBS = $L/cs50ds.a
CLIBS = $C/common.a

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C -lm -std=gnu99
MAKE = make

$(PROG): $(OBJS) $(LLIBS) $(CLIBS)
	$(CC) $(CFLAGS) $^  -o $@

server.o:

# build the library
$L/cs50ds.a: 
	cd $L; $(MAKE)

$C/common.a: 
	cd $C; $(MAKE)

clean: 
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
	cd $L; $(MAKE) clean
	cd $C; $(MAKE) clean



# all: 
# 	@echo "TODO: make game_servergame_server..."

# clean:
# 	@echo "TODO: make clean game_server..."
