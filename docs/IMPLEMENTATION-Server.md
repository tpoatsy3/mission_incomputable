# Implementation Spec for Mission Incomputable
# Team Topaz

## Game Server

### EXTENSIONS IMPLEMENTED IN THE SERVER

	 - LEVEL 3
		void GAGameStatusEnemyIteratorThree(void *key, void* data, void* farg);
		void GAGameStatusEnemyHashIteratorThree(void *key, void* data, void* farg);
	
	for details of testing, check TESTING.md
	
	 -  HAVERSINE FORMULA
		double dist(double x1, double y1, double x2, double y2);
	
	for details of testing, check TESTING.md
	
	-   GAME DURATION
		Inside the `while` loop of the `game server` function (L343 - L348)
		bool game_server (char *argv[], hashStruct_t *allGameInfo)
		
	-   ASCII MAP
		
		void asciiDrawing(hashStruct_t *allGameInf);
		void printingDropCode(void *key, void* data, void* farg);
		void printingPlayer(void *key, void* data, void* farg);
		double getYDimension(double num, double max_y);
		double getXDimension(double num, double max_x);
		
### Psudocode

The way we implement the `game server` is by going throw the detailed pseudo-code step by step, and verify the method, results, and any memory leaks before we move to the next step. In case, a data structure needs to be deleted, a `delete` function will be used at the end of the code to ensure accuracy in detecting any memory leaks. Also, we are planning to leverage the `list` and `counters` data structures and pre-tested functions.
The Game server code will follow roughly the following outline:


1. Execute from the command line with usage syntax
    * `./gameserver [--log] [--game= ####] [--level = 1 or 3] [--time (in Minutes)] [-a (ascii display)] codeDropPath GSport`
    * where `codeDropPath` represents the path name for the code-drop file, 
    * where `GSport` is the game server port number,
    * where `time` is the duration of the game in minutes
    * `[--log=raw]` for more extensive logging option
    * `[--level]` to choose level 1 or 3 of the game (1 by default)
	* `[--time] to choose a duration of the game or infinity by default`
	* `[-a] ascii map
	* `[--game= ####] game ID selcted by the user up to 8 hex or random`


2. parse the command line, validate parameters, initialize other models
    - confirming that `codeDropPath` is not `NULL`
    - confirming that `codeDropPath` has the right format
    - confirming that `time` has the right format (digits only), if zero, it is considered unlimited
    - create a deaddrop code structure for each code to be neutralized (lat, long, status, neutralzing team)
    - count the number of deaddrops
	- create a master struct that can point to all the lists in the game (FA list, GA list, and codedrop list)
    - create a list of pointers to deaddrop code structures with the specific Hex code as the key
    - confirming that `GSport` is not `NULL` and has valid digits and only digits
    - setup the socket on which to receive messages using `GSport`
        - create socket on which to listen
        - name and bind to socket using `GSport`
        - exit if setting up the socket encounters any errors
3. create a random game number between 1 and 65535 for the new game if a game ID was not provided by the user
4. create a new statistics structure that includes starting time, number of remaining code drops, and information about each team statistics
4. create a logfile `guideserver.log` in a log directory
    - update the log with start-up information (host/port where server is running, number of deaddrops, game number, game statistics information, ..etc)
5. while there are deaddrops to be neutralized or error happens or the termination flag is not set or the time is not expired
    - draw the ASCII map if the option was given
        - For each active agent in the list, draw  their location on the map
        - For each active code drop in the list,  draw its location on the map
    - By using `select`, we will listen for messages from either the server or the guide
	- our timeout is only 5000 millisecond
    - receive `OPCODE` messages from guide agents and/or field agents
    - validate that the message is not an empty string
    - log the message (if the raw option chosen) into the log file
    - parse the `OPCODE` and validate it(proper `OPCODE`)
        - if it is not a proper `OPCODE`, an error code sent to any agent with a proper message (GS_RESPONSE)
        - log it in the logfile
    - parse and validate the subsequent messages until the message string is `NULL` -  tokenize by pipes
        - if `gameID` is not valid (matching the above game number), an error code sent to any agent with a proper message (GS_RESPONSE)
        - if `teamName` is not valid (matching the above game number), an error code sent to any agent with a proper message (GS_RESPONSE)
    - process the messages according to OPCODE as stated below.
    - update the statistics structure
        - use the computer time to detect the current time and compare it to the starting time to calculate the duration of the game
        - update the remaining dead drops count by counting the number of codes that haven't been neutralized
        - scan the field agent list for statistics regarding each team (using the data fields and flags in the field agent structure)
6. Send a `gameover` message to all players including the gameID, number of remaining active code drops (if the game ends early), and each team's statistics
6. close the socket
7. delete all the `lists` and `counters` created 

&nbsp;

**Receivable OPCODE messages Pseudocode:**

* `GA_HINT`
    * Check to see if message has the correct number of arguments
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If the `pebbleID` is valid
        * Forward the message with the same information as given to that `pebbleID`.
    * If the `pebbleID` is `*`
        * Get a reference to the message `teamName` value
        * Cycle through the list of player structs
            * If the `teamName` matches
                * Get the `pebbleID` of that pebble
                * Change the `pebbleID` of the given message with current player’s `pebble ID`.
                forward the message to the pebble
                * Update `last-contact-time` of GA
                * log it
* `GA_STATUS`
    * Check to see if message has the correct number of arguments
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If `gameID` is 0, then register the player
        * Check to see if the player exists
            * If it does, report `MI_ERROR_INVALID_GAME_ID`
            * If it doesnt, create a new player struct and initialize a new GA player
                * Record the `remoteAddress`, `remotePort`, `playerName`, `teamName`, `agentID`, `last-contact-time` in the GA player struct.
                * add the new guide to the log file
        * Check to see if the team exists
            * if it does not, then add the team to the list of teams
        * Add the player to the list of players
    * If `gameID` is the `gameID`
        * Update last contact time of the GA player
        * If `statusReq == 1` send a `GAME_STATUS` message back to that `playerID`
* `FA_LOCATION`
    * Check to see if message has the correct number of arguments
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If `gameID` is 0, then register the player
        * Check to see if the pebbleID is unique
        * If it is unique, create a new player struct and initialize a new FA player
            * Record the `remoteAddress`, `remotePort`, `playerName`, `teamName`, `pebbleID`, `status`, `last-contact-time`, `captureCode`, and `capturedBy`, `long`, and `lat` in the FA player struct.
            * If the teamName doesnt already exists, then add the team to the list of teams
            * Add the player to the list of players
            * * add the new field player to the log file
        * If it is and marked captured, do nothing
        * If it is not and not marked captured
            * Update the player’s current lat and long in that player’s FA player struct
            * Update `last-contact-time` in the FA player struct.
    * If `statusReq == 1`, send a `GAME_STATUS` message back to that FA.
* `FA_NEUTRALIZE`
    * Check to see if the message has the correct number of arguments.
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * For each codeDrop
        * If codeDrop is the same as givenCodeDrop,
            * If the code status is neutralized, send it is already neutralized message.
                * Confirm that the lat and long of the codeDrop and the lat long of the FA player are within 10m.
                * If they are within 10m, set the code to neutralized
                    * Send a MI_NEUTRALIZED message
                    * log it
* `FA_CAPTURE`
    * Check to see if the message has the correct number of arguments.
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If `captureID` is 0
        * save the capturing player's lat and long
        * for each player in the player list
            * calculate the distance between players
            * if less than 10m, send that player a GS_CAPTURE_ID message
    * If `captureID` is not 0
        * cycle through the players in the player list
            * if the `captureID` is the same as the player's `captureCode`, set that player's `status` to `caputured`
            * set `capturedBy` to the `teamName` in the OPCODE.
            * send `MI_CAPTURED` message to the captured FA player
            * send `MI_CAPTURED_SUCCESS` message to the capturing FA player
            * log it 
    * Update capturing FA player's `last-contact-time`
* `GS_CAPTURE_ID`
    * Check to see if the message has the correct number of arguments.
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)

