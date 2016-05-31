/*
 * guideAgent.c - In the mission incomputable game, this agent communicated with 
 *	the game server using UDP to send hints and recieve game updates.
 *
 * Read messages from stdin(Later GUI) and sen them as OPCODEs to server at host/port
 * Read messages from the UDP socket and parse them to update the GUI
 *
 * usage: ./guideAgent [-v|-log=raw] [-id=########] teamName GShost GSport
 *
 * Kyra Maxwell, May 2016
 *
 * Much of the GTK usability had been derived from bootstrap - a program
 *	by Peter Saisi, May 2016
 * Much of the server communication has been derived from chatclient - a program
 *	by David Kotz, May 2016
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>		// logging and creating random hexs
#include <unistd.h>	      // read, write, close
#include <string.h>	      // strlen
#include <strings.h>	      // bcopy, bzero
#include <netdb.h>	      // socket-related structures
#include <arpa/inet.h>	      // socket-related calls
#include <sys/select.h>	      // select-related stuff 
#include "common/file.h"	   // readline
#include "lib/hashtable/hashtable.h"
#include "lib/list/list.h"
//#include "guideAgent.h"

/******************** local types *************************/
//Field Agent Struct: Stores a name, status, location, and team 
typedef struct fieldagent {
		char *name;
		char *status;
		double lat;
		double lng;
		char *team;
		double lastcontact;
} fieldagent_t;
//Guide Agent Struct: Stores a name and status (corresponding to the guide agent running the program)
typedef struct guideagent {
		char *name;
		char *id;
		char *teamName;
		char *gameId;
} guideagent_t;

//Code Drop Struct: Stores a string status, and a double location, and a neutralizing team
typedef struct codedrop {
		char *status;
		double lat;
		double lng;
		char *team;
} codedrop_t;
/************* local constants **************/
static const int BUFSIZE = 1024;
/************* local constants **************/
/*DELETE LATER*/
static int socket_setup(char *GShost, int GSport, 
							struct sockaddr_in *themp);
static int handle_stdin(int comm_sock, struct sockaddr_in *themp, 
	hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, 
	bool raw, FILE* log);
static int internalUpdate(int comm_sock, int recieve, struct sockaddr_in *themp, 
	hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, 
	bool raw, FILE* log);

char *createStatus(char *response, guideagent_t *thisGuide, bool raw, FILE* log);
char *createHint(char *response, hashtable_t *agents, guideagent_t *thisGuide, 
	bool raw, FILE* log);

static int handle_socket(int comm_sock, struct sockaddr_in *themp, 
	hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, 
	bool raw, FILE* log);

int parsingMessages(char* line, char ** messageArray, char *delimiter);
void freeArray(char** array, int size);
int copyValidKeywordsToQueryArray( char ** array, char* word, int count);

void parseGameEnd (char **messageArray, int count, bool raw, FILE* log);
void updateGame (char **messageArray, int count, hashtable_t *agents, 
		hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log);

void updateAgents(char *message, hashtable_t *agents, bool raw, FILE* log);
void updateCodeDrops(char *message, hashtable_t *codeDrops, bool raw, FILE* log);

static void fa_delete(void *data);
static void cd_delete(void *data);

void freeDrops(void *key, void *data, void *farg);
void freeAgents(void *key, void *data, void *farg);
void agentPrint(void *key, void *data, void *farg);
void codeDropsPrint(void *key, void *data, void *farg);

int randomHex();
bool isValidInt(char *intNumber);
bool isValidFloat(char *floatNumber);

void logger(FILE *fp, char* message);
/******************** global types ************************/

