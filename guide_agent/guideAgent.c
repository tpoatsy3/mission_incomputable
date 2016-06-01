/*
 * guideAgent.c - In the mission incomputable game, this agent communicated with 
 *	the game server using UDP to send hints and recieve game updates.
 *
 * Read messages from stdin(Later GUI) and sen them as OPCODEs to server at host/port
 * Read messages from the UDP socket and parse them to update the GUI
 *
 * usage: ./guideAgent [-v|-log=raw] [-id=########] teamName playerName GShost GSport
 *
 * Topaz, May 2016
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
#include "guideAgent.h"

/******************** local types *************************/
/* Field Agent Struct: Stores a name, status, latitude,  
 * longitude, team, and tiem since last contact
 */
typedef struct fieldagent {
		char *name;
		char *status;
		double lat;
		double lng;
		char *team;
		double lastcontact;
} fieldagent_t;

/* Guide Agent Struct: Stores a name, id, team name, and the current gameId
 * (corresponding to the guide agent running the program)
 */
struct guideagent {
		char *name;
		char *id;
		char *teamName;
		char *gameId;
};

/* Code Drop Struct: Stores a status, latitude, longitude, and the neutralizing team
 */
typedef struct codedrop {
		char *status;
		double lat;
		double lng;
		char *team;
} codedrop_t;

/************* local constants **************/
//Buffer size for the socket
static const int BUFSIZE = 1024;

