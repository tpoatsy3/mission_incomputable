/*File: hashtable.c - a data structure module for a hashtable of (string, data)pairs
*
*Lab 3 Question 4 hashtable.c 
*
* A module that export three functions: creating a new hashtable, finding nodes in the
hashtable based on the key (tag) and inserting items into the hashtable if they are linked to a
*new key	 
*
*Ihab Basri, CS50, Dartmouth College, April 24th, 2016*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hashtable.h"

/*********local types******/

// A structure for each node in the list
typedef struct list_node {
	void *data; //pointer to data for this node
	char *key; //pointer to the key string (tag)
	struct list_node *next; //pointer to the next node
} list_node_t;


// a structure for the  list
typedef struct list {
	struct list_node *head; //pointer to the list or the dummy head of the linked list
} list_t;

/*********global types******/

// a structure for the hashtable
typedef struct hashtable {
	int size; //the number of slots in the table
	struct list **table; // a double pointer to the hashtable
	void (*datadelete)(void *data); //pointer to the delete function
	void (*iterator)(void *key, void* data, void* arg);
}hashtable_t;


/*********local functions******/

//not visible outside this file
list_t *list_new(void);
static list_node_t *list_insert_helper(list_node_t *node,char *key , void *data);
bool list_insert(list_t *list, char *key, void *data);
unsigned long JenkinsHash(const char *str, unsigned long mod);
void *list_find(list_t *list, char *key);

static void *list_find_helper(list_node_t *node,char *key);
static void hashtable_delete_helper(hashtable_t *hashtable);
static void hash_print_helper(hashtable_t *hashtable);
static void hash_iterate_helper(hashtable_t *hashtable, void (*itemfunc)(void *key, void*data, void* farg), void* arg);


/*********global variable******/
static int same_key = 0;

// creating a new empty list data structure
hashtable_t *
hashtable_new(const int num_slots, void (*datadelete)(void *data), void (*iterator)(void *key, void*data, void* arg)){
	
	hashtable_t *hashtable=NULL;
	if (num_slots < 1) return NULL; // return error if zero slots were provided
	
	else{
		
		// allocating memory for the pointer to the hashtable, return NULL if error happens
		if ((hashtable = malloc( sizeof( hashtable_t ))) == NULL) return NULL; 

	}
	
	// allocating memory for each slot in the hashtable, return NULL if error happens
	if( ( hashtable->table = malloc( sizeof( list_t * ) * num_slots ) ) == NULL ) {
		return NULL;
	}
	
	
	// create the list array for each slot
	for (int i=0; i<num_slots; i++){
		hashtable -> table[i] = list_new();
	}
	
	hashtable -> size = num_slots; //assign the size of the hashtable
	hashtable -> datadelete = datadelete;
	hashtable -> iterator = iterator;
	
	return hashtable;//return the pointer to the hashtable created
}


// creating a new empty list data structure
list_t *
list_new(void)
{
	list_t *list = malloc(sizeof(list_t)); // allocating memory for the pointer to the list
	
	if (list == NULL) return NULL; //error allocating list
	else {
		
		//initialize contents of list structure
		list -> head = NULL;
		return list;//return the pointer to the list created
	}
} 

// adding a new item to the hashtable
// returns false if the key already exists, and true otherwise
//returns false if the pointer to the hashtable is NULL or encountered any error 
bool
hashtable_insert(hashtable_t *ht, char *key, void *data){
	
	if (ht != NULL && key != NULL){
		
		unsigned long index = JenkinsHash(key, ht -> size); // call the hash function to acquire the index
		
		//call the list_insert function to insert the node if the key does not exist
		//otherwise return false
		if (!list_insert(ht -> table[(int) index], key, data)) return false;
		
		return true;
		
	} else return false;
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
			new_node -> key = malloc(strlen(key)+1);
			
			// if it does not fail, copy the key into the new node, otherwise return NULL
			if (new_node -> key != NULL) strcpy(new_node -> key, key);
			else return NULL;
			
			new_node -> next = node; //the new node will point to the previous one in the list
			return new_node;
		
		} else { //error allocating memory for the new node
			return NULL;
		}
}


//the hash function
//copied from CS50 website

unsigned long
JenkinsHash(const char *str, unsigned long mod)
{
    if (str == NULL)
      return 0;

    size_t len = strlen(str);
    unsigned long hash, i;

    for (hash = i = 0; i < len; ++i)
    {
        hash += str[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash % mod;
}

// return data for the given key, or NULL of not found
void *hashtable_find(hashtable_t *ht, char *key){
	
	if (ht != NULL && key != NULL){
		
		unsigned long index = JenkinsHash(key, ht -> size);// call the hash function to get the index
		
		return list_find(ht -> table[(int) index], key); //find the node in the corresponding array
		
	} else return false; // return false if it encounters error
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

//printing the contents of the hashatable

void hash_print(hashtable_t *hashtable){

	if (hashtable != NULL){//if the appropriate hashtable pointer was provided
	
		// call hashtable_delete_helper (invisible to the user to delete the structure and the contents)
		hash_print_helper(hashtable);
	} 
}

static void hash_print_helper(hashtable_t *hashtable){

	for (int i = 0;i < hashtable-> size; i++){

		for (list_node_t *curr_node=hashtable-> table[i] ->head; curr_node !=NULL; curr_node = curr_node->next)

			printf("%s -> ", (char*)curr_node -> key);

		printf("\n");
	}	

}
/*
 * hash_iterate:
 * takes a hashtable and a pointer to a function (itemfunc)
 * iterates through the hashtable keys
 * runs the code of itemfunc on hashtable contents
 */

void hash_iterate(hashtable_t *hashtable, void (*itemfunc)(void *key, void*data, void* farg), void* arg) {  
	 if (hashtable == NULL || itemfunc == NULL) {
	    return; // table null or null function
	}
	hash_iterate_helper(hashtable, itemfunc, arg);

}


static void hash_iterate_helper(hashtable_t *hashtable, void (*itemfunc)(void *key, void*data, void* farg), void* arg) {  

    for (int i = 0;i < hashtable-> size; i++){
	    for (list_node_t *curr_node= hashtable-> table[i]-> head; curr_node !=NULL; curr_node = curr_node->next){
	      
	      (*itemfunc)(curr_node->key, curr_node -> data, arg); 

	    }
	}
}

/*
 * hash_iterate:
 * iterates through the hashtable keys
 * deletes keys
 *
 */


void
hashtable_delete(hashtable_t *hashtable){
	
	if (hashtable != NULL){//if the appropriate hashtable pointer was provided
		// call hashtable_delete_helper (invisible to the user to delete the structure and the contents)
		hashtable_delete_helper(hashtable);
	} 
}

static void hashtable_delete_helper(hashtable_t *hashtable){

	for (int i = 0;i < hashtable -> size; i++){
		
		for (list_node_t *test=hashtable-> table[i] ->head; test !=NULL; test = hashtable-> table[i] ->head){
			
			hashtable-> table[i] ->head = test -> next; //to initialize the head pointer after the whole node was deleted
			
			if (hashtable -> datadelete != NULL)
					(*hashtable -> datadelete)(test -> data); //deleting the data
			
			free (test -> key); //deleting the key
			free (test);// delete each node
			
		}
		
		free (hashtable-> table[i]);	//deleting the slot
	}
	free (hashtable-> table); // deleting the pointer to the slot
	free (hashtable); // delete the hashtable
}
 