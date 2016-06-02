/* 
 * file utilities
 * 
 * David Kotz, April 2016
 */

#ifndef __INDEX_H
#define __INDEX_H

#include <stdio.h>
#include <stdlib.h>   
#include <string.h>
#include <stdbool.h>



/*
 * is_readable_directory:
 * Takes a directory name
 * Checks if a given directory is accessable and readable
*/

 bool is_readable_directory(char *fileDire);

/*
 *is_readable_file:
 *Takes a file name
 *Checks if readable by opening (if exists)
*/
bool is_readable_file(char *filename);




#endif // __INDEX_H