/************************** main() ************************/
int
main(const int argc, char *argv[])
{
	/* validate command line parameters */
	if (argc < 5){
			printf("Error: Too few parameters\n");
			printf("usage: ./guideAgent [-v|-log=raw] [-id=########] teamName playerName GShost GSport\n");
			exit(1);
	}
	
	// Parameters necessary to parse flags
	int count;
	unsigned int hexcode = 0;
	char *hex;
	char *flag;
	bool raw = false;
	bool flagsOkay = true; // There hasn't been a non-flag yet

	// Parse arguments looking for flags and ensuring the correct order
	for (int i = 1; i < argc; i++) {

		// This paramete is a flag
		if(argv[i][0] == '-') {
			if(flagsOkay){

				// Either flag can be used to signal verbose logging
				if(strcmp(argv[i],"-v") == 0 || strcmp(argv[i], "-log=raw") == 0){
					raw = true;

				// Confirms a properly formatted id and id flag
				} else if (strlen(argv[i]) == 12) {
					if (argv[i][1] == 'i' && argv[i][2] == 'd') {
						if ((flag = strtok(argv[i], "=")) != NULL) {
							if ((hex =strtok(NULL, "=")) != NULL) {
								if(sscanf(hex, "%x", &hexcode) == 1)
									;
								else
									printf("Error: incorrectly formatted -id flag\n");
							} else {
								printf("Error: incorrectly formatted -id flag\n");
							}
						}
					// If flag is not recognized alert user but continue
					} else {
						printf("Error: flag '%s' not recognized\n", argv[i]);
					}
				} else {
					printf("Error: flag '%s' not recognized\n", argv[i]);
				}
			//There has been a non-flag
			} else {
				printf("Error: flags must come at the beginning\n");
				exit(2); // must exit as this would alter the order of all params
			}
		} else {
			// Once a non-flag parameter is seen, flags are not okay
			if (flagsOkay)
				count = i;
			flagsOkay = false;
		}
	}
	
	// Assigns parameters to variables based on how many flags came before
	char *teamName = argv[count];
	char *name = argv[count+1];
	char *GShost = argv[count+2];
	
	//Confirms that the port is a number
	int GSport;
	char *GSportChar = argv[count+3]; //Storing this char * makes initial log easier
	if (sscanf(argv[count+3], "%d", &GSport) != 1) {
		printf("Error: GSport must be a number\n"); 
		exit(3); 
	}
	
	//Create logger file
	char logText[1000]; 
	sprintf(logText, "log/guideagent%ld.log", time(NULL) );
	FILE *log; 
	if ((log=fopen(logText, "a")) == NULL) {
		printf("Error: Unable to create logfile\n");
		exit(4);
	 } 

	// Initialize a hashtable of agents
	// Where the agent's pebbleid is the key and a fieldagent data structure is the data
	hashtable_t *agents = hashtable_new(50, fa_delete, NULL);
	if(agents == NULL)
			exit(12); //Unable to malloc hashtable - exit

	// Initialize a hashtable of code drops
	// Where the code drop's id is the key and a codeDrop data structure is the data
	hashtable_t *codeDrops = hashtable_new(50, cd_delete, NULL);
	if (codeDrops == NULL)
		exit(12); //Unable to malloc hashtable - exit
	
	// create guide agent struct
	guideagent_t *thisGuide;
	if((thisGuide = malloc(sizeof(guideagent_t))) == NULL)
		exit(11); //malloc has failedi

	// Create or parse the guideId
	char guideId[9]; // hexcode will not be more than 8 characters
	// id was found while parsing flags
	if (hexcode != 0) {
		sprintf(guideId, "%x", hexcode);
		thisGuide->id = guideId;
	} else {
		//create a random 8 digit hex id
		randomHex(guideId);
		thisGuide->id = guideId;
	}

	//Initialize the rest of the guide agent parameters
	thisGuide->teamName = teamName;	
	thisGuide->gameId = "0"; // Because we do not know the gameId yet
	thisGuide->name = name;
	
	// the server's address
	struct sockaddr_in them;

	// set up socket, if socket fails - exit
	int comm_sock = socket_setup(GShost, GSport, &them);
	if (comm_sock == -1)
		exit(5);

	// Update logfile with initialization information (host and port)
	char *toLog;

	// Where 32 is the length of the standard message
	if((toLog = malloc(strlen(GShost) + strlen(GSportChar) + 32)) == NULL)
		exit(11); //malloc has failed
	sprintf(toLog, "Initialized on port %s with host %s", GSportChar, GShost);
	logger(log, toLog);
	free(toLog);
	// send OPCODE notifying the server that a guide has joined
	// update until the server returns a GAME_STATUS
	// meaning the guide's id has been approved
	
	bool gameStart = false;
	while (!gameStart) {
		// automatically send a message to the server - no user input
		if (internalUpdate(comm_sock, 1, &them, agents, codeDrops, thisGuide, raw, log)
					   	== -1) {
				printf("Error: Socket Error in internalUpdate");
				exit(6);
		}

		// wait for some response from the server
		int sockResp = handle_socket(comm_sock, &them, agents, codeDrops, thisGuide, 
				raw, log);
		if (sockResp == 1) { // got a GAME_STATUS - guideId approved
			gameStart = true;	
		} else if (sockResp == 2) // exit loop, GAME_OVER was passed from the server
			break; 
		else if (sockResp == -1 || sockResp == -2) { // failure within the socket parsing
			printf("Error: Unable to parse socket response");
			exit(7);
		} else if (sockResp == 4) { //gameId was not approved
			// create a new random id
			randomHex(guideId);
			thisGuide->id = guideId;
		} else if (sockResp == 3) { //For some other reason joining the game has failed
			printf("Error: Game start was not approved\n");
			exit(8);
		}
	}

	

	// Initial prompt
	printf("Enter a hint in the format: pebbleID|message\n\
or a 1 to send your game status and recieve an update\n\
or a 0 to send your game status without recieving an update\n");
	
	// while game is in play
	// read from either the socket or stdin, whichever is ready first;
	// if stdin, read a line and send it to create hint;
	// if socket, receive message parse it, update the data structures, 
	// and  write to stdout.
	// modified from the select loop in chatclient
	while (true) {	      // loop exits on EOF from stdin
		// for use with select()
		fd_set rfds;	//set of file descriptors we want to read
		struct timeval timeout;   //how long we're willing to wait
		const struct timeval sixtysec = {60,0};   //sixty seconds
    
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
		
		//After 60 seconds with no communication automatically send an update to the GS
		} else if (select_response == 0) {
			if (internalUpdate(comm_sock, 0, &them, agents, codeDrops, thisGuide, 
					raw, log) == -1) {
				printf("Error: Socket Error in internalUpdate");
				exit(6);
			}

		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)) { 
				int stdinResp;
				stdinResp = handle_stdin(comm_sock, &them, agents, codeDrops, thisGuide, 
						raw, log);
				if (stdinResp == EOF)
					break; // exit loop if EOF on stdin
				// there was an error when parsing stdin - exit
				else if (stdinResp == -1) {
					printf("Error: Unable to parse stdin message");
					exit(10);
				}
			}
			if (FD_ISSET(comm_sock, &rfds)) {
				int sockResp;
				sockResp = handle_socket(comm_sock, &them, agents, codeDrops, thisGuide, 
						raw, log);
				if (sockResp == 2)
					break; // exit loop if GAME_OVER is passed to socket
				// there was an error parsing the socket - exit
				else if (sockResp == -1 || sockResp == -2) {
					printf("Error: Unable to parse socket response");
					exit(7);
				}
			}

			// print a fresh prompt
			printf("Enter a hint in the format: pebbleID|message\n\
or a 1 to send your game status and recieve an update\n\
or a 0 to send your game status without recieving an update\n");
			fflush(stdout);
		}
	}

	// clean up once the game is over

	// free hashtables
	hashtable_delete(agents);
	hashtable_delete(codeDrops);
	
	// free the guideId and it's only memory allocated param, gameId
	free(thisGuide->gameId);
	free(thisGuide);	
	
	//close logfile and comm socket
	fclose(log);
	close(comm_sock);

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
		return -1;
	}

	//initialize fields for the server address
	themp->sin_family = AF_INET;
	bcopy(hostp->h_addr_list[0], &themp->sin_addr, hostp->h_length);
	  themp->sin_port = htons(port);

	//create the socket
	int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (comm_sock < 0) {
		printf("Error: Opening datagram socket\n");
		return -1;
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
 * return 4 if GS_RESPONSE is MI_ERROR_INVALID_ID
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
		printf("Error: Receiving from socket\n");
		return -1;
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
					// in verbose mode, log the message
					if (raw)
						logger(log, buf);
					// split the message into an array by pipe
					char *messageArray[20];
					int count = parsingMessages(buf, messageArray, "|");
					// parse based on expected contents
					if (strcmp(messageArray[0], "GAME_STATUS") == 0) {
						
							updateGame(messageArray, count, agents, codeDrops, thisGuide,
									raw, log);
						retVal = 1;

					} else if (strcmp(messageArray[0], "GAME_OVER") == 0) {
						parseGameEnd(messageArray, count, raw, log);
						retVal = 2;
					
					// parse the error codes send by the server
					} else if (strcmp(messageArray[0], "GS_RESPONSE") == 0) {
						if (count != 4) {
							printf("Error: Wrong number of parameters\n");
						} else {
							for (int i = 0; i < count-1; i++) {
								printf(messageArray[i]);
								printf("|");
							}
							printf("%s\n", messageArray[count-1]);
							retVal = 3;
							if(strcmp(messageArray[2], "MI_ERROR_INVALID_ID") == 0) 
								retVal = 4;
						}
						// Done with the array - free it
						freeArray(messageArray, count);
					} else {
						printf("Recieved invalid OPCODE from game server\n");
						// Done with the array - free it
						freeArray(messageArray, count);
					}	
				}
			} else {
				printf("Error: Unexpected server");
				return -2;
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
	char *toParse;
	if ((toParse = malloc(strlen(response) + 1)) == NULL)
		exit(11); //malloc has failed
	// so we can process the response twice
	strcpy(toParse, response);
	
	if (themp->sin_family != AF_INET) {
		printf("Error: server is not AF_INET.\n");
		return 0;
	} 

	char *OPCODE;
	// should not recieve a message with more than one pipe from stdin
	char *respArray[3];
	
	int count = parsingMessages(response, respArray, "|");
	// since respArray was really just created to find out the number of params,
	// free ot
	for(int i = 0; i < count; i++) {
		free(respArray[i]);
	}
	if ( count == 1)
		// one parameter, means the user wants to send a status update	
		OPCODE = createStatus(toParse, thisGuide, raw, log);
	else if (count == 2) {
		// two parameters, means the user wants to send a hint
		OPCODE = createHint(toParse, agents, thisGuide, raw, log); 
	} else { 
		printf("Sorry that message doesn't seem to be formatted correctly\n");}
	if (OPCODE != NULL) {
		// confirm to the user attempting to send the formatted opcode
		printf("SENT: %s\n", OPCODE);
		// if verbose logging is on send the logger
		if(raw)
			logger(log, OPCODE);
		//send OPCODE to the server
		if (sendto(comm_sock, OPCODE, strlen(OPCODE), 0, 
					(struct sockaddr *) themp, sizeof(*themp)) < 0){
			printf("Error: sending in datagram socket\n");
			return -1;
		}
	}

	// clean up
	free(OPCODE);
	free(response);
	free(toParse);
	return 1;
}