/************************** main() ************************/
int
main(const int argc, char *argv[])
{
	/* validate command line parameters */
	if (argc < 5){
			printf("Error: Too few parameters");
			printf("usage: ./guideAgent [-v|-log=raw] [-id=########] teamName playerName GShost GSport\n");
			exit(1);
	}
	
	int count;
	unsigned int hexcode = 0;
	char *hex;
	char *flag;
	bool raw = false;
	bool flagsOkay = true;
	for (int i = 1; i < argc; i++) {
		//This is a flag
		if(argv[i][0] == '-') {
			if(flagsOkay){
				if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i], "-log=raw") == 0){
					raw = true;
				} else if (strlen(argv[i]) == 12) {
					if (argv[i][1] == 'i' && argv[i][2] == 'd') {
						if ((flag = strtok(argv[i], "=")) != NULL) {
							if ((hex =strtok(NULL, "=")) != NULL) {
								if(sscanf(hex, "%x", &hexcode) == 1)
									printf("success, got %x\n", hexcode);
								else
									printf("Error: incorrectly formatted -id flag\n");
							} else {
								printf("Error: incorrectly formatted -id flag\n");
							}
						}
					} else {
						printf("Error: flag '%s' not recognized\n", argv[i]);
					}
				} else {
					printf("Error: flag '%s' not recognized\n", argv[i]);
				}
			} else {
				printf("Error: flags must come at the beginning\n");
				exit(2); // must exit as this would alter the order of all params
			}
		} else {
			if (flagsOkay)
				count = i;
			flagsOkay = false;
		}
	}

	char *teamName = argv[count];
	char *name = argv[count+1];
	char *GShost = argv[count+2];
	int GSport;
	if (sscanf(argv[count+3], "%d", &GSport) != 1) {
		printf("Error: GSport must be a number\n"); 
		exit(2); 
	}
	
	//Create logger file
	char logText[1000]; 
	sprintf(logText, "log/guideagent%ld.log", time(NULL) );
	FILE *log; 
	if ((log=fopen(logText, "a")) == NULL) {
		printf("Error: Unable to create logfile");
		exit(3);
	 } 

	// Initialize a hashtable of agents
	// Where the agent's name is the key and a fieldagent data structure is the data
	hashtable_t *agents = hashtable_new(50, fa_delete, NULL);

	// Initialize a hashtable of code drops
	// Where the code drop's name is the key and a codeDrop data structure is the data
	hashtable_t *codeDrops = hashtable_new(50, cd_delete, NULL);
	
	// create guide agent struct
	guideagent_t *thisGuide = malloc(sizeof(guideagent_t)); 
	char guideId[9]; // hexcode will not be more than 8 characters
	if (hexcode != 0) {
		sprintf(guideId, "%x", hexcode);
		thisGuide->id = guideId;
	} else {
		sprintf(guideId, "%d", randomHex());
		thisGuide->id = guideId;
	}
	thisGuide->teamName = teamName;	
	thisGuide->gameId = "0";
	thisGuide->name = name;
	// the server's address
	struct sockaddr_in them;

	int comm_sock = socket_setup(GShost, GSport, &them);
	
	// send OPCODE notifying the server that a guide has joined
	// update until the server returns a GAME_STATUS
	bool gameStart = false;
	while (!gameStart) {
		internalUpdate(comm_sock, 1, &them, agents, codeDrops, thisGuide, raw, log);
		int resp = handle_socket(comm_sock, &them, agents, codeDrops, thisGuide, 
				raw, log);
		if (resp == 1) { // got a GAME_STATUS
			gameStart = true;
		} else {
			sprintf(guideId, "%d", randomHex());
			thisGuide->id = guideId;
		}
	}

	// Update logfile with initialization information (host and port)


	
	// Initialize a list of notifications
	// list_t *notifications = list_new(data_delete);

	// Initial prompt
	printf("Enter a hint in the format: playername|message\n\
or a 1 to send your game status and recieve an update\n\
or a 0 to send your game status without recieving an update\n");
	
	// while game is in play
	// read from either the socket or stdin, whichever is ready first;
	// if stdin, read a line and send it to create hint;
	// if socket, receive message parse it, update the data structures, and  write to stdout.
	while (true) {	      // loop exits on EOF from stdin
		// for use with select()
		fd_set rfds;	      // set of file descriptors we want to read
		struct timeval timeout;   // how long we're willing to wait
		const struct timeval sixtysec = {60,0};   // sixty seconds
    
		// Watch stdin (fd 0) and the UDP socket to see when either has input.
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);	      // stdin
		FD_SET(comm_sock, &rfds); // the UDP socket
		int nfds = comm_sock+1;   // highest-numbered fd in rfds

		// Wait for input on either source, up to five seconds.
		timeout = sixtysec;
		int select_response = select(nfds, &rfds, NULL, NULL, &timeout);
		// note: 'rfds' updated, and value of 'timeout' is now undefined
    
		if (select_response < 0) {
			// some error occurred
			perror("select()");
			exit(9);
		
		//After 60 seconds with no communication send an update to the GS
		} else if (select_response == 0) {
			internalUpdate(comm_sock, 0, &them, agents, codeDrops, thisGuide, 
				raw, log);
		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)) 
				if (handle_stdin(comm_sock, &them, agents, codeDrops, thisGuide, 
						raw, log) == EOF)
					break; // exit loop if EOF on stdin

			if (FD_ISSET(comm_sock, &rfds)) {
				handle_socket(comm_sock, &them, agents, codeDrops, thisGuide, 
						raw, log);
			}

			// print a fresh prompt
			printf("Enter a hint in the format: playername|message\n\
or a 1 to send your game status and recieve an update\n\
or a 0 to send your game status without recieving an update\n");
			fflush(stdout);
		}
	}

	printf("Made it through what's been written");
	
	hashtable_delete(agents);
	hashtable_delete(codeDrops);

	free(thisGuide->gameId);
	free(thisGuide);	

	close(comm_sock);
	putchar('\n');
	exit(0);
}

