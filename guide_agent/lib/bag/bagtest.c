/*File: bagtest.c - a test program for a data structure module for a collection of indistinguishable items
*
*Lab 3 Question 1 bagtest.c 	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bag.h"

static void delete(void *data);

int main() 
{
	bag_t *bag;		      // testing the delete function (test1)
	bag = bag_new(delete);		

	printing_bag_contents(bag); 
  
	void *test_delete=malloc(10);// allocating some 
	void *test_delete2=malloc(10);

	bag_insert(bag, test_delete);
	bag_insert(bag, test_delete2);

	bag_delete(bag);
	
	//printing_bag_contents(bag);  //it will seg fault here
}

//delete function
static void delete(void *data){
	if (data){
		free(data);
	}
}

//end