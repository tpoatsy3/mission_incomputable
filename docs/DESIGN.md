# Design Spec
![alt text](MI-network.png "Network Diagram courtesy of Travis Peters")
### Field Agent
##### User Interface
- The field agent is run through a pebble watch connected to a smartphone. The user interacts through buttons on the pebble watch
    - Status at the top
    - Three buttons
        -  Neutralize Code Drop -> enter hex code -> success of failure
        - Capture Player -> enter hex code -> success or failure
        - Request for status: which includes but not limited to:
            - A game status information
            - Time left for the game
            - Numbers of players left
            - Number of dead drops left
   

##### Inputs
- Hints: Helpful messages from the Guide Agent
- Targeted Player Codes: 4-digit hex code indicating this Field Agent is target of capture
- Code drops or Capture player Success: indication that this Field Agent has been successfully captured or the code has been neutralized
##### Outputs
- `FA_LOCATION|gameId|pebbleId|teamName|playerName|lat|long|statusReq`
- Where:
    - `gameId` is an integer indentifying the game
    - `pebbleId` is the specific ID code baked into the pebble
    - `teamName` is the name of the team that the Field Agent is on
    - `playerName` is the identifying name of a Field Agent
    - `lat` is the Field Agent’s Latitude
    - `long` is the Field Agent’s Longitude
    - `status` is the Field Agent’s Status (Active, Captured, Targeted, Capturing)
##### Functional Decomposition into Modules
- `joinGame()` - Sends a message (`FA_LOCATION OPCODE`) to the server that this guide agent has entered the game with the game number 0, pebbleID, team name,  and player name to be registered by the game server
- `onNeutralizePressed()` – a class called when neutralized button is pressed, it calls the (`FA_NEUTRALIZE OPCODE`) with the same information as join game except for the updated gameID and the code ID (4 digit hex)
- `onCapturePressed()` – a class called when capturing button is pressed, it calls the (`FA_CAPTURE OPCODE`) with the same information as above except including the captured player’s hex code and does not include location 
- `onMessagesPressed()` – a class called when messages is pressed
- `sendLocation()` – a class to send the Game Server the player’s current location four times per minute
- `codeDropNeutralized()` – informs the Game Server when this player neutralizes a code drop, returns a 4-digit hex code to server
- `playerCaptured()` – informs the Game Server when this player has captured another player, returns a 4-digit hex code to server
- `readCode()` – a class to open a keyboard and send the information from it to the server indicating to the server whether it is code to be neutralized or a player to be captured, takes an int determining whether it was opened by `onNeutralizePressed()` or `onCapturedPressed()`
- `onEnterPressed()` – a class called when enter button in `readCode()` UI is pressed
- `broadcastReciever()` – get messages from server
    - Hints from the Guide Agent
    - 4-digit hex code indicating field agent is target of capture
    - Indication Field Agent was successfully captured
- `setStatus()` – sets the status of the app, set by messages sent and actions from users
    - “Active” – once the app has started
    - “Capturing” – once a player has pressed capture
    - “Targeted” – once another player in 10 meter radius has pressed capture
    - “Captured” – once another player has entered your hex code

##### Major Data Structures
`list()` – used for storing the messages
##### Data Flow Through Modules
- UI Neutralize Code Drop button is pressed
    1. Calls `onNeutralizePressed()`
    2. Calls `readCode()` to get string
		- UI Enter Button is pressed
			- Calls `codeDropNeutralized()`
- UI Capture Player button is pressed
    1. Call `onCapturePressed()`
    2. Calls `readCode()` to get string
		- UI Enter Button is pressed
			- Calls `codeDropNeutralized()`
- UI Messages button is pressed
    1. Call `onMessagesPressed()`
    2. Calls `readCode()` to get string
		- UI Enter Button is pressed
			- Calls `codeDropNeutralized()`


### Guide Agent
##### Command Line Interface
For the user interface, the user will use bash (UNIX) terminal on a Mac OS and will execute the `./guideagent [-log=raw] teamName GShost GSport [guideId]` command from the command line in the directory that contains executable file.