/************************ internalUpdate *****************/
/* sends status updates directly to ther server
 * used when timeouts happen as well as when determining the guideId
 */
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
	// check that either a 0 or one was passed
	if (recieve == 0) {
		response = "0";
	} else if (recieve == 1) {
		response = "1";
	} else {
		printf("Error: statusreq must be a zero or a one\n");
		return -1;
	}

	// create the response to send
	OPCODE = createStatus(response, thisGuide, raw, log);
	if (OPCODE != NULL) {
		printf("SENT: %s\n", OPCODE); // alert the user that we are sending
		// if verbose logging is on, log the sent mesage
		if (raw)
			logger(log, OPCODE);

		// send message to the socket
		if (sendto(comm_sock, OPCODE, strlen(OPCODE), 0, 
					(struct sockaddr *) themp, sizeof(*themp)) < 0){
			printf("Error: sending in datagram socket\n");
			return -1;
		}
	}

	//clean up
	free(OPCODE);
	return 1;
}

/************** createStatus ***********************/
/* Puts the string from stdin in OPCODE form:
 * GA_STATUS|gameId|guideId|teamName|playerName|statusReq
 */

char *
createStatus(char *response, guideagent_t *thisGuide, bool raw, FILE* log)
{
	// all other params for the OPCODE
	char *gameId = thisGuide->gameId;
	char *guideId = thisGuide->id;
	char *teamName = thisGuide->teamName;
	char *name = thisGuide->name;
	char *statusReq;
	
	// make sure we have either a one or a zero
	if(strcmp(response, "0") == 0 || strcmp(response, "1") == 0)
		statusReq = response;
	else {
		printf("Sorry that message doesn't seem to be formatted correctly\n");
		return NULL;
	}
	
	//create the length for the OPCODE
	// the 15 is the length og "GA_STATUS" and the pipes
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
	
	// return the concatenated OPCODE
	return CODE;
}

