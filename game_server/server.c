/*
Mission Incomputable!
Team Topaz

gameserver.c - The game server application coordinates one and only one game each times it runs.
it interacts with the field agent through pebble and proxy (UDP) and the guide agent though UDP 
communication

May, 2016
Ihab Basri, Ted Poatsy
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>	
#include <stdbool.h>
#include <getopt.h> //for options


#include <errno.h> //for socket
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>	  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>	 

#include "file.h" // to read lines from files 
#include <time.h>  // using time functions
#include <math.h> // for equations

//to create a list, our list structures had some bags
// we are using one row though in the hashtable to create a one dimentional list
#include "hashtable/hashtable.h"


/************* Function Declarations ****************/

// for the command options (optand)
const struct option longOpts[] = {
	{ "level", required_argument, 0, 'l'}, //level of the game
	{ "time", required_argument, 0, 't'}, //duration of the game
	{ "log", no_argument, 0, 'r'}, // for logging
	{ "game", required_argument, 0, 'g'}, // for gameID
	{ 0, 0, 0, 0}
};

// game statistics and important flags
typedef struct game {
	char * gameID; 
	int level; //level 3 flag
	int timeVal; //time option flag
	int rawlogFl; //raw logging flag
	int GSPort; //game port
	int deadDropRemaining; //number of deadDrop remaining
	int comm_sock; //socket
	long int startingTime; //start of the game (reference)
	long int elapsedTime; //duration of the game
	FILE *fp; //logging file
} game_t;

//code drop information
typedef struct code_drop {
	double lng; //longitude
	double lat; //latitude
	int status; //neutralization status (0 means active)
	char *team; // neutralizing team
} code_drop_t;

//a struct that combines the socket and the sending message to use with iterators
typedef struct sendingInfo {
	int comm_sock;
	char *message;
} sendingInfo_t;


// a general struct that can be used to get information into and out of iterators
// named utility because it is our "utility tool" that we can use for so many different
// combinations of information
typedef struct utility {
	void *param1;
	void *param2;
	void *param3;
	void *param4;
	void *param5;
} utility_t;

//The master information struct
typedef struct hashStruct{
	hashtable_t *GA; // all guide agents
	hashtable_t *FA; // all field agents
	hashtable_t *CD; // all code drops
	game_t *game; // current game info
} hashStruct_t;


// combining the receiver address
typedef struct receiverAddr{
	in_port_t port; //port
	struct in_addr inaddr; // IP address
	int sin_family;
}receiverAddr_t;

// a struct that combines all the player info
typedef struct FAPlayer{
	char *PlayerName;
	char *TeamName;
	int status; // active (0), capturing(3), captured(1), maybe captured(2)
	double lat; //latitude
	double lng; //longitude
	long int lastContact; //time since last contact
	receiverAddr_t *addr; // contact info
	int capturedPlayers; // enemy players captured by this player
	int Neutralized; //codeDrop neutralized by this player
	long int capturingTime; //time when this player was targeted
	char *capturingHexCode; //random hex code assigned upon being targeted
} FAPlayer_t;

typedef struct GAPlayer{
	char *PlayerName;
	char *TeamName;
	int status; //0 is active, 1 is inactive
	long int lastContact; //time since last contact
	receiverAddr_t *addr; //player's contact information
} GAPlayer_t;

/*************** Constants**********************/
static const int BUFSIZE = 9000;  
#define R 6371 //earth radium
#define TO_RAD (3.1415926536 / 180) //To convert into radion
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
void GA_STATUS_handler(hashStruct_t *allGameInfo,char **messageArray, int arraySize,receiverAddr_t * playerAddr);
void GA_HINT_handler(hashStruct_t *allGameInfo,char **messageArray, int arraySize,receiverAddr_t * playerAddr);
void INVALID_ENTRY_handler(hashStruct_t *allGameInfo,char **messageArray, int arraySize,receiverAddr_t * playerAddr);
void GAME_OVER_handler(hashStruct_t *allGameInfo);
void GAPlayerGameStatusHandler(hashStruct_t *allGameInfo, GAPlayer_t *currentGA);
void GAPlayerGameStatusHandlerThree(hashStruct_t *allGameInfo, GAPlayer_t *currentGA);

int gameIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr);
int playerIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr);
bool teamNameHandler(hashStruct_t *allGameInfo, char**messageArray, receiverAddr_t * playerAddr);
bool playerNameHandler(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t * playerAddr);
void FAPlayerGameStatusHandler(hashStruct_t *allGameInfo, FAPlayer_t *playerAddr);
bool latHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
bool lngHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
int statusReqHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo);
bool validateHexInt(int hexValue, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo);
bool codeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo);
bool messageCodeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo);
void logger(FILE *fp,char* message);

void sending(int comm_sock, hashtable_t *tempHash, char *message);

void addPlayer(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t *addr);

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
void GAGameStatusFriendlyIterator(void *key, void* data, void* farg);
void CDGameStatusFriendlyIterator(void *key, void* data, void* farg);
void captureProximityIterator(void *key, void* data, void* farg);
void GS_CAPTURE_IDIterator(void *key, void* data, void* farg);
void findCapturedPlayer(void *key, void* data, void* farg);
void GA_HINTIterator(void *key, void* data, void* farg);
void FAMatchingTeam(void *key, void* data, void* farg);
void GAGameStatusFriendlyIteratorThree(void *key, void* data, void* farg);
void GAGameStatusCodeIteratorThree(void *key, void* data, void* farg);
void GAGameStatusCodeHashIteratorThree(void *key, void* data, void* farg);
void GAGameStatusEnemyIteratorThree(void *key, void* data, void* farg);
void GAGameStatusEnemyHashIteratorThree(void *key, void* data, void* farg);

double dist(double th1, double ph1, double th2, double ph2);

char *makeRandomHex();

void delete(void *data);
void freeArray(char** array, int size);
void deleteHashStruct(hashStruct_t *allGameInfo);
void deleteTempHash(void *data);
void deleteFAPlayer(void *data);
void deleteGAPlayer(void *data);
void deleteCodeDrop(void *data);
/***Debugging Functions***/
void printArray(char** array, int size);


int main(int argc, char *argv[]){
	
	srand(time(NULL));

	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	printf("Hostname: %s\n", hostname);
	
	//need if malloc fails messages
	hashStruct_t *allGameInfo;
	if ((allGameInfo = malloc(sizeof(hashStruct_t))) == NULL) exit(11);
	hashtable_t *GAPlayers = hashtable_new(1, deleteGAPlayer, NULL);
	hashtable_t *FAPlayers = hashtable_new(1, deleteFAPlayer, NULL);
	hashtable_t *codeDrop = hashtable_new(1, deleteCodeDrop, NULL);

	game_t *game_p;
	if ((game_p = malloc(sizeof(game_t))) == NULL) exit(11);

	//populating the master game structure
	allGameInfo->game = game_p;
	allGameInfo->GA = GAPlayers;
	allGameInfo->FA = FAPlayers;
	allGameInfo->CD = codeDrop;

	//initializing the game values with defaults
	game_p->level = 1;
	game_p->timeVal = 0;
	game_p->rawlogFl = 0;
	game_p->GSPort = 0;
	game_p->deadDropRemaining = 0;
	game_p->startingTime = time(NULL);


	//Assign Game ID
	game_p->gameID = makeRandomHex();
	
	// Verifying and setting the values input with flags
	if (!verifyFlags(argc, argv, game_p)){
		deleteHashStruct(allGameInfo);
		exit(1);
	}

	// Verifying the number of arguments
	if ((argc-optind) != 2){
		printf("Invalid number of arguments\n");
		printf("Usage: ./gameserver [--log] [--level = 1 or 3] [--time (in Minutes)] codeDropPath GSport\n");
		deleteHashStruct(allGameInfo);
		exit(2);
	}

	// Verifying that the port that is given is valid
	if (!verifyPort(argc, argv, game_p)){
		deleteHashStruct(allGameInfo);
		exit(3);
	}

	// Verifying that the given codeDrop file exists and is readable
	if (!verifyFile(argc, argv)){
		deleteHashStruct(allGameInfo);
		exit(4);
	}

	// Starting the process of the game
	if (!game_server(argv, allGameInfo)) {
		printf("something went wrong\n");
		deleteHashStruct(allGameInfo);
		exit(5);
	}

	deleteHashStruct(allGameInfo);
	//Successfully exiting
	exit(0);
}

