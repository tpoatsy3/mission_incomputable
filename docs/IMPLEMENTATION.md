# Implementation Spec for Mission Incomputable
# Team Topaz

##Field Agent
### Pseudocode

1. call main() function when app opens
2. send message to server that guide agent has entered game with game number 0, the pebbleID, the team name and player name
3. call init() function to set up UI elements
    1. create the window with window_create()
    2. set the window handlers to window_load and window_unload
    3. push the window to the top of the stack
4. call window_load() to actually load these elements
    1. create a text layer for the status
    2. create a simplemenulayer with three options
        1. neutralize code drop
        2. capture player
        3. view messages
5. call app_event_loop to wait for clicks
    1. if server sends a message to pebble
        1. parse the status and put it into the string
        2. update status
        3. put that message into the list of messages
    2. if neutralize code drop is pressed
        - call select_number() four times
            1. each time will load a simplemenulayer with choices from 0-10 and retain each choice
            2. call confirm_choice()
                - will load menu similar to first
                    1. textlayer is the code chosen
                    2. menu options are confirm or cancel
                    3. if confirm is pressed, calls the (FA_NEUTRALIZE OPCODE) with the same information as join game except for the updated gameID and the code ID (4 digit hex)
                    4. returns to original screen
    3. if capture player is pressed
        - call select_number() four times
            1. each time will load a simplemenulayer with choices from 0-10 and retain each choice
            2. call confirm_choice()
                - will load menu similar to first
                    1. textlayer is the code chosen
                    2. menu options are confirm or cancel
                    3. if confirm is pressed, calls the (FA_CAPTURE OPCODE) with the same information as above except including the captured player’s hex code and does not include location 
                    4. returns to original screen
    4. if view messages is pressed
        1. displays a text layer
        2. loads each message from the list into the text layer
6. call window unload to destroy window elements
    1. destroy the text layer
    2. destroy the simplemenulayer
7. call deinit() to destroy app elements
    - call window_destroy on the window element

### Data Structures

– Window Struct: holds the UI elements of the Pebble face, declared in the Pebble SDK
– TextLayer Struct: a UI element that displays text
– SimpleMenuLayer Struct: a UI element that allows you to display and select an item from a menu
– List Struct: holds messages recieved by the server


## Guide Agent
### Pseudocode
1. execute from the commandline with usage syntax
    - `./guideagent [-log=raw] teamName GShost GSport [guideId]`
    - where `-log=raw` is the optional logging mode
    - where `teamName` is the Team name
    - where `GShost` is the Game Server host
    - where `GSport` is the Game Server port number
    - where `guideId` is the optional guideId
2. Create a logfile
3. look up the Game Server host  
4. if host is null  
    - error and quit 
5. initialize fields for the server address  
6. create the socket  
7. send OPCODE notifying the server that a guide has joined
8. Recieve and save gameNumber
9. Update logfile with initialization information (host and port)
10. create guide agent struct
11. Initialize a hashtable of agents
12. Initialize a hashtable of code drops
13. Initialize a list of notifications
14. while game is in play  
    1. draw GUI using gtk
        1. For each agent in the hashtable draw their location on the map
        2. For each code drop in the hashtable draw its location on the map
    2. using select wait for input from either the server or the guide   
    3. if OPCODE is recieved 
        1. if in raw-mode
            - log the OPCODE
        1. parse OPCODE - tokenize by pipes   
        2. validate that OPCODE was correctly formatted/acceptable  
        3. if code drop was updated (only update *should* be neutralization)
            1. update code drop in the code drop hashtable 
            2. log code drop neutralization
        4. if agent struct was updated  
            1. update agent struct in the agent hashtable  
            2. if agent was captured or new agent joined
                - log capture/join in logfile
        5. add notification to the list of notifications  
        6. updateGUI  
    4. if hint is entered  
        1. read hint and verify length
        2. create properly formattedOPCODE
        3. pass OPCODE to the game server  
15. close the socket

### Data Structures
- Field Agent Struct: Stores a name, status, location, and team
- Guide Agent Struct: Stores a name and status (corresponding to the guide agent running the program)
- Code Drop Struct: Stores a string name, a string status, and a double location
- Hashtables of Field Agents: Hashtable storing all field agents on all teams if level 1 and only field agents on the guide's team if level 2
- Hashtable of Code Drops: Hashtable storing all the code drops
- List of Notifications: Record of previous notifications recieved from the game server, i.e. when new agents are added, locations are updated, or statuses are updated

## Game Server
### The Game Server can recieve the following `OPCODE` messages:
* `GA_HINT`
    * Check to see if message has the correct number of arguments
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If the `pebbleID` is valid
        * Forward the message witht the same information as given to that `pebbleID`.
    * If the `pebbleID` is `*`
        * Get a reference to the message `teamName` value
        * Cycle through the list of player structs
            * If the `teamName` matches
                * Get the `pebbleID` of that pebble
                * Change the `pebbleID` of the given message with current player’s `pebble ID`.
                forward the message to the pebble
                * Update `last-contact-time` of GA
* `GA_STATUS`
    * Check to see if message has the correct number of arguments
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)
    * If `gameID` is 0, then register the player
        * Check to see if the player exists
            * If it does, report `MI_ERROR_INVALID_GAME_ID`
            * If it doesnt, create a new player struct and initialize a new GA player
                * Record the `remoteAddress`, `remotePort`, `playerName`, `teamName`, `agentID`, `last-contact-time` in the GA player struct.
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
            * If the code status is neutralized, break.
                * Confirm that the lat and long of the codeDrop and the lat long of the FA player are within 10m.
                * If they are within 10m, set the code to neutralized
                    * Send a MI_NEUTRALIZED message
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
    * Update capturing FA player's `last-contact-time`
* `GS_CAPTURE_ID`
    * Check to see if the message has the correct number of arguments.
    * Validate all of the given elements of the message (`gameID`, `guideID`, `teamName`, `playerName`)

### The Game Server can send the following `OPCODE` messages to Field Agents:
* `GAME_STATUS`
    * Create a message that has the following components in the following order:
        * `gameID` is the game number.
        * `guideID` is the identifier of the team's Guide Agent.
        * `NumRemainingCodeDrops` is the number of active code drops.
        * `NumFriendlyOperatives` is the number of active FAs on the same team as the requester
        * `NumFoeOperatives` which is the number of active FAs that are not on the player's team
* `GA_HINT`
    * This message will mostly be forwarded from from the Guide Agent to the Field Agent mostly without any changes.
    * It will have the following parts:
        * `gameID` is the game number
        * `guideID` is the guide identifier
        * `teamName` is the name of the team.
        * `pebbleID` is the ID of the pebble that is being contacted (at this point there will be no '*' character)
        * `messages` is the hint message.


### The Game Server can send the following `OPCODE` messages to Guide Agents:
* `GAME_STATUS|gameID|fa1:fa2:fa3...faN|cd1:cd2:cd3:...:cdM`
    * Create a message that has the following components of the above format:
        * `gameID` is the game number.
        * `fa` followed by a number represent each of the Field Agents. Field Agents are separated by colons and their information in the struct is separated by commas. The information included for each Field Agent is each section of the FA player struct.
        * `cd` followed by a number represent each of the Code Drops. Code Drops are separated by colons and their information in the struct is separated by commas. The information included for each Code Drop is the `codeID`, `lat`, `long`, and `neutralizingTeam`


