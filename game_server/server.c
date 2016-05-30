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
#include <time.h>
#include <math.h>

#define R 6371
#define TO_RAD (3.1415926536 / 180)


/************* Function Declarations ****************/
const struct option longOpts[] = {
	{ "level", required_argument, 0, 'l'},
	{ "time", required_argument, 0, 't'},
	{ "rawlog", no_argument, 0, 'r'},
	{ "game", required_argument, 0, 'g'},
	{ 0, 0, 0, 0}
};

typedef struct game {
	long gameID; 
	int level;
	int timeVal;
	int rawlogFl;
	int GSPort;
	int deadDropRemaining;
	int comm_sock;
} game_t;

typedef struct code_drop {
	double lng;
	double lat;
	int status; // 0 means not neutralized, 1 means neutralized
	char *team;
} code_drop_t;

typedef struct sendingInfo {
	int comm_sock;
	char *message;
} sendingInfo_t;

typedef struct utility {
	void *param1;
	void *param2;
	void *param3;
} utility_t;

typedef struct hashStruct{
	hashtable_t *GA;
	hashtable_t *FA;
	hashtable_t *temp;
	hashtable_t *CD;
	game_t *game;
} hashStruct_t;

typedef struct receiverAddr{
	in_port_t port;
	struct in_addr inaddr;
	int sin_family;
}receiverAddr_t;

typedef struct FAPlayer{
	char *PlayerName;
	char *TeamName;
	int status;
	double lat;
	double lng;
	long int lastContact; //unclearType
	receiverAddr_t *addr;
	int capturedPlayers;
	int Neutralized;
} FAPlayer_t;

typedef struct GAPlayer{
	char *PlayerName;
	char *TeamName;
	int status;
	long int lastContact; //unclearType
	receiverAddr_t *addr;
} GAPlayer_t;

/*************** Kinda Global? **********************/
static const int BUFSIZE = 9000;  

/************* Function Declarations ****************/

bool verifyFlags(int argc, char *argv[], game_t *game_p);
bool verifyPort(int argc, char *argv[], game_t *game_p);
bool verifyFile(int argc, char *argv[]);
bool isItValidInt(char *depth);
bool isItValidFloat(char *floatNumber);
bool load_codeDrop(hashtable_t* codeDropHash, char* codeDropPath);
bool load_codeDropPath(code_drop_t * code_drop, char* line, char* token, hashtable_t* codeDropHash);

bool game_server (char *argv[], hashStruct_t *allGameInfo);
int socket_setup(int GSPort);
void handle_socket(hashStruct_t *allGameInfo);
bool handle_stdin();

bool processing(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr);
int parsingMessages(char* line, char ** messageArray);
int copyValidKeywordsToQueryArray( char ** array, char* word, int count);

void FA_LOCATION_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr);
void FA_NEUTRALIZE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr);
void FA_CAPTURE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr);
// void GA_STATUS_handler(int comm_sock, hashtable_t *hash,  char **messageArray, int arraySize);
// void GA_HINT_handler(int comm_sock, hashtable_t *hash,  char **messageArray, int arraySize);
// void INVALID_ENTRY_handler(int comm_sock, hashtable_t *hash, char **messageArray, int arraySize);
void GAME_OVER_handler(hashStruct_t *allGameInfo);

int gameIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr);
int playerIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr);
bool teamNameHandler(hashStruct_t *allGameInfo, char**messageArray, receiverAddr_t * playerAddr);
bool playerNameHandler(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t * playerAddr);
void FAPlayerGameStatusHandler(hashStruct_t *allGameInfo, FAPlayer_t *playerAddr);
bool latHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
bool lngHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
int statusReqHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
bool validateHexCode(char *hexString, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo);
bool codeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo);


void sending(int comm_sock, hashtable_t *tempHash, char *message);

void addPlayer(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t *addr);

// void GA_HINT_iterator(void* key, void* data, void* farg);
void sendIterator(void* key, void* data, void* farg);
void hashTable_print(void *key, void* data, void* farg);
void numberOfcodeDrops(void* key, void* data, void* farg);
void GAMatchingTeam(void *key, void* data, void* farg);
void FAPlayerNamesIterator(void *key, void* data, void* farg);
void hashTable_FA_print(void *key, void* data, void* farg);
void GAguideIDIterator(void *key, void* data, void* farg);
void RemainingOperativesIterator(void *key, void* data, void* farg);
void RemainingFoeIterator(void *key, void* data, void* farg);
void GSResponseHandler(hashStruct_t *allGameInfo, receiverAddr_t *playerAddr, char* Response, char* respCode);
void gameOverIterator(void *key, void* data, void* farg);
void GAcopyingAddressIterator(void *key, void* data, void* farg);
void FAcopyingAddressIterator(void *key, void* data, void* farg);
void numberOfPlayersIterator(void* key, void* data, void* farg);
void numberCapturedIterator(void* key, void* data, void* farg);
void numberCapturingIterator(void* key, void* data, void* farg);
void numberNeutralizedIterator(void* key, void* data, void* farg);
void existingHexCodeIterator(void *key, void* data, void* farg);

double dist(double th1, double ph1, double th2, double ph2);

int makeRandomHex();

