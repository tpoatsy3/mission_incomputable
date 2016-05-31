# Makefile for 'guideAgent' data type
# 
# Topaz, May 2016

L = ./lib
C = ./common

PROG = guideAgent
OBJS = guideAgent.o
LLIBS = $L/cs50ds.a
CLIBS = $C/common.a

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C
MAKE = make

$(PROG): $(OBJS) $(LLIBS) $(CLIBS)
	$(CC) $(CFLAGS) $^  -o $@

guideAgent.o:

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
	

#all: 
#	@echo "TODO: make guide_agent..."

#clean:
#	@echo "TODO: make clean guide_agent..."