/*************** createHint **********************/
/* Puts two pieces of a line read from stdin in OPCODE form:
 * GA_HINT|gameId|guideId|teamName|playerName|pebbleId|message
 */
char *
createHint(char *response, hashtable_t *agents, guideagent_t *thisGuide, 
		bool raw, FILE* log)
{
	// parameters to be parsed from the stdin line
	char *pebbleId;
	char *message;

	// make sure neither is null
	if ((pebbleId = strtok(response, "|")) == NULL){ 
			return NULL;
	}
	message = strtok(NULL, "|");
	if (message == NULL) {
			return NULL;
	}

	// check that the message is under 140 characters
	if (strlen(message) > 140) {
		printf("Message is too long\n");
		return NULL;
	}

	// check that the agent the user is trying to send to exists
	// and is on the user's team
	fieldagent_t *agent;
	if (strcmp(pebbleId, "*") != 0) {
		if ((agent = hashtable_find(agents, pebbleId)) == NULL) {
			printf("Sorry that agent doesn't exist\n");
			return NULL;
		} else if (strcmp(agent->team, thisGuide->teamName) != 0) {
			printf("You can only send hints to agents on your team, %s\n", thisGuide->teamName);
			return NULL;
		}
	}
   	
	// get relevant information from the guideAgent struct
	char *gameId = thisGuide->gameId;
	char *guideId = thisGuide->id;
	char *teamName = thisGuide->teamName;
	char *name = thisGuide->name;

	// create the length for the OPCODE
	// where 14 is the length of "GA_HINT" and all the pipes
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

	//return the concatenated hint
	return hint;
}