##### Inputs
- `teamName`, `GShost` and `GSport` from the commandline
- `OPCODE|...|...|...GAME_STATUS|gameId|fa1:fa2:...:faN|cd1:cd2:...:cdM`, Where:
    - `gameId` is the game number.
    - `fa` represents a record for one agent known to the GS. Each `fa` record is separated by colons. Each `fa` record contains comma-separated sub-fields `pebbleId`, `teamName`, `playerName`, `playerStatus`, `lastKnownLat`, `lastKnownLong`, `secondsSinceLastContact`
    - where
        - `pebbleId` is a unique number baked into each Pebble device.
        - `teamName` is the name of the team to which the FA belongs.
        - `playerName` is the name of the FA player.
        - `playerStatus` is the “status” of a FA (active or captured).
        - `lastKnownLat` is the latitude of the last known position of the FA.
        - `lastKnownLong` is the longitude of the last known position of the FA.
        - `secondsSinceLastContact` is the number of seconds elapsed since the GS had any message from the FA.

##### Outputs
- `OPCODE|...|...|...GA_STATUS|gameId|guideId|teamName|playerName|statusReq`, Where:
    - `gameId` is an integer identifying the game
    - `guideId` is a code identifying the Guide
    - `teamName` is the name of the team that the guide is on
    - `playerName` is the name of the Guide Agent
    - `statusReq` is a boolean flag indicating if the GA would like to recieve a game status update (1 = "request update", 0 = "no update").

- `OPCODE|...|...|...GA_HINT|gameId|guideId|teamName|playerName|faName|message`, Where:
    - `gameId` is the game number.
    - `guideId` is the guide identifier.
    - `teamName` is the name of the team to which the GA belongs.
    - `playerName` is the name of the GA player.
    - `faName` is the player name of the FA to which a hint will be sent, which must match the name of a player on the GA’s team; alternately faName may be * to “broadcast” the hint to all FAs on the GA’s team.
    - `message` is the “hint” message (free text) that the GA wants to send to a specific FA. A hint has 1..140 printable characters (as defined by `isprint()`), excluding the pipe `|` symbol.

##### Functional Decomposition into Modules
- `joinGame()` - Sends a message in OPCODE format to the server that this guide agent has entered the game
- `broadcastReceiver()` - Receives messages from the server
    - Notifications of new players entering
    - locations of field agents and enemy agents
    - Status and locations of code drops
- `sendHint()` - alert the server to send a hint to a field agent in OPCODE format
- `notifyGuide()` - provide an interface for notifying guide of changes in players’ statuses, new players, and code drops being neutralized
- `log()` - log all information to a logfile
- `readHint()` - reads the hint from an input on the GUI, checks that it's under 140 chars and parses it to be in OPCODE form
##### Major Data Structures
- Hashtable  of players 
- Field agent struct - stores a name, status, location of each field agent
- Hashtable of code drops
- Code drop struct - stores a name, status, location
- List of notifications - new players being added, location updates, status updates

##### Data Flow Through Modules
Once the bash command is run, `joinGame()` will be called as well as a loop calling `broadcastReciever()` to read and parse information coming in from the server	
If `broadcastReciever()` parses changes in the field agents or code drops, it will update them in their respective hashtable, then `notifyGuide()` will be called to display the alert to the guide and add it to the list


### Game Server
##### Command Line Interface
- For the user interface, the user will use bash (UNIX) terminal on a Mac OS and will execute the `./gameserver codeDropPath GSport` command from the command line in the directory that contains executable file, where codeDropPath represents the pathname for the code-drop file and GSport is the Game Server port number.

##### Inputs
- The pathname for the code-drop
- Game server port port number
- Messages from the guide agent		
- Status requests, IDs and team/player information,  and location updates from the field agents
- Code neutralization and capturing requests from field agents
- Status requests, IDs and team/player information,  from the guide agents
- Hints from the Guide to be forwarded to a specific field agent.

##### Outputs
- Hints from the guide agent to the field agent
- Changing status of the field agents into being captured
- Verifying if code neutralization or player capturing is a success
- Status and location updates about field agents and code drops to guide agent
- “Game over: messages to guide and field agents indicating the end of the current session of the game
- log files, logging updates on start-up information and relevant game state, and in some cases all the incoming and going string messages, into gameserver.log
- An error message to any agent that sends OPCODE with improper format

