
# Makefile for UDP socket examples
#
# David Kotz, May 2016

PROGS = chatclient chatserver
LIBS = 
LLIBS = 

# added  -Wno-braced-scalar-init because Linux and MacOS disagree
FLAGS = # 
CFLAGS = -Wall -Wno-braced-scalar-init -pedantic -std=c11 -ggdb $(FLAGS)
CC = gcc
MAKE = make
# for memory-leak tests
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all

all: $(PROGS)

########### chatclient ##################
chatclient: chatclient.o file.o $(LLIBS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

chatclient.o: file.h

########### chatserver #################
chatserver: chatserver.o file.o $(LLIBS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

chatserver.o: 

clean:
	rm -rf *~ *.o *.dSYM
	rm -f $(PROGS) 