/********************* parseGameEnd ******************/
/* Prints the end game stats to stdout */

void
parseGameEnd (char **messageArray, int count, bool raw, FILE* log) {
	printf("----------GAME OVER----------\n");
	if(count >  3) {
			printf("Game Id %s\n", messageArray[1]);
			printf("Number of Remaining Code Drops %s\n", messageArray[2]);
	}
	if(count > 4) {
		char *teamsRecieved[50]; // Assuming there are no more than 50 teams
		int teamCount = parsingMessages(messageArray[3], teamsRecieved, ":");
	
		// Iterate through all the teams that have been passed
		for (int i = 0; i < teamCount; i++) {
			char *teamStats[10]; // Should only be stats passed
			// Split the game stats by comma and put them into an array 
			 int statCount = parsingMessages(teamsRecieved[i], teamStats, ",");
			 // check that there are 5 stats
			 if (statCount != 5) { 
				printf("Error: Server sent wrong number of stats\n");
				return; 
			 // assumes that the stats are in the correct order
			 } else {
				printf("Team Name : %s\n", teamStats[0]);
				printf("Number of Players : %s\n", teamStats[1]);
				printf("Number of Captures : %s\n", teamStats[2]);
				printf("Number Captured : %s\n", teamStats[3]);
				printf("Number of Drops Neutralized : %s\n", teamStats[4]);
			 }
		// clean up the arrays
		freeArray(teamStats, statCount);
		}
		// clean up the arrays continued
		freeArray(teamsRecieved, teamCount);
	}	
	freeArray(messageArray, count);
}

/********************** updateGame **********************/
/* Parses the array passed from handle_socket, checks some paramaters
 * calls updateAgents and updateCodeDrops when applicable
 * and calls hashtable_iterate on the agents and codeDrops arrays to 
 * print the updated code to stdin
 */
void
updateGame (char **messageArray, int count, hashtable_t *agents, hashtable_t *codeDrops,
		guideagent_t *thisGuide, bool raw, FILE* log) 
{
	if (strcmp(thisGuide->gameId,"0") == 0) { // Haven't gotten the gameId yet
		if((thisGuide->gameId = malloc(strlen(messageArray[1]) + 1)) == NULL)
		exit(11); //malloc has failed
		strcpy(thisGuide->gameId, messageArray[1]); // Set the gameId
	}

	if (count == 4) { //if there are 4 parameters, parse drops and agents
		char *agentsUpdate;
		if((agentsUpdate = malloc(strlen(messageArray[2]) +1)) == NULL)
			exit(11); //malloc has failed
		strcpy(agentsUpdate, messageArray[2]);
		
		char *dropsUpdate;
		if((dropsUpdate = malloc(strlen(messageArray[3]) +1)) == NULL)
			exit(11); //malloc has failed
		strcpy(dropsUpdate, messageArray[3]);
		
		// update the agent hashtable and the codedrop hashtable
		updateAgents(agentsUpdate, agents, raw, log);
		updateCodeDrops(dropsUpdate, codeDrops, raw, log);
	}
	if (count == 3) { // There are only codeDrops no agents
		char *dropsUpdate;
		if((dropsUpdate = malloc(strlen(messageArray[2]) +1)) == NULL)
			exit(11); //malloc has failed
		strcpy(dropsUpdate, messageArray[2]);
		
		// only need to update the codedrops
		updateCodeDrops(dropsUpdate, codeDrops, raw, log);
	}

	// print the new agents and code drops to stdout
	hash_iterate(agents, agentPrint, NULL);
	hash_iterate(codeDrops, codeDropsPrint, NULL);
	
	//clean up
	freeArray(messageArray, count);
}

