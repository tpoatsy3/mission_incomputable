REARME for hashtable.c
Ihab Basri, April 2016

compiling:
			type 'make' in the terminal and it will call Makefile (See below) and it will compile
			 all the files -> then call the executable (in this case hashtabletest).
			
			
Makefile:

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
			
Usage:

/*File: hashtable.c - a hashtable data structure module for a list of (string, data)pairs
*
*Lab 3 Question 4 hashtable.c 
*
* A module that export three functions: 
*creating a new hashtable: a function to create a new empty hashtable data structure
*and return NULL if the there is an error allocating memory for it
*
*finding nodes in the hashtable based on the key (tag): it will return data for the node with the matching key
* and NULL if it was not found
*
*inserting items into the hashtable if they are linked to a new key:it will insert the node with the attached data to the beginning of the list
*in the corresponding row if key is not matching 
*NULL hashtable pointers will return false
*and return false if it failed to do create the node or if there was a matching key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

Example command lines:
						$ make
						$ hashtabletest
		
Exit status: (based on the test file you use)
	0 - success
	1 - any failure
	
Assumptions:
- the user is responsible to provide an appropriate pointer to hashtable after creating it, or the module will return NULL
- the user is resposible to creating the hashtable before inserting or trying to find nodes.
- the user is responsible to provide a pointer to the data to be stored and not the data itself.
  
Limitations: 
- the hashtable will insert and return data in the node as void and it is the user responsibilty to know the type of the pointer being
received after extraction (the module is free to return any node from the list)

Documentations: 
- the hashtable is implemented as a table of linked lists where nodes are inserted at the beginning of the list if the no mathcing keys
- the number of slots has to be one or more when creating the hashtable or NULL will be returned
- jenkins hash function was adapted in this module (copied from CS50 class notes) in creating indices.

//end

