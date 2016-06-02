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

#include <ncurses.h>
#include <unistd.h>

//to create a list, our list structures had some bags
// we are using one row though in the hashtable to create a one dimentional list
#include "hashtable/hashtable.h"


/************* Function Declarations ****************/

// for the command options (optand)
const struct option longOpts[] = {
	{ "level", required_argument, 0, 'l'}, //level of the game
	{ "time", required_argument, 0, 't'}, //duration of the game
	{ "log", required_argument, 0, 'r'}, // for logging
	{ "game", required_argument, 0, 'g'}, // for gameID
	{ 0, 0, 0, 0}
};

// game statistics and important flags
typedef struct game {
	char * gameID; 
	int level; //level 3 flag
	int ascii; //ascii display
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
void numberOfcodeDrops(void* key, void* data, void* farg);
void GAMatchingTeam(void *key, void* data, void* farg);
void FAPlayerNamesIterator(void *key, void* data, void* farg);
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
void printingDropCode(void *key, void* data, void* farg);
void printingPlayer(void *key, void* data, void* farg);
double getYDimension(double num, double max_y);
double getXDimension(double num, double max_x);
double dist(double th1, double ph1, double th2, double ph2);
char *makeRandomHex();
void asciiDrawing(hashStruct_t *allGameInf);
void delete(void *data);
void freeArray(char** array, int size);
void deleteHashStruct(hashStruct_t *allGameInfo);
void deleteTempHash(void *data);
void deleteFAPlayer(void *data);
void deleteGAPlayer(void *data);
void deleteCodeDrop(void *data);

int main(int argc, char *argv[]){
	
	srand(time(NULL)); //seed for random hex
	
	hashStruct_t *allGameInfo; //creating memory of for struct that contains all the info
	if ((allGameInfo = malloc(sizeof(hashStruct_t))) == NULL) exit(11);
	
	hashtable_t *GAPlayers; // a list for all the guide agents in the game
	if ((GAPlayers = hashtable_new(1, deleteGAPlayer, NULL)) == NULL) exit(11);
	
	hashtable_t *FAPlayers; // a list for all the field agent in the game
	if ((FAPlayers = hashtable_new(1, deleteFAPlayer, NULL)) == NULL) exit(11);
	
	hashtable_t *codeDrop; // a list for all the code drops in the game
	if ((codeDrop= hashtable_new(1, deleteCodeDrop, NULL)) == NULL) exit(11);

	game_t *game_p; // statistics struct
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
	game_p->ascii = 0;
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
		printf("Usage: ./gameserver [--log] [--level = 1 or 3] [--time (in Minutes) "
		"[-a (ascii display)] codeDropPath GSport\n");
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
		printf("The game cannot start, check your conenction or memory\n");
		deleteHashStruct(allGameInfo);
		exit(5);
	}
	
	//Successfully exiting
	deleteHashStruct(allGameInfo);
	exit(0);
}

// a function that runs the whole game
bool game_server (char *argv[], hashStruct_t *allGameInfo) {
	
	int comm_sock; //socket info
	
	char logText[1000]; //creating the message for logging
	logText[999] = '\0';
	char logText2[1000];
	char *arg1 = argv[optind]; //code drop file path 
	
	//if error with loading the codes from file
	if (!load_codeDrop(allGameInfo->CD, arg1)) return false;
	
	//checking how many codes are left
	hash_iterate(allGameInfo->CD, numberOfcodeDrops, allGameInfo->game);

	//setting up the socket
	if ((comm_sock = socket_setup(allGameInfo->game->GSPort)) == -1) return false;
	allGameInfo->game->comm_sock = comm_sock;

	//creaing a log file with a unique ID
    sprintf(logText, "log/gameserver%ld.log", time(NULL) );
	
	// opening the file with an appending option
	if ((allGameInfo->game->fp=fopen(logText, "a")) == NULL) {
		printf("You need to create a log directory\n");
		return false;
	}
	logger(allGameInfo->game->fp, "Game Started");
	printf ("Game Started\n");
	
	//logging
	gethostname(logText, 999); // getting host name
	sprintf(logText2, "Host: %s", logText);
	printf ("%s\n", logText2);
	logger(allGameInfo->game->fp, logText2);
	sprintf(logText2, "Port: %d", allGameInfo->game->GSPort);
	printf ("%s\n", logText2);
	logger(allGameInfo->game->fp, logText2);
	
	// Receive datagrams, process them, create a response or ignore, send it back
	// loop exits on EOF from stdin
	while (true) {  
	
		 // set of file descriptors we want to read
		fd_set rfds;       
		
		//calculating elapsed time
		allGameInfo->game->elapsedTime = time(NULL) - allGameInfo->game->startingTime;
		
		//ending the game if time is up or remaining code drops are zero
		if (allGameInfo->game->elapsedTime >= 
			allGameInfo->game->timeVal && allGameInfo->game->timeVal != 0) break;
		else if (allGameInfo->game->deadDropRemaining == 0) break;
		
		// if the ascii option is chosen, draw the campus map
		if (allGameInfo->game->ascii == 1) asciiDrawing(allGameInfo);

		// Watch stdin (fd 0) and the UDP socket to see when either has input.
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);       // stdin
		FD_SET(comm_sock, &rfds); // the UDP socket
		int nfds = comm_sock+1;   // highest-numbered fd in rfds
		struct timeval timeout;   // how long we're willing to wait
		const struct timeval t = {0,5000};   // five millseconds
		timeout = t;
		int select_response = select(nfds, &rfds, NULL, NULL, &timeout);
    
		if (select_response < 0) {
			// some error occurred
			close(comm_sock);
			return false;
			
    
		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)){
				if (!handle_stdin()) //check if the user wants to end the game
					break;
			} 

			if (FD_ISSET(comm_sock, &rfds)) { // response to the message received
				handle_socket(allGameInfo);
			}
			fflush(stdout);
		}
	}
	//sending end game message and cleaning up used memory
	GAME_OVER_handler(allGameInfo);
	printf ("Game Over\n");
	logger(allGameInfo->game->fp,"Game Over");
	close(comm_sock);
	fclose(allGameInfo->game->fp);
	return true;
}