/******************** updateAgents *****************/
/* updates the agents hashtable based on new information
 * provided by the game server
 */
void 
updateAgents(char *message, hashtable_t *agents, bool raw, FILE* log) 
{
	char *agentsRecieved[50];	// Assuming there are no more than 50 agents
	int agentCount = parsingMessages(message, agentsRecieved, ":");
	free(message);
	// Iterate through all the agents that have been passed
	for (int i = 0; i < agentCount; i++) {
		char *agentUpdate[10];// Should only be 7
		
		// Split data about the agent into an array split by commas 
		int paramCount = parsingMessages(agentsRecieved[i], agentUpdate, ",");
		free(agentsRecieved[i]);
		
		// There need to be 7 parameters based on the protocol
		if (paramCount != 7) {
			printf("Error: only %d parameters per agent\n", paramCount);
			return;
		}

		else {
			// Look for the agent in the hashtable, assumes pebbleId is in the first slot
			fieldagent_t *current = hashtable_find(agents, agentUpdate[0]);

			//Create temp variables for the non char* parameters
			int status;
			float lat;
			float lng;
			float lastcontact;
			
			if (current == NULL) {
				// check number parameters
				if(isItValidInt(agentUpdate[3]) && isItValidFloat(agentUpdate[4]) && 
						isItValidFloat(agentUpdate[5]) && isItValidFloat(agentUpdate[6])) {
					sscanf(agentUpdate[3], "%d", &status);
					sscanf(agentUpdate[4], "%f", &lat);
					sscanf(agentUpdate[5], "%f", &lng);
					sscanf(agentUpdate[6], "%f", &lastcontact);	
				
					//fieldagent doesn't exist, so create a new struct for it
					if ((current = malloc(sizeof(fieldagent_t))) == NULL)
						exit(11); //malloc has failed)

					// use given params to fill in the new field agent	
					if((current->team = malloc(strlen(agentUpdate[1]) +1)) == NULL)
						exit(11); //malloc has failed
					strcpy(current->team, agentUpdate[1]);
					
					if((current->name = malloc(strlen(agentUpdate[2]) +1)) == NULL)
						exit(11); //malloc has failed
					strcpy(current->name, agentUpdate[2]);
					
					// Parse status, 1 = captured, otherwise assume active
					// (the agent may be 'maybe-captured' but not important to GA
					if (status != 1)
						current->status="active";
					else 
						current->status="captured";
					
					// set numerical parameters
					current->lat = lat;
					current->lng = lng;
					current->lastcontact = lastcontact;
					
					//insert new fieldagent into the hashtable
					hashtable_insert(agents, agentUpdate[0], current);
					
					// log because a new agent has been added 
					char *toLog;	
					if((toLog = malloc(strlen(agentUpdate[2]) + 13)) == NULL)
						exit(11); //malloc has failed
					sprintf(toLog, "Agent %s added", agentUpdate[2]);
					logger(log, toLog);
					free(toLog);
				} else {
					printf("Error: Incorrectly formatted parameters for Agent '%s'\n", 
						agentUpdate[2]);
				}	
			
			} else {
				// Check numerical parameters
				if(isItValidInt(agentUpdate[3]) && isItValidFloat(agentUpdate[4]) && 
						isItValidFloat(agentUpdate[5]) && isItValidFloat(agentUpdate[6])) {
					sscanf(agentUpdate[3], "%d", &status);
					sscanf(agentUpdate[4], "%f", &lat);
					sscanf(agentUpdate[5], "%f", &lng);
					sscanf(agentUpdate[6], "%f", &lastcontact);	
					
					// free the memory allocated params so they can be replaced
					free(current->name);
					free(current->team);
					
					// check to see if the agent was previously active
					// so if the agent has been captured it can be logged
					bool wasActive = false;
					if (strcmp(current->status, "active") == 0)
						wasActive = true;	
					
					// update the memory allocated params
					if((current->team = malloc(strlen(agentUpdate[1]) +1)) == NULL)
						exit(11); //malloc has failed
					strcpy(current->team, agentUpdate[1]);
					if((current->name = malloc(strlen(agentUpdate[2]) +1)) == NULL)
						exit(11); //malloc has failed
					strcpy(current->name, agentUpdate[2]);
					
					// Parse status, 1 = captured, otherwise assume active
					// (the agent may be 'maybe-captured' but not important to GA
					if (status != 1)
						current->status="active";
					else { 
						current->status="captured";
						// if the status has changed, log it
						if(wasActive){
							char *toLog;
							if((toLog = malloc(strlen(agentUpdate[2]) + 16)) 
									== NULL)
								exit(11); //malloc has failed;
							sprintf(toLog, "Agent %s captured", agentUpdate[2]);
							logger(log, toLog);
							free(toLog);
						}
					}
					
					// set numerical parameters
					current->lat = lat;
					current->lng = lng;
					current->lastcontact = lastcontact;
				} else {
					printf("Error: Incorrectly formatted parameters for Agent '%s'\n", 
						agentUpdate[2]);
				}	
			} 
		} 
		// clean up
		freeArray(agentUpdate, paramCount);
	}
}

/******************** updateCodeDrops *****************/
/* updates the code drop hashtable based on new information
 * provided by the game server
 */
void 
updateCodeDrops(char *message, hashtable_t *codeDrops, bool raw, FILE* log) 
{
	char *codeDropsRecieved[50];//Assuming there are no more than 50 codeDrops
	int dropCount = parsingMessages(message, codeDropsRecieved, ":");
	free(message);
	//Iterate through all the drops that have been passed
	for (int i = 0; i < dropCount; i++) {
		
		char *dropUpdate[10];//Should only be 4
		
		//Split data about the code drop into an array split by commas 
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

			//Create temp variables for the numerical parameters
			float lat;
			float lng;
			
			// if the code drop is not in the hashtale yet
			if(current == NULL) {
				if((current = malloc(sizeof(codedrop_t))) == NULL)
					exit(11); //malloc has failed);

				// validate numerical parameters	
				if(isItValidFloat(dropUpdate[1]) && isItValidFloat(dropUpdate[2])) {
					sscanf(dropUpdate[1], "%f", &lat);
					sscanf(dropUpdate[2], "%f", &lng);
				
					if((current->team = malloc(strlen(dropUpdate[3]) +1)) == NULL)
						exit(11); //malloc has failed;
					strcpy(current->team, dropUpdate[3]);

					// if no team has neutralized the drop, it's active
					// otherwise it's neutralized
					if (strcmp(current->team, "NONE") == 0)
						current->status = "active";
					else
						current->status = "neutralized";
					
					// set numerical parameters
					current->lat = lat;
					current->lng = lng;

					//insert new fieldagent into the hashtable
					hashtable_insert(codeDrops, dropUpdate[0], current);
				} else {
					printf("Error: Incorrectly formatted parameters for drop '%s'", 
							dropUpdate[0]);
				}

			} else {
				//validate numerical parameters	
				if(isItValidFloat(dropUpdate[1]) && isItValidFloat(dropUpdate[2])) {
					sscanf(dropUpdate[1], "%f", &lat);
					sscanf(dropUpdate[2], "%f", &lng);
					bool wasActive = false;
					 //To tell if a drop was netralized
					if(strcmp(current->status,"active") == 0)
						wasActive = true;
					// free the only memory allocated parameter
					free(current->team);
					
					//update parameters
					if((current->team = malloc(strlen(dropUpdate[3]) +1)) == NULL)
						exit(11); //malloc has failed;
					strcpy(current->team, dropUpdate[3]);
					if (strcmp(current->team, "NONE") == 0)
					
					// if no team has neutralized the drop, it's active
					// otherwise it's neutralized
						current->status = "active";
					else {
						current->status = "neutralized";
						// If code drop was previously active and is now neutralized
						// log the neutralization
						if (wasActive) {
							char *toLog;
							if((toLog = malloc(strlen(dropUpdate[0]) + 23)) 
									== NULL)
								exit(11); //malloc has failed;
							sprintf(toLog, "Code Drop %s neutralized", dropUpdate[0]);
							logger(log, toLog);
							free(toLog);
						}
					}

					// update numerical params
					current->lat = lat;
					current->lng = lng;
				} else {
					printf("Error: Incorrectly formatted parameters for drop '%s'\n", 
							dropUpdate[0]);
				}
			}			
				
		}
		//clean up
		freeArray(dropUpdate, paramCount);	
	}
	//clean up
	freeArray(codeDropsRecieved, dropCount);
}

/*************** Destructor functions ****************/
/***************** cd_delete ************************/
/* deletes a code drop directory
 * frees the memory allocated data in the code drop struct
 * then frees the struct
 */
static void
cd_delete(void *data)
{
	if(data != NULL){
		free(((codedrop_t *) data)->team);
		free(data);
	}
}

/***************** fa_delete ************************/
/* deletes a field agent directory
 * frees the memory allocated data in the field agent struct
 * then frees the struct
 */
static void 
fa_delete(void *data) 
{
	if (data != NULL){
		free(((fieldagent_t *) data)->name);
		free(((fieldagent_t *) data)->team);
		free(data);
	}
}

/*********** agentPrint ***************************/
/* Function to be passed to hashtable iterate, 
 * prints a field agent stored in the hashtable
 */
void 
agentPrint(void *key, void *data, void *farg)
{
	fieldagent_t *current = data;
	printf("Agent %s\n", current->name);
	printf("Pebble ID: %s\n", (char *)key);
	printf("Team: %s\n",current->team);
	printf("Status: %s\n", current->status);
	printf("Latitude: %lf\n", current->lat);
	printf("Longitude: %lf\n", current->lng);
	printf("Seconds since last contact: %lf\n\n", current->lastcontact);
	printf("\n");
}

/*********** codeDropsPrint ***************************/
/* Function to be passed to hashtable iterate, 
 * prints a code drop stored in the hashtable
 */
void 
codeDropsPrint(void *key, void *data, void *farg)
{
	printf("Code Drop %s\n", (char *)key);
	codedrop_t *current = data;			
	printf("Latitude: %lf\n",current->lat);
	printf("Longitude: %lf\n",current->lng);
	printf("Status: %s\n", current->status);
	printf("Team: %s\n\n", current->team);

}

/************** randomHex ***************************/
/* generates a random 8 digit hex
 * stores it in a passed in string
 */
void 
randomHex(char *toBeHex)
{
	time_t t;
	char temp[9]; //Length of the hex
	/* Intializes random number generator */
	srand((unsigned) time(&t));
    const char *hex_digits = "0123456789ABCDEF";
    for(int i = 0; i < 8; i ++) {
		temp[i] = hex_digits[rand() % 16];
	}
	temp[8] = '\0';
	strcpy(toBeHex, temp);
}


/************************ SHOULD BE MOVED TO COMMON FILE **********************/
/*****************************************************************************/
/*****************************************************************************/


/******************** parsingMessages ******************/
/* Splits a line by a delimiter and puts it into a passed array
 */
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

	if((array[count] = malloc(strlen(word) +1)) == NULL)
		exit(11); //malloc has failed
	
	if (array[count]  == NULL) { 
		freeArray(array, count); //if it fails to allocate memory, delete the query
		return 0; //terminate the matching analysis
	}
	
	strcpy(array[count], word); //add the keywords into the query array
	count++; //increment the number of keywords counter
	return count;
}

bool 
isItValidInt(char *intNumber)
{

	int validInt = 0;
	char * isDigit;
	
	if ((isDigit = malloc(strlen(intNumber) +1)) == NULL) 
		exit(11); //malloc has failed
	
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
isItValidFloat(char *floatNumber){	
	double validFloat = 0;
	char * isDigit;
	if((isDigit = malloc(strlen(floatNumber) +1)) == NULL)
		exit(11); //malloc has failed
	
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
