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
#include "hashtable/hashtable.h"
#include "file.h"
#include <strings.h>	      
#include <arpa/inet.h>	      
#include <sys/select.h>	  
#include <sys/socket.h>
#include <netinet/in.h>  


/************* Function Declarations ****************/
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
	int deadDropRemaining;
} game_t;

typedef struct code_drop {
	float lng;
	float lat;
	int status; // 0 means not neutralized
	char *team;
} code_drop_t;

typedef struct sendingInfo {
	int comm_sock;
	char *message;
} sendingInfo_t;

// typedef struct location;

typedef struct receiverAddr { //going to be part of the player struct
	in_port_t port;
	struct in_addr inaddr;
	int sin_family;
} receiverAddr_t;

typedef struct playSel {
	hashtable_t *hash;
	void *param;
} playSel_t;

// typedef struct theSuperHashtableReferenceStructure{}

/*************** Kinda Global? **********************/
static const int BUFSIZE = 1024;  

/************* Function Declarations ****************/

bool verifyFlags(int argc, char *argv[], game_t *game_p);
bool verifyPort(int argc, char *argv[], game_t *game_p);
bool verifyFile(int argc, char *argv[]);
bool isItValidInt(char *depth);
hashtable_t* load_codeDrop(char* codeDropPath);
bool readFile(char* codeDropPath, hashtable_t* codeDropHash);
bool game_server (char *argv[], game_t *game_p);
void delete(void *data);
void hashTable_print(void *key, void* data, void* farg);
void numberOfcodeDrops(void* key, void* data, void* farg);
bool isItValidFloat(char *floatNumber);
bool load_codeDropPath(code_drop_t * code_drop, char* line, char* token, hashtable_t* codeDropHash);
static int socket_setup(int GSPort);
static void handle_socket(int comm_sock, hashtable_t *hash);
static void sending(int comm_sock, hashtable_t *hash, char *message);
void sendIterator(void* key, void* data, void* farg);
bool handle_stdin();
void deleteTempHash(void *data);

void FA_LOCATION_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void FA_NEUTRALIZE_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void FA_CAPTURE_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void GA_STATUS_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
void GA_HINT_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
void INVALID_ENTRY_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);

void GAME_OVER_handler(int comm_sock, hashtable_t *hash);
void GA_HINT_iterator(void* key, void* data, void* farg);
bool processing(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);



int main(int argc, char *argv[]){
	
	game_t *game_p;
	if ((game_p = malloc(sizeof(game_t))) == NULL) exit(5);
	
	game_p->level = 1;
	game_p->timeVal = 0;
	game_p->rawlogFl = 0;
	game_p->GSPort = 0;
	game_p->deadDropRemaining = 0;
	
	if (!verifyFlags(argc, argv, game_p)){
		free(game_p);
		exit(1);
	}
	
	if ((argc-optind) != 2){
		printf("Invalid number of arguments\n");
		printf("Usage: ./gameserver [-log] [-level = 1 or 2] [-time (in Minutes)] codeDropPath GSport\n");
		free(game_p);
		exit(2);
	}

	if (!verifyPort(argc, argv, game_p)){
		free(game_p);
		exit(3);
	}

	if (!verifyFile(argc, argv)){
		free(game_p);
		exit(4);
	}
	
	if (!game_server(argv, game_p)) {
		printf("something went wrong\n");
		free(game_p);
		exit(5);
	}

/***DEBUGGING***/
	printf("Level: %d\n", game_p->level);
	printf("time: %d\n", game_p->timeVal);
	printf("log: %d\n", game_p->rawlogFl);
	printf("GSPort: %d\n", game_p->GSPort);
	printf("deadDropRemaining: %d\n", game_p->deadDropRemaining);
/***DEBUGGING***/
	
	free(game_p);
	exit(0);
}