// verifying the optional flags
bool verifyFlags(int argc, char *argv[], game_t *game_p){
	
	int option = 0;
	int tempInt; 
	
	while  ((option = getopt_long (argc, argv, "+l:t:rg:a", longOpts, NULL)) != -1){
		switch (option){
			case 'l' : //level
				if (!isItValidInt(optarg)){ //check if a number was provided
					printf("Level Flag is invalid\n");
					return false;
				} 
				tempInt = atoi(optarg); //convert it into integer
				
				if (tempInt > 0 && tempInt < 4){ //level 1 and 3 are supported
					if (tempInt == 2){
						printf("I'm sorry. Currently level 2 is not supported.\n");
						return false;
					} else {
						game_p->level = tempInt;
						break;
					}
 				} else {
 					printf("Level %d is not supported\n", tempInt);
					return false;
 				}
				
			case 't': //to set a time for the game
				if (!isItValidInt(optarg)){ //check if a number was provided
					printf("Time Flag is invalid\n");
					return false;
				} 
				tempInt = atoi(optarg);
				
				if (tempInt >= 0){ //convert it into minutes
					game_p->timeVal = tempInt * 60;
					break;
 				} else {
 					printf("Time Argument is not in range\n");
					return false;
				}

			case 'r': //for raw logging
				if (strcmp(optarg, "raw") == 0) {
				game_p->rawlogFl = 1;
				break;
				}
				printf("Wrong flag argument\n");
				return false;
			
			case 'a': //ascii map
				game_p->ascii = 1;
				break;

			case 'g':
				//verify that the input is 8 hex maximum
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


// to create the campus map using ascii art library
void asciiDrawing(hashStruct_t *allGameInf){		
		
		initscr();
		noecho();
		curs_set(FALSE);
		
		start_color(); //white color
		init_pair(1, COLOR_WHITE,     COLOR_BLACK);
		attrset(COLOR_PAIR(1));
		
		//drawing the main locations on campus
		mvprintw(5, 95, "--------------------------");
		mvprintw(6, 95, "@@@MISSION INCOMPUTABLE@@@");
		mvprintw(7, 95, "--------------------------");
		mvprintw(40, 105, "Library");
		mvprintw(54, 105, "Green");
		mvprintw(61, 98, "Hanover");
		mvprintw(45, 0, "Thayer");
		mvprintw(3, 170, "LSC");
		mvprintw(49, 192, "EW");
		
		//placing the dead drops and players
		hash_iterate(allGameInf->CD, printingDropCode, NULL);
		hash_iterate(allGameInf->FA, printingPlayer, NULL);	
		
		sleep(1); //refress every one second
		clear();
		endwin(); // Restore normal terminal behavior
}

// printing remaining dead drops
void printingDropCode(void *key, void* data, void* farg) {
	
	code_drop_t* code_drop_p = (code_drop_t*) data;
	
	init_pair(2, COLOR_YELLOW,     COLOR_BLACK); //color yellow
	attrset(COLOR_PAIR(2));
	
	if (code_drop_p->status == 0 ) { //if it is not neutralize, print it
		mvprintw(getYDimension(code_drop_p->lng, 0), getXDimension(code_drop_p->lat, 0), "X %s", (char*) key);
		refresh();
	}
}

// printing active players
void printingPlayer(void *key, void* data, void* farg) {
	
	FAPlayer_t* FAPlayer_p = (FAPlayer_t*) data;
	
	init_pair(3, COLOR_GREEN,   COLOR_BLACK); //setting colors
	init_pair(4, COLOR_RED,     COLOR_BLACK);
	init_pair(5, COLOR_BLUE,    COLOR_BLACK);
	
	if (FAPlayer_p->status == 0 ) { // if still active, green
		attrset(COLOR_PAIR(3));
		mvprintw(getYDimension(FAPlayer_p->lat, 0), getXDimension(FAPlayer_p->lng, 0), "O %s", FAPlayer_p->PlayerName);
		refresh();
	} else if (FAPlayer_p->status == 2 ){// if being captured, red
		attrset(COLOR_PAIR(4));
		mvprintw(getYDimension(FAPlayer_p->lat, 0), getXDimension(FAPlayer_p->lng, 0), "O %s", FAPlayer_p->PlayerName);
		refresh();
	} else if (FAPlayer_p->status == 3 ){// if capturing, blue
		attrset(COLOR_PAIR(5));
		mvprintw(getYDimension(FAPlayer_p->lat, 0), getXDimension(FAPlayer_p->lng, 0), "O %s", FAPlayer_p->PlayerName);
		refresh();
	}
}

// convert it into screen pixels (y dimension)
double getYDimension(double num, double max_y){
	return ((11000) *((43.71267477-num)/(43.71267477 - 41.70073945)));
}

// convert it into screen pixels (x dimension)
double getXDimension(double num, double max_x){
	return ((209) *((-72.29492692-(num))/(-72.29492692 + 72.28279)));
}

// checking and storing if the port number provided as an argument is valid
bool verifyPort(int argc, char *argv[], game_t *game_p){
	
	int tempInt;
	char *arg2 = argv[optind+1];
	
	if (!isItValidInt(arg2)){
		printf("GSPort Argument is invalid\n");
		return false;
	}
	tempInt = atoi(arg2);
	
	// it accepts port numebr up to 100000
	if (tempInt < 0 || tempInt > 100000){
		printf("GSPort Argument is out of range\n");
		return false;
	}
	game_p->GSPort = tempInt;
	return true;
}

//verifying that the codeDrop file provided is valid and readable
bool verifyFile(int argc, char *argv[]){
	
	int status;
	char *arg1 = argv[optind];
 	struct stat buf;
	
	status = stat(arg1, &buf);
	
	if (status != 0) {
	    fprintf(stderr, "There is an error in processing %s\n", arg1);
	    return false;
	} else {
		if (!S_ISREG(buf.st_mode)) {
        	printf("%s is a not regular file.\n", arg1);
	    	return false;
    	}
	 	if (access(arg1, R_OK) == -1) {
	    	printf("%s is not readable.\n", arg1);
	    	return false;
	    }
	}
	return true;
}

//verify if the integer provide is in a valid format
bool isItValidInt(char *intNumber){

	int validInt = 0;
	char * isDigit;
	
	if (((isDigit = malloc(strlen(intNumber) +1))) == NULL) exit(11); 
	
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
			free(code_drop); //free everything and return false if it fails
			free(line); 
	    	fclose(readFrom);
			return false;
		}
		
	   	free(line); //free the line before returning to the loop for a new line
	} 
	fclose(readFrom); 	// close the code drop path file after reading its contents
	return true; // nothing went wrong
}


bool load_codeDropPath(code_drop_t * code_drop, char* line, char* token, hashtable_t* codeDropHash){
	
	//parse lat
	if ((token = strtok(line, ", ")) == NULL) return false;	
	
	// if valid float, store it
	if (!isItValidFloat(token)) return false;
	code_drop -> lng = atof(token);
		
	// parse long
	if ((token = strtok(NULL, ", ")) == NULL) return false;
	
	// if valid float, store it
	if (!isItValidFloat(token)) return false;
	code_drop -> lat = atof(token);
	
	// parse the hexcode 
	if ((token = strtok(NULL, ", ")) == NULL) return false;
	int i = strlen(token);
	if(i == 5) token[i-1] = '\0';
	
	// if it fails to insert, that suggests duplicate (not acceptable ) or memory allocation error
	if (!hashtable_insert(codeDropHash, token, code_drop)) return false;
	return true;
}

//Frees memory found in the data category of a hashtable
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

//This was directly taken from chatserver1.c and slightly modified.
int socket_setup(int GSPort) {
  // Create socket on which to listen (file descriptor)
  int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (comm_sock < 0) {
    printf("opening datagram socket\n");
    return -1;
  }

  // Name socket using wildcards
  struct sockaddr_in server;  // server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(GSPort);
  if (bind(comm_sock, (struct sockaddr *) &server, sizeof(server))) {
    printf("binding socket name\n");
    return -1; 
  }
  return (comm_sock);
}

//Allows the server to interpret standard in. Useful for ending the game with a quit message.
//All other input, it prints to the console and otherwise ignores
bool handle_stdin(){
	
	char *terminalResponse = readline(stdin); // read the input
	
	// if it is EOF or quit,  the game will be terminated
	if (terminalResponse == NULL || strcmp(terminalResponse, "quit") == 0){
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
	
	// stores the receivers' addresses and a message and call the sender function
	if ((sendingInfo_p = malloc(sizeof(sendingInfo_t))) == NULL) exit(11);
	sendingInfo_p->comm_sock = comm_sock;
	sendingInfo_p->message = message;
	hash_iterate(tempHash, sendIterator, sendingInfo_p);

	free(sendingInfo_p);
}

// Iterates through the singleton hashtable from `sending` and does the technical work of sending.
// This was modified from a function found in chatserver.c 
void sendIterator(void* key, void* data, void* farg) {

	sendingInfo_t *sendingInfo_p = (sendingInfo_t *) farg;
	receiverAddr_t *recieverp = (receiverAddr_t *) data;
	
	//retrieve each receiver's address
	struct sockaddr_in sender;    
	sender.sin_port = recieverp->port;
	sender.sin_addr = recieverp->inaddr;
	sender.sin_family = recieverp->sin_family;

	struct sockaddr_in them = {0,0,{0}};
	struct sockaddr_in *themp = &them;
    *themp = sender;
	
	// send to each targeted user
	if (sendto(sendingInfo_p->comm_sock, sendingInfo_p->message, strlen(sendingInfo_p->message), 
		0, (struct sockaddr *) themp, sizeof(*themp)) < 0){
    printf("It failed to send the datagram\n");
	}
}

// checks the socket for messages and passes them off to `parsing`
// copied from Prof. Kotz's notes and modified
void handle_socket(hashStruct_t *allGameInfo){
	
	char buf[BUFSIZE];        // buffer for reading data from socket
   
   // socket has input ready
	struct sockaddr_in sender;     // sender of this message
	struct sockaddr *senderp = (struct sockaddr *) &sender;
	socklen_t senderlen = sizeof(sender);  // must pass address to length
	
	int nbytes = recvfrom(allGameInfo->game->comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
	
	if (nbytes <= 0) {
	    printf("Empty or improper message was recieved\n");
	    return ;
	
	} else {      
	    buf[nbytes] = '\0';     // null terminate string

	    // where was it from?
	    if (sender.sin_family == AF_INET) {

	        // saving the current address of the sender 
	        receiverAddr_t *currentAddr;
	        if ((currentAddr = malloc(sizeof(receiverAddr_t)))==NULL) exit(11);  
	        currentAddr->port = sender.sin_port;
	        currentAddr->inaddr = sender.sin_addr;
			currentAddr->sin_family = sender.sin_family;

		    //logging
			printf("[%s@%d]: %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buf);
			if (allGameInfo->game->rawlogFl == 1)
				logger(allGameInfo->game->fp, buf);
			
			// Create an array the save all the components of the message
			char ** messageArray;
			if((messageArray = malloc(8 *((strlen(buf)+1)/2 + 1) )) ==NULL) exit(11); 
			
			//parse the message into its components and save the total number
			int count = parsingMessages(buf, messageArray);
			//process the message
			processing(allGameInfo, messageArray, count, currentAddr);

			
			free(currentAddr);
			freeArray(messageArray, count);
	    }
		fflush(stdout);
	}
}

// directs the message (in the form of a messageArray) to the appropriate message handler according to OPCODE
bool processing(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	
	//Comparing the first argument in the array with the standard OPCODEs and call the appropriate handler
	if(strcmp(messageArray[0], "FA_LOCATION") == 0){
		FA_LOCATION_handler(allGameInfo, messageArray, arraySize, playerAddr);
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
		// send an error message to the user
		GSResponseHandler(allGameInfo, playerAddr, "Please enter a proper OPCODE",  "MI_ERROR_INVALID_OPCODE");
		return false;
	}
	return false;
}

// since all OPCODEs that the Server recieves have the same first five fields, this is a function that checks indexes
// one through 4. It does not check the opcode other than to determine whether a player should be added.
bool validateArgumentsOneThroughFour(hashStruct_t *allGameInfo, char** messageArray, receiverAddr_t *playerAddr){
	
	int gameIDFlag = gameIDHandler(allGameInfo, messageArray, playerAddr); //validate teh gameID
	int playerIDFlag = playerIDHandler(allGameInfo, messageArray, playerAddr); //validate teh pebbleID or guideID

	if (!teamNameHandler(allGameInfo, messageArray, playerAddr)) return false; //validate the team name
	
	//if the message is a status update...
	if (((strcmp(messageArray[0], "FA_LOCATION")) == 0) || ((strcmp(messageArray[0], "GA_STATUS")) == 0)){
		
		if (gameIDFlag == -1){ //invalid gameID
			return false;
		} else if(playerIDFlag == -1){ //invalid pebbleID or guideID
			return false;
		} else if (playerIDFlag == 2){ //player is inactive, their message will be ignored
			return false;
		} else if ((gameIDFlag == 1) && (playerIDFlag == 1)){ // new player
			addPlayer(allGameInfo, messageArray, playerAddr); // add the new player
			return true;
		} else if ((gameIDFlag == 1) && (playerIDFlag != 1)){ // the player was added before
			GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
			return false;
		} else if ((gameIDFlag != 1) && (playerIDFlag == 1)){ // a none-registered player sending the wrong messages
			GSResponseHandler(allGameInfo, playerAddr, "This ID is not registered",  "MI_ERROR_INVALID_ID");
			return false;
		} 
		return true;
	}
	else {
		
		if ((gameIDFlag != 0) || (playerIDFlag != 0)){ 
			if (gameIDFlag == 1){ // the player was added before
			GSResponseHandler(allGameInfo, playerAddr, "You were added to this game before",  "MI_ERROR_INVALID_GAME_ID");
			return false;
			} else if (gameIDFlag != 1){ // a none-registered player sending the wrong messages
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
	
	int statusFlag;

	// Checking for the correct size of the messageArray.
	if (arraySize != 8){
		GSResponseHandler(allGameInfo, playerAddr, "you need 8 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	
	//checking the first 4 component of the message
	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	
	//checking the latitude 
	if (!latHandler(messageArray,playerAddr, allGameInfo)) return;
	
	//checking the longitude 
	if (!lngHandler(messageArray,playerAddr, allGameInfo)) return;
	
	//checking the status request flag
	if ((statusFlag = statusReqHandler(messageArray,playerAddr, allGameInfo)) == -1);

	//if they are here, they exist, in our list --> update their info
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	
	//update their name
	free(currentFA->PlayerName);
	if ((currentFA->PlayerName = malloc(strlen(messageArray[4]) +1)) ==NULL) exit(11);
	strcpy(currentFA->PlayerName, messageArray[4]);
	
	//update their location
	currentFA->lat = atof(messageArray[5]);
	currentFA->lng = atof(messageArray[6]);
	
	//Update Last Contact Time
	currentFA->lastContact = time(NULL);
	
	// if capturing time is over and the player was not captured, reset their capturing hexcode and status
	if (currentFA->capturingTime != 0 && time(NULL) - currentFA->capturingTime > 60){
		currentFA->capturingTime = 0;
		free(currentFA->capturingHexCode);
		if ((currentFA->capturingHexCode  = malloc(5)) == NULL) exit(11);
		strcpy(currentFA->capturingHexCode, "P00P");
		currentFA->status = 0;
	}
	
	//Update Player's Address in case of disconnection
	currentFA->addr->port = playerAddr->port;
	currentFA->addr->inaddr = playerAddr->inaddr;
	currentFA->addr->sin_family = playerAddr->sin_family;

	//if they are requesting an update nessage
	if (statusFlag == 1) FAPlayerGameStatusHandler(allGameInfo, currentFA);
}

//Creates the appropriate response for Field Agent's status request
void FAPlayerGameStatusHandler(hashStruct_t *allGameInfo, FAPlayer_t *currentFA){
	
	char message[BUFSIZE]; //logging message
	char* guideID = "0"; //no current guide ID
	int numRemainingFriendlies = 0; 
	int numRemainingFoe = 0;
	
	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p->param1 = guideID;
	utility_p->param2 = currentFA -> TeamName;
	
	// if a guideID exists, retrieve their name
	hash_iterate(allGameInfo->GA, GAguideIDIterator, utility_p);
	
	utility_p->param1 = &numRemainingFriendlies;
	
	//calculate the number of remaining team players
	hash_iterate(allGameInfo->FA, RemainingOperativesIterator, utility_p);
	
	utility_p->param1 = &numRemainingFoe;
	//calculate the number of remaining enemy players
	hash_iterate(allGameInfo->FA, RemainingFoeIterator, utility_p);
	
	// create the GAME_STATUS message
	sprintf(message, "GAME_STATUS|%s|%s|%d|%d|%d",allGameInfo->game->gameID, guideID, allGameInfo->game->deadDropRemaining, numRemainingFriendlies, numRemainingFoe);
	
	// add the receiver address to the list 
	hashtable_t *tempHash;
	if ((tempHash = hashtable_new(1, deleteTempHash, NULL)) == NULL) exit(11);
	hashtable_insert(tempHash, currentFA->PlayerName, currentFA->addr);
	
	//logging
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	
	printf("%s\n", message);
	//send the message back to the user
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

	hashtable_t *FAFriendlyHash;
	if ((FAFriendlyHash= hashtable_new(1, deleteTempHash, NULL)) == NULL) exit(11);
	hashtable_t *FANearEnemyHash;
	if((FANearEnemyHash = hashtable_new(1, deleteTempHash, NULL)) == NULL) exit(11);

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
	hashtable_t *tempHash;
	if ((tempHash= hashtable_new(1, deleteTempHash, NULL)) == NULL) exit (11);
	hashtable_insert(tempHash, "key", currentGA->addr); 
	
	//logging
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	printf("%s\n", message);
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
	
	//get all the player info
	hash_iterate(allGameInfo->FA, GAGameStatusFriendlyIterator, utility_p);
	
	utility_p->param1 = codeDropInfo;
	utility_p->param2 = &y;
	
	//get all the codeDrop info
	hash_iterate(allGameInfo-> CD, CDGameStatusFriendlyIterator, utility_p);
	
	// creaing the game status message
	sprintf(message, "GAME_STATUS|%s|%s|%s",allGameInfo->game->gameID, playerInfo, codeDropInfo);
	
	//sending the message to the requesting GA
	hashtable_t *tempHash; 
	if((tempHash= hashtable_new(1, deleteTempHash, NULL)) == NULL) exit (11);
	hashtable_insert(tempHash, "key", currentGA->addr); //the key doesnt matter
	
	//logging
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	printf("%s\n", message);
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	//clean up
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
	
	char *allPlayers= utility_p->param1; //playerinfo string
	
	long int currenttime = time(NULL); //current time
	
	long int lastContact = currenttime - FAPlayer_p->lastContact; //last contact in seconds
	
	//putting all the info on one string
	sprintf(playerInfo, "%s,%s,%s,%d,%lf,%lf,%ld",(char*) key, FAPlayer_p->TeamName,
		FAPlayer_p->PlayerName, FAPlayer_p->status, FAPlayer_p->lat, FAPlayer_p->lng, lastContact);
	
	//to avoid creating an unnecesssary ":" in the beginning of the array
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
	hashStruct_t *allGameInfo = (hashStruct_t *) utility_p->param3;

	utility_t *utility_p2;
	if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

	utility_p2->param1 = &(FAFriendlyPlayer_p->lat);
	utility_p2->param2 = &(FAFriendlyPlayer_p->lng);
	utility_p2->param3 = FAFriendlyPlayer_p->TeamName; // TeamName
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
	
	char* allCodes= utility_p->param1; // a string for all code drop info

	
	if (code_drop_p->team == NULL) { // if the code drop was not captured, the team of capturing NONE
		free(code_drop_p);
		if((code_drop_p->team = malloc(100)) == NULL) exit(11);
		strcpy(code_drop_p->team, "NONE");
	}
	
	//fill the information into the string
	sprintf(codeDropInfo, "%s,%lf,%lf,%s", (char*) key,code_drop_p->lat,code_drop_p->lng,code_drop_p->team);
	
	//this is here because the formatting is different for the first peice of code.
	if(*(int*)utility_p->param2 == 0) 
		strcat(allCodes, codeDropInfo);
	
	else {
		strcat(allCodes, ":");
		strcat(allCodes, codeDropInfo);
	}
	*(int*)utility_p->param2 += 1;
}

// A function that can be used to send and erro  message either to one reciever 
// or many messages to many recievers in the GSResponse Format
void GSResponseHandler(hashStruct_t *allGameInfo, receiverAddr_t *playerAddr, char* Response, char* respCode){
	
	char message[BUFSIZE]; 
		
	//create the response message
	sprintf(message, "GS_RESPONSE|%s|%s|%s",allGameInfo->game->gameID, respCode, Response);
	
	//logging
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	
	printf("%s\n", message);
	//creating the list of receivers
	hashtable_t *tempHash; 
	if((tempHash= hashtable_new(1, deleteTempHash, NULL)) == NULL) exit (11);
	
	hashtable_insert(tempHash, "key", playerAddr); 
	
	//sending to all the 
	sending(allGameInfo->game->comm_sock, tempHash, message);
		
	hashtable_delete(tempHash);
}

// creates individualized GS_CAPTURE messages to inform players that they are 
// targeted and to give them their unique hash code. Uses a helper iterator.
void GS_CAPTURE_IDHandler(hashStruct_t *allGameInfo, hashtable_t *hash){

	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

	char message[BUFSIZE];	//writing the GS_CAPTURE message 
	sprintf(message, "GS_CAPTURE_ID|%s",allGameInfo->game->gameID);
	
	//logging
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	printf("%s\n", message);

	utility_p->param1 = allGameInfo;
	utility_p->param2 = message;
	
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
	
	if (strlen(messageArray[2]) > 8){ //checking the validity of the hex code
		GSResponseHandler(allGameInfo, playerAddr, "Not a valid hex code",  "MI_ERROR_INVALID_ID");
		return -1;
	}
	if (messageArray[0][0] == 'G'){ //player is a guide agent
		GAPlayer_t *foundPlayerGA;
		FAPlayer_t *foundPlayerFA;
		if ((foundPlayerGA = hashtable_find(allGameInfo->GA, messageArray[2])) != NULL){
			if (foundPlayerGA->status != 1){
				return 0; //valid
			} else {
				return 2; //ignoring inactive GA		
			}
		} else if ((foundPlayerFA = hashtable_find(allGameInfo->FA, messageArray[2])) != NULL) {
			return 0;
		} else {
			//Pebble ID is within the range and is known.
			return 1;
		}
	} else { //Player is FA
		FAPlayer_t *foundPlayerFA;
		if ((foundPlayerFA = hashtable_find(allGameInfo->FA, messageArray[2])) != NULL){
			if (foundPlayerFA->status != 1){
				return 0;
			} else {
				return 2; //since we just need to ignore it
			}
		} else {
			//Pebble ID is within the range and is known.
			return 1;
		}
	}
}

// verifies input Team Names
// verifies there is at most one GA per team
// ensure that no player switches teams.
bool teamNameHandler(hashStruct_t *allGameInfo, char**messageArray, receiverAddr_t * playerAddr){
	if (messageArray[0][0] == 'G'){ // a guide agent
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
			
			if(errorFlag == 0){ // the team does not exist
				return true;
			} else {	
				// the team exists, send an error message
				GSResponseHandler(allGameInfo, playerAddr, "The team already exists",  "MI_ERROR_INVALID_TEAMNAME");
				return false;
			}
			
		} else{ // a field agent
		
			//player is known, so they need to have a matching team name
			if ((strcmp(foundPlayerGA->TeamName, messageArray[3])) == 0){
				//and team matches
				return true;
			}
			else { // they do not match, send an error message
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
		
		//checking that this name already exists
		FAPlayer_t *foundPlayerFA = (FAPlayer_t *) hashtable_find(allGameInfo->FA, messageArray[2]);
		
		if((foundPlayerFA == NULL)){ 	//player is not known. 


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
	
	if (isItValidFloat(messageArray[5])){ //validate latitude
		
		if (atol(messageArray[5]) > 90.0 || atol(messageArray[5]) < -90.0) { // send an error message
			GSResponseHandler(allGameInfo, playerAddr, " ",  "MI_ERROR_INVALID_LAT");
			return false;
		} else return true;
		
	}
	return false;
}

//for the FA OPCODES, verifies that the given longitude is a valid Float and that it is within the correct range
bool lngHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	
	if (isItValidFloat(messageArray[6])){ //validate latitude
		if (atol(messageArray[6]) > 180.0 || atol(messageArray[6]) < -180.0) {
			GSResponseHandler(allGameInfo, playerAddr, " ",  "MI_ERROR_INVALID_LONG");
			return false;
		} else return true;
	}
	return false;
}

// Parses the status request flag and creates a flag that triggers a response message.
int statusReqHandler(char **messageArray, receiverAddr_t * playerAddr, hashStruct_t *allGameInfo){
	
	if (messageArray[0][0] == 'G'){ //if it is guide agent message
		
		if(isItValidInt(messageArray[5])) { //validate the status
			
			if((atoi(messageArray[5])== 0)) return 0; //valid message (no request)
			else if ((atoi(messageArray[5])== 1)) return 1; //valid message (request)
			else{
				//invalid message, send an error
				GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
				return -1;
			}
		} //invalid message, send an error
		GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
		return -1;
		
	} else if (isItValidInt(messageArray[7])){
		if((atoi(messageArray[7])== 0)){//valid message (no request)
			return 0;
		} else if ((atoi(messageArray[7])== 1)){ //valid message (request)
			return 1;
		} else{//invalid message, send an error
			GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
			return -1;
		}
	}//invalid message, send an error
	GSResponseHandler(allGameInfo, playerAddr, "Send either 0 or 1",  "MI_ERROR_INVALID_STATUS_REQ");
	return -1;
}

// Add any player to the game. Mallocs memory and initializes all of the player's information.
void addPlayer(hashStruct_t *allGameInfo, char **messageArray, receiverAddr_t *addr){
	//assuming that everything is verified.
	char logText[1000];

	//only doing the FA side because I dont know what all of fields of the GA player struct are
	if ((strcmp(messageArray[0], "FA_LOCATION")) == 0){
		
		FAPlayer_t *newFA; // new player pointer
		receiverAddr_t *addrFA; //address of the current player
		
		//mallocing memory for the components of the player
		if ((newFA = malloc(sizeof(FAPlayer_t))) == NULL) exit(11);
		if ((newFA->PlayerName = malloc((strlen(messageArray[4]) + 1))) == NULL) exit(11);
		if ((newFA->TeamName = malloc((strlen(messageArray[3]) + 1))) == NULL) exit(11);
		if ((addrFA = malloc(sizeof(receiverAddr_t))) == NULL) exit(11);
		
		//updating the address
		addrFA->port = addr->port;
		addrFA->inaddr = addr->inaddr;
		addrFA->sin_family = addr->sin_family;
		
		//updating the name
		strcpy(newFA->PlayerName, messageArray[4]);
		strcpy(newFA->TeamName, messageArray[3]);
		
		newFA->status = 0; //default is 0 (active)
		newFA->capturedPlayers = 0; // captured 0 players
		newFA->Neutralized = 0; // neutralized 0 code
		
		//address
		newFA->lat = atol(messageArray[5]);
		newFA->lng = atol(messageArray[6]);
		
		//last contact time
		newFA->lastContact = time(NULL);
		newFA->addr = addrFA;
		newFA->capturingTime = 0; // it is not being captured at the moment
		
		char *tempHashCode;
		if ((tempHashCode = malloc(5)) == NULL) exit(11);
		
		//defaulting the capturing hexcode
		strcpy(tempHashCode, "P00P");
		newFA->capturingHexCode = tempHashCode;
		
		//logging
		sprintf(logText, "Field Agent %s was added to team %s", messageArray[4], messageArray[3]);
		logger(allGameInfo->game->fp, logText);
		printf("%s\n", logText);
		
		//inserted into the list
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
		printf("%s\n", logText);
		hashtable_insert(allGameInfo->GA, messageArray[2], newGA);
	}
}

// Verifies that given OPCODEs are of the correct length (4 hexDigits) to neutralize code
// Also checks to see if the input HexCodes match any in the master DropCode hashtable
bool codeIDHandler(char **messageArray, receiverAddr_t *playerAddr, hashStruct_t *allGameInfo){
	
	//check for a valid Hex Code
	if (strlen(messageArray[7]) != 4){
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
	} else if (strlen(messageArray[5]) != 4){ //invalid hexcode
		return false;
	} else {
		int existsFlag = 0;

		utility_t *utility_p;
		if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);

		utility_p->param1 = &existsFlag; //existsFlag
		utility_p->param2 =  messageArray[5]; //hexString
		
		//checking if the code exists in the list
		hash_iterate(allGameInfo->FA, existingHexCodeIterator, utility_p);

		if ( existsFlag == 1){ //it exists
			free(utility_p);
			return true;
		} else { // it does not exist
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
	//check if the message is appropriate
	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	if (!latHandler(messageArray,playerAddr, allGameInfo)) return;
	if (!lngHandler(messageArray,playerAddr, allGameInfo)) return;
	
	//check the validity of the hexcode
	if (!codeIDHandler(messageArray, playerAddr, allGameInfo)) return;
	
	//Update the CodeDrop's Information.
	code_drop_t *foundCode = hashtable_find(allGameInfo->CD, messageArray[7]); 
	if (foundCode->status == 0){
		
		//measure the distance
		double distance = dist(atof(messageArray[5]), atof(messageArray[6]), foundCode->lng, foundCode->lat);
		
		// if it is within 10
		if (distance < 10) {
			
			foundCode->status = 1; //neutralized, update statistics 
			strcpy(foundCode->team, messageArray[3]);
			allGameInfo->game->deadDropRemaining -= 1;
			
			//create the response message and log it
			sprintf(logText, "Field Agent %s neutralized codeDrop %s", messageArray[4], messageArray[7]);
			logger(allGameInfo->game->fp, logText);
			printf("%s\n", logText);
			GSResponseHandler(allGameInfo, playerAddr, "Congratulations! You've neutralized a piece of code!", "MI_NEUTRALIZED");
			
		} else { //invalid
			return;
		}
	} else {//invalid
		return;
	}

	//update the FA's Neutralization number
	FAPlayer_t *currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	currentFA->Neutralized = currentFA->Neutralized + 1; 
	currentFA->lat = atof(messageArray[6]);
	currentFA->lat = atof(messageArray[5]);
}

// handles the appropriate actions for FA_CAPTURE OPCODES.
// handles both the case of initiating a capture or completing a capture.
void FA_CAPTURE_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	char logText[1000];

	//Validate the number of arguments in the message
	if (arraySize != 6){ //invalid message
		GSResponseHandler(allGameInfo, playerAddr, "you need 6 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	//validating the message components
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
			printf ("%s\n", logText);
				
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

	if (arraySize != 6){ //inavalid
		GSResponseHandler(allGameInfo, playerAddr, "you need 6 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	//validate the first 4 parameters
	if (! validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	
	//validate the statusflag
	if ((statusFlag = statusReqHandler(messageArray,playerAddr, allGameInfo)) == -1) return;

	// if they are here, they exist, in our list --> update their info
	// update the GA's information
	GAPlayer_t *currentGA = hashtable_find(allGameInfo->GA, messageArray[2]);
	
	//update their name
	free(currentGA->PlayerName);
	if ((currentGA->PlayerName = malloc(strlen(messageArray[4]) +1)) == NULL) exit(11);
	strcpy(currentGA->PlayerName, messageArray[4]);

	currentGA->lastContact = time(NULL);
	
	//Update Player's Address in case of disconnection.
	currentGA->addr->port = playerAddr->port;
	currentGA->addr->inaddr = playerAddr->inaddr;
	currentGA->addr->sin_family = playerAddr->sin_family;

	//send a message back.
	if (statusFlag == 1 && allGameInfo->game->level == 1){ //level 1
		GAPlayerGameStatusHandler(allGameInfo, currentGA);
	} else if (statusFlag == 1 && allGameInfo->game->level == 3){ //level 3
		GAPlayerGameStatusHandlerThree(allGameInfo, currentGA);
	}
}

// Handles the GA_HINT OPCODE messages. Sends messages to either one or all of a team. 
void GA_HINT_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	
	//Validate given Arguments
	if (arraySize != 7){
		GSResponseHandler(allGameInfo, playerAddr,
		"you need 7 pieces of information",  "MI_ERROR_INVALID_OPCODE_LENGTH");
		return;
	}
	if (!validateArgumentsOneThroughFour(allGameInfo, messageArray, playerAddr)) return;
	if (!messageCodeIDHandler(messageArray, playerAddr, allGameInfo)) return;

	//Create the list of the receiver
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	//Populate the hashtable with the team
	if ((strcmp("*", messageArray[5])) == 0){ // to all the members of the team

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
		sprintf(message1, "GA_HINT|%s|%s|%s|%s|%s", 
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
	
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL); // list of all the FAs and GAs
	hashtable_t *tempHash2 = hashtable_new(1, deleteTempHash, NULL); 
	
	hash_iterate(allGameInfo->FA, FAcopyingAddressIterator, tempHash); // update the list with all the players
	hash_iterate(allGameInfo->GA, GAcopyingAddressIterator, tempHash); 

	utility_t *utility_p;
	if ((utility_p = malloc(sizeof(utility_t))) == NULL) exit(11);
	
	utility_p->param1 = allTeamInfo;
	utility_p->param2 = tempHash2;
	utility_p->param3 = allGameInfo;
	
	hash_iterate(allGameInfo->FA, gameOverIterator, utility_p); //update the the statistics for each team
	
	
	//create the game over message and update the log
	sprintf(message, "GAME_OVER|%s|%d|%s",allGameInfo->game->gameID, allGameInfo->game->deadDropRemaining, allTeamInfo);
	if (allGameInfo->game->rawlogFl == 1)
		logger(allGameInfo->game->fp, message);
	printf ("%s\n", message);
	
	// to send to all the users
	sending(allGameInfo->game->comm_sock, tempHash, message);
	
	//cleaning
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
	
	char eachTeamInfo[BUFSIZE];// for the statistics
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
	
	// this insert is to make sure we get the list of of the teams without redundancy
	if (hashtable_insert(hash, FAPlayer_p->TeamName, NULL)){
		
		utility_t *utility_p2;
		if ((utility_p2 = malloc(sizeof(utility_t))) == NULL) exit(11);

		utility_p2->param1 = FAPlayer_p->TeamName;
		utility_p2->param2 = &numberOfPlayers;//total number of active players in each team
		hash_iterate(allGameInfo->FA, numberOfPlayersIterator, utility_p2); 
		
		utility_p2->param2 = &numberCapturing;//total number of capturing players in each team
		hash_iterate(allGameInfo->FA, numberCapturingIterator, utility_p2); 
		
		utility_p2->param2 = &numberCaptured;//total number of captured players in each team
		hash_iterate(allGameInfo->FA, numberCapturedIterator, utility_p2); 
		
		utility_p2->param2 = &numberNeutralized;//total number of neutralized code in each team
		hash_iterate(allGameInfo->FA, numberNeutralizedIterator, utility_p2);
		
		sprintf(eachTeamInfo, "%s,%d,%d,%d,%d",FAPlayer_p->TeamName, 
			numberOfPlayers, numberCapturing, numberCaptured, numberNeutralized);
		
		// to avoid printing unnecessary ":" in the beginning of the string
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
	int r = rand() %65535; // up to 4 hex digits
	char * hexString;
	
	if ((hexString = malloc(10)) == NULL) exit(11);
	sprintf(hexString, "%X", r); // converting it into hex
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

// Searches a hashtable that has GA data for any player that has a given TeamName.
void GAMatchingTeam(void *key, void* data, void* farg){ 
	utility_t *utility_p = (utility_t *) farg;
	GAPlayer_t *GAPlayer_p = (GAPlayer_t *) data;

	char *name = (char *) utility_p->param2;

	if ((strcmp(GAPlayer_p->TeamName, name)) == 0){
		*(int *) utility_p->param1 = 1;
	}
}

// Searches a hashtable that has FA data for any player that has a given TeamName.
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

// Searches a hashtable that has GA data for any player that has a given GuideID.
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
				FAPlayer_p->status = 2; //captured
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
		
		// if it matches, update their info
		if ((strcmp(FAPlayer_p->capturingHexCode, givenCode)) == 0){
			*(int*) utility_p->param3 = 1;
			FAPlayer_t *capturedPlayer = FAPlayer_p;
			capturedPlayer->status = 1;
			utility_p->param2 = capturedPlayer;
		}
	}
}

// A function that calulate the distance between two (lat, long) points using Haversine Formula
//return distance in meters 
// Adapted from rosettacode.org websit.
double dist(double x1, double y1, double x2, double y2){
	double distanceX, distanceY, distanceZ;
	y1 -= x2;
	y1 *= TO_RAD, x1 *= TO_RAD, x2 *= TO_RAD;
 
	distanceX = cos(y1) * cos(x1) - cos(x2);
	distanceY = sin(y1) * cos(x1);
	distanceZ = sin(x1) - sin(x2);
	
	return (asin(sqrt(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ) / 2) * R * 2) * 1000;
}

// printing into the log file
void logger(FILE *fp, char* message){
	time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    fprintf(fp, "%s                             %s\n\n",asctime( localtime(&ltime) ), message );
}