bool game_server (char *argv[], hashStruct_t *allGameInfo) {
	int comm_sock;
	char logText[1000];
	char logText2[1000];
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

    sprintf(logText, "log/gameserver%ld.log", time(NULL) );
	
	if ((allGameInfo->game->fp=fopen(logText, "a")) == NULL) {
		return false;
	} 
	logger(allGameInfo->game->fp, "Game Started");
	logText[999] = '\0';
	gethostname(logText, 999);
	sprintf(logText2, "Host: %s", logText);
	logger(allGameInfo->game->fp, logText2);
	sprintf(logText2, "Port: %d", allGameInfo->game->GSPort);
	logger(allGameInfo->game->fp, logText2);
	
	// Receive datagrams, print them out, read response from term, send it back
	while (true) {        // loop exits on EOF from stdin
		// for use with select()
		fd_set rfds;        // set of file descriptors we want to read
		allGameInfo->game->elapsedTime = time(NULL) - allGameInfo->game->startingTime;
		if (allGameInfo->game->elapsedTime >= allGameInfo->game->timeVal && allGameInfo->game->timeVal != 0) break;
		else if (allGameInfo->game->deadDropRemaining == 0) break;

		// Watch stdin (fd 0) and the UDP socket to see when either has input.
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);       // stdin
		FD_SET(comm_sock, &rfds); // the UDP socket
		int nfds = comm_sock+1;   // highest-numbered fd in rfds
		struct timeval timeout;   // how long we're willing to wait
		const struct timeval onesec = {1,0};   // five seconds
    
		timeout = onesec;
		int select_response = select(nfds, &rfds, NULL, NULL, &timeout);
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
		} else if (select_response == 0) {
			fflush(stdout);
		}
	}

	//sending end game message and cleaning up used memory
	GAME_OVER_handler(allGameInfo);
	logger(allGameInfo->game->fp,"Game Over");
	close(comm_sock);
	fclose(allGameInfo->game->fp);
	return true;
}


