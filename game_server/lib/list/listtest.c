/*File: listtest.c - a test program for a data structure module for a list of (sting, data) pairs
*
*Lab 3 Question 2 listtest.c 	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

static void delete(void *data);

int main() 
{
	list_t *list;		      // testing the delete function (test1)
	list = list_new(delete);	
 
	printing_list_contents(list); 
 
	
	void *test_delete=malloc(10); //allocating memory
	void *test_delete2=malloc(10);
	void *test_delete3=malloc(10);
	void *test_delete4=malloc(10);
	
	list_insert(list, "KEY1", test_delete);
	list_insert(list, "KEY2", test_delete2);
	list_insert(list, "K3", test_delete3);
	list_insert(list, "K4", test_delete4);
	
	list_delete(list); //deleting the whole structure
	
	//printing_list_contents(list);  //it will seg fault here
  
}

// delete function
static void delete(void *data){
	if (data){
		free(data);
	}
}

//end