##### Functional Decomposition into Modules
- `main()` - Create and begin running a game and eventually ending the game
- `createAndBind()` - create and bind a UDP socket on a pre-configured host/IP and port  to  connect and communicate with the guide agent and field agents
- `addTeam()` - Create a new team for the game (some team names will pre-installed) when the guide agent signs-up
- `addPlayer()` - Create a new player struct for a player adding his/herself to the game (some team names will be pre-installed)
- `broadcastGuides()` - Update the guides on with new information on code drops and field agents locations and statuses as well as game statistics
- `notifyField()` - Notify field agents when status changes to “maybe-captured” or “captured”
- `updateField()` - Update field agents with the status of other players on their team and the game statistics when requested
- `sendMessage()` - Pass a message from a guide to the corresponding field agent
- `readCodeDrops()` - Read code drop locations from a file
- `trackCodeDrops()` - track status of all code drops
- `trackFieldAgents()` - track status and locations of all agents
- `recieveNeutralize()` - receive and process attempts to neutralize code drops
- `recieveFieldRequest()` - receive and process requests to capture or neutralize code drops
- `sendCapture()` - Send all agents within 10m of the ‘capturing’ agent a distinct 4 digit hex code 
- `randomHex()` -  creating random hex codes for maybe-captured players for 60 seconds
- `recieveCapture()` - receive and process a hex code from a ‘capturing’ agent
- `trackStats()` - track the game statistics
- `logger()`- log updates on start-up information and relevant game state, and in some cases all the incoming and going string messages, into gameserver.log

##### Major Data Structures
- A list of pointers to dead code structures
- A list of pointers to players structures
- A list of pointers to team structures(guide agent)
- a statistic structure
- Socket structure (UDP)

##### Data Flow Through Modules
1. `main()` - to start the game
    1. `createAndBind()` - create and bind a UDP socket on a pre-configured host/IP and port
    2. `readCodeDrops()` - Read code drop locations from a file
    3.	**LOOPING**  until the game ends or error happens{
        1. `receiveMessages()` - receive OPCODE and interprets the results
        2. `parse()`- Ihab will do
        3. `trackStats()` - track the game statistics
        4. `logger()`- log updates on start-up information and relevant game state, and in some cases all the incoming and going string messages, into gameserver.log
        5. `addTeam()` - Create a new team for the game
        6. `broadcastGuides()` - Update the guides on with new information on code drops and field agents
            1. `sendMessage()` - Pass a message from a guide to the corresponding field agent
            2. `addPlayer()` - Create a new player struct for a player adding his/herself to the game	
            3. `updateField()` - Update field agents with the status of other players on their team and the game statistics when requested
            4. `notifyField()` - Notify field agents when status changes to “maybe-captured” or “captured”
            5. `recieveNeutralize()` - receive and process attempts to neutralize code drops
            6. `recieveFieldRequest()` - receive and process requests to capture or neutralize code drops
            7. `randomHex()` -  creating random hex codes for maybe-captured players for 60 second
            8. `sendCapture()` - Send all agents within 10m of the ‘capturing’ agent a distinct 4 digit hex code 
		
    4. `trackCodeDrops()` - track status of all code drops
    5. `trackFieldAgents()` - track status and locations of all agents
	
2. `main()`to end the game (when the time expires or all codes have been neutralized)

### Extensions to Implement
- 10 points: Game server provides a graphical game summary, e.g., displaying a marker for each code drop and each player, overlaid on a campus map. You may find the gtk package useful.
- 10 points: Guide Agent provides a graphical game summary, e.g., displaying a marker for each code drop and each player, overlaid on a campus map. You may find the gtk package useful.(Limited to 5 points if your game server also provides a graphical view.)
- 5 points: A “Level 2” game, in which the game server sends the Guide coordinates only for field agents on the Guide’s team; for each field agent, it provides two pieces of information for each code drop and opposing agent, specifically, the direction and approximate distance: near (<10m),close (<100m), and far.
- 5 points: Use the Pebble’s accelerometer API to detect when the Field Agent is stationary or moving, sending less-frequent location updates to the Game Server when stationary. The purpose is to save energy. The Pebble should report at least once every minute so Guide does not worry.
- 2 points: Run games for a given duration; the game server’s status updates inform all players about time remaining in the game. When time runs out, all players are notified of the game-end and final statistics.
- 3 points: Compute the distance between two lat/long points with the proper equations, allowing the game to be played over much larger distances than our little campus.
    - Since we will already be reporting the players’ location 4 times every minute, we will store that data 