bool verifyFlags(int argc, char *argv[], game_t *game_p){
	int option = 0;
	int tempInt;
	while  ((option = getopt_long (argc, argv, "+l:t:rg:", longOpts, NULL)) != -1){
		switch (option){
			case 'l' :
				if (!isItValidInt(optarg)){
					printf("Level Flag is invalid\n");
					return false;
				} 
				tempInt = atoi(optarg);
				if (tempInt > 0 && tempInt < 4){ //might change to 4
					if (tempInt == 2){
						printf("I'm sorry. Currently level 2 is not supported.\n");
						return false;
					} else {
						game_p->level = tempInt;
						break;
					}
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
					game_p->timeVal = tempInt * 60;
					break;
 				} else {
 					printf("Time Argument is not in range\n");
					return false;
				}

			case 'r':
				if (strcmp(optarg, "raw") == 0) {
				game_p->rawlogFl = 1;
				break;
				}
				printf("Wrong flag argument\n");
				return false;

			case 'g':
				//verify that the input is an integer between 1 and 4294967295
				free(game_p->gameID);
				if ((game_p->gameID = malloc(10)) == NULL) exit(11);
				strcpy(game_p->gameID, optarg);
				if ((strlen(optarg) > 9) || (strlen(optarg) < 4)){
					printf("Invalid gameID\n");
					return false;
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
	
	if (((isDigit = malloc(strlen(intNumber) +1))) == NULL) exit(11); //NULL to check
	
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
		code_drop_t *code_drop;
		if ((code_drop = malloc(sizeof(code_drop_t))) == NULL) exit(11);
		
		if (code_drop == NULL){ 	// checking if memory was allocated for each struct
			free(line); //free everything and return false if it fails
	    	fclose(readFrom);
			return false;
		} 
		
		//initialize the struct
		code_drop -> lng = 0.0;
		code_drop -> lat = 0.0;
		code_drop -> status = 0; 
		if ((code_drop -> team = malloc(100)) ==NULL) exit(11);
		strcpy(code_drop->team, "NONE");
		
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
	int i = strlen(token);
	if(i == 5) token[i-1] = '\0';
	// if it fails to insert, that suggests duplicate (not acceptable ) or memory allocation error
	if (!hashtable_insert(codeDropHash, token, code_drop)) return false;
	return true;
}

// Frees memory found in the data category of a hashtable
void delete(void *data){
	if (data != NULL){//if valid
		free(data);
	}
}

// frees the memory found in the data category of a hashtable, specific to the overall Guide Agent hashtable
void deleteGAPlayer(void *data){
	if (data != NULL){
		free(((GAPlayer_t *) data)->PlayerName);
		free(((GAPlayer_t *) data)->TeamName);
		free(((GAPlayer_t *) data)->addr);
		free(data);
	}
}

//frees the memory found in the data category of a hashtable, specific to full Field Agent Hashtable
void deleteFAPlayer(void *data){
	if (data != NULL){
		free(((FAPlayer_t *) data)->PlayerName);
		free(((FAPlayer_t *) data)->TeamName);
		free(((FAPlayer_t *) data)->addr);
		free(((FAPlayer_t *) data)->capturingHexCode);
		free(data);
	}
}

//frees the memory found in the data category of a hashtable, specicic to the top level CodeDrop hashtable
void deleteCodeDrop(void *data){
	if (data != NULL){
		free(((code_drop_t *) data)->team);
		free(data);
	}
}

//Iterator that counts the amount of codeDrops
void numberOfcodeDrops(void* key, void* data, void* farg) {
	((game_t*)(farg))->deadDropRemaining +=1;
}

// Function used for confirming that a value is an appropriate int (used only the digis 0-9)
bool isItValidFloat(char *floatNumber){
	
	double validFloat = 0;
	char * isDigit;
	if((isDigit = malloc(strlen(floatNumber) +1)) == NULL) exit(11); //NULL to check
	
	
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

/*****DEBUGGING*****/
void hashTable_FA_print(void *key, void* data, void* farg){
	if (key && data){ //if valid
		printf("Key: %s\n", (char *) key);
		printf("Team: %s, Player: %s\n", ((FAPlayer_t*)(data))->TeamName, ((FAPlayer_t*)(data))->PlayerName);
	}
}

/***DEBUGGING***/
void hashTable_print(void *key, void* data, void* farg){
	if (key && data){ //if valid
		printf("%s, ", (char*)key);
		printf("%lf, %lf", ((code_drop_t*)(data))->lng, ((code_drop_t*)(data))->lat);

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

//Allows the server to interpret standard in. Useful for ending the game with a quit message.
// All other input, it prints to the console and otherwise ignores
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

// A funciton that takes a singleton hashtable and gives the necessary infromation to sendIterator
// an abstract funtion that allows us to send a message through the UDP socket
void sending(int comm_sock, hashtable_t *tempHash, char *message){
  	sendingInfo_t *sendingInfo_p;
	if ((sendingInfo_p = malloc(sizeof(sendingInfo_t))) == NULL) exit(11);
	sendingInfo_p->comm_sock = comm_sock;
	sendingInfo_p->message = message;

	hash_iterate(tempHash, sendIterator, sendingInfo_p);

	free(sendingInfo_p);
}

// Iterates through the singleton hashtable from `sending` and does the technical work of sending.
// This was modified from a function found in chatserver.c 
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

// checks the socket for messages and passes them off to `parsing`
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
	        receiverAddr_t *trial;
	        if ((trial = malloc(sizeof(receiverAddr_t)))==NULL) exit(11);  
	        trial->port = sender.sin_port;
	        trial->inaddr = sender.sin_addr;
			trial->sin_family = sender.sin_family;
		    /***FOR THIS FILE ONLY, SHOULD BE PARSE***/

		    /****LOGGING***/
			printf("[%s@%d]: %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buf);
			/****LOGGING***/
			
			if (allGameInfo->game->rawlogFl == 1)
				logger(allGameInfo->game->fp, buf);
			
			// Create array
			char ** messageArray;
			if((messageArray = malloc(8 *((strlen(buf)+1)/2 + 1) )) ==NULL) exit(11); 

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

// directs the message (in the form of a messageArray) to the appropriate message handler according to OPCODE
bool processing(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	
	//Comparing the first argument in the array with the standard OPCODEs
	if(strcmp(messageArray[0], "FA_LOCATION") == 0){
		FA_LOCATION_handler(allGameInfo, messageArray, arraySize, playerAddr); //these params are a model for the rest
		return true;
	} else if(strcmp(messageArray[0], "FA_NEUTRALIZE") == 0){
		FA_NEUTRALIZE_handler(allGameInfo, messageArray, arraySize, playerAddr);
		return true;
	} else if(strcmp(messageArray[0], "FA_CAPTURE") == 0){
		FA_CAPTURE_handler(allGameInfo, messageArray, arraySize, playerAddr);
		return true;
	} else if(strcmp(messageArray[0], "GA_STATUS") == 0){
		GA_STATUS_handler(allGameInfo, messageArray, arraySize, playerAddr);
		return true;
	} else if(strcmp(messageArray[0], "GA_HINT") == 0){
		GA_HINT_handler(allGameInfo, messageArray, arraySize, playerAddr);
		return true;
	} else {
		GSResponseHandler(allGameInfo, playerAddr, "Please enter a proper OPCODE",  "MI_ERROR_INVALID_OPCODE");
		return false;
	}
	return false;
}

// since all OPCODEs that the Server recieves have the same first five fields, this is a function that checks indexes
// one through 4. It does not check the opcode other than to determine whether a player should be added.
bool validateArgumentsOneThroughFour(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t *playerAddr){
	
	int gameIDFlag = gameIDHandler(allGameInfo, messageArray, playerAddr);
	int playerIDFlag = playerIDHandler(allGameInfo, messageArray, playerAddr);

	if (!teamNameHandler(allGameInfo, messageArray, playerAddr)) return false;

	if (((strcmp(messageArray[0], "FA_LOCATION")) == 0) || ((strcmp(messageArray[0], "GA_STATUS")) == 0)){
		if (gameIDFlag == -1){
			return false;
		} else if(playerIDFlag == -1){
			return false;
		} else if (playerIDFlag == 2){
			return false;
		} else if ((gameIDFlag == 1) && (playerIDFlag == 1)){
			addPlayer(allGameInfo, messageArray, playerAddr);
			return true;
		} else if ((gameIDFlag == 1) && (playerIDFlag != 1)){
			GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
			return false;
		} else if ((gameIDFlag != 1) && (playerIDFlag == 1)){
			GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
			return false;
		} 
		return true;
	}
	else {
		if ((gameIDFlag != 0) || (playerIDFlag != 0)){
			if (gameIDFlag == 1){
			GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
			return false;
			} else if (gameIDFlag != 1){
				GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
				return false;
			}
		}
		return true;
	}
	return true;
}


// handles all messages with the OPCODE "FA_LOCATION"
void FA_LOCATION_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	//returns a message to the sender
	int statusFlag;

	// Checking for the correct size of the messageArray.
	if (arraySize != 8){
		GSResponseHandler(allGameInfo, playerAddr, "you need 8 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}

	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	
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

	if ((statusFlag = statusReqHandler(messageArray,playerAddr, allGameInfo)) == -1);

	//if they are here, they exist, in our hashtable --> update their info
	//update the FA's information
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	//1: GameID, does not change
	//2: PebbleID, does not change
	//3: TeamName, not allowing them to change teams.
	//4: PlayerName, can switch it. Hashtable has already been checked for doups.
	free(currentFA->PlayerName);
	if ((currentFA->PlayerName = malloc(strlen(messageArray[4]) +1)) ==NULL) exit(11);
	strcpy(currentFA->PlayerName, messageArray[4]);
	//5 & 6: Lat and Long both should be updated
	currentFA->lat = atof(messageArray[5]);
	currentFA->lng = atof(messageArray[6]);
	//Update Last Contact Time
	currentFA->lastContact = time(NULL);
	//Update Player's Address in case of disconnection.

	if (currentFA->capturingTime != 0 && time(NULL) - currentFA->capturingTime > 60){
		currentFA->capturingTime = 0;
		free(currentFA->capturingHexCode);
		if ((currentFA->capturingHexCode  = malloc(5)) == NULL) exit(11);
		strcpy(currentFA->capturingHexCode, "P00P");
		currentFA->status = 0;
	}

	currentFA->addr->port = playerAddr->port;
	currentFA->addr->inaddr = playerAddr->inaddr;
	currentFA->addr->sin_family = playerAddr->sin_family;



	if (statusFlag == 1){
		//send a message back.
		FAPlayerGameStatusHandler(allGameInfo, currentFA);
	
	}
}

//Creates the appropriate response for Field Agent's status request
void FAPlayerGameStatusHandler(hashStruct_t *allGameInfo, FAPlayer_t *currentFA){
	
	char message[BUFSIZE];
	char* guideID = "0";
	int numRemainingFriendlies = 0;
	int numRemainingFoe = 0;
	
	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p->param1 = guideID;
	utility_p->param2 = currentFA -> TeamName;
	
	hash_iterate(allGameInfo->GA, GAguideIDIterator, utility_p);
	
	
	utility_p->param1 = &numRemainingFriendlies;
	
	hash_iterate(allGameInfo->FA, RemainingOperativesIterator, utility_p);
	
	utility_p->param1 = &numRemainingFoe;
	
	hash_iterate(allGameInfo->FA, RemainingFoeIterator, utility_p);
	
	
	sprintf(message, "GAME_STATUS|%s|%s|%d|%d|%d",allGameInfo->game->gameID, guideID, allGameInfo->game->deadDropRemaining, numRemainingFriendlies, numRemainingFoe);
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, currentFA->PlayerName, currentFA->addr); //the key doesnt matter
	
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
	free(utility_p);
}

// Generates the appropriate response for Guide Agent's status request for level 3
void GAPlayerGameStatusHandlerThree(hashStruct_t *allGameInfo, GAPlayer_t *currentGA){
	char message[BUFSIZE];
	char playerInfo[9000]= "\0";
	char codeDropInfo[9000]= "\0";
	int x = 0;
	int y = 0;

	//setting up two iterators that will get a list of all friendly and any enemy player close to them
	utility_t *utility_p;
	utility_t *utility_p2;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);
	if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

	hashtable_t *FAFriendlyHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_t *FANearEnemyHash = hashtable_new(1, deleteTempHash, NULL);

	utility_p->param1 = playerInfo;
	utility_p->param2 = &x;
	utility_p->param3 = FAFriendlyHash;
	utility_p->param4 = currentGA->TeamName;

	//Iterating to get all of the members of one team into a hashtable.
	//Also creates the formatted message for the friendly players in this iterator.
	hash_iterate(allGameInfo->FA, GAGameStatusFriendlyIteratorThree, utility_p);

	//finding any player thats close to any one of the friendly players and is not on the same team 
	utility_p->param1 = playerInfo;
	utility_p->param3 = allGameInfo;

	hash_iterate(FAFriendlyHash, GAGameStatusEnemyIteratorThree, utility_p);

	//doing the same for the codeDrops
	utility_p2->param1 = codeDropInfo;
	utility_p2->param2 = &y;
	utility_p2->param3 = allGameInfo;
	
	hash_iterate(FAFriendlyHash, GAGameStatusCodeIteratorThree, utility_p2);//HERE HERE HERE!!!
	
	//creating the final message
	sprintf(message, "GAME_STATUS|%s|%s|%s",allGameInfo->game->gameID, playerInfo, codeDropInfo);
	
	//sending the message to the requesting GA
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, "key", currentGA->addr); 
	
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
	
	//clean up
	hashtable_delete(tempHash);
	hashtable_delete(FAFriendlyHash);
	hashtable_delete(FANearEnemyHash);
	free (utility_p2);
	free (utility_p);
}

// creates and sends the Game Status message for Guide Agent's status request for level one
void GAPlayerGameStatusHandler(hashStruct_t *allGameInfo, GAPlayer_t *currentGA){
	
	char message[BUFSIZE];
	char playerInfo[9000]= "\0";
	char codeDropInfo[9000]= "\0";
	int x = 0;
	int y = 0;
	
	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p->param1 = playerInfo;
	utility_p->param2 = &x;
	
	hash_iterate(allGameInfo->FA, GAGameStatusFriendlyIterator, utility_p);
	
	utility_p->param1 = codeDropInfo;
	utility_p->param2 = &y;
	
	hash_iterate(allGameInfo-> CD, CDGameStatusFriendlyIterator, utility_p);
	
	
	sprintf(message, "GAME_STATUS|%s|%s|%s",allGameInfo->game->gameID, playerInfo, codeDropInfo);
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, "key", currentGA->addr); //the key doesnt matter
	
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
	free (utility_p);
}

// a helper iterator for "GAPlayerGameStatusHandler" that creates the string 
// describing all of the FA's informaiton
// specific to level 1
void GAGameStatusFriendlyIterator(void *key, void* data, void* farg){

	char playerInfo[BUFSIZE];
	
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	
	utility_t *utility_p = (utility_t *) farg;
	
	char *allPlayers= utility_p->param1;
	
	
	long int currenttime = time(NULL);
	long int lastContact = currenttime - FAPlayer_p->lastContact;
	sprintf(playerInfo, "%s,%s,%s,%d,%lf,%lf,%ld",(char*) key, FAPlayer_p->TeamName,FAPlayer_p->PlayerName, FAPlayer_p->status, FAPlayer_p->lat, FAPlayer_p->lng, lastContact);
	
	if (*(int*)utility_p->param2 == 0) 
		strcat(allPlayers, playerInfo);
	else {
		strcat(allPlayers, ":");
		strcat(allPlayers, playerInfo);
	}
	*(int*)utility_p->param2 += 1;
}

// a helper function for "GAPlayerGameStatusHandlerThree" that creates the string 
//describing all of the FA's informaiton
// specific to level 3
void GAGameStatusFriendlyIteratorThree(void *key, void* data, void* farg){

	char playerInfo[BUFSIZE];
	char *PlayerID = (char *) key;
	
	//renaming arguments
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	utility_t *utility_p = (utility_t *) farg;

	//interpreting passed information
	char *allPlayers= (char *) utility_p->param1;
	hashtable_t *FAHash = (hashtable_t *) utility_p->param3;
	char *TeamName = (char *) utility_p->param4;

	//if the player is not on the same team, add them to the hashtable and generate their part of the whole message.
	if((strcmp(FAPlayer_p->TeamName, TeamName)) == 0){
		hashtable_insert(FAHash, PlayerID, FAPlayer_p);

		long int currenttime = time(NULL);
		long int lastContact = currenttime - FAPlayer_p->lastContact;
		sprintf(playerInfo, "%s,%s,%s,%d,%lf,%lf,%ld",(char*) key, FAPlayer_p->TeamName,
				FAPlayer_p->PlayerName, FAPlayer_p->status, 
				FAPlayer_p->lat, FAPlayer_p->lng, lastContact);

		//this if, else condition is here becasue the formatting for the first person in the message is different
		//than the formatting for any other person.
		if (*(int*)utility_p->param2 == 0) 
			strcat(allPlayers, playerInfo);
		else {
			strcat(allPlayers, ":");
			strcat(allPlayers, playerInfo);
		}
		*(int*)utility_p->param2 += 1;
	}
}

// a helper itertor for "GAPlayerGameStatusHandlerThree" that creates the string
// describing all of the CodeDrop informaiton
// specific to level 3
void GAGameStatusCodeIteratorThree(void *key, void* data, void* farg){
	//Iterating through the hashtable of only friendly operatives.
	//In this loop, I have to iterate though all of the field agents and add those agents 
	int y = 0;

	//renaming arguments
	FAPlayer_t *FAFriendlyPlayer_p = (FAPlayer_t *) data;
	utility_t *utility_p = (utility_t *) farg;

	//interpreting passed information
	char* allCodes= utility_p->param1;
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param3;

	//overwriting some information in Utility to pass to the next function
	utility_p->param5 = &(FAFriendlyPlayer_p->lat);
	utility_p->param2 = &(FAFriendlyPlayer_p->lng);
	utility_p->param4 = &y; 
	utility_p->param1 = allCodes;

	//iterating through all the CDs to find any code close to the given set of FAs
	hash_iterate(allGameInfo->CD, GAGameStatusCodeHashIteratorThree, utility_p);
}

// a helper iterator for "GAGameStatusCodeIteratorThree" that creates a 
//string of codeDrops that are within 100m of any friendly FA
// Iterates through the master FA hashtable.
// Speficic to level 3
void GAGameStatusCodeHashIteratorThree(void *key, void* data, void* farg){
	char codeDropInfo[BUFSIZE];

	//renaming arguments
	code_drop_t *code_drop_p = (code_drop_t *) data;
	utility_t *utility_p = (utility_t *) farg;
	
	//interpreting passed information
	double lng = *(double *) utility_p->param5;
	double lat = *(double *) utility_p->param2;
	char *allCodes = (char *) utility_p->param1;

	//calculating distance between the players
	double distance = dist(lat, lng, code_drop_p->lat, code_drop_p->lng);

	//if the code is within range and not captured, rename the null argument of captuered team to NONE
	if (distance <= 100){
		if (code_drop_p->team == NULL) {
			free(code_drop_p);
			if ((code_drop_p->team = malloc(100)) == NULL) exit(11);
			strcpy(code_drop_p->team, "NONE");
		}
		//create message for this piece of code
		sprintf(codeDropInfo, "%s,%lf,%lf,%s", (char*) key,code_drop_p->lat,code_drop_p->lng,code_drop_p->team);

		//this is here because the formatting is different for the first peice of code.
		if(*(int*)utility_p->param4 == 0) 
			strcat(allCodes, codeDropInfo);
		
		else {
			strcat(allCodes, ":");
			strcat(allCodes, codeDropInfo);
		}

		*(int*)utility_p->param4 += 1;
	}
}

// a helper iterator for "GAPlayerGameStatusHandlerThree" that calls another iterator in 
// order to make a string of enemy FAs that are within 100m of any friendly FA
// Iterates through the frineldy FA hashtable.
// specific to level 3
void GAGameStatusEnemyIteratorThree(void *key, void* data, void* farg){
	//Iterating through the hashtable of only friendly operatives.
	//In this loop, I have to iterate though all of the field agents and add those agents 	
	FAPlayer_t *FAFriendlyPlayer_p = (FAPlayer_t *) data;
	utility_t *utility_p = (utility_t *) farg;
	
	char *allPlayers = (char *) utility_p->param1;
	// hashtable_t *FAEnemyHash = (hashtable_t *) utility_p->param2;
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param3;

	utility_t *utility_p2;
	if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p2->param1 = &(FAFriendlyPlayer_p->lat);
	utility_p2->param2 = &(FAFriendlyPlayer_p->lng);
	utility_p2->param3 = FAFriendlyPlayer_p->TeamName; // TeamName
	// utility_p2->param4 = FAEnemyHash; // Hashtable
	utility_p2->param5 = allPlayers;

	hash_iterate(allGameInfo->FA, GAGameStatusEnemyHashIteratorThree, utility_p2);
	free(utility_p2);
}

// a helper iterator for "GAGameStatusEnemyIteratorThree" makes a string of enemy FAs
// that are within 100m of any friendly FA
// Iterates through the master FA hashtable.
//specific to level 3
void GAGameStatusEnemyHashIteratorThree(void *key, void* data, void* farg){
	char playerInfo[BUFSIZE];
	
	//renaming arguments
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	utility_t *utility_p = (utility_t *) farg;
	
	//interpreting passed information
	double lat = *(double *) utility_p->param1;
	double lng = *(double *) utility_p->param2;
	char *friendlyTeamName = (char *) utility_p->param3;
	char *allPlayers = (char *) utility_p->param5;
	
	//calculate distance
	double distance = dist(lat, lng, FAPlayer_p->lat, FAPlayer_p->lng);

	//any enemy player who is within 100 meters is added to this string
	if (distance <= 100){
		if((strcmp(FAPlayer_p->TeamName, friendlyTeamName)) != 0){
			long int currenttime = time(NULL);
			long int lastContact = currenttime - FAPlayer_p->lastContact;
			sprintf(playerInfo, "%s,%s,%s,%d,%lf,%lf,%ld",(char*) key, FAPlayer_p->TeamName,
					FAPlayer_p->PlayerName, FAPlayer_p->status, 
					FAPlayer_p->lat, FAPlayer_p->lng, lastContact);
			
			//we do not need the if statement here because the enemy players always come after the friendly players
			//... and if there were no friendly players (and the enemies would go first), there would be no friendly
			//... players to be close to.
			strcat(allPlayers, ":");
			strcat(allPlayers, playerInfo);

		}
	}
}

// a helper iterator that makes a string from all of the remaining code Drops
// specific to level 1
void CDGameStatusFriendlyIterator(void *key, void* data, void* farg){

	char codeDropInfo[BUFSIZE];

	code_drop_t *code_drop_p = (code_drop_t *) data;
	
	utility_t *utility_p = (utility_t *) farg;
	
	char* allCodes= utility_p->param1;

	
	if (code_drop_p->team == NULL) {
		free(code_drop_p);
		if((code_drop_p->team = malloc(100)) == NULL) exit(11);
		strcpy(code_drop_p->team, "NONE");
	}
	sprintf(codeDropInfo, "%s,%lf,%lf,%s", (char*) key,code_drop_p->lat,code_drop_p->lng,code_drop_p->team);
	
	if(*(int*)utility_p->param2 == 0) 
		strcat(allCodes, codeDropInfo);
	
	else {
		strcat(allCodes, ":");
		strcat(allCodes, codeDropInfo);
	}
	*(int*)utility_p->param2 += 1;
}

// A function that can be used to send either one message to one reciever 
// or many messages to many recievers in the GSResponse Format
void GSResponseHandler(hashStruct_t *allGameInfo, receiverAddr_t *playerAddr, char* Response, char* respCode){
	char message[BUFSIZE];	
	sprintf(message, "GS_RESPONSE|%s|%s|%s",allGameInfo->game->gameID, respCode, Response);
	
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, "key", playerAddr); //the key doesnt matter
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
}

// creates individualized GS_CAPTURE messages to inform players that they are 
// targeted and to give them their unique hash code. Uses a helper iterator.
void GS_CAPTURE_IDHandler(hashStruct_t *allGameInfo, hashtable_t *hash){

	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	char message[BUFSIZE];	
	sprintf(message, "GS_CAPTURE_ID|%s",allGameInfo->game->gameID);

	utility_p->param1 = allGameInfo;
	utility_p->param2 = message;

	printf("%s\n", message);

	hash_iterate(hash, GS_CAPTURE_IDIterator, utility_p);
	
	free(utility_p);
}

// a function that handles all possibe GameID inputs
// in the appropriate context, if the GameID is zero, it calls a function that adds a player to the game
// otherwise, it checks to make sure that the GameID is correct.
int gameIDHandler(hashStruct_t *allGameInfo, char** messageArray,receiverAddr_t * playerAddr){

	if (strcmp(messageArray[1], "0") == 0){
		if ((strcmp(messageArray[0], "FA_LOCATION") == 0) || (strcmp(messageArray[0], "GA_STATUS") == 0)){
			return 1; //can be added
		} else {
			GSResponseHandler(allGameInfo, playerAddr, "Send 'FA_LOCATION' or 'GA_STATUS' OPCODE if you want to be added ",  "MI_ERROR_INVALID_GAME_ID");
			return -1;
		}
	} else if ((strcmp(messageArray[1], allGameInfo->game->gameID)) ==0){
		return 0; //valid
	} else {
		GSResponseHandler(allGameInfo, playerAddr, "Send 0 to be registered",  "MI_ERROR_INVALID_GAME_ID");
		return -1;
	}
}

// a function that handles all possibe playerID inputs
// it confirms that all players have unique hex codes
// in the appropriate context, this funciton allows a GA player to be created if '0' is given as the argument
int playerIDHandler(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t * playerAddr){
	
	if (strlen(messageArray[2]) > 8){
		GSResponseHandler(allGameInfo, playerAddr, "Not a valid hex code",  "MI_ERROR_INVALID_ID");
		return -1;
	}
	
		
	if (messageArray[0][0] == 'G'){
		GAPlayer_t *foundPlayerGA;
		FAPlayer_t *foundPlayerFA;
		if ((foundPlayerGA = hashtable_find(allGameInfo->GA, messageArray[2])) != NULL){
			if (foundPlayerGA->status != 1){
				printf("Player is known and not idle\n");
				return 0; //valid
			} else {
				printf("Player is known and idle\n");
				return 2; //since we just need to ignore it
			}
		} else if ((foundPlayerFA = hashtable_find(allGameInfo->FA, messageArray[2])) != NULL) {
			return 0;
		} else {
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

// verifies input Team Names
// verifies there is at most one GA per team
// ensure that no player switches teams.
bool teamNameHandler(hashStruct_t *allGameInfo, char**messageArray, receiverAddr_t * playerAddr){
	if (messageArray[0][0] == 'G'){
		GAPlayer_t *foundPlayerGA;
		if((foundPlayerGA = hashtable_find(allGameInfo->GA, messageArray[2])) == NULL){
			//player is not known
			//check if that team name already exists
			utility_t *utility_p;
			if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);
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

//This function ensures that no two FAs have the same name
// for GA's, since they are the only GA on a team, just returns true.
bool playerNameHandler(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t * playerAddr){
	if (messageArray[0][0] == 'F'){
		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);
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

//for the FA OPCODEs, verifies that the give lattitude is a valid Float and that it is within the correct range
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

//for the FA OPCODES, verifies that the given longitude is a valid Float and that it is within the correct range
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

// Parses the status request flag and creates a flag that triggers a response message.
int statusReqHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	if (messageArray[0][0] == 'G'){
		if(isItValidInt(messageArray[5])) {
			if((atoi(messageArray[5])== 0)){
				return 0;
			} else if ((atoi(messageArray[5])== 1)){
				return 1;
			} else{
				GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
				return -1;
			}
		}
		GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
		return -1;
		
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
	GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
	return -1;
}

// Add any player to the game. Mallocs memory and initializes all of the player's information.
void addPlayer(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t *addr){
	//assuming that everything is verified.
	char logText[1000];

	//only doing the FA side because I dont know what all of fields of the GA player struct are
	if ((strcmp(messageArray[0], "FA_LOCATION")) == 0){
		FAPlayer_t *newFA;
		receiverAddr_t *addrFA;
		if ((newFA = malloc(sizeof(FAPlayer_t))) == NULL) exit(11);
		if ((newFA->PlayerName = malloc((strlen(messageArray[4]) + 1))) == NULL) exit(11);
		if ((newFA->TeamName = malloc((strlen(messageArray[3]) + 1))) == NULL) exit(11);
		if ((addrFA = malloc(sizeof(receiverAddr_t))) == NULL) exit(11);

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
		newFA->capturingTime = 0;
		char *tempHashCode;
		if ((tempHashCode = malloc(5)) == NULL) exit(11);
		strcpy(tempHashCode, "P00P");
		newFA->capturingHexCode = tempHashCode;
		sprintf(logText, "Field Agent %s was added to team %s", messageArray[4], messageArray[3]);
		logger(allGameInfo->game->fp, logText);
		hashtable_insert(allGameInfo->FA, messageArray[2], newFA);
	} else {
		GAPlayer_t *newGA;
		receiverAddr_t *addrGA;
		if ((newGA = malloc(sizeof(GAPlayer_t))) == NULL) exit(11);
		if ((newGA->PlayerName = malloc((strlen(messageArray[4]) + 1))) == NULL) exit(11);
		if ((newGA->TeamName = malloc((strlen(messageArray[3]) + 1))) == NULL) exit(11);
		if ((addrGA = malloc(sizeof(receiverAddr_t))) == NULL) exit(11);

		addrGA->port = addr->port;
		addrGA->inaddr = addr->inaddr;
		addrGA->sin_family = addr->sin_family;

		strcpy(newGA->PlayerName, messageArray[4]);
		strcpy(newGA->TeamName, messageArray[3]);
		newGA->status = 0; //default is 0 (active)
		newGA->lastContact = time(NULL);
		newGA->addr = addrGA;
		sprintf(logText, "Guide Agent %s was added to team %s", messageArray[4], messageArray[3]);
		logger(allGameInfo->game->fp, logText);
		hashtable_insert(allGameInfo->GA, messageArray[2], newGA);
	}
}

// Verifies that given OPCODEs are of the correct length (4 hexDigits) to neutralize code
// Also checks to see if the input HexCodes match any in the master DropCode hashtable
bool codeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo){
	//check for a valid Hex Code
	if (strlen(messageArray[7]) != 4){
		// error message already sent inside of ValidateHexCode
		printf("Please insert a valid HexCode of 4 digits\n");
		return false;
	} else {

		int existsFlag = 0;

		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);


		utility_p->param1 = &existsFlag; //existsFlag
		utility_p->param2 =  messageArray[7]; //hexString
		
		hash_iterate(allGameInfo->CD, existingHexCodeIterator, utility_p);

		if ( existsFlag == 1){
			free(utility_p);
			return true;
		} else {
			free(utility_p);
			return false;
		}
	}
}

// Specific to the GA_HINT function. Verifies that the given HexCode is either a "*" or a 
// valid hexCode that matches an FA. 
bool messageCodeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo){
	//check for a valid Hex Code
	if ((strcmp("*", messageArray[5])) == 0){
		return true;
	} else if (strlen(messageArray[5]) != 4){
		// error message already sent inside of ValidateHexCode
		printf("Please insert a valid HexCode of 4 digits\n");
		return false;
	} else {
		int existsFlag = 0;

		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

		utility_p->param1 = &existsFlag; //existsFlag
		utility_p->param2 =  messageArray[5]; //hexString
		
		hash_iterate(allGameInfo->FA, existingHexCodeIterator, utility_p);

		if ( existsFlag == 1){
			free(utility_p);
			return true;
		} else {
			free(utility_p);
			return false;
		}
	}
}

// handles the appropriate actions to neutralize a dropcode. 
// Validates the message, neutralizes the dropCode, updates any relevant information.
void FA_NEUTRALIZE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	char logText[1000];
	
	//Validate the number of arguments in the message
	if (arraySize != 8){
		GSResponseHandler(allGameInfo, playerAddr, "you need 8 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}

	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	
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

	//Update the CodeDrop's Information.
	code_drop_t *foundCode = hashtable_find(allGameInfo->CD, messageArray[7]); 
	if (foundCode->status == 0){
		double distance = dist(atof(messageArray[5]), atof(messageArray[6]), foundCode->lng, foundCode->lat);
		if (distance < 10) {
			foundCode->status = 1;
			strcpy(foundCode->team, messageArray[3]);
			allGameInfo->game->deadDropRemaining -= 1;
			sprintf(logText, "Field Agent %s neutralized codeDrop %s", messageArray[4], messageArray[7]);
			logger(allGameInfo->game->fp, logText);
			GSResponseHandler(allGameInfo, playerAddr, "Congratulations! You've neutralized a piece of code!", "MI_NEUTRALIZED");
		} else {
			return;
		}
	} else {
		return;
	}

	//update the FA's Neutralization number
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	currentFA->Neutralized = currentFA->Neutralized + 1; 
}

// handles the appropriate actions for FA_CAPTURE OPCODES.
// handles both the case of initiating a capture or completing a capture.
void FA_CAPTURE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	char logText[1000];

	//Validate the number of arguments in the message
	if (arraySize != 6){
		GSResponseHandler(allGameInfo, playerAddr, "you need 6 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;

	//To initialize the capture, we read 0, then create a hashtable of players close enough to capture
	//Then we send each player in that hashtable an individualized message with a unique random hashcode.
	if ((strcmp(messageArray[5], "0") )== 0) {

		// setting up for iteration
		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);
		
		hashtable_t *closePlayers = hashtable_new(1, deleteTempHash, NULL);

		FAPlayer_t *capturingPlayer = hashtable_find(allGameInfo->FA, messageArray[2]);

		// passing the capturing player's lat, long, and TeamName as well as the hashtable we will send messages with
		utility_p->param1 = &(capturingPlayer->lat);
		utility_p->param2 = &(capturingPlayer->lng);
		utility_p->param3 = closePlayers;
		utility_p->param4 = capturingPlayer->TeamName;

		// fill the Close Players hashtable
		hash_iterate(allGameInfo->FA, captureProximityIterator, utility_p);

		// send the appropriate message each player in the hashtable of close players
		GS_CAPTURE_IDHandler(allGameInfo, closePlayers);

		//clean up
		hashtable_delete(closePlayers);
		free(utility_p);

	//this is the instance that the capturing player is finishing the capture by submitting another player's randomly assigned hex
	} else {

		//prepare the iterator
		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

		//int i is a flag to see if the input hashcode is valid. 0 if not found, 1 if found.
		int i = 0;

		utility_p->param1 = messageArray[5];
		utility_p->param3 = &i; //capturedPlayer

		//search for the players whose hashcode was submitted
		hash_iterate(allGameInfo->FA, findCapturedPlayer, utility_p);

		//This is player who was captured.
		FAPlayer_t *capturedPlayer = (FAPlayer_t *) utility_p->param2;

		// If the capture was successful, issue the following messages and update statstics.
		if (i != 0){
			GSResponseHandler(allGameInfo, capturedPlayer->addr, "You've been captuerd", "MI_CAPTURED");

			FAPlayer_t *capturingPlayer = hashtable_find(allGameInfo->FA, messageArray[2]);

			capturingPlayer->capturedPlayers = capturingPlayer->capturedPlayers + 1;
			GSResponseHandler(allGameInfo, playerAddr, "Congratulations! Your capture was successful!", "MI_CAPTURE_SUCCESS");

			//logging
			sprintf(logText, "Field Agent %s from team %s was captured", capturedPlayer->PlayerName, capturedPlayer->TeamName);
			logger(allGameInfo->game->fp, logText);
		}
		// clean up
		free(utility_p);
	}
}

// Creates ands Sends the appropriate response to the GA_STATUS OPCODE as well as updating the 
// relevant information.
void GA_STATUS_handler(hashStruct_t *allGameInfo,char **messageArray, int arraySize,receiverAddr_t * playerAddr){
	//returns a message to the sender
	
	int statusFlag;

	if (arraySize != 6){
		GSResponseHandler(allGameInfo, playerAddr, "you need 6 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}

	if (! validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;

	if ((statusFlag = statusReqHandler(messageArray,playerAddr, allGameInfo)) == -1) return;

	// if they are here, they exist, in our hashtable --> update their info
	// update the GA's information
	GAPlayer_t *currentGA = hashtable_find(allGameInfo->GA, messageArray[2]);

	free(currentGA->PlayerName);
	if ((currentGA->PlayerName = malloc(strlen(messageArray[4]) +1)) == NULL) exit(11);
	strcpy(currentGA->PlayerName, messageArray[4]);

	currentGA->lastContact = time(NULL);
	//Update Player's Address in case of disconnection.
	
	currentGA->addr->port = playerAddr->port;
	currentGA->addr->inaddr = playerAddr->inaddr;
	currentGA->addr->sin_family = playerAddr->sin_family;



	if (statusFlag == 1 && allGameInfo->game->level == 1){
		//send a message back.
		GAPlayerGameStatusHandler(allGameInfo, currentGA);
	} else if (statusFlag == 1 && allGameInfo->game->level == 3){
		GAPlayerGameStatusHandlerThree(allGameInfo, currentGA);
	}
}

// Handles the GA_HINT OPCODE messages. Sends messages to either one or all of a team. 
void GA_HINT_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	//Validate given Arguments
	if (arraySize != 7){
		GSResponseHandler(allGameInfo, playerAddr, "you need 7 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	if (! validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	if (messageCodeIDHandler(messageArray, playerAddr, allGameInfo)) {
		printf("Inputed HexCode exists\n");
	} else {
		printf("Inputed HexCode does not exist, ignoring message\n");
		return;
	}

	//The design is to make a hashtable of those players who are to recieve a message (either one person or an entire team)
	//Create a message for each person in that hashtable.
	//Send the indiviualized message to each member of the hashtable
	//Delete the hashtable.

	//Create hashtable
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	//Populate the hashtable with the team
	if ((strcmp("*", messageArray[5])) == 0){

		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

		// Iterate through the hashtable of all FA's and add only those who are on a given team into tempHash.
		utility_p->param1 = messageArray[3]; //TeamName
		utility_p->param2 = tempHash;
		
		hash_iterate(allGameInfo->FA, FAMatchingTeam, utility_p);

		// Starting to create an individualized message with the iterator
		utility_t *utility_p2;
		if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

		//create the first part of the message string from the 
		char message1[BUFSIZE];	
		sprintf(message1, "%s|%s|%s|%s|%s", 
				messageArray[1], messageArray[2], messageArray[3], 
				messageArray[4], messageArray[5]);

		// pass the message pieces into an the iterator.
		utility_p2->param1 = allGameInfo;
		utility_p2->param2 = message1;
		utility_p2->param3 = messageArray[6]; //the Hint given by the GA

		//finishing the creation of the message and sending it
		hash_iterate(tempHash, GA_HINTIterator, utility_p2);

		//clean up
		free(utility_p);
		free(utility_p2);

	// For the case of only sending to one player
	} else {
		// Find the recieving FA player
		FAPlayer_t *FAPlayer_p = hashtable_find(allGameInfo->FA, messageArray[5]);

		//Insert the FA into the 
		hashtable_insert(tempHash, messageArray[5], FAPlayer_p->addr);

		//create the messsage
		char message[BUFSIZE];
		sprintf(message, "%s|%s|%s|%s|%s|%s|%s", 
		messageArray[0], messageArray[1],
		messageArray[2], messageArray[3],
		messageArray[4], messageArray[5],
		messageArray[6]);

		//send to the player
		sending(allGameInfo->game->comm_sock, tempHash, message);
	}

	//clean up
	hashtable_delete(tempHash);
}

// Compiles the game statistics to create and send a message at the end of game.
void GAME_OVER_handler(hashStruct_t *allGameInfo){
	
	char message[BUFSIZE];
	char allTeamInfo[9000]= "\0";
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_t *tempHash2 = hashtable_new(1, deleteTempHash, NULL);
	
	hash_iterate(allGameInfo->FA, FAcopyingAddressIterator, tempHash);
	hash_iterate(allGameInfo->GA, GAcopyingAddressIterator, tempHash);

	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p->param1 = allTeamInfo;
	utility_p->param2 = tempHash2;
	utility_p->param3 = allGameInfo;
	
	hash_iterate(allGameInfo->FA, gameOverIterator, utility_p);
	
	
	sprintf(message, "GAME_OVER|%s|%d|%s",allGameInfo->game->gameID, allGameInfo->game->deadDropRemaining, allTeamInfo);
	
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	sending(allGameInfo->game->comm_sock, tempHash, message);
	
	hashtable_delete(tempHash);
	hashtable_delete(tempHash2);
	free(utility_p);
	
}

// helper iterator that copies the sending addresses of FAPlayers in one hashtable into another hashtable.
void FAcopyingAddressIterator(void *key, void* data, void* farg){
	hashtable_t *hash = (hashtable_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	hashtable_insert(hash, (char*) key, FAPlayer_p->addr);
}

// helper iterator that copies the sending address of GAPlayers in one hashtable into another one.
void GAcopyingAddressIterator(void *key, void* data, void* farg){
	hashtable_t *hash = (hashtable_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;
	hashtable_insert(hash, (char*) key, GAPlayer_p->addr);
}

// The iterator that compiles game statistics from each FAPlayer at the end of the game.
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
		
		utility_t *utility_p2;
		if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

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

// returns the number of FAPlayers in a given hashtable
void numberOfPlayersIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += 1;
	}
}

// returns the number of GAPlayers in a given hashtable
void numberCapturingIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += FAPlayer_p->capturedPlayers;
	}
}

// counts the total number of neutralized codes by summing each FAPlayer's neutralized counts.
void numberNeutralizedIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
			*(int *) utility_p->param2 += FAPlayer_p->Neutralized;
	}
}

//counts the total number of captured players by summing each of the FAPlayer's captured-player counts in a given hashtable.
void numberCapturedIterator(void* key, void* data, void* farg) {
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param1;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
		if (FAPlayer_p->status == 1)
			*(int *) utility_p->param2 += 1;
	}
}

// a helper function that removes the data from a temperary hashtable with no malloc'd memory
void deleteTempHash(void *data){
	if (data){	//if valid
	}
}

// function that translates the input message into a messageArray.
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

/***** debugging *****/
void printArray(char** array, int size){
	for(int i = 0; i < size ; i++){ //looping through the query array
		printf("Array[%d]: %s\n", (i+1), array[i]);
	}
}
/***** debugging *****/

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

//Creates a random HexCode by getting a random intiger and converting it to hexidecimal format.
char *makeRandomHex(){
	int r = rand() %65535;
	char * hexString;
	if ((hexString = malloc(10)) == NULL) exit(11);
	sprintf(hexString, "%X", r);
	return hexString;
}

//Deletes the toplevel datastructure. 
void deleteHashStruct(hashStruct_t *allGameInfo){
	hashtable_delete(allGameInfo->CD);
	hashtable_delete(allGameInfo->GA);
	hashtable_delete(allGameInfo->FA);
	free(allGameInfo->game->gameID);
	free(allGameInfo->game);
	free(allGameInfo);
}

// Searches a hashtable that has GAPlayer_t* data for any player that has a given TeamName.
void GAMatchingTeam(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(GAPlayer_p->TeamName, name)) == 0){
		*(int *) utility_p->param1 = 1;
	}
}

// Searches a hashtable that has FAPlayer_t* data for any player that has a given TeamName.
void FAMatchingTeam(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;
	char *hexKey = (char *) key;

	char *name = (char *) utility_p->param1;
	hashtable_t *hash = (hashtable_t *) utility_p->param2;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
		hashtable_insert(hash, hexKey, FAPlayer_p->addr);
	}
}

// Searches a hashtable that has GAPlayer_t* data for any player that has a given GuideID.
void GAguideIDIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(GAPlayer_p->TeamName, name)) == 0){
		utility_p->param2 = (char*) key;
	}
}

