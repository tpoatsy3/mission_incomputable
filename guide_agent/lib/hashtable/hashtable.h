/*File: hashtable.h - a header file for a data structure module for a hashtable of (string, data)pairs
*
*Lab 3 Question 4 hashtable.h 
*
* A module that export three functions: creating a new hashtable, finding nodes in the
hashtable based on the key (tag) and inserting items into the hashtable if they are linked to a
*new key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdbool.h>

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

//global types
typedef struct hashtable hashtable_t; //opaque to the users of the module

/**** functions *****/

//a function to create a new empty hashtable data structure
//and return NULL if the there is an error allocating memory for it
hashtable_t *hashtable_new(const int num_slots, void (*datadelete)(void *data), void (*iterator)(void *key, void* data, void* farg));

// it will insert the node with the attached data to the beginning of the list in the corresponding array
// after calling the hash function, if key is not matching
// NULL list pointers will return false
// and return false if it failed to create the node or if there was a matching key
bool hashtable_insert(hashtable_t *ht, char *key, void *data);

// it will return data for the node with the matching key after calling the hash function to find
// the array of interst
// and NULL if it was not found
void *hashtable_find(hashtable_t *ht, char *key);

//for printing the contents of the hashtable
void hash_print(hashtable_t *hashtable);

//iterates through hashtable and performs action based on function given
void hash_iterate(hashtable_t *hashtable, void (*itemfunc)(void *key, void*data, void* farg), void* arg);

//iterates through hashtable and delete keys
void hashtable_delete(hashtable_t *hashtable);



#endif // __HASHTABLE_H