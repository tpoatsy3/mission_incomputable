REARME for counters.c
Ihab Basri, April 2016

compiling:
			type 'make' in the terminal and it will call Makefile (See below) and it will compile
			all the files -> then call the executable (in this case counterstest).
			
			
Makefile:

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb
PROG = counterstest
OBJS = counterstest.o counters.o
LIBS =

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

counterstest.o: counters.h
counters.o: counters.h

clean:
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
	
	
Files needed:
	counters.c
	counters.h
	counterstest.c
	Makefile
			
Usage:

/*File: counters.c - a data structure module for a list of (counters
*
*Lab 3 Question 3 counters.c 
*
* A module that export three functions: creating a new counters list, adding the counters if matching keys
* getting the values of the counters for matching keys and deleting the whole structure.
*
* create a new empty counters data structure
*
*
*incremente the counter if key exists. if not, it will set the counter to zero
*
*it will return the current value of the counter for matching key or zero if the key does not exist
*
*it will delete the whole structure with its contents
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

Example command lines:
						$ make
						$ counterstest
		
Exit status: (based on the test file you use)
	0 - success
	1 - any failure
	
Assumptions:
- the user is responsible to provide an appropriate pointer to counters after creating it, or the module will ignore the request
- the user is resposible to creating the counters before incrementating and acquiring the values of counters.

Documentations: 
- the bag is implemented as a linked list were nodes are inserted at the beginning of the list
- if NULL is inserted as key, it will be treated as a zero and it will increment any counters with a tag "zero"

//end