bool game_server (char *argv[], game_t *game_p) {
	int comm_sock;
	
	hashtable_t* codeDropPath;
	char *arg1 = argv[optind];
	
	//if error with loading the codes from file
	if ((codeDropPath = load_codeDrop(arg1))== NULL) return false;
	
	//hashtable print
	hash_iterate(codeDropPath, hashTable_print, NULL);
	
	//seeing how many codes are left
	hash_iterate(codeDropPath, numberOfcodeDrops, game_p);

	//setting up the socket
	comm_sock = socket_setup(game_p->GSPort);

	hashtable_t *hash = hashtable_new( 1, delete, NULL);

	// Receive datagrams, print them out, read response from term, send it back
	while (true) {        // loop exits on EOF from stdin
    
		// for use with select()
		fd_set rfds;        // set of file descriptors we want to read

		// Watch stdin (fd 0) and the UDP socket to see when either has input.
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);       // stdin
		FD_SET(comm_sock, &rfds); // the UDP socket
		int nfds = comm_sock+1;   // highest-numbered fd in rfds

		int select_response = select(nfds, &rfds, NULL, NULL, NULL);
		// note: 'rfds' updated, and value of 'timeout' is now undefined
    
		if (select_response < 0) {
			// some error occurred
			perror("select()");
			close(comm_sock);
			hashtable_delete(hash);
			exit(9);
    
		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)){
				if (!handle_stdin())
					break;
			} 

			if (FD_ISSET(comm_sock, &rfds)) {
				handle_socket(comm_sock, hash);
			}
			fflush(stdout);
		}
	}
	GAME_OVER_handler(comm_sock, hash);
	close(comm_sock);
	hashtable_delete(hash);
	hashtable_delete(codeDropPath);
	return true;
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

bool isItValidInt(char *intNumber){
	
	int validInt = 0;
	char * isDigit;
	
	if ((isDigit = malloc(strlen(intNumber) +1)) == NULL) return false; //NULL to check
	
	//if intNumber given is an integer & between 0 and max
	if(sscanf(intNumber, "%d%s", &validInt, isDigit) != 1) {
		free(isDigit);
		return false;
	} else {
		free(isDigit);
		return true;
	}
	return true;
}

hashtable_t* load_codeDrop(char* codeDropPath) {
	
	//creating the new index
	hashtable_t* codeDropHash = hashtable_new(1, delete, NULL);
	
	// checking if memory was allocated for the index
	if (codeDropHash == NULL) return NULL;
		
	// calling a function that reads the file and fill the hashtable
	// if an improper index file was provided, it will free all the allocated memory andd return false
	if (!readFile(codeDropPath, codeDropHash)){
		hashtable_delete(codeDropHash);
		return NULL;
	}
	return codeDropHash;
}

bool readFile(char* codeDropPath, hashtable_t* codeDropHash){

	char * line; // each line in the code drop path file
	FILE *readFrom = fopen(codeDropPath,"r"); //opening the code drop path file					   									   
	char *token; // different contents in each line (long, lat, or hex code) 
				 //after truncating the line		 
	
	//going through every line in code drop path file until it reaches EOF
	while( (line = readline(readFrom) ) != NULL){
		
		//creating a struct for each code drop  
		code_drop_t *code_drop = malloc(sizeof(code_drop_t));
		
		if (code_drop == NULL){ 	// checking if memory was allocated for each struct
			free(line); //free everything and return false if it fails
	    	fclose(readFrom);
			return false;
		} 
		
		//initialize the struct
		code_drop -> lng = 0.0;
		code_drop -> lat = 0.0;
		code_drop -> status = 0; 
		code_drop -> team = NULL;
		
		if(!load_codeDropPath(code_drop, line, token, codeDropHash)) {
			free(line); //free everything and return false if it fails
	    	fclose(readFrom);
			free(code_drop);
			return false;	
		} 

		
		if ((token = strtok(NULL, ", ")) != NULL) {
			free(line); //free everything and return false if it fails
	    	fclose(readFrom);
			return false;
		}
		
	   	free(line); //free the line before returning to the loop for a new line
	} 
	fclose(readFrom); 	// close the code drop path file after reading its contents
	return true; // nothing went wrong
}


