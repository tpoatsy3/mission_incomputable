/*File: bag.c - a data structure module for a collection of indistinguishable items
*
*Lab 3 Question 1 bag.c 
*
* A module that export three functions: creating a new bag, inserting items into the bag
* and removing items from the bag and returning it to the user		 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bag.h"

/*********local types******/

// A structure for each item in the bag
typedef struct bag_item {
	void *data; //pointer to data for this item
	struct bag_item *next; //pointer to the next item in the linked list
} bag_item_t;

/*********global types******/

// a structure for the bag
typedef struct bag {
	struct bag_item *head; //pointer to the bag or the dummy head of the linked list
	void (*datadelete)(void *data); //to delete the data
} bag_t;

/*********local functions******/

//not visible outside this file
static bag_item_t * bag_insert_helper(bag_item_t *node, void *data);
static void* bag_extract_helper(bag_item_t *node, bag_t *bag);
void bag_delete_helper(bag_item_t *node, bag_t *bag);


// creating a new empty bag data structure
bag_t *
bag_new(void (*datadelete)(void *data))
{
	bag_t *bag = malloc(sizeof(bag_t));// allocating memory for the pointer to the bag
	
	if (bag == NULL) return NULL; //error allocating bag
	else {
		
		//initialize contents of bag structure
		bag -> head = NULL;
		bag -> datadelete = datadelete;
		return bag; //return the pointer to the bag created
	}
} 

// adding a new item to the bag
void 
bag_insert(bag_t *bag, void *data)
{	
	// call bag_insert_helper (invisible to the user to add an item and return the address)
	if (bag != NULL) { bag->head = bag_insert_helper(bag->head, data);
	
	} 
	/** testing if the bag pointer is valid **/
	//else printf("the pointer to the bag is not valid\n");	 
}

//bag insert helper
// it will insert the item with the attached data to the beginning of the bag
// and return NULL if it failed to do so
static bag_item_t *
bag_insert_helper(bag_item_t *node, void *data)
{	
	bag_item_t *new_node = malloc(sizeof(bag_item_t)); // allocating memory for each item in the bag
	if (new_node != NULL) {
		new_node -> data = data; //including data in each item
		new_node -> next = node; //the new item will point to the previous one in the list
		return new_node;
		
	} else return NULL; //error allocating the item
}

// removing the first item in the bag and returns it to the user
// it returns NULL if bag is empty or wrong the wrong bag pointer was provided
void *
bag_extract (bag_t *bag)
{
	// call bag_extract_helper (invisible to the user to extract the item and return it to the user)
	if (bag != NULL){	
	return bag_extract_helper(bag->head, bag);
	} else return NULL; //error allocating the bag
}

//extracting the first item in the bag and return its data to the user
// it returns NULL if the bag is empty
static void *
bag_extract_helper(bag_item_t *node, bag_t *bag) {
	if (node != NULL) { //bag is not empty
		
		bag -> head = node -> next; //the second item now becomes the first
		void *returndata = node -> data; 
		free (node); //freeing the item (deleting it)
		return returndata; // return the data to the user
		
	} else {
		return NULL; // the bag is empty
	}
}

//printing the contents of the bag
void printing_bag_contents(bag_t *bag){
		/** testing insert, everytime the bag_insert is called, it will print out
		the contents of the bag whether an item was inserted or failed to be inserted **/
		for (bag_item_t *test=bag ->head; test !=NULL; test = test->next)
			printf("%s -> ", (char*)test -> data);
		printf("\n");
		
}


void
bag_delete(bag_t *bag){
	
	if (bag != NULL){//if the appropriate bag pointer was provided
	
		// call bag_delete_helper (invisible to the user to delete the structure and the contents)
		bag_delete_helper(bag->head, bag);
	} 
}

//bag delete helper
// it will delete the whole structure and its contents by deleting the first item and it will keep going through
//the end of the list
void
bag_delete_helper(bag_item_t *node, bag_t *bag)
{	
	for (bag_item_t *test=node; test !=NULL; test = bag -> head) {
			
			bag -> head = test -> next; //to initialize the head pointer after the whole item was deleted
			if (bag -> datadelete != NULL)
					(*bag -> datadelete)(test -> data);
			free (test);// delete each item
	}
	free (bag);// delete the bag
}

//end
