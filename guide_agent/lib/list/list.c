/*File: list.c - a data structure module for a list of (string, data)pairs
*
*Lab 3 Question 2 list.c 
*
* A module that export three functions: creating a new list, finding nodes in the
list based on the key (tag) and inserting items into the list if they are linked to a
*new key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"

/*********local types******/

// A structure for each node in the list
typedef struct list_node {
	void *data; //pointer to data for this node
	char *key; //pointer to the key string (tag)
	struct list_node *next; //pointer to the next node
} list_node_t;

/*********global types******/

// a structure for the  list
typedef struct list {
	struct list_node *head; //pointer to the list or the dummy head of the linked list
	void (*datadelete)(void *data); // pointer to a function that deletes the data
} list_t;

/*********local functions******/

//not visible outside this file
static list_node_t *list_insert_helper(list_node_t *node,char *key , void *data);
static void *list_find_helper(list_node_t *node,char *key);
void list_delete_helper(list_node_t *node, list_t *list);

/*********global variable******/
static int same_key = 0;

// creating a new empty list data structure
list_t *
list_new(void (*datadelete)(void *data))
{
	list_t *list = malloc(sizeof(list_t)); // allocating memory for the pointer to the list
	
	if (list == NULL) return NULL; //error allocating list
	else {
		
		//initialize contents of list structure
		list -> head = NULL;
		list -> datadelete = datadelete;
		return list;//return the pointer to the list created
	}
} 


// adding a new item to the hashtable
// returns false if the key already exists, and true otherwise
//returns false if the pointer to the list is NULL or encountered any error
bool
list_insert(list_t *list, char *key, void *data)
{	

	if (list != NULL && key != NULL){
		
		// call list_insert_helper (invisible to the user to add a node and return the address)
		list->head = list_insert_helper(list->head, key, data);
		
		// if nothing was added or the key existed before, it will return false
		if (list -> head  == NULL || same_key == 1) return false;
		
		return true;
	
	} else return false;
}

//list insert helper
// it will insert the node with the attached data to the beginning of the list
// if the key did not exist
// and return NULL if it failed to do so
//it will also set the flag of whether key was detected or not
static list_node_t *
list_insert_helper(list_node_t *node,char *key , void *data)
{		
		same_key = 0; //resetting the key detector flag
		
		//scanning the list to look for a matching key
		for (list_node_t *test=node; test !=NULL; test = test->next) {
		
			if (strcmp(key, test->key) == 0) { // if it exists, set the flag and escape the function
				same_key = 1;
				return node;
			}
		}
		
		// if the key was not detected, allocating memory for each node in the list
		list_node_t *new_node = malloc(sizeof(list_node_t));
		
		if (new_node != NULL) { //no error
			new_node -> data = data; // data will be stored in the new node
			
			//allocating memory to the key string
			new_node -> key = malloc(sizeof(strlen(key)+1));
			
			// if it does not fail, copy the key into the new node, otherwise return NULL
			if (new_node -> key != NULL) strcpy(new_node -> key, key);
			else return NULL;
			
			new_node -> next = node; //the new node will point to the previous one in the list
			return new_node;
		
		} else { //error allocating memory for the new node
			return NULL;
		}
}

// finding the node with the matching key and return the data attched
// it returns NULL if key is not found
void *
list_find(list_t *list, char *key){
	
	if (list != NULL && key != NULL){ //if the appropriate key and list were provided
	
		// call list_find_helper (invisible to the user to find the node with the matching key
		// and return its data to the user)
		return list_find_helper(list->head, key);
	
	} else return NULL; // return NULL otherwise
}

// finding the node with the matching key and return the data attched
//(invisible to the user)
static void *
list_find_helper(list_node_t *node,char *key)
{	

	for (list_node_t *test=node; test !=NULL; test = test->next) {
		
		//scanning the list to look for a matching key
		if (strcmp(key, test->key) == 0) {
			// key exists; return the data
			return test -> data;
		}
	}

	return NULL;

}

void printing_list_contents(list_t *list){
		/** testing insert, everytime the list_insert is called, it will print out
		the contents of the list whether an item was inserted or failed to be inserted **/
		for (list_node_t *test=list ->head; test !=NULL; test = test->next)
			printf("%s -> ", (char*)test -> data);
		printf("\n");
		
}

void
list_delete(list_t *list){
	
	if (list != NULL){//if the appropriate list pointer was provided
	
		// call list_delete_helper (invisible to the user to delete the structure and the contents)
		list_delete_helper(list->head, list);
	} 
}

//list delete helper
// it will delete the whole structure and its contents by deleting the first node and it will keep going through
//the end of the list
void
list_delete_helper(list_node_t *node, list_t *list)
{	
	for (list_node_t *test=node; test !=NULL; test = list -> head) {
			
			list -> head = test -> next; //to initialize the head pointer after the whole node was deleted
			
			if (list -> datadelete != NULL) (*list -> datadelete)(test -> data); //delete the data
			
			free (test -> key); //delete the key
			free (test);// delete each node
	}
	free (list);// delete the list
}

//end