/***************** socket_setup **********************/
/* parse the remaining arguments and set up the socket.
 * Exit on any error of arguments in the setup.
 * Derived from the socket_setup outlined in David Kotz's chatclient
 */
static int 
socket_setup(char *GShost, int GSport, struct sockaddr_in *themp)
{
	char *hostname;		//server hostname
	int port;			//server port	

	hostname = GShost;
	port = GSport;

	//look up the Game Server host 
	struct hostent *hostp = gethostbyname(hostname);
	if (hostp == NULL) {
		printf("Error: unknown host '%s'\n", hostname);
		//return -1;
		exit(2);
	}

	//initialize fields for the server address
	themp->sin_family = AF_INET;
	bcopy(hostp->h_addr_list[0], &themp->sin_addr, hostp->h_length);
	  themp->sin_port = htons(port);

	//create the socket
	int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (comm_sock < 0) {
		printf("Error: Opening datagram socket");
		//return -1;
		exit(3);
	}

	return comm_sock;
}


/*************** handle_socket ***********************/
/* Socket has input; recieve and parse it
 * 'themp' should be a valid address, ignore messages from other senders
 * return -1 on any socket error
 * return -2 on unexpected server
 * return 1 if GAME_STATUS
 * return 2 if GAME_OVER
 * return 3 if GS_RESPONSE
 */

