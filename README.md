NOTE: The top `makefile` only runs Guide and the server. Use the makefile inside the field agent to compile it

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

**COMPILING:**

We have two ways for compiling the lab for this week.

- Either from a top-level Makefile inside tse


	> Type `make` in the terminal in the top level directory `project-starter-kit` to call main `Makefile` of `server` in ./game_server which compiles `gameserver`. It will compile all the files in the library of abstract structures (specifically hashtable) into `cs50ds.a` library and `file.c` in `common.a`-> then the user needs to call the executables (in this case `gameserver`) following the right format.
	
	&nbsp;
	
	> Type `make` in the terminal in the `game_server` directory, which compiles `gameserver`. It will compile all the files in the library of abstract structures (specifically hashtable) into `cs50ds.a` library and `file.c` in `common.a`-> then the user needs to call the executables (in this case `gameserver`) following the right format.
	
	&nbsp;

	> type `make clean` to delete all object files, executables, libraries, and any other intermediate files.

&nbsp;

**MAKEFILE:**

	# Makefile for 'server' data type
	#
	# Topaz, May 2016
	
	#path of libraries
	L = ./lib
	C = ./common
	
	PROG = gameserver
	OBJS = server.o 
	LLIBS = $L/cs50ds.a
	CLIBS = $C/common.a
	
	CC = gcc
	CFLAGS = -Wall -pedantic -std=c11 -ggdb -I$L -I$C -lm -std=gnu99 -lncurses
	MAKE = make
	
	#compiling
	$(PROG): $(OBJS) $(LLIBS) $(CLIBS)
		$(CC) $(CFLAGS) $^  -o $@
	
	server.o:
	
	# build the libraries
	$L/cs50ds.a: 
		cd $L; $(MAKE)
	
	$C/common.a: 
		cd $C; $(MAKE)
	
	#cleaning
	clean: 
		rm -f *~
		rm -f *.o
		rm -f $(PROG)
		cd $L; $(MAKE) clean
		cd $C; $(MAKE) clean



&nbsp;

> hashtable.c 		
> 
> hashtable.h 		
> 
> Makefile(hashtable)
> cs50ds.a 		
> 
> Makefile(for the cs50ds.a library)


> server.c 		
>
> word.c 		
> 
> word.h 		
> 
> file.c
>
> file.h
>
> Makefile(for the common.a library)
>
> Log directory
	
&nbsp;
		
**USAGE:**
	
	/*
	Mission Incomputable!
	Team Topaz
	
	gameserver.c - The game server application coordinates one and only one game each times it runs.
	it interacts with the field agent through pebble and proxy (UDP) and the guide agent though UDP 
	communication
	
	May, 2016
	Ihab Basri, Ted Poatsy
	*/

&nbsp;

**EXAMPLE COMMAND LINES:**

	[hooby@wildcat ~/cs50/projects/project-starter-kit/game_server]$ make
	gcc -Wall -pedantic -std=c11 -ggdb -I./lib -I./common -lm -std=gnu99 -lncurses   -c -o server.o server.c
	gcc -Wall -pedantic -std=c11 -ggdb -I./lib -I./common -lm -std=gnu99 -lncurses server.o lib/cs50ds.a common/common.a  -o gameserver

	[hooby@wildcat ~/cs50/projects/project-starter-kit/game_server]$ gameserver codeDrop 12345
	Game Started
	Host: wildcat.cs.dartmouth.edu
	Port: 12345

	[hooby@wildcat ~/cs50/projects/project-starter-kit/game_server]$ gameserver --time=10 -a --level=3 --log=raw --game=5555 codeDrop 12345
	Game Started
	Host: wildcat.cs.dartmouth.edu
	Port: 12345




**this is just a snippet of the output when running `make` (for clarity).  

&nbsp;

**EXIT STATUS:**

> 0 - success
> 
> 1 - improper flags/options were provided
> 	
> 2 - more or less than 2 mandatory parameters were provided
> 	
> 3 - improper port was provided
>
> 4 - Invalid code drop was provided

> 5 - something went wrong in running the game for multiple reasons
		(it could be because log directory was not provided, failed to bind to a socket... etc). A message describing what's wrong will be printed on the screen
> 11 - memory allocation problems

&nbsp;

**ASSUMPTION:**

- A log directory is created to store the log file 

- The ascii will run on a full screen terminal (66 X 210)

- The server needs the guide agent and field agent to be connected into the same port

- The game is played on Dartmouth campus or it will not be displayed on the ascii map

- The maximum filename inside the log directory is a 100 digit number. 

- A codeDrop file with a valid format needs to be provided to store the information and run the game

- The guide agent will not go idle

- The field agent will not go idle

- The field agent re-enter the same team and gameId if they are get disconnected due to internet issue

- Any hexcode used in communication between the server and/or field and guide agents should not exceed 8 hexcodes

- it is assumed that the guide agents and field agents will request a status back if they are trying to register to the game, or they will not know the gameID and will be considered existing players (failed to communicate with them since the gameID does not match while their pebbleId is registered) 

- The hexcode received is actually a hexcode (0-F)

&nbsp;

**LIMITATIONS:**

- memory leaks when the ascii map option is chosen. It is lcurser memory leak. We tested it on basic starting closing the library and it was leaking (confirmed it is not our code)

- character overwriting each other when they happen to overlap on the map

- If guide agent joined the game but no other field agent joined that same team before the end of the game, this guide agent's team will not be considered a team in the statistics sent at the end

- Our server did not communicate directly with the pebble. So we have no way to confirm that we are processing and responding to and from the pebble correctly. However, we created a test cases where we used the pebble was connected and our code worked perfectly.

- The server user cannot end the game before the time is up or no remaining codes left  when ascii display is chosen. However, it works perfectly without the ascii (see documentation)

- The server user will not be able to see the inbound or outbound messages (textual update) when the ascii is displayed


**Documentation:**

- type quit and `EOF` to end the game

- the game needs to be connected to a free socket
