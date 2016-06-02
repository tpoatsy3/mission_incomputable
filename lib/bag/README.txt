REARME for bag.c
Ihab Basri, April 2016

compiling:
			type 'make' in the terminal and it will call Makefile (See below) and it will compile
			all the files -> then call the executable (in this case bagtest).
			
			
Makefile:

		CC = gcc
		CFLAGS = -Wall -pedantic -std=c11 -ggdb
		PROG = bagtest
		OBJS = bagtest.o bag.o
		LIBS =

		$(PROG): $(OBJS)
		$(CC) $(CFLAGS) $^ $(LIBS) -o $@

		bagtest.o: bag.h		
		bag.o: bag.h

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

/*File: bag.h - a header file for a data structure module for a collection of indistinguishable items
*
*Lab 3 Question 1 bag.h 
*
* A module that export three functions: creating a new bag, inserting items into the bag
* and removing items from the bag and returning it to the user
*
* bag_new: a function to create a new empty bag data structure
*			and return NULL if the there is an error allocating memory for it	 
*
* bag_insert:	it will insert the item with the attached data to the beginning of the bag
*				NULL bag is ignored
*				and return NULL if it failed to do so
*
* bag extract: removing the first item in the bag and returns its data to the user
*				it returns NULL if bag is empty or the wrong bag pointer was provided
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

Example command lines:
						$ make
						$ bagtest
		
Exit status: (based on the test file you use)
	0 - success
	1 - any failure
	
Assumptions:
- the user is responsible to provide an appropriate pointer to bag after creating it, or the module will ignore the request
- the user is resposible to creating the bag before inserting or extracting items.
- the user is responsible to provide a pointer to the data to be stored and not the data itself
- the user is responsible to provide the appropriate delete function for the data (if used malloc) to delete the data
- the user is assumed not to use the bag pointer after deleting the whole structure or it will seg fault
  
Limitations: 
- the bag will insert and extract data in the item as void and it is the user responsibilty to know the type of the pointer being
received after extraction (the module is free to return any item from the bag)

Documentations: 
- the bag is implemented as a linked list were items are inserted at the beginning of the list
-the way items are extracted is through last inserted first extracted logic

//end