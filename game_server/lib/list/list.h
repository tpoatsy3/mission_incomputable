/*File: list.h - a header file for a data structure module for a list of (string, data)pairs
*
*Lab 3 Question 2 list.h
*
* A module that export three functions: creating a new list, finding nodes in the
list based on the key (tag) and inserting items into the list if they are linked to a
*new key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdbool.h>

#ifndef __LIST_H
#define __LIST_H

//global types
typedef struct list list_t;//opaque to the users of the module

/**** functions *****/

//a function to create a new empty list data structure
//and return NULL if the there is an error allocating memory for it
list_t *list_new(void (*datadelete)(void *data));

// it will insert the node with the attached data to the beginning of the list
// if key is not matching 
// NULL list pointers will return false
// and return false if it failed to create the node or if there was a matching key
bool list_insert(list_t *list, char *key, void *data);

// it will return data for the node with the matching key
// and NULL if it was not found
void *list_find(list_t *list, char *data);

//printing the content of the list
void printing_list_contents(list_t *list);

// deleting the list
void list_delete(list_t *list);

#endif // __LIST_H

//end