void delete(void *data);
void freeArray(char** array, int size);
void deleteHashStruct(hashStruct_t *allGameInfo);
void deleteTempHash(void *data);
void deleteFAPlayer(void *data);
void deleteGAPlayer(void *data);

/***Debugging Functions***/
void printArray(char** array, int size);




int main(int argc, char *argv[]){
	
	srand(time(NULL));
	
	//need if malloc fails messages
	hashStruct_t *allGameInfo = malloc(sizeof(hashStruct_t));
	hashtable_t *GAPlayers = hashtable_new(1, deleteGAPlayer, NULL);
	hashtable_t *FAPlayers = hashtable_new(1, deleteFAPlayer, NULL);
	hashtable_t *codeDrop = hashtable_new(1, delete, NULL);

	game_t *game_p;
	if ((game_p = malloc(sizeof(game_t))) == NULL) exit(5);
	
	allGameInfo->game = game_p;
	allGameInfo->GA = GAPlayers;
	allGameInfo->FA = FAPlayers;
	allGameInfo->CD = codeDrop;

	game_p->level = 1;
	game_p->timeVal = 0;
	game_p->rawlogFl = 0;
	game_p->GSPort = 0;
	game_p->deadDropRemaining = 0;


	//Assign Game ID
	game_p->gameID = makeRandomHex();
	
	
	if (!verifyFlags(argc, argv, game_p)){
		deleteHashStruct(allGameInfo);
		exit(1);
	}

	if ((argc-optind) != 2){
		printf("Invalid number of arguments\n");
		printf("Usage: ./gameserver [-log] [-level = 1 or 2] [-time (in Minutes)] codeDropPath GSport\n");
		deleteHashStruct(allGameInfo);
		exit(2);
	}

	if (!verifyPort(argc, argv, game_p)){
		deleteHashStruct(allGameInfo);
		exit(3);
	}

	if (!verifyFile(argc, argv)){
		deleteHashStruct(allGameInfo);
		exit(4);
	}


	if (!game_server(argv, allGameInfo)) {
		printf("something went wrong\n");
		deleteHashStruct(allGameInfo);
		exit(5);
	}

	printf("%ld\n", game_p->gameID);

	deleteHashStruct(allGameInfo);
	exit(0);
}

bool game_server (char *argv[], hashStruct_t *allGameInfo) {
	int comm_sock;
	
	char *arg1 = argv[optind];
	
	//if error with loading the codes from file
	if (!load_codeDrop(allGameInfo->CD, arg1)) return false;
	
	//hashtable print
	hash_iterate(allGameInfo->CD, hashTable_print, NULL);
	
	//seeing how many codes are left
	hash_iterate(allGameInfo->CD, numberOfcodeDrops, allGameInfo->game);

	//setting up the socket
	if ((comm_sock = socket_setup(allGameInfo->game->GSPort)) == -1) {
		return false;
	} else {
		allGameInfo->game->comm_sock = comm_sock;
	}

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
			return false;
    
		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)){
				if (!handle_stdin())
					break;
			} 

			if (FD_ISSET(comm_sock, &rfds)) {
				handle_socket(allGameInfo);
			}
			fflush(stdout);
		}
	}
	GAME_OVER_handler(allGameInfo);
	close(comm_sock);
	return true;
}