// Helper iterator that counts the number of remaining players on a given team in a hashtable with FAPlayer_t* data
void RemainingOperativesIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->TeamName, name)) == 0){
		if (FAPlayer_p->status == 0)
			*(int *) utility_p->param1 += 1;
	}
}

// Helper iterator that counts the number of remaining players not on a given team in a hashtable with FAPlayer_t* data
void RemainingFoeIterator(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->TeamName, name)) != 0){
		if (FAPlayer_p->status == 0)
			*(int *) utility_p->param1 += 1;
	}
}

// Searches a hashtable that has FAPlayer_t* data for any player that has a given PlayerName.
void FAPlayerNamesIterator(void *key, void* data, void* farg){  //PLAYER NAME
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(FAPlayer_p->PlayerName, name)) == 0){
		*(int *) utility_p->param1 = 1;
	}
}

// Searches a hashtable for the existence of a given hashcode. Used to find matching hexCodes at codeDrops.
void existingHexCodeIterator(void *key, void* data, void* farg){
	utility_t *utility_p = (utility_t *) farg;
	char *code_drop_key = (char *) key;

	char *hexString = (char *) utility_p->param2;

	if ((strcmp(hexString, code_drop_key)) == 0){
		*(int*) utility_p->param1 = 1;
	}
}

