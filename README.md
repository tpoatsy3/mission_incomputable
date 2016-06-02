# Mission Incomputable
Virginia Cook Ihab Basri Kyra Maxwell Ted Poatsy
## Readme for GuideAgent 
Compiling: See Makefile  
Usage: `./guideAgent [-v|-log=raw] [-id=########] teamName playerName GShost GSport`  
Topaz, May 2016

guideAgent.c communicates with the Game Server via the protocol to receive updates about the current location and status of players on guide’s team, of other team’s players, and of code drops and prints them to stdin, and asks the Game Server to send hints to a given player as well as provides it will status updates.

###### Functions:
- static int socket_setup: Parses args and sets up the socket
- static int handle_stdin: stdin has input ready, read a line and parse it.
- static int internalUpdate: sends status updates directly to ther server
- char *createStatus: Puts the string from stdin in OPCODE form: `GA_STATUS|gameId|guideId|teamName|playerName|statusReq`
- char *createHint: Puts two pieces of a line read from stdin in OPCODE form: `GA_HINT|gameId|guideId|teamName|playerName|pebbleId|message`
- static int handle_socket: Socket has input; recieve and parse it
- void parseGameEnd: Prints the end game stats to stdout
- void updateGame: Parses the array passed from handle_socket, updates and prints code drops and agents when applicable
- void updateAgents: updates the agents hashtable based on new information provided by the game server
- void updateCodeDrops: updates the code drop hashtable based on new information provided by the game server
- static void fa_delete: deletes a field agent directory, frees the memory allocated data in the field agent struct then frees the struct
- static void cd_delete: deletes a code drop directory, frees the memory allocated data in the code drop struct then frees the struct
- void agentPrint: Function to be passed to hashtable iterate, prints a field agent stored in the hashtable
- void codeDropsPrint: Function to be passed to hashtable iterate, prints a code drop stored in the hashtable
- void randomHex(char *toBeHex): generates a random 8 digit hex, stores it in a passed in string
- int parsingMessages: parses a char *, tokenizes it by a given delimiter, puts the tokens into a provided array
- void freeArray: Frees an array and its contents
- int copyValidKeywordsToQueryArray: Checks that words are valid before putting them in a passed array
- bool isItValidInt: checks that a char * is a valid int
- bool isItValidFloat: checks that a char * is a valid float
- void logger: formats text to be logged

###### Exit Status:
0 - Success  
1 - Too Few Parameters  
2 - Flags in the wrong order  
3 - GSport is not a number  
4 - Unable to create logfile  
5 - Error setting up the socket  
6 - Socket error in function internalUpdate  
7 - Unable to parse socket response  
8 - Game start was not approved  
9 - Error with select response  
10 - Unable to parse stdin message  
11 - Malloc failed  
12 - Hashtable creation failed  

###### Assumptions and Limitations:
- Assumes that all non-numerical parameters read from the server are accurate
- Assumes that hexcodes coming from the server are accurate
- Assumes there is a log directory in the current directory
- Certain things are limited by buffers, ie assumes when reading from the socket there will be no more than 50 teams, 50 players, or 50 code drops at one time - would need to be manually updated if the game was played on a large scale

## Readme for Field Agent
A Pebble adventure game. 

##### Compiling
Either "make" from top level directory or "pebble build" from field agent directory if you only wish to compile this part of the app. 

##### Usage
This is an app for the Pebble watch that keeps track of a user's location and direction and sends messages every time the user is on the move. From the Pebble watch the user is able to neutralize a code, capture a player and view messages. The user's status and direction is displayed at the top of the home screen of the watch. 

From the home page of the app the user has four options: Neutralize Code, Code Drop, Request Status and Messages. If neutralize code is pressed, then a drop down menu of characters 0-9 and A-F is presented for the user to input the four-digit hex code of the code to neutralize. If capture is pressed, then the same keyboard is presented, allowing the user to also select a four digit hex. For both of these options, after the hex has been inputted the user is taken to a confirmation page, and upon pressing "Yes" the app sends the code to the server, specifying whether it was a code drop or a player captured. The request status button sends a message to the server to return the status of the game. The messages button takes the user to a list of messages that have come from the server. 

If the player is on the move, then it will send its location to the server every fifteen seconds. If the player is stationary then it only sends once a minute. 

##### Limitations
Because we could not get the Pebble to connect to the server we could not test much of the functionality. 