bool verifyFlags(int argc, char *argv[], game_t *game_p){

	int option = 0;	
	int tempInt;
	long tempLong;
	while  ((option = getopt_long (argc, argv, "+l:t:rg:", longOpts, NULL)) != -1){
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

			case 'g':
				//verify that the input is an integer between 1 and 4294967295
				tempLong = atol(optarg);
				if (!isItValidInt(optarg) || (tempLong > 4294967295 || tempLong < 1)){
					printf("Invalid gameID\n");
					return false;
				} else {
					game_p->gameID = tempLong;
				}
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

bool load_codeDrop(hashtable_t* codeDropHash, char* codeDropPath){

	char *line; // each line in the code drop path file
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
			free(code_drop);
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
	if (data != NULL){//if valid
		free(data);
	}
}

void deleteGAPlayer(void *data){
	if (data != NULL){
		free(((GAPlayer_t *) data)->PlayerName);
		free(((GAPlayer_t *) data)->TeamName);
		free(((GAPlayer_t *) data)->addr);
		free(data);
	}
}

void deleteFAPlayer(void *data){
	if (data != NULL){
		free(((FAPlayer_t *) data)->PlayerName);
		free(((FAPlayer_t *) data)->TeamName);
		free(((FAPlayer_t *) data)->addr);
		free(data);
	}
}

void numberOfcodeDrops(void* key, void* data, void* farg) {
	((game_t*)(farg))->deadDropRemaining +=1;
}

bool isItValidFloat(char *floatNumber){
	
	double validFloat = 0;
	char * isDigit;
	if((isDigit = malloc(strlen(floatNumber) +1)) == NULL) return false; //NULL to check
	
	
	//if depth given is an integer & between 0 and max
	if(sscanf(floatNumber, "%lf%s", &validFloat, isDigit) != 1) {
		free(isDigit);
		return false;
	} else {
		free(isDigit);
		return true;
	}
	return true;
}

void hashTable_FA_print(void *key, void* data, void* farg){
	if (key && data){ //if valid
		printf("Key: %s\n", (char *) key);
		printf("Team: %s, Player: %s\n", ((FAPlayer_t*)(data))->TeamName, ((FAPlayer_t*)(data))->PlayerName);
	}
}

/***DEBUGGING***/
void hashTable_print(void *key, void* data, void* farg){
	if (key && data){ //if valid
		printf("%lf, %lf", ((code_drop_t*)(data))->lng, ((code_drop_t*)(data))->lat);
		printf(", %s", (char*)(key));//prints to file, building index file

		printf("\n"); // a new line in the file
	}
}

//This was directly taken from chatserver1.c and slightly modified.
/* All the ugly work of preparing the datagram socket;
 * exit program on any error.
 */
int socket_setup(int GSPort) {
  // Create socket on which to listen (file descriptor)
  int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (comm_sock < 0) {
    perror("opening datagram socket");
    return -1;
  }

  // Name socket using wildcards
  struct sockaddr_in server;  // server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(GSPort);
  if (bind(comm_sock, (struct sockaddr *) &server, sizeof(server))) {
    perror("binding socket name");
    return -1; //exit codes?
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

void sending(int comm_sock, hashtable_t *tempHash, char *message){
  	sendingInfo_t *sendingInfo_p;
	sendingInfo_p = malloc(sizeof(sendingInfo_t));
	sendingInfo_p->comm_sock = comm_sock;
	sendingInfo_p->message = message;

	hash_iterate(tempHash, sendIterator, sendingInfo_p);

	free(sendingInfo_p);
}

void sendIterator(void* key, void* data, void* farg) {
	// We will never change this block of code
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

void handle_socket(hashStruct_t *allGameInfo){
    // socket has input ready
	struct sockaddr_in sender;     // sender of this message
	struct sockaddr *senderp = (struct sockaddr *) &sender;
	socklen_t senderlen = sizeof(sender);  // must pass address to length
	char buf[BUFSIZE];        // buffer for reading data from socket
	int nbytes = recvfrom(allGameInfo->game->comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
	if (nbytes <= 0) {
	    printf("Empty or improper message was recieved\n");
	    return ;
	} else {      
	    buf[nbytes] = '\0';     // null terminate string

	    // where was it from?
	    if (sender.sin_family == AF_INET) {

	        // /****DEBUGGING***/
	        // char name[100];
	        // sprintf(name, "%s %d", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
	        // /****DEBUGGING***/

	        /***FOR THIS FILE ONLY, SHOULD BE PARSE***/
		        receiverAddr_t *trial = malloc(sizeof(receiverAddr_t));  
		        trial->port = sender.sin_port;
		        trial->inaddr = sender.sin_addr;
				trial->sin_family = sender.sin_family;
		    /***FOR THIS FILE ONLY, SHOULD BE PARSE***/

		    /****LOGGING***/
			printf("[%s@%d]: %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buf);
			/****LOGGING***/

			// Create array
			char** messageArray = malloc(8 *((strlen(buf)+1)/2 + 1) ); 

			if (messageArray == NULL){
				free(trial);
				return false;
			} //checking if it failed to allocate memory 
			int count = parsingMessages(buf, messageArray);
			processing(allGameInfo, messageArray, count, trial);
			/***DEBUGGING***/
			//printArray(messageArray, count);
			free(trial);
			freeArray(messageArray, count);
	    }
		fflush(stdout);
	}
}

bool processing(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){

	if(strcmp(messageArray[0], "FA_LOCATION") == 0){
		//return to user
		FA_LOCATION_handler(allGameInfo, messageArray, arraySize, playerAddr); //these params are a model for the rest
		return true;
	} else if(strcmp(messageArray[0], "FA_NEUTRALIZE") == 0){
		//return to user
		FA_NEUTRALIZE_handler(allGameInfo, messageArray, arraySize, playerAddr);
		return true;
	// } else if(strcmp(buf, FA_CAPTURE) == 0){
	// 	//People based on Location and Status and Team
	// 	FA_CAPTURE_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(buf, GA_STATUS) == 0){
	// 	//Back to the user
	// 	GA_STATUS_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(messageArray[0], "GA_HINT") == 0){
	// 	//Either one FA or all on team
	// 	GA_HINT_handler(comm_sock, hash, messageArray, arraySize);
	// 	return true;
	} else {
		GSResponseHandler(allGameInfo, playerAddr, "Please enter a proper OPCODE",  "MI_ERROR_INVALID_OPCODE");
		return false;
	}
	return true;
}

// void INVALID_ENTRY_handler(int comm_sock, hashtable_t *hash, char** messageArray, int arraySize){
// 	//returns a message to the sender
// 	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

// 	receiverAddr_t *sendingPlayer;

// 	if ((sendingPlayer = (receiverAddr_t *) hashtable_find(hash, messageArray[0])) != NULL){
// 		hashtable_insert(tempHash, messageArray[0], sendingPlayer); //the key dooesnt matter
// 		sending(comm_sock, tempHash, "You sent an invalid message");
// 	}
// 	hashtable_delete(tempHash);
// }


void FA_LOCATION_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	//returns a message to the sender
	int gameIDFlag, playerIDFlag, statusFlag;

	if (arraySize != 8){
		GSResponseHandler(allGameInfo, playerAddr, "you need 8 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}

	gameIDFlag = gameIDHandler(allGameInfo, messageArray, playerAddr);

	playerIDFlag = playerIDHandler(allGameInfo, messageArray, playerAddr);


	if (!teamNameHandler(allGameInfo, messageArray, playerAddr)) return;


	if (playerNameHandler(allGameInfo, messageArray,playerAddr)) {
		printf("player true\n");
	} else {
		printf("player false\n");
		return;
	}
	
	if (latHandler(messageArray,playerAddr, allGameInfo)){
		printf("lat true\n");
	} else {
		printf("lat false\n");
		return;
	}
	
	if (lngHandler(messageArray,playerAddr, allGameInfo)) {
		printf("lng true\n");
	} else {
		printf("lng false\n");
		return;
	}

	statusFlag = statusReqHandler(messageArray,playerAddr, allGameInfo);


	if (gameIDFlag == -1){
		return;
	} else if(playerIDFlag == -1){
		return;
	} else if (playerIDFlag == 2){
		return;
	} else if ((gameIDFlag == 1) && (playerIDFlag == 1)){
		addPlayer(allGameInfo, messageArray, playerAddr);
	} else if ((gameIDFlag == 1) && (playerIDFlag != 1)){
		GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
		return;
	} else if ((gameIDFlag != 1) && (playerIDFlag == 1)){
		GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
		return;
	} 

	//if they are here, they exist, in our hashtable --> update their info
	//update the FA's information
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	//1: GameID, does not change
	//2: PebbleID, does not change
	//3: TeamName, not allowing them to change teams.
	//4: PlayerName, can switch it. Hashtable has already been checked for doups.
	free(currentFA->PlayerName);
	currentFA->PlayerName = malloc(strlen(messageArray[4]) +1);
	strcpy(currentFA->PlayerName, messageArray[4]);
	//5 & 6: Lat and Long both should be updated
	currentFA->lat = atof(messageArray[5]);
	currentFA->lng = atof(messageArray[6]);
	//Update Last Contact Time
	currentFA->lastContact = time(NULL);
	//Update Player's Address in case of disconnection.


	currentFA->addr->port = playerAddr->port;
	currentFA->addr->inaddr = playerAddr->inaddr;
	currentFA->addr->sin_family = playerAddr->sin_family;



	if (statusFlag == 1){
		//send a message back.
		FAPlayerGameStatusHandler(allGameInfo, currentFA);


		// char GAME_STATUS_message[80];
		// sprintf(GAME_STATUS_message, "GAME_STATUS|%ld|%s|%d|%d|%d", 
		// 	allGameInfo->game->gameID, 
		// 	FIND THE MATCHING TEAM GUIDE ID, 
		// 	allGameInfo->game->numberOfcodeDrops,
		// 	FIND NUMBER OF FRIENDLIES,
		// 	FIND NUMBER OF ENEMIES)
		
	

	}
}

void FAPlayerGameStatusHandler(hashStruct_t *allGameInfo, FAPlayer_t *currentFA){
	
	char message[BUFSIZE];
	char* guideID = "0";
	int numRemainingFriendlies = 0;
	int numRemainingFoe = 0;
	
	
	utility_t *utility_p = malloc(sizeof(utility_t));

	utility_p->param1 = guideID;
	utility_p->param2 = currentFA -> TeamName;
	
	hash_iterate(allGameInfo->GA, GAguideIDIterator, utility_p);
	
	
	utility_p->param1 = &numRemainingFriendlies;
	
	hash_iterate(allGameInfo->FA, RemainingOperativesIterator, utility_p);
	
	utility_p->param1 = &numRemainingFoe;
	
	hash_iterate(allGameInfo->FA, RemainingFoeIterator, utility_p);
	
	
	sprintf(message, "GAME_STATUS|%ld|%s|%d|%d|%d",allGameInfo->game->gameID, guideID, allGameInfo->game->deadDropRemaining, numRemainingFriendlies, numRemainingFoe);
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, "key", currentFA->addr); //the key doesnt matter
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
	free(utility_p);
}

void GSResponseHandler(hashStruct_t *allGameInfo, receiverAddr_t *playerAddr, char* Response, char* respCode){
	char message[BUFSIZE];	
	sprintf(message, "GS_RESPONSE|%ld|%s|%s",allGameInfo->game->gameID, respCode, Response);
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, "key", playerAddr); //the key doesnt matter
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
}

int gameIDHandler(hashStruct_t *allGameInfo, char** messageArray,receiverAddr_t * playerAddr){

	if (strcmp(messageArray[1], "0") == 0){
		if ((strcmp(messageArray[0], "FA_LOCATION") == 0) || (strcmp(messageArray[0], "GA_STATUS") == 0)){
			return 1; //can be added
		} else {
			GSResponseHandler(allGameInfo, playerAddr, "Send 'FA_LOCATION' or 'GA_STATUS' OPCODE if you want to be added ",  "MI_ERROR_INVALID_GAME_ID");
			return -1;
		}
	} else if (atol(messageArray[1]) == allGameInfo->game->gameID){
		return 0; //valid
	} else {
		GSResponseHandler(allGameInfo, playerAddr, "Send 0 to be registered",  "MI_ERROR_INVALID_GAME_ID");
		return -1;
	}
}

int playerIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr){
	
	if (!isItValidInt (messageArray[2])){
		GSResponseHandler(allGameInfo, playerAddr, "Not a valid hex code",  "MI_ERROR_INVALID_ID");
		return -1;
	}
	int hexValue;
	sscanf(messageArray[2], "%d", &hexValue);

	if (hexValue > 65535 || hexValue < 0){
		GSResponseHandler(allGameInfo, playerAddr, "Not a valid hex code",  "MI_ERROR_INVALID_ID");
		return -1;
		
	} else if (messageArray[0][0] == 'G'){
		GAPlayer_t *foundPlayerGA;
		if ((foundPlayerGA = hashtable_find(allGameInfo->GA, messageArray[2])) != NULL){
			if (foundPlayerGA->status != 1){
				printf("Player is known and not idle\n");
				return 0; //valid
			} else {
				printf("Player is known and idle\n");
				return 2; //since we just need to ignore it
			}
		}
		else {
			printf("Player is not known\n");
			//Pebble ID is within the range and is known.
			return 1;
		}
	} else { //Player is FA
		FAPlayer_t *foundPlayerFA;
		if ((foundPlayerFA = hashtable_find(allGameInfo->FA, messageArray[2])) != NULL){
			if (foundPlayerFA->status != 1){
				printf("Player is known and not captured with a valid PebbleID\n");
				return 0;
			} else {
				printf("Player is known and captured with a valid PebbleID");
				return 2; //since we just need to ignore it
			}
		} else {
			printf("Player is not known with a valid PebbleID\n");
			//Pebble ID is within the range and is known.
			return 1;
		}
	}
}

//should be after we verify ID
bool teamNameHandler(hashStruct_t *allGameInfo, char**messageArray, receiverAddr_t * playerAddr){
	if (messageArray[0][0] == 'G'){
		GAPlayer_t *foundPlayerGA;
		if((foundPlayerGA = hashtable_find(allGameInfo->GA, messageArray[2])) == NULL){
			//player is not known
			//check if that team name already exists
			utility_t *utility_p = malloc(sizeof(utility_t));
			int errorFlag = 0;

			utility_p->param1 = &errorFlag;
			utility_p->param2 = messageArray[3];

			hash_iterate(allGameInfo->GA, GAMatchingTeam, utility_p);

			free(utility_p);
			
			if(errorFlag == 0){
				return true;
			} else {	
				GSResponseHandler(allGameInfo, playerAddr, "The team already exists",  "MI_ERROR_INVALID_TEAMNAME");
				return false;
			}
		} else{
			//player is known, so they need to have a matching team name
			if ((strcmp(foundPlayerGA->TeamName, messageArray[3])) == 0){
				//and team matches
				return true;
			}
			else {
				GSResponseHandler(allGameInfo, playerAddr, "You cannot change teams",  "MI_ERROR_INVALID_TEAMNAME");
				return false;
			}
		}
	} else{
		FAPlayer_t *foundPlayerFA;
		if((foundPlayerFA = hashtable_find(allGameInfo->FA, messageArray[2])) == NULL){
			//player is not known, so they can make whatever team name they want
			return true;
		} else{
			//player is known, so they need to have a matching team name
			if ((strcmp(foundPlayerFA->TeamName, messageArray[3])) == 0){
				//and team matches
				return true;
			}
			else {
				GSResponseHandler(allGameInfo, playerAddr, "You cannot change teams",  "MI_ERROR_INVALID_TEAMNAME");
				return false;
			}
		}
	}
}

//should be after we verify ID
bool playerNameHandler(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t * playerAddr){
	if (messageArray[0][0] == 'F'){
		utility_t *utility_p = malloc(sizeof(utility_t));
		int errorFlag = 0;
		utility_p->param1 = &errorFlag;
		utility_p->param2 = messageArray[4];
		FAPlayer_t *foundPlayerFA = (FAPlayer_t *) hashtable_find(allGameInfo->FA, messageArray[2]);
		if((foundPlayerFA == NULL)){
			//player is not known. 

			//check for a duplicate name.
			hash_iterate(allGameInfo->FA, FAPlayerNamesIterator, utility_p);

			//if there is not a douplicate, then True.
			if(errorFlag == 0){
				free(utility_p);
				return true;
			} else {
				//if there is a dup, then False.
				free(utility_p);
				GSResponseHandler(allGameInfo, playerAddr, "This name is taken",  "MI_ERROR_INVALID_TEAMNAME");
				return false;
			}
		} else {
			//player is known
			//make sure that he still has his same name.
			// hash_iterate(allGameInfo->FA, hashTable_FA_print, NULL);
			if ((strcmp(foundPlayerFA->PlayerName, messageArray[4]))== 0) {
				//player has the same name, already verified no-doups, true
				free(utility_p);
				return true;
			} else {
				//Changing Player Name, must check for douplicates.
				hash_iterate(allGameInfo->FA, FAPlayerNamesIterator, utility_p);

				//if there is not a douplicate, then True.
				if(errorFlag == 0){
					free(utility_p);
					return true;
				} else {
					//if there is a doup, then False.
					free(utility_p);
					GSResponseHandler(allGameInfo, playerAddr, "This name is taken",  "MI_ERROR_INVALID_PLAYERNAME");
					return false;
				}
			}
			free(utility_p);
			return true;
		}
	}
	return true;
}

bool latHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	if (isItValidFloat(messageArray[5])){
		if (atol(messageArray[5]) > 90.0 || atol(messageArray[5]) < -90.0) {
			GSResponseHandler(allGameInfo, playerAddr, " ",  "MI_ERROR_INVALID_LAT");
			return false;
		} else {
			return true;
		}
	}
		return false;
}

bool lngHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	if (isItValidFloat(messageArray[6])){
		if (atol(messageArray[6]) > 180.0 || atol(messageArray[6]) < -180.0) {
			GSResponseHandler(allGameInfo, playerAddr, " ",  "MI_ERROR_INVALID_LONG");
			return false;
		} else {
			return true;
		}
	}
	return false;
}

int statusReqHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	
	if (messageArray[0][0] == 'G' && isItValidInt(messageArray[5])){
		if((atoi(messageArray[5])== 0)){
			return 0;
		} else if ((atoi(messageArray[5])== 1)){
			return 1;
		} else{
			GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
			return -1;
		}
	} else if (isItValidInt(messageArray[7])){
		if((atoi(messageArray[7])== 0)){
			return 0;
		} else if ((atoi(messageArray[7])== 1)){
			return 1;
		} else{
			GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
			return -1;
		}
	}
	return -1;
}

void addPlayer(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t *addr){
	//assuming that everything is verified.

	//only doing the FA side because I dont know what all of fields of the GA player struct are
	if ((strcmp(messageArray[0], "FA_LOCATION")) == 0){
		FAPlayer_t *newFA = malloc(sizeof(FAPlayer_t));
		newFA->PlayerName = malloc((strlen(messageArray[4]) + 1));
		newFA->TeamName = malloc((strlen(messageArray[3]) + 1));
		receiverAddr_t *addrFA = malloc(sizeof(receiverAddr_t));

		addrFA->port = addr->port;
		addrFA->inaddr = addr->inaddr;
		addrFA->sin_family = addr->sin_family;

		strcpy(newFA->PlayerName, messageArray[4]);
		strcpy(newFA->TeamName, messageArray[3]);
		newFA->status = 0; //default is 0 (active)
		newFA->capturedPlayers = 0;
		newFA->Neutralized = 0;
		newFA->lat = atol(messageArray[5]);
		newFA->lng = atol(messageArray[6]);
		newFA->lastContact = time(NULL);
		newFA->addr = addrFA;
		hashtable_insert(allGameInfo->FA, messageArray[2], newFA);
	} else {
		GAPlayer_t *newGA = malloc(sizeof(GAPlayer_t));

		newGA->PlayerName = malloc((strlen(messageArray[4]) + 1));
		newGA->TeamName = malloc((strlen(messageArray[3]) + 1));
		receiverAddr_t *addrGA = malloc(sizeof(receiverAddr_t));

		addrGA->port = addr->port;
		addrGA->inaddr = addr->inaddr;
		addrGA->sin_family = addr->sin_family;

		strcpy(newGA->PlayerName, messageArray[4]);
		strcpy(newGA->TeamName, messageArray[3]);
		newGA->status = 0; //default is 0 (active)
		newGA->lastContact = time(NULL);
		newGA->addr = addrGA;
		hashtable_insert(allGameInfo->GA, messageArray[2], newGA);
	}
}

bool validateHexCode(char *hexString, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo){
	int hexValue;
	sscanf(hexString, "%d", &hexValue);

	if (hexValue > 65535 || hexValue < 0){
		GSResponseHandler(allGameInfo, playerAddr, "Not a valid hex code",  "MI_ERROR_INVALID_ID");
		return false;
	} else {
		return true;
	}
}

bool codeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo){
	//check for a valid Hex Code
	if (!validateHexCode(messageArray[7], playerAddr, allGameInfo)){
		// error message already sent inside of ValidateHexCode
		return false;
	} else {

		int existsFlag = 0;

		utility_t *utility_p = malloc(sizeof(utility_t));


		utility_p->param1 = &existsFlag; //existsFlag
		utility_p->param2 =  messageArray[7]; //hexString
		
		hash_iterate(allGameInfo->FA, existingHexCodeIterator, utility_p);

		if ( existsFlag == 1){
			return true;
		} else {
			return false;
		}
	}
}

void FA_NEUTRALIZE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	int gameIDFlag, playerIDFlag;

	//Validate the number of arguments in the message
	if (arraySize != 8){
		GSResponseHandler(allGameInfo, playerAddr, "you need 8 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}

	//Validate each of the parameters
	gameIDFlag = gameIDHandler(allGameInfo, messageArray, playerAddr);
	playerIDFlag = playerIDHandler(allGameInfo, messageArray, playerAddr);

	if (!teamNameHandler(allGameInfo, messageArray, playerAddr)) return;

	if (playerNameHandler(allGameInfo, messageArray,playerAddr)) {
		printf("player true\n");
	} else {
		printf("player false\n");
		return;
	}
	
	if (latHandler(messageArray,playerAddr, allGameInfo)){
		printf("lat true\n");
	} else {
		printf("lat false\n");
		return;
	}
	
	if (lngHandler(messageArray,playerAddr, allGameInfo)) {
		printf("lng true\n");
	} else {
		printf("lng false\n");
		return;
	}

	if (codeIDHandler(messageArray, playerAddr, allGameInfo)) {
		printf("Inputed HexCode exists\n");
	} else {
		printf("Inputed HexCode does not exist, ignoring message\n");
		return;
	}

	if ((gameIDFlag != 0) || (playerIDFlag != 0)){
		if (gameIDFlag == 1){
		GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
		return;
		} else if (gameIDFlag != 1){
			GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
			return;
		}
	}

	//Update the CodeDrop's Information.
	code_drop_t *foundCode = hashtable_find(allGameInfo->CD, messageArray[7]);
	double distance = dist(atol(messageArray[5]), atol(messageArray[6]), foundCode->lat, foundCode->lng);
	if (distance < 10) {
		foundCode->status = 1;
		foundCode->team = messageArray[3];
		GSResponseHandler(allGameInfo, playerAddr, "Congratulations! You've neutralized a piece of code!", "MI_NEUTRALIZED");
	} else {
		return;
	}

	//update the FA's Neutralization number
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	currentFA->Neutralized = currentFA->Neutralized + 1; 
}

// void FA_CAPTURE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
// 	int gameIDFlag, playerIDFlag;

// 	//Validate the number of arguments in the message
// 	if (arraySize != 6){
// 		GSResponseHandler(allGameInfo, playerAddr, "you need 6 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
// 		return;
// 	}

// 	//Validate each of the parameters
// 	gameIDFlag = gameIDHandler(allGameInfo, messageArray, playerAddr);
// 	playerIDFlag = playerIDHandler(allGameInfo, messageArray, playerAddr);

// 	if (!teamNameHandler(allGameInfo, messageArray, playerAddr)) return;

// 	if (playerNameHandler(allGameInfo, messageArray,playerAddr)) {
// 		printf("player true\n");
// 	} else {
// 		printf("player false\n");
// 		return;
// 	}

// 	if ((gameIDFlag != 0) || (playerIDFlag != 0)){
// 		if (gameIDFlag == 1){
// 		GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
// 		return;
// 		} else if (gameIDFlag != 1){
// 			GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
// 			return;
// 		}
// 	}

// 	FAPlayer_t *capturingPlayer = hashtable_find(allGameInfo->FA, messageArray[2]);



// }
// void GA_STATUS_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
// void GA_HINT_handler(hashStruct *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
// 	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

// 	char *Ihab = "129.170.214.160";

// 	utility_t *utility_p = malloc(sizeof(utility_t));
// 	utility_p->param1 = tempHash;
// 	utility_p->param2 = Ihab;
// 	// receiverAddr_t *sendingPlayer;
// 	hash_iterate(allGameInfo->GA, GA_HINT_iterator, utility_p);
// 	sending(allGameInfo->game->comm_sock, tempHash, "GA_HINT received");

// 	hashtable_delete(tempHash);
// 	free(utility_p);
// }

// void GA_HINT_iterator(void* key, void* data, void* farg){
// 	char IP1[20];

// 	utility_t *utility_p = (utility_t *) farg;
// 	char *key1 = (char *)key;

// 	hashtable_t *tempHash = utility_p->param1;
// 	char *passedStr = (char *) utility_p->param2;
	

// 	sscanf(key1, "%s *", IP1);

// 	if (strcmp(IP1, passedStr) == 0){
// 		hashtable_insert(tempHash, key1, data); //the key dooesnt matter
// 	} 
// }

void GAME_OVER_handler(hashStruct_t *allGameInfo){
	
	char message[BUFSIZE];
	char allTeamInfo[9000]= "\0";
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_t *tempHash2 = hashtable_new(1, deleteTempHash, NULL);
	
	hash_iterate(allGameInfo->FA, FAcopyingAddressIterator, tempHash);
	hash_iterate(allGameInfo->GA, GAcopyingAddressIterator, tempHash);

	

	utility_t *utility_p = malloc(sizeof(utility_t));

	utility_p->param1 = allTeamInfo;
	utility_p->param2 = tempHash2;
	utility_p->param3 = allGameInfo;
	
	hash_iterate(allGameInfo->FA, gameOverIterator, utility_p);
	
	
	sprintf(message, "GAME_STATUS|%ld|%d|%s",allGameInfo->game->gameID, allGameInfo->game->deadDropRemaining, allTeamInfo);
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
	
	hashtable_delete(tempHash);
	hashtable_delete(tempHash2);
	free(utility_p);
}

void FAcopyingAddressIterator(void *key, void* data, void* farg){
	hashtable_t *hash = (hashtable_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	hashtable_insert(hash, (char*) key, FAPlayer_p->addr);
}

void GAcopyingAddressIterator(void *key, void* data, void* farg){
	hashtable_t *hash = (hashtable_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;
	hashtable_insert(hash, "key", GAPlayer_p->addr);
}

void gameOverIterator(void *key, void* data, void* farg){
	char eachTeamInfo[BUFSIZE];
	static int firstInsert = 0;
	int numberOfPlayers = 0;
	int numberCapturing = 0;
	int numberCaptured = 0;
	int numberNeutralized = 0;
	
	
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	
	hashtable_t *hash = (hashtable_t *) utility_p->param2;
	char *allTeamInfo = (char *) utility_p->param1;
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param3;
	

	if (hashtable_insert(hash, FAPlayer_p->TeamName, NULL)){
		
		utility_t *utility_p2 = malloc(sizeof(utility_t));

		utility_p2->param1 = FAPlayer_p->TeamName;
		utility_p2->param2 = &numberOfPlayers;
		
		hash_iterate(allGameInfo->FA, numberOfPlayersIterator, utility_p2);
		
		utility_p2->param2 = &numberCapturing;
		
		hash_iterate(allGameInfo->FA, numberCapturingIterator, utility_p2);
		
		utility_p2->param2 = &numberCaptured;
		
		hash_iterate(allGameInfo->FA, numberCapturedIterator, utility_p2);
		
		utility_p2->param2 = &numberNeutralized;
		
		hash_iterate(allGameInfo->FA, numberNeutralizedIterator, utility_p2);
		
		sprintf(eachTeamInfo, "%s,%d,%d,%d,%d",FAPlayer_p->TeamName, numberOfPlayers, numberCapturing, numberCaptured, numberNeutralized);
		if (firstInsert != 0) strcat(allTeamInfo, ":");
		strcat(allTeamInfo, eachTeamInfo);
		firstInsert++;
		free(utility_p2);
	}
}

void numberOfPlayersIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += 1;
	}
}

void numberCapturingIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += FAPlayer_p->capturedPlayers;
	}
}

void numberNeutralizedIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += FAPlayer_p->Neutralized;
	}
}

void numberCapturedIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
		if (FAPlayer_p->status == 1)
			*(int *) utility_p->param2 += 1;
	}
}

void deleteTempHash(void *data){
	if (data){	//if valid
	}
}

int parsingMessages(char* line, char ** messageArray){

	char* word; // the keyword
	int count = 0; //restart the counter that counts the number of keywords per query
	
	//allocating memory for the string array that stores the words after checking 
	// the words are valid for matching
	// 8 was chosen because the size of each pointer is 8
	// and the other equation was chosen to account for the worst case scenario (words composed of one letter)
	
	if ((word = strtok(line, "|")) == NULL) return count; //parsing the first keyword
	// if the word meets the conditions of query keywords, it will be added to the array
	// if the line is empty, it will be ignored
	if ( (count = copyValidKeywordsToQueryArray(messageArray, word, count)) == 0 ) return count;
	
	// parsing the other keywords
	// if the word meets the conditions of query keywords, it will be added to the array
	//when the parsing reaches the end of line, it will terminate parsing
	while( (word = strtok(NULL, "|")) != NULL)  {
		if ( (count = copyValidKeywordsToQueryArray(messageArray, word, count)) == 0 ) return count;
	}

	return count;
}