bool load_codeDropPath(code_drop_t * code_drop, char* line, char* token, hashtable_t* codeDropHash){

	if ((token = strtok(line, ", ")) == NULL) return false;	

	if (!isItValidFloat(token)) return false;
	code_drop -> lng = atof(token);
		
	
	if ((token = strtok(NULL, ", ")) == NULL) return false;

	if (!isItValidFloat(token)) return false;
	code_drop -> lat = atof(token);
	
	
	if ((token = strtok(NULL, ", ")) == NULL) return false;
		
	// if it fails to insert, that suggests duplicate (not acceptable ) or memory allocation error
	if (!hashtable_insert(codeDropHash, token, code_drop)) return false;
	
	return true;
}


void delete(void *data){
	if (data){//if valid
		free(data);
	}
}

void numberOfcodeDrops(void* key, void* data, void* farg) {
	((game_t*)(farg))->deadDropRemaining +=1;
}


bool isItValidFloat(char *floatNumber){
	
	float validFloat = 0;
	char * isDigit;
	if((isDigit = malloc(strlen(floatNumber) +1)) == NULL) return false; //NULL to check
	
	
	//if depth given is an integer & between 0 and max
	if(sscanf(floatNumber, "%f%s", &validFloat, isDigit) != 1) {
		free(isDigit);
		return false;
	} else {
		free(isDigit);
		return true;
	}
	return true;
}

/***DEBUGGING***/
void hashTable_print(void *key, void* data, void* farg){
	if (key && data){ //if valid
		printf("%f, %f", ((code_drop_t*)(data))->lng, ((code_drop_t*)(data))->lat);
		printf(", %s", (char*)(key ));//prints to file, building index file

		printf("\n"); // a new line in the file
	}
}

//This was directly taken from chatserver1.c and slightly modified.
/* All the ugly work of preparing the datagram socket;
 * exit program on any error.
 */
static int socket_setup(int GSPort) {
  // Create socket on which to listen (file descriptor)
  int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (comm_sock < 0) {
    perror("opening datagram socket");
    exit(11);
  }

  // Name socket using wildcards
  struct sockaddr_in server;  // server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(GSPort);
  if (bind(comm_sock, (struct sockaddr *) &server, sizeof(server))) {
    perror("binding socket name");
    exit(12); //exit codes?
  }

  return (comm_sock);
}

bool handle_stdin(){
	char *terminalResponse = readline(stdin);
	if (terminalResponse == NULL || strcmp(terminalResponse, "quit") == 0){
		printf("LOG: Game is over. Sorry about that. If you have a problem please email Mr. David Kotz!\n");
		free(terminalResponse);
		return false;
	}
	free(terminalResponse);
	return true;
}

static void sending(int comm_sock, hashtable_t *hash, char *message){
  	sendingInfo_t *sendingInfo_p;
	sendingInfo_p = malloc(sizeof(sendingInfo_t));
	sendingInfo_p->comm_sock = comm_sock;
	sendingInfo_p->message = message;

	hash_iterate(hash, sendIterator, sendingInfo_p);

	free(sendingInfo_p);
}

void sendIterator(void* key, void* data, void* farg) {
	//We will never change this block of code
	sendingInfo_t *sendingInfo_p = (sendingInfo_t *) farg;
	receiverAddr_t *recieverp = (receiverAddr_t *) data;

	struct sockaddr_in sender;    
	sender.sin_port = recieverp->port;
	sender.sin_addr = recieverp->inaddr;
	sender.sin_family = recieverp->sin_family;

	struct sockaddr_in them = {0,0,{0}};
	struct sockaddr_in *themp = &them;
    *themp = sender;

	if (sendto(sendingInfo_p->comm_sock, sendingInfo_p->message, strlen(sendingInfo_p->message), 
		0, (struct sockaddr *) themp, sizeof(*themp)) < 0){
    printf("it failed to send the datagram\n");
	}
}