**Send-able OPCODE messages Pseudocode for FIELD AGENT:**

* `GAME_STATUS`
    * Create a message that has the following components in the following order:
        * `gameID` is the game number.
        * `timeleft` to the end of the game
        * `guideID` is the identifier of the team's Guide Agent.
        * `NumRemainingCodeDrops` is the number of active code drops.
        * `NumFriendlyOperatives` is the number of active FAs on the same team as the requester
        * `NumFoeOperatives` which is the number of active FAs that are not on the player's team
    * log it
* `GA_HINT`
    * This message will mostly be forwarded from from the Guide Agent to the Field Agent mostly without any changes.
    * It will have the following parts:
        * `gameID` is the game number
        * `guideID` is the guide identifier
        * `teamName` is the name of the team.
        * `pebbleID` is the ID of the pebble that is being contacted (at this point there will be no '*' character)
        * `messages` is the hint message.
    * log it


**Send-able OPCODE messages Pseudocode for GUIDE AGENT:**

###### Level 1
* `GAME_STATUS|gameID|timeleft|fa1:fa2:fa3...faN|cd1:cd2:cd3:...:cdM`
    * Create a message that has the following components of the above format:
        * `gameID` is the game number.
        * `timeleft` to the end of the game
        * `fa` followed by a number represent each of the Field Agents. Field Agents are separated by colons and their information in the struct is separated by commas. The information included for each Field Agent is each section of the FA player struct.
        * `cd` followed by a number represent each of the Code Drops. Code Drops are separated by colons and their information in the struct is separated by commas. The information included for each Code Drop is the `codeID`, `lat`, `long`, and `neutralizingTeam`
   * log it
###### Level 2

* `GAME_STATUS|gameId|fa1:fa2:fa3...faN|fa1cd1:fa1cd2:...:fa1cdM|fa2cd1:fa2cd2:...:fa2cdM|...|faXcd1:faXcd2:...:faXcdM`
    * Create a message that has the following components of the above format:
        * `gameID` is the game number.
        * `fa` followed by a number represent each of the friendly Field Agents. Field Agents are separated by colons and their information in the struct is separated by commas. The information included for each Field Agent is only the `long` and the `lat` values.
        * `fa#cd` followed by a number represent each of the Code Drops. For each field agent/code drop combination, there will be a direction and a relative distance (near, close, or far). These two values will be separated by commas.
   * log it




### Data structures (e.g., struct names and members)

- Field Agent Struct: Stores `remoteAddress`, `remotePort`, `playerName`, `teamName`, `status`, `last-contact-time`, `captureCode`, and `capturedBy`, `long`, and `lat`
- Guide Agent Struct: Stores `remoteAddress`, `remotePort`, `playerName`, `teamName`, `last-contact-time` 
- Code Drop Struct: Stores `lat`, `long`, `status`, and `neutralizing_team`
- game statistics structure: stores starting time, number of remaining code drops, and information about each team statistics
- UDP structure: from the standard library
- A utility struct that has 5 general pointers
- A master information struct that points to all the struct
- list of all the field players  with a pebbleID as the key
- list of all the guide players with a guideID as the key


*Security and privacy properties, Error handling and recovery*

1. Helper functions within structure codes to prevent user access to data structures
2. Defensive coding against incorrect inputs
3. `valgrind` command and memory.c

&nbsp;

*Persistent storage (log files)*

-We will only keep two kinds of log files
    - the game server log file (gameserver.log)
    - the guide agent logfile (guideagent.log)

&nbsp;