// A function that loops through the query array indeces and frees contents after the matching analysis is over
// or if error happens when validating query keywords. It frees the array pointer after it is done.
void freeArray(char** array, int size){

	for(int i = 0; i < size ; i++){ //looping through the query array
		free(array[i]); //delete the keywords
	}
	free(array); //delete the the query array
}

void printArray(char** array, int size){
	for(int i = 0; i < size ; i++){ //looping through the query array
		printf("Array[%d]: %s\n", (i+1), array[i]);
	}
}

// After validating the correctness of the keywords, they are added to the query array
// otherwise, it dellocate the memory created to store the keywords and terminate matching for the query
int copyValidKeywordsToQueryArray( char ** array, char* word, int count){

	array[count] = malloc(strlen(word) +1);
	
	if (array[count]  == NULL) { 
		freeArray(array, count); //if it fails to allocate memory, delete the query
		return 0; //terminate the matching analysis
	}
	
	strcpy(array[count], word); //add the keywords into the query array
	count++; //incremenete the number of keywords counter
	return count;
}

int makeRandomHex(){
	int r = rand() %65535;
	return r;
}

void deleteHashStruct(hashStruct_t *allGameInfo){
	hashtable_delete(allGameInfo->GA);
	hashtable_delete(allGameInfo->FA);
	hashtable_delete(allGameInfo->CD);
	free(allGameInfo->game);
	free(allGameInfo);
}