static int
handle_socket(int comm_sock, struct sockaddr_in *themp, hashtable_t *agents, 
	hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log)
{
	int retVal = -1; // int to be returned
	//socket has input ready
	struct sockaddr_in sender;		//sender of this message
	struct sockaddr *senderp = (struct sockaddr *) &sender;
	socklen_t senderlen = sizeof(sender);	//must pass address to length
	char buf[BUFSIZE];		//buffer for reading data from socket
	int nbytes = recvfrom(comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
	
	if (nbytes < 0) {
		printf("Error: Receiving from socket");
		//return -1;
		exit(4);
	} else {      
		buf[nbytes] = '\0';     // null terminate string
		
		//where was it from?
		if (sender.sin_family != AF_INET) {
			printf("From non-internet address: Family %d\n", sender.sin_family);
		} else {
			//was it from the expected server
			if (sender.sin_addr.s_addr == themp->sin_addr.s_addr && 
				sender.sin_port == themp->sin_port) {
				//parse the message
				if(strcmp(buf,"\0") != 0) {
					if (raw)
						logger(log, buf);
					char *messageArray[20];
					int count = parsingMessages(buf, messageArray, "|");
					if (strcmp(messageArray[0], "GAME_STATUS") == 0) {
						//Empty current hashtables
						//hash_iterate(agents, freeAgents, NULL);
						//hash_iterate(codeDrops, freeDrops, NULL);

						updateGame(messageArray, count, agents, codeDrops, thisGuide,
									raw, log);
						retVal = 1;
					} else if (strcmp(messageArray[0], "GAME_OVER") == 0) {
						parseGameEnd(messageArray, count, raw, log);
						retVal = 2;
					} else if (strcmp(messageArray[0], "GS_RESPONSE") == 0) {
						for (int i = 0; i < count-1; i++) {
							printf(messageArray[i]);
							printf("|");
						}
						printf("%s\n", messageArray[count-1]);
						retVal = 3;
						freeArray(messageArray, count);
					} else {
						printf("Recieved invalid OPCODE from game server\n");
						freeArray(messageArray, count);
					}	
				}
			} else {
				//return -2;
				exit(5);
			}	
		}	
	}
	return retVal;
}

/**************** handle_stdin ***********************/
/* stdin has input ready; read a line and parse it.
 * return EOF if EOF was encountered in stdin
 * return 0 if there is no client to whom we can send
 * return 1 if the message is sent successfully
 * return -1 on any socket error
 * Derived from the handle_stdin function in David Kotz's chatclient
 */
static int
handle_stdin(int comm_sock, struct sockaddr_in *themp, hashtable_t *agents,
	hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log)
{
	char *response = readline(stdin);
	if (response == NULL) 
		return EOF;
	
	char *toParse = malloc(strlen(response) + 1);
	strcpy(toParse, response);
	
	if (themp->sin_family != AF_INET) {
		printf("Error: server is not AF_INET.\n");
		return 0;
	} 
	char *OPCODE;
	char *respArray[3];
	printf("RESPONSE %s\n", response);
	int count = parsingMessages(response, respArray, "|");
	for(int i = 0; i < count; i++) {
		free(respArray[i]);
	}
	printf("Count in std in %d\n", count);
	if ( count == 1) 
		OPCODE = createStatus(toParse, thisGuide, raw, log);
	else if (count == 2) {
		printf("RESPONSE %s\n", toParse);
		OPCODE = createHint(toParse, agents, thisGuide, raw, log); 
	} else { 
		printf("Sorry that message doesn't seem to be formatted correctly");}
	if (OPCODE != NULL) {
		printf("%s\n", OPCODE);
		if(raw)
			logger(log, OPCODE);
		if (sendto(comm_sock, OPCODE, strlen(OPCODE), 0, 
					(struct sockaddr *) themp, sizeof(*themp)) < 0){
			printf("Error: sending in datagram socket");
			//return -1;
			exit(3);
		}
	}
	free(OPCODE);
	free(response);
	free(toParse);
	return 1;
}

/************************ internalUpdate *****************/
static int
internalUpdate(int comm_sock, int recieve, struct sockaddr_in *themp, 
	hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, 
	bool raw, FILE* log)
{

	if (themp->sin_family != AF_INET) {
		printf("Error: server is not AF_INET.\n");
		return 0;
	} 
	char *OPCODE;
	char *response;
	if (recieve == 0) {
		response = "0";
	} else if (recieve == 1) {
		response = "1";
	} else {
		printf("Error: statusreq must be a zero or a one");
		return -1;
	}
	OPCODE = createStatus(response, thisGuide, raw, log);
	if (OPCODE != NULL) {
		printf("%s\n", OPCODE);
		if (raw)
			logger(log, OPCODE);
		if (sendto(comm_sock, OPCODE, strlen(OPCODE), 0, 
					(struct sockaddr *) themp, sizeof(*themp)) < 0){
			printf("Error: sending in datagram socket");
			//return -1;
			exit(3);
		}
	}
	free(OPCODE);
	return 1;
}

/************** createName ***********************/
/* Puts the string from stdin in OPCODE form:
 * GA_STATUS|gameId|guideId|teamName|playerName|statusReq
 */

char *
createStatus(char *response, guideagent_t *thisGuide, bool raw, FILE* log)
{
	char *gameId = thisGuide->gameId;
	char *guideId = thisGuide->id;
	char *teamName = thisGuide->teamName;
	char *name = thisGuide->name;
	char *statusReq;
	
	if(strcmp(response, "0") == 0 || strcmp(response, "1") == 0)
		statusReq = response;
	else {
		printf("Sorry that message doesn't seem to be formatted correctly");
		return NULL;
	}
	
	//create the length for the OPCODE
	int length = 15 + strlen(gameId) + strlen(guideId) + strlen(teamName) 
		+ strlen(name) + strlen(statusReq);
	char *CODE = calloc(length, sizeof(char));
	char *OPCODE = "GA_STATUS";
	char *pipe = "|";
	//Actually create the string
	strcpy(CODE, OPCODE);
	strcat(CODE, pipe);
	strcat(CODE, gameId);
	strcat(CODE, pipe);
	strcat(CODE, guideId);
	strcat(CODE, pipe);
	strcat(CODE, teamName);
	strcat(CODE, pipe);
	strcat(CODE, name);
	strcat(CODE, pipe);
	strcat(CODE, statusReq);
	
	return CODE;
}

/*************** createHint **********************/
/* Puts first two pieces of a line read from stdin in OPCODE form:
 * Anything after a second pipe is ignored
 * GA_HINT|gameId|guideId|teamName|playerName|pebbleId|message
 */
char *
createHint(char *response, hashtable_t *agents, guideagent_t *thisGuide, 
		bool raw, FILE* log)
{
	char *pebbleId;
	char *message;
	if ((pebbleId = strtok(response, "|")) == NULL){ 
			return NULL;
	}
	message = strtok(NULL, "|");
	if (message == NULL) {
			return NULL;
	}
	if (strlen(message) > 140) {
		printf("Message is too long\n");
		return NULL;
	}
	fieldagent_t *agent;
	if ((agent = hashtable_find(agents, pebbleId)) == NULL) {
		printf("Sorry that agent doesn't exist");
		return NULL;
	} else if (strcmp(agent->team, thisGuide->teamName) != 0) {
		printf("You can only send hints to agents on your team, %s\n", thisGuide->teamName);
		return NULL;
	}
   	
	char *gameId = thisGuide->gameId;
	char *guideId = thisGuide->id;
	char *teamName = thisGuide->teamName;
	char *name = thisGuide->name;
	//create the length for the OPCODE
	int length = 14 + strlen(gameId) + strlen(guideId) + strlen(teamName) + strlen(name)
		+ strlen(pebbleId) + strlen(message);
	char *hint = calloc(length, sizeof(char));
	char *OPCODE = "GA_HINT";
	char *pipe = "|";
	//Actually create the string
	strcpy(hint, OPCODE);
	strcat(hint, pipe);
	strcat(hint, gameId);
	strcat(hint, pipe);
	strcat(hint, guideId);
	strcat(hint, pipe);
	strcat(hint, teamName);
	strcat(hint, pipe);
	strcat(hint, name);
	strcat(hint, pipe);
	strcat(hint, pebbleId);
	strcat(hint, pipe);
	strcat(hint, message);

	printf("%s\n", hint);
	return hint;
}


/******************** parsingMessages ******************/
int 
parsingMessages(char* line, char ** messageArray, char *delimiter)
{

	char* word; // the keyword
	int count = 0; //restart the counter that counts the number of keywords per query
	
	//allocating memory for the string array that stores the words after checking 
	// the words are valid for matching
	// 8 was chosen because the size of each pointer is 8
	// and the other equation was chosen to account for the worst case scenario (words composed of one letter)
	
	if ((word = strtok(line, delimiter)) == NULL) return count; //parsing the first keyword
	// if the word meets the conditions of query keywords, it will be added to the array
	// if the line is empty, it will be ignored
	if ( (count = copyValidKeywordsToQueryArray(messageArray, word, count)) == 0 ) return count;
	
	// parsing the other keywords
	// if the word meets the conditions of query keywords, it will be added to the array
	//when the parsing reaches the end of line, it will terminate parsing
	while( (word = strtok(NULL, delimiter)) != NULL)  {
		if ( (count = copyValidKeywordsToQueryArray(messageArray, word, count)) == 0 ) return count;
	}

	return count;
}
/************** freeArray ****************************/
/* A function that loops through the query array indeces 
 * and frees contents after the matching analysis is over
 * or if error happens when validating query keywords. 
 * It frees the array pointer after it is done.
 */
void 
freeArray(char** array, int size)
{

	for(int i = 0; i < size ; i++){ //looping through the query array
		free(array[i]); //delete the keywords
	}
	//free(array); //delete the the query array
}


/************** copyValidKeywordsToQueryArray *********/
/* After validating the correctness of the keywords, 
 * they are added to the query array
 * otherwise, it dellocates the memory created to store 
 * the keywords and terminates matching for the query
 */
int 
copyValidKeywordsToQueryArray( char ** array, char* word, int count)
{

	array[count] = malloc(strlen(word) +1);
	
	if (array[count]  == NULL) { 
		freeArray(array, count); //if it fails to allocate memory, delete the query
		return 0; //terminate the matching analysis
	}
	
	strcpy(array[count], word); //add the keywords into the query array
	count++; //increment the number of keywords counter
	return count;
}

void
parseGameEnd (char **messageArray, int count, bool raw, FILE* log) {

}

void
updateGame (char **messageArray, int count, hashtable_t *agents, hashtable_t *codeDrops,
		guideagent_t *thisGuide, bool raw, FILE* log) 
{
	if (strcmp(thisGuide->gameId,"0") == 0) { // Haven't gotten the gameId yet
		thisGuide->gameId = malloc(strlen(messageArray[1]) + 1);
		strcpy(thisGuide->gameId, messageArray[1]); // Set the gameId
	}

	if (count == 4) {
		char *agentsUpdate = malloc(strlen(messageArray[2]) +1);
		strcpy(agentsUpdate, messageArray[2]);
		char *dropsUpdate = malloc(strlen(messageArray[3]) +1);
		strcpy(dropsUpdate, messageArray[3]);
		
		updateAgents(agentsUpdate, agents, raw, log);
		updateCodeDrops(dropsUpdate, codeDrops, raw, log);
	}
	if (count == 3) { //There are only codeDrops no agents
		char *dropsUpdate = malloc(strlen(messageArray[2]) +1);
		strcpy(dropsUpdate, messageArray[2]);

		updateCodeDrops(dropsUpdate, codeDrops, raw, log);
	}
	hash_iterate(agents, agentPrint, NULL);
	hash_iterate(codeDrops, codeDropsPrint, NULL);
	
	freeArray(messageArray, count);
}

void 
updateAgents(char *message, hashtable_t *agents, bool raw, FILE* log) 
{
	char *agentsRecieved[50];//Assuming there are no more than 50 agents
	int agentCount = parsingMessages(message, agentsRecieved, ":");
	free(message);
	//Iterate through all the agents that have been passed
	for (int i = 0; i < agentCount; i++) {
		char *agentUpdate[10];//Should only be 7
		
		//Split data about the agent into an array split by commas 
		int paramCount = parsingMessages(agentsRecieved[i], agentUpdate, ",");
		free(agentsRecieved[i]);
		//There need to be 7 parameters based on the protocol
		if (paramCount != 7) {
			printf("Error: only %d parameters per agent\n", paramCount);
			return;
		}

		else {
			//Look for the agent in the hashtable, (name should be in the third spot)
			fieldagent_t *current = hashtable_find(agents, agentUpdate[0]);

			//Create temp variables for the non char* parameters
			float lat;
			float lng;
			float lastcontact;
			
			if (current == NULL) {
				//Check Params
				if(isValidFloat(agentUpdate[4]) && isValidFloat(agentUpdate[5]) &&
						isValidFloat(agentUpdate[6])) {
					sscanf(agentUpdate[4], "%f", &lat);
					sscanf(agentUpdate[5], "%f", &lng);
					sscanf(agentUpdate[6], "%f", &lastcontact);
				
					//fieldagent doesn't exist, so create new data for it
					current = malloc(sizeof(fieldagent_t));

					//Use given params to fill in the new field agent	
			
					current->team = malloc(strlen(agentUpdate[1]) +1);
					strcpy(current->team, agentUpdate[1]);
					current->name = malloc(strlen(agentUpdate[2]) +1);
					strcpy(current->name, agentUpdate[2]);
					current->status = malloc(strlen(agentUpdate[3]) +1);
					strcpy(current->status, agentUpdate[3]);
			
					current->lat = lat;
					current->lng = lng;
					current->lastcontact = lastcontact;
					
					//insert new fieldagent into the hashtable
					hashtable_insert(agents, agentUpdate[0], current);
				} else {
					printf("Error: Incorrectly formatted parameters for Agent '%s'\n", 
						agentUpdate[2]);
				}	
			
			} else {
				// Check params
				if(isValidFloat(agentUpdate[4]) && isValidFloat(agentUpdate[5]) &&
						isValidFloat(agentUpdate[6])) {
					sscanf(agentUpdate[4], "%f", &lat);
					sscanf(agentUpdate[5], "%f", &lng);
					sscanf(agentUpdate[6], "%f", &lastcontact);
					
					free(current->name);
					free(current->status);
					free(current->team);
			
			
					current->team = malloc(strlen(agentUpdate[1]) +1);
					strcpy(current->team, agentUpdate[1]);
					current->name = malloc(strlen(agentUpdate[2]) +1);
					strcpy(current->name, agentUpdate[2]);
					current->status = malloc(strlen(agentUpdate[3]) +1);
					strcpy(current->status, agentUpdate[3]);
			
					current->lat = lat;
					current->lng = lng;
					current->lastcontact = lastcontact;
				} else {
					printf("Error: Incorrectly formatted parameters for Agent '%s'\n", 
						agentUpdate[2]);
				}	
			}
		} freeArray(agentUpdate, paramCount);
	}
}

void 
updateCodeDrops(char *message, hashtable_t *codeDrops, bool raw, FILE* log) 
{
	char *codeDropsRecieved[50];//Assuming there are no more than 50 codeDrops
	int dropCount = parsingMessages(message, codeDropsRecieved, ":");
	free(message);
	//Iterate through all the drops that have been passed
	for (int i = 0; i < dropCount; i++) {
		
		char *dropUpdate[10];//Should only be 4
		
		//Split data about the agent into an array split by commas 
		int paramCount = parsingMessages(codeDropsRecieved[i], dropUpdate, ",");
		
		//There need to be 4 parameters based on the protocol
		if (paramCount != 4) {
			printf("Error: %d parameters per code drop, there should be 4\n", 
				paramCount);
			return;
		}

		else {
			//Look for the code drop in the hashtable, (id should be in the first spot)
			codedrop_t *current = hashtable_find(codeDrops, dropUpdate[0]);

			//Create temp variables for the non char* parameters
			float lat;
			float lng;
			
			if(current == NULL) {
				current = malloc(sizeof(codedrop_t));

				//Use given params to fill in the new field agent	
				if(isValidFloat(dropUpdate[1]) && isValidFloat(dropUpdate[2])) {
					sscanf(dropUpdate[1], "%f", &lat);
					sscanf(dropUpdate[2], "%f", &lng);
				
					current->team = malloc(strlen(dropUpdate[3]) +1);
					strcpy(current->team, dropUpdate[3]);
					if (strcmp(current->team, "NONE") == 0)
						current->status = "active";
					else
						current->status = "neutralized";
					current->lat = lat;
					current->lng = lng;

					//insert new fieldagent into the hashtable
					hashtable_insert(codeDrops, dropUpdate[0], current);
				} else {
					printf("Error: Incorrectly formatted parameters for drop '%s'", 
							dropUpdate[0]);
				}

			} else {
				//Use given params to fill in the new field agent	
				if(isValidFloat(dropUpdate[1]) && isValidFloat(dropUpdate[2])) {
					sscanf(dropUpdate[1], "%f", &lat);
					sscanf(dropUpdate[2], "%f", &lng);
					bool wasActive = false;
					 //To tell if a drop was netralized
					if(strcmp(current->status,"active") == 0)
						wasActive = true;
					free(current->team);
					
					current->team = malloc(strlen(dropUpdate[3]) +1);
					strcpy(current->team, dropUpdate[3]);
					if (strcmp(current->team, "NONE") == 0)
						current->status = "active";
					else {
						current->status = "neutralized";
						if (wasActive && !raw)
							logger(log, "Code Drop %s was neutralized");
					}
					current->lat = lat;
					current->lng = lng;
				} else {
					printf("Error: Incorrectly formatted parameters for drop '%s'", 
							dropUpdate[0]);
				}
			}			
				
		}
		freeArray(dropUpdate, paramCount);	
	}
	freeArray(codeDropsRecieved, dropCount);
}

/*************** Destructor functions ****************/

static void
cd_delete(void *data)
{
	if(data != NULL){
		free(((codedrop_t *) data)->team);
		free(data);
	}
}

static void 
fa_delete(void *data) 
{
	if (data != NULL){
		free(((fieldagent_t *) data)->name);
		free(((fieldagent_t *) data)->status);
		free(((fieldagent_t *) data)->team);
		free(data);
	}
}

void 
freeDrops(void *key, void *data, void *farg)
{
	if (data != NULL){
		//free(((codedrop_t *) data)->status);
		free(((codedrop_t *) data)->team);
		free(data);
	}
	free(key);
}	

void 
freeAgents(void *key, void *data, void *farg)
{
	if (data != NULL){
		free(((fieldagent_t *) data)->name);
		free(((fieldagent_t *) data)->status);
		free(((fieldagent_t *) data)->team);
		free(data);
	}
	free(key);
}

void 
agentPrint(void *key, void *data, void *farg)
{
	fieldagent_t *current = data;
	printf("Agent %s", current->name);
	printf("\n");
	printf("Pebble ID: %s\n", (char *)key);
	printf("Team: %s\n",current->team);
	printf("Status: %s\n", current->status);
	printf("Latitude: %lf\n", current->lat);
	printf("Longitude: %lf\n", current->lng);
	printf("Seconds since last contact: %lf\n\n", current->lastcontact);
	printf("\n");
}

void 
codeDropsPrint(void *key, void *data, void *farg)
{
	printf("Code Drop %s", (char *)key);
	printf("\n");
	codedrop_t *current = data;			
	printf("Latitude: %lf\n",current->lat);
	printf("Longitude: %lf\n",current->lng);
	printf("Status: %s\n", current->status);
	printf("Team: %s\n\n", current->team);

}

int
randomHex()
{
	time_t t;
	   
	/* Intializes random number generator */
	srand((unsigned) time(&t));
	
	return rand() % 65536;
}

bool 
isValidInt(char *intNumber)
{

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
bool 
isValidFloat(char *floatNumber){	
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

void 
logger(FILE *fp, char* message)
{
	time_t ltime; /* calendar time */
	ltime=time(NULL); /* get current cal time */
	fprintf(fp, "%s%s\n\n",asctime( localtime(&ltime)), message);
}
