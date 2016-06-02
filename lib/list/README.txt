REARME for list.c
Ihab Basri, April 2016

compiling:
			type 'make' in the terminal and it will call Makefile (See below) and it will compile
			 all the files -> then call the executable (in this case listtest).
			
			
Makefile:

CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -ggdb
PROG = listtest
OBJS = listtest.o list.o
LIBS =

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

listtest.o: list.h
list.o: list.h

clean:
	rm -f *~
	rm -f *.o
	rm -f $(PROG)
	
Files needed:
	bag.c
	bag.h
	bagtest.c
	Makefile
			
Usage:

/*File: list.c - a data structure module for a list of (string, data)pairs
*
*Lab 3 Question 2 list.c 
*
* A module that export three functions: 
*creating a new list: a function to create a new empty list data structure
*and return NULL if the there is an error allocating memory for it
*
*finding nodes in the list based on the key (tag): it will return data for the node with the matching key
* and NULL if it was not found
*
*inserting items into the list if they are linked to a new key:it will insert the node with the attached data to the beginning of the list
*if key is not matching 
*NULL list pointers will return false
*and return false if it failed to do create the node or if there was a matching key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

Example command lines:
						$ make
						$ listtest
		
Exit status: (based on the test file you use)
	0 - success
	1 - any failure
	
Assumptions:
- the user is responsible to provide an appropriate pointer to list after creating it, or the module will return NULL
- the user is resposible to creating the list before inserting or tring to find nodes.
- the user is responsible to provide a pointer to the data to be stored and not the data itself.
- the user is responsible to provide the appropriate delete function for the data (if used malloc) to delete the data
- the user is assumed not to use the bag pointer after deleting the whole structure or it will seg fault
  
Limitations: 
- the list will insert and return data in the node as void and it is the user responsibilty to know the type of the pointer being
received after extraction (the module is free to return any node from the list)

Documentations: 
- the list is implemented as a linked list where nodes are inserted at the beginning of the list if the no mathcing keys

//end
