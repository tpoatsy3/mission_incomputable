/*
Mission Incomputable!
Team Topaz

server.c - The file for the game server

May, 2016
Ihab Basri, Ted Poatsy
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

//Data Structures
const struct option longOpts[] = {
	{ "level", required_argument, 0, 'l'},
	{ "time", required_argument, 0, 't'},
	{ "rawlog", no_argument, 0, 'r'},
	{ 0, 0, 0, 0}
};

typedef struct game {
	int level;
	int timeVal;
	int rawlogFl;
	int GSPort;
} game_t;


bool verifyFlags(int argc, char *argv[], game_t *game_p);
bool verifyPort(int argc, char *argv[], game_t *game_p);
bool verifyFile(int argc, char *argv[]);
bool isItValidInt(char *depth);


int main(int argc, char *argv[]){
	game_t *game_p = malloc(sizeof(game_t));
	game_p->level = 1;
	game_p->timeVal = 0;
	game_p->rawlogFl = 0;
	game_p->GSPort = 0;

	if (!verifyFlags(argc, argv, game_p)){
		free(game_p);
		exit(1);
	}

	if (!verifyPort(argc, argv, game_p)){
		free(game_p);
		exit(3);
	}

	if (!verifyFile(argc, argv)){
		free(game_p);
		exit(4);
	}

	if ((argc-optind) != 2){
		printf("Invalid number of arguments\n");
		printf("Usage: ./gameserver [-log] [-level = 1 or 2] [-time (in Minutes)] codeDropPath GSport\n");
		free(game_p);
		exit(2);
	}


	printf("Level: %d\n", game_p->level);
	printf("time: %d\n", game_p->timeVal);
	printf("log: %d\n", game_p->rawlogFl);
	printf("GSPort: %d\n", game_p->GSPort);

	free(game_p);
	exit(0);
}


bool verifyFlags(int argc, char *argv[], game_t *game_p){
	int option = 0;	
	int tempInt;
	while  ((option = getopt_long (argc, argv, "+l:t:r", longOpts, NULL)) != -1){
		switch (option){
			case 'l' :
				if (!isItValidInt(optarg)){
					printf("Level Flag is invalid\n");
					return false;
				} 
				tempInt = atoi(optarg);
				if (tempInt > 0 && tempInt < 3){ //might change to 4
					game_p->level = tempInt;
					break;
 				} else {
 					printf("Level Argument is not in range: %d\n", tempInt);
					return false;
 				}
			case 't':
				if (!isItValidInt(optarg)){
					printf("Time Flag is invalid\n");
					return false;
				} 
				tempInt = atoi(optarg);
				if (tempInt >= 0){ 
					game_p->timeVal = tempInt;
					break;
 				} else {
 					printf("Time Argument is not in range\n");
					return false;
				}

			case 'r':
				game_p->rawlogFl = 1;
				break;

			default:
				fprintf(stderr, "Invalid option(s) used\n");
				return false;	
		}
	}
	return true;
}

bool verifyPort(int argc, char *argv[], game_t *game_p){
	int tempInt;
	char *arg2 = argv[optind+1];
	if (!isItValidInt(arg2)){
		printf("GSPort Argument is invalid\n");
		return false;
	}
	tempInt = atoi(arg2);
	if (tempInt < 0 || tempInt > 1000000){
		printf("GSPort Argument is out of range\n");
		return false;
	}
	game_p->GSPort = tempInt;
	return true;
}

bool verifyFile(int argc, char *argv[]){
	int status;
	char *arg1 = argv[optind];
 	struct stat buf;
	status = stat(arg1, &buf);
	if (status != 0) {
	    fprintf(stderr, "There is an error with %s's status, error number = %d\n", arg1, errno);
	    return false;
	} else {
		if (!S_ISREG(buf.st_mode)) {
        	fprintf(stderr, "%s is a not regular file.\n", arg1);
	    	return false;
    	}
	 	if (access(arg1, R_OK) == -1) {
	    	fprintf(stderr, "%s is not readable.\n", arg1);
	    	return false;
	    }
	}
	return true;
}

bool isItValidInt(char *depth){
	int validInt = 0;
	char * isDigit= malloc(strlen(depth) +1); //NULL to check
	//if depth given is an integer & between 0 and max
	if(sscanf(depth, "%d%s", &validInt, isDigit) != 1) {
		free(isDigit);
		return false;
	} else {
		free(isDigit);
		return true;
	}
	return true;
}