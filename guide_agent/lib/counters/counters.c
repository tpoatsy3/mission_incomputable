/*File: counters.c - a data structure module for a list of (counters
*
*Lab 3 Question 3 counters.c 
*
* A module that export three functions: creating a new counters list, adding the counters if matching keys
* getting the values of the counters for matching keys and deleting the whole structure.	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "counters.h"

/*********local types******/

// A structure for each node in the counters list
typedef struct counters_node {
	int counter; // counters to be incremented
	int key; // key (node tag)
	struct counters_node *next; //pointer to the next node
} counters_node_t;

/*********global types******/
typedef struct counters {
	struct counters_node *head; //pointer to the counters list or the dummy head
	void (*iterator)(int key, int data, void* arg);
} counters_t;

/*********local functions******/

//not visible outside this file
static counters_node_t *counters_add_helper(counters_node_t *node,int key);
int counters_get_helper(counters_node_t *node,int key);
void counters_delete_helper(counters_node_t *node, counters_t *counters);
static counters_node_t *counters_set_helper(counters_node_t *node,int key, int counter);

// creating a new empty counters list data structure
counters_t *
counters_new(void (*iterator)(int key, int data, void* arg))
{
	counters_t *counters = malloc(sizeof(counters_t)); // allocating memory for the pointer to the counters list
	
	if (counters == NULL) return NULL; //error allocating list
	else {
		//initialize contents of counters list structure
		counters -> head = NULL;
		counters -> iterator = iterator;
		return counters;//return the pointer to the counters list created
	}
} 

//incrementing the keys or initialize them at 1
//it will ignore the request if the pointer to counters is not appropriate
void
counters_add(counters_t *ctrs, int key)
{	
	if (ctrs != NULL){ 
		
		// call counters_add_helper (invisible to the user to initialize or increment the counters)
		ctrs->head = counters_add_helper(ctrs->head, key);
	}
}

//counters add helper
// if key exists, it will increment the counter.
// if it does not, it will create a node between the header and the first node and initialize the counter to 1
// it returns NULL if error happens in allocating the memory
static counters_node_t *
counters_add_helper(counters_node_t *node,int key)
{	
		//scanning the counters list to look for a matching key
		for (counters_node_t *test=node; test !=NULL; test = test->next) {
		
			if (key == test -> key) {// if it matches, increment the counter of that node
				test -> counter = test -> counter + 1;
				return node;
			}
		}
		
		// if the key was not detected, allocating memory for each node in the counters list
		counters_node_t *new_node = malloc(sizeof(counters_node_t));
		
		if (new_node != NULL) {//no error
			
			new_node -> counter = 1; //initialize the counter of that node to 1
			new_node -> key = key; //tag the node with the corresponding key
			new_node -> next = node; //the new node will point to the previous one in the list
			return new_node;
		
		} else return NULL; //error allocating memory for the new node
}

// returning the current value of the counter for a matching key
// it returns 0 otherwise
int
counters_get(counters_t *counters, int key){
	
	if (counters != NULL){  //if the appropriate counters list pointer was provided
	
		// call counters_get_helper (invisible to the user to find the node with the matching key
		// and return the value of the counter to the user)
		return counters_get_helper(counters->head, key);
	
	} else return 0; 
}


// finding the node with the matching key and return the value of the counters attched
//(invisible to the user)
int
counters_get_helper(counters_node_t *node,int key)
{	
	for (counters_node_t *test=node; test !=NULL; test = test->next) {
		
		//scanning the list to look for a matching key
		if (key == test -> key) {
			return test -> counter; // key exists; return the value of the counter
		}
	}

	return 0;// return zero if there was a not matching key

}

// deleting the whole counters list structure with its contents
//ignores the request if a NULL counters pointer was provided
void
counters_delete(counters_t *counters){
	
	if (counters != NULL){//if the appropriate counters list pointer was provided
	
		// call counters_delete_helper (invisible to the user to delete the structure and the contents
		counters_delete_helper(counters->head, counters);
	} 
}

//counters delete helper
// it will delete the whole structure and its contents by deleting the first item and it will keep going through
//the end of the list
void
counters_delete_helper(counters_node_t *node, counters_t *counters)
{	
	for (counters_node_t *test=node; test !=NULL; test = counters -> head) {
			counters -> head = test -> next; //to initialize the head pointer after the whole list was deleted
			free (test);// delete each node
	}
	free (counters);// delete the counters
}

//printing the counters list
void printing_counters_contents(counters_t *counters){
		/** testing insert, everytime the list_add is called, it will print out
		the contents of the list whether an item was inserted or failed to be inserted **/
		for (counters_node_t *test=counters ->head; test !=NULL; test = test->next)
			printf("%d -> ", test -> counter);
		printf("\n");	
}

/*
 * counters_iterate:
 * takes a counters pointer and a pointer to a function (itemfunc)
 * iterates through the counter nodes
 * runs the code of itemfunc on node data
 */
void counters_iterate(counters_t *counters, void (*itemfunc)(int key, int data, void* farg), void* arg) {  
  if (counters == NULL || itemfunc == NULL) {
    return; // bad bag or null function
  } else {
    // scan the bag

	    for (counters_node_t *curr_node = counters -> head; curr_node !=NULL; curr_node = curr_node->next){
	      
				(*itemfunc)(curr_node->key, curr_node -> counter, arg); 

	    }
	}
  return;
}

//set the counters if the key exists, if not, it will create a counter and set its key to the user's value
//it will ignore the request if the pointer to counters is not appropriate
void
counters_set(counters_t *ctrs, int key, int counter)
{	
	if (ctrs != NULL && counter > 0){ 
		
		// call counters_set_helper (invisible to the user)
		ctrs->head = counters_set_helper(ctrs->head, key, counter);
	}
}

//counters set helper
// if key exists, it will set the counter to the user's value.
// if it does not, it will create a node between the header and the first node and set the counter to the user's value
// it returns NULL if error happens in allocating the memory
static counters_node_t *
counters_set_helper(counters_node_t *node,int key, int counter)
{	
		//scanning the counters list to look for a matching key
		for (counters_node_t *test=node; test !=NULL; test = test->next) {
		
			if (key == test -> key) {// if it matches, set the counter of that node
				test -> counter = counter;
				return node;
			}
		}
		
		// if the key was not detected, allocating memory for each node in the counters list
		counters_node_t *new_node = malloc(sizeof(counters_node_t));
		
		if (new_node != NULL) {//no error
			
			new_node -> counter = counter; //set the counter of that node to the user's value
			new_node -> key = key; //tag the node with the corresponding key
			new_node -> next = node; //the new node will point to the previous one in the list
			return new_node;
		
		} else return NULL; //error allocating memory for the new node
}



//end