/*File: bag.h - a header file for a data structure module for a collection of indistinguishable items
*
*Lab 3 Question 1 bag.h 
*
* A module that export three functions: creating a new bag, inserting items into the bag
* and removing items from the bag and returning it to the user		 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#ifndef __BAG_H
#define __BAG_H

//global types
typedef struct bag bag_t; //opaque to the users of the module

/**** functions *****/

//a function to create a new empty bag data structure
//and return NULL if the there is an error allocating memory for it
bag_t *bag_new(void (*datadelete)(void *data));

// it will insert the item with the attached data to the beginning of the bag
// NULL bag is ignored
// and return NULL if it failed to do so
void bag_insert(bag_t *bag, void *data);

// removing the first item in the bag and returns its data to the user
// it returns NULL if bag is empty or the wrong bag pointer was provided
void *bag_extract (bag_t *bag);

void printing_bag_contents(bag_t *bag);

// to delete the whole bag
void bag_delete(bag_t *bag);

#endif // __BAG_H

//end