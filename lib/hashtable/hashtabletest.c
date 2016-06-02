//Ihab Basri

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef struct hashes {
	hashtable_t *hashA; //pointer to data for this node
	hashtable_t *hashB; //pointer to the key string (tag)
} hashes_t;


static void delete(void *data);
void hash_union_helper(void *key, void *data, void *arg);
void hash_union(hashes_t* h, hashtable_t* hash1);

void hash_intersection(hashes_t* h, hashtable_t* hash1);
void hash_intersection_helper(void *key, void * data, void *changedList);



int main() 
{

//docid key

	hashes_t * h = malloc(sizeof(hashes_t));
	h -> hashA = hashtable_new(1, NULL, NULL);
	h -> hashB = hashtable_new(1, NULL, NULL);


	hashtable_t *hash1=NULL;
	hashtable_t *hash2=NULL;
	hashtable_t *hash3=NULL;


 	hash1= hashtable_new(1, delete, NULL);
 	hash2 = hashtable_new(1, delete, NULL);
 	hash3 = hashtable_new(1, delete, NULL);

 	hashtable_insert(hash1, "a", NULL);
 	hashtable_insert(hash1, "b", NULL);
 	hashtable_insert(hash1, "c", NULL);
 	hashtable_insert(hash1, "x", NULL);

 	hashtable_insert(hash2, "c", NULL);
 	hashtable_insert(hash2, "x", NULL);
 	hashtable_insert(hash2, "y", NULL);
 	hashtable_insert(hash2, "z", NULL);

 	hashtable_insert(hash3, "g", NULL);
 	hashtable_insert(hash3, "m", NULL);
 	hashtable_insert(hash3, "y", NULL);
 	hashtable_insert(hash3, "z", NULL);

 	hash_union(h, hash1);

 	hash_print(h->hashA);



 	hash_intersection(h, hash2);
 	hashtable_delete(h -> hashA);
	h -> hashA = hashtable_new(1, NULL, NULL);
	hash_union (h, h -> hashB); 
	hashtable_delete(h -> hashB);	
	h -> hashB = hashtable_new(1, NULL, NULL);


 	hash_print(h->hashA);

 	hash_intersection(h, hash3);
 	hashtable_delete(h -> hashA);
	h -> hashA = hashtable_new(1, NULL, NULL);


	hash_union (h, h -> hashB); 	


 	hash_print(h->hashA);

   hashtable_delete(hash1);
   hashtable_delete(hash2);
   hashtable_delete(hash3);
   hashtable_delete(h -> hashA);
   hashtable_delete(h -> hashB);
   free (h);
}

void hash_union(hashes_t* h, hashtable_t* hash1)
{
  hash_iterate(hash1, hash_union_helper, h);
}

/* Consider one item for insertion into the other list.                         
 */
void hash_union_helper(void *key, void * data, void *changedList)
{
  hashes_t* h= changedList;

  if (hashtable_insert(h->hashA , (char*)key, data));

}


void hash_intersection(hashes_t* h, hashtable_t* hash1)
{
  hash_iterate(hash1, hash_intersection_helper, h);
}

/* Consider one item for insertion into the other list.                         
 */
void hash_intersection_helper(void *key, void * data, void *changedList)
{
  hashes_t* h= changedList;
  printf("makes it\n");
  if (!hashtable_insert(h->hashA , (char*)key, data)){
  		printf("FAILED KEY %s\n", (char*)key);

  		hashtable_insert(h->hashB , (char*)key, data);

  }


}

// delete function
static void delete(void *data){
	if (data){
		free(data);
	}
}

//end