// adds all of the players inside of a 10m radius to a hashtable. Used for determining eligable players to capture.
void captureProximityIterator(void *key, void* data, void* farg){ 
	//renaming the given arguments
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	//Interpreting the passed information through utility
	double lat = *(double *) utility_p->param1;
	double lng = *(double *) utility_p->param2;
	hashtable_t *hash = (hashtable_t *) utility_p->param3;
	char *TeamName = (char *) utility_p->param4;

	//calculating the distance between players
	double distance = dist(lat, lng, FAPlayer_p->lat, FAPlayer_p->lng);

	// if they are not on the same team and within 10m, assign them a random hex code and add them to the hashtable
	if ((strcmp(TeamName, FAPlayer_p->TeamName)) != 0){
		if (distance < 10){
			if (FAPlayer_p->status != 1){
				FAPlayer_p->status = 2;
				free(FAPlayer_p->capturingHexCode);

				FAPlayer_p->capturingHexCode = makeRandomHex();
				FAPlayer_p->capturingTime = time(NULL);
				hashtable_insert(hash, FAPlayer_p->PlayerName, FAPlayer_p);
			}
		}
	}
}

// Creates and sends an individualized message for each member of a team whose guideAgent sent a hint.
void GA_HINTIterator(void *key, void* data, void* farg){
	//renaming the given arguments
	utility_t *utility_p = (utility_t *) farg;
	receiverAddr_t *addr = (receiverAddr_t *) data;
	char *PlayerID = (char *) key;

	//Interpreting the passed information through utility
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param1;
	char *inMessage1= (char *) utility_p->param2;
	char *inMessage2 = (char *) utility_p->param3;

	//creating the individualized message
	char message[BUFSIZE];	
	sprintf(message, "%s|%s|%s", 
		inMessage1, PlayerID, inMessage2);

	//create a hashtable of recipients for sending purposes (it is a singleton hashtable since the message is individualized)
	hashtable_t *hash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(hash, PlayerID, addr); //the key doesnt matter
	
	//conplete the send
	sending(allGameInfo->game->comm_sock, hash, message);
		
	hashtable_delete(hash);
}

