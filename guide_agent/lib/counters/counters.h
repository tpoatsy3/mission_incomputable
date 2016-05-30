/*File: counters.h - a header file for data structure module for a list of (counters)
*
*Lab 3 Question 3 counters.h 
*
* A module that export three functions: creating a new counters list, adding the counters if matching keys
* getting the values of the counters for matching keys and deleting the whole structure.	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#ifndef __COUNTERS_H
#define __COUNTERS_H

//global types
typedef struct counters counters_t;//opaque to the users of the module

/**** functions *****/

//create a new empty counters data structure
counters_t *counters_new(void (*iterator)(int key, int data, void* arg));

//incremente the counter if key exists
//if not, it will set the counter to zero
void counters_add(counters_t *ctrs, int key);

//it will return the current value of the counter for matching key
// or zero if the key does not exist
int counters_get(counters_t *counters, int key);

//it will delete the whole structure with its contents
void counters_delete(counters_t *counters);

//print the contents of the list
void printing_counters_contents(counters_t *counters);

//iterates through counter nodes of counter struct pointer,
//performs action based on function passed
void counters_iterate(counters_t *counters, void (*itemfunc)(int key, int data, void* farg), void* arg);

void counters_set(counters_t *ctrs, int key, int counter);
#endif // __COUNTERS_H

//end