static void handle_socket(int comm_sock, hashtable_t *hash){
    // socket has input ready
	struct sockaddr_in sender;     // sender of this message
	struct sockaddr *senderp = (struct sockaddr *) &sender;
	socklen_t senderlen = sizeof(sender);  // must pass address to length
	char buf[BUFSIZE];        // buffer for reading data from socket
	int nbytes = recvfrom(comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
	if (nbytes < 0) {
	    perror("receiving from socket");
	    hashtable_delete(hash);
	    close(comm_sock);
	    exit(13);
	} else {      
	    buf[nbytes] = '\0';     // null terminate string

	    // where was it from?
	    if (sender.sin_family == AF_INET) {

	        /****DEBUGGING***/
	        char name[100];
	        sprintf(name, "%s %d", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
	        /****DEBUGGING***/

	        /***FOR THIS FILE ONLY, SHOULD BE PARSE***/
		    if ((hashtable_find(hash, name)) == NULL){
		        printf("%s\n", name);
		        receiverAddr_t *trial = malloc(sizeof(receiverAddr_t));  
		        trial->port = sender.sin_port;
		        trial->inaddr = sender.sin_addr;
				trial->sin_family = sender.sin_family;
		        hashtable_insert(hash, name, trial);
		    }
		    /***FOR THIS FILE ONLY, SHOULD BE PARSE***/

		    /****LOGGING***/
			printf("[%s@%05d]: %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buf);
			/****LOGGING***/

			processing(comm_sock, hash, buf, name);

	    }
		fflush(stdout);
	}
}

bool processing(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	if(strcmp(buf, "FA_LOCATION") == 0){
		//return to user
		FA_LOCATION_handler(comm_sock, hash, buf, thisWillBeDeleted);
		return true;
	// } else if(strcmp(buf, FA_NEUTRALIZE) == 0){
	// 	//return to user
	// 	FA_NEUTRALIZE_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(buf, FA_CAPTURE) == 0){
	// 	//People based on Location and Status and Team
	// 	FA_CAPTURE_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(buf, GA_STATUS) == 0){
	// 	//Back to the user
	// 	GA_STATUS_handler(comm_sock, hash, buf, thisWillBeDeleted);
	} else if(strcmp(buf, "GA_HINT") == 0){
		//Either one FA or all on team
		GA_HINT_handler(comm_sock, hash, buf, thisWillBeDeleted);
		return true;
	} else {
		INVALID_ENTRY_handler(comm_sock, hash, buf, thisWillBeDeleted);

		return false;
	}
	return true;
}

void INVALID_ENTRY_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	//returns a message to the sender
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	receiverAddr_t *sendingPlayer;

	if ((sendingPlayer = (receiverAddr_t *) hashtable_find(hash, thisWillBeDeleted)) != NULL){
		hashtable_insert(tempHash, thisWillBeDeleted, sendingPlayer); //the key dooesnt matter
		sending(comm_sock, tempHash, "You sent an invalid message");
	}
	hashtable_delete(tempHash);
}


void FA_LOCATION_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	//returns a message to the sender
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	receiverAddr_t *sendingPlayer;

	if ((sendingPlayer = (receiverAddr_t *) hashtable_find(hash, thisWillBeDeleted)) != NULL){
		hashtable_insert(tempHash, thisWillBeDeleted, sendingPlayer); //the key dooesnt matter
		sending(comm_sock, tempHash, "FA_LOCATION received");
	}
	hashtable_delete(tempHash);
}

// void FA_NEUTRALIZE_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
// void FA_CAPTURE_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
// void GA_STATUS_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
void GA_HINT_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	char *Ihab = "129.170.214.160";

	playSel_t *playSel_p = malloc(sizeof(playSel_t));
	playSel_p->hash = tempHash;
	playSel_p->param = Ihab;
	// receiverAddr_t *sendingPlayer;
	hash_iterate(hash, GA_HINT_iterator, playSel_p);
	sending(comm_sock, tempHash, "GA_HINT received");

	hashtable_delete(tempHash);
	free(playSel_p);
}

void GA_HINT_iterator(void* key, void* data, void* farg){
	char IP1[20];

	playSel_t *playSel_p = (playSel_t *) farg;
	char *key1 = (char *)key;

	hashtable_t *tempHash = playSel_p->hash;
	char *param1 = (char *) playSel_p->param;
	

	sscanf(key1, "%s *", IP1);

	if (strcmp(IP1, param1) == 0){
		hashtable_insert(tempHash, key1, data); //the key dooesnt matter
	} 
}

void GAME_OVER_handler(int comm_sock, hashtable_t *hash){
	sending(comm_sock, hash, "GAME_OVER. We win. Haha.");
}

void deleteTempHash(void *data){
	if (data){	//if valid
	}
}