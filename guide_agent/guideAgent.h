/*
 * guideAgent.h - Header file for the Mission Incomputable Guide Agent
 *
 * Topaz, 2016
 */

#ifndef __GUIDEAGENT_H
#define __GUIDEAGENT_H

/********** Structs ****************/
struct guideagent;
typedef struct guideagent guideagent_t;
/************ Functions ***********/


/* parse the remaining arguments and set up the socket.
 * Exit on any error of arguments in the setup.
 * Derived from the socket_setup outlined in David Kotz's chatclient
 */
static int socket_setup(char *GShost, int GSport, struct sockaddr_in *themp);

/* stdin has input ready; read a line and parse it.
 * return EOF if EOF was encountered in stdin
 * return 0 if there is no client to whom we can send
 * return 1 if the message is sent successfully
 * return -1 on any socket error
 * Derived from the handle_stdin function in David Kotz's chatclient
 */
static int handle_stdin(int comm_sock, struct sockaddr_in *themp, hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log);

/* sends status updates directly to ther server
 * used when timeouts happen as well as when determining the guideId
 */
static int internalUpdate(int comm_sock, int recieve, struct sockaddr_in *themp, hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log);

/* Puts the string from stdin in OPCODE form:
 * GA_STATUS|gameId|guideId|teamName|playerName|statusReq
 */
char *createStatus(char *response, guideagent_t *thisGuide, bool raw, FILE* log);

/* Puts two pieces of a line read from stdin in OPCODE form:
 * GA_HINT|gameId|guideId|teamName|playerName|pebbleId|message
 */
char *createHint(char *response, hashtable_t *agents, guideagent_t *thisGuide, bool raw, FILE* log);

/* Socket has input; recieve and parse it
 * 'themp' should be a valid address, ignore messages from other senders
 * return -1 on any socket error
 * return -2 on unexpected server
 * return 1 if GAME_STATUS
 * return 2 if GAME_OVER
 * return 3 if GS_RESPONSE
 * return 4 if GS_RESPONSE is MI_ERROR_INVALID_ID
 */
static int handle_socket(int comm_sock, struct sockaddr_in *themp, hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log);

/* Prints the end game stats to stdout */
void parseGameEnd (char **messageArray, int count, bool raw, FILE* log);

/* Parses the array passed from handle_socket, checks some paramaters
 * calls updateAgents and updateCodeDrops when applicable
 * and calls hashtable_iterate on the agents and codeDrops arrays to 
 * print the updated code to stdin
 */
void updateGame (char **messageArray, int count, hashtable_t *agents, hashtable_t *codeDrops, guideagent_t *thisGuide, bool raw, FILE* log);

/* updates the agents hashtable based on new information
 * provided by the game server
 */
void updateAgents(char *message, hashtable_t *agents, bool raw, FILE* log);

/* updates the code drop hashtable based on new information
 * provided by the game server
 */
void updateCodeDrops(char *message, hashtable_t *codeDrops, bool raw, FILE* log);

/* deletes a field agent directory
 * frees the memory allocated data in the field agent struct
 * then frees the struct
 */
static void fa_delete(void *data);

/* deletes a code drop directory
 * frees the memory allocated data in the code drop struct
 * then frees the struct
 */
static void cd_delete(void *data);

/* Function to be passed to hashtable iterate, 
 * prints a field agent stored in the hashtable
 */
void agentPrint(void *key, void *data, void *farg);

/* Function to be passed to hashtable iterate, 
 * prints a code drop stored in the hashtable
 */
void codeDropsPrint(void *key, void *data, void *farg);

/* generates a random 8 digit hex
 * stores it in a passed in string
 */
void randomHex(char *toBeHex);

/* THE REST OF THESE SHOULD BE MOVED TO A COMMON FILE */
int parsingMessages(char* line, char ** messageArray, char *delimiter);
void freeArray(char** array, int size);
int copyValidKeywordsToQueryArray( char ** array, char* word, int count);

bool isItValidInt(char *intNumber);
bool isItValidFloat(char *floatNumber);

void logger(FILE *fp, char* message);

#endif // __GUIDEAGENT_H