// Sends a GS_CAPTURE message to all of those who are in a hashtable with each player's individualized OPCODE.
void GS_CAPTURE_IDIterator(void *key, void* data, void* farg){
	//Renaming the arguments
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;

	//Interpreting the passed information through utility
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param1;
	char *inMessage = (char *) utility_p->param2;

	//creating the personalized messages
	char message[BUFSIZE];	
	sprintf(message, "%s|%s", inMessage, FAPlayer_p->capturingHexCode);
	printf("%s\n", message);

	//creating a new hashcode for sending purposes
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
	hashtable_insert(tempHash, FAPlayer_p->PlayerName, FAPlayer_p->addr); 
	
	//completing the send
	sending(allGameInfo->game->comm_sock, tempHash, message);
	
	//clean up
	hashtable_delete(tempHash);
}

// finds a player with a given temporaryCaptureHexCode in order to mark them as captured
void findCapturedPlayer(void *key, void* data, void* farg){
	utility_t *utility_p = (utility_t *) farg;
	FAPlayer_t *FAPlayer_p = (FAPlayer_t *) data;


	char *givenCode = (char *) utility_p->param1;

	if (*(int*) utility_p->param3 == 0){
		if ((strcmp(FAPlayer_p->capturingHexCode, givenCode)) == 0){
			*(int*) utility_p->param3 = 1;
			FAPlayer_t *capturedPlayer = FAPlayer_p;
			capturedPlayer->status = 1;
			utility_p->param2 = capturedPlayer;
		}
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

void logger(FILE *fp, char* message){
	time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    fprintf(fp, "%s                             %s\n\n",asctime( localtime(&ltime) ), message );
}
