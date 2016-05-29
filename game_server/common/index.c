

#include <stdio.h>
#include <stdlib.h>   
#include <string.h>
#include <stdbool.h>

#include "index.h"

/*
 * is_readable_directory:
 * Takes a directory name
 * Checks if a given directory is accessable and readable
*/

 bool is_readable_directory(char *fileDire){
  
  FILE *fp;
  //contructing a string "directory" that is the directory name followed by "/" and a file name
  char *directory = malloc (strlen(fileDire)+strlen("/.crawler")+1);
  
  if (directory == NULL){ //if it fails to allocate memory
    return false;
  }
  //tests directory by reading crawler file
  strcpy(directory, fileDire);
  strcat(directory, "/.crawler");
  
  fp = fopen(directory, "r"); //opening the file
  
  if (fp != NULL) { // if the file exists and can be read, the directory is a readable
    fclose(fp);
    free (directory); 
      return true;
    
  } else { // the command is not a readable directory
    printf("failed to read file in the given page directory\n");
    free(directory);
    return false;
    }
}


/*
 *is_readable_file:
 *Takes a file name
 *Checks if readable by opening (if exists)
*/
bool is_readable_file(char *filename){
  
  FILE* fp= fopen(filename, "r"); //opening the file or creating it
  
  if (fp!= NULL) { //if it is readeable, return true
    fclose(fp);
      return true;
    
  } else { // the argument is not a readable file
    printf("failed to read the index file\n");
    //fclose(fp);
    return false;
    }
}