void GAMatchingTeam(void *key, void* data, void* farg){ //TEAM NAME
	utility_t *utility_p = (utility_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(GAPlayer_p->TeamName, name)) == 0){
		*(int *) utility_p->param1 = 1;
	}
}

void GAguideIDIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(GAPlayer_p->TeamName, name)) == 0){
		utility_p->param2 = (char*) key;
	}
}

void RemainingOperativesIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
		if (FAPlayer_p->status == 0)
			*(int *) utility_p->param1 += 1;
	}
}

void RemainingFoeIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->TeamName, name)) != 0){
		if (FAPlayer_p->status == 0)
			*(int *) utility_p->param1 += 1;
	}
}

void FAPlayerNamesIterator(void *key, void* data, void* farg){  //PLAYER NAME
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->PlayerName, name)) == 0){
		*(int *) utility_p->param1 = 1;
	}
}

void existingHexCodeIterator(void *key, void* data, void* farg){
	utility_t *utility_p = (utility_t *) farg;
	char *code_drop_key = (char *) key;

	char *hexString = (char *) utility_p->param2;

	if ((strcmp(hexString, code_drop_key)) == 0){
		*(int*) utility_p->param1 = 1;
	}
}

// This function was copy and pasted exactly from rosettacode.org and then slightly modified.
// https://rosettacode.org/wiki/Haversine_formula#C
// returns distance in meters.
double dist(double th1, double ph1, double th2, double ph2){
	double dx, dy, dz;
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;
 
	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	return (asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R) * 1000;
}