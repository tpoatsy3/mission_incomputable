# Testing Document

## Game Server Component

### Basic Functionality

#### The Command Line

We will run gameserver with too many and too few arguments. Here are the command lines and results:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver
		Invalid number of arguments
		Usage: ./gameserver [--log] [--level = 1 or 3] [--time (in Minutes) [-a (ascii display)] codeDropPath GSport
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver codeDrop 2136 extraArg
		Invalid number of arguments
		Usage: ./gameserver [--log] [--level = 1 or 3] [--time (in Minutes) [-a (ascii display)] codeDropPath GSport
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136

Now we will test the flags. Here is an example of all the flags being used, each flag being used alone, and errors for the flags that require arguments.

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --level codeDrop 2136
		Level Flag is invalid
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --level=1 codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		quit
		GAME_OVER|B603|3|
		Game Over
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=1 codeDrop 2136
		Invalid gameID
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=ACF0 codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		quit
		GAME_OVER|ACF0|3|
		Game Over
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --time codeDrop 2136
		Time Flag is invalid
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --time=0 codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		quit    
		GAME_OVER|F309|3|
		Game Over
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver -v codeDrop 2136
		gameserver: invalid option -- 'v'
		Invalid option(s) used
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver -log codeDrop 2136
		Level Flag is invalid
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver -log=raw codeDrop 2136
		Level Flag is invalid
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --log=raw codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		quit
		GAME_OVER|349C|3|
		Game Over
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver -a codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136

		GAME_OVER|F0DE|3|
		Game Over
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --time=1 --level=3 --log=raw -a --game=FFFF codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136


#### Read code drops from file

Our load file is codeDrop. We will test this by first picking a file that does not work. Then by trying to load a directory. Then by changing the access to the file. Each of these errors are caught by the program. We know that this work because we would iterate through the hashtable printing all of the codeDrop HexCodes and locations at the beginning of our program for debugging purposes earlier.

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ mkdir badDir
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ chmod 000 codeDrop 
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver wrongFile 2136
		There is an error in processing wrongFile
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver codeDrop 2136
		codeDrop is not readable.
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver badDir 2136
		badDir is a not regular file.
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ chmod 777 codeDrop 
		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136

#### create and manage socket

To test this we will create a socket at 2136. In order to test that it works, we will send a message to a listener who will recieve a message. For the reciever, we are using chatclient.c from class. In order to manage the socket, we save the socket information into our game data type to ensure that we always have access to it.

On the server side: 

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@53922]: Hello Server!
		GS_RESPONSE|94A6|MI_ERROR_INVALID_OPCODE|Please enter a proper OPCODE


On the client side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server/chatserver/ServerTesting]$ chatclient flume.cs.dartmouth.edu 2136
		: Hello Server!
		[129.170.214.115@02136]: GS_RESPONSE|94A6|MI_ERROR_INVALID_OPCODE|Please enter a proper OPCODE

#### ignore messages for wrong game number

Here we will set a game number of 34FD and have a field agent who is already in the game try to use it. Rather than ignoring this case, we send an error message to agent who used the wrong game number. 

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@48948]: FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		Field Agent Ted was added to team topaz
		GAME_STATUS|34FD|0|3|1|0
		[129.170.214.115@48948]: FA_LOCATION|1111|1738|topaz|Ted|43.706552|-72.000000|1
		GS_RESPONSE|34FD|MI_ERROR_INVALID_GAME_ID|Send 0 to be registered

On the client side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server/chatserver/ServerTesting]$ chatclient flume.cs.dartmouth.edu 2136
		: FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|1|0
		FA_LOCATION|1111|1738|topaz|Ted|43.706552|-72.000000|1
		[129.170.214.115@02136]: GS_RESPONSE|34FD|MI_ERROR_INVALID_GAME_ID|Send 0 to be registered

#### accept new Agents, and respond with game number.

This is a straightforward field to test. I do this by adding three new agents. Each enters a game number of zero and recieves the actual game number.

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@35739]: FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		Field Agent Ted was added to team topaz
		GAME_STATUS|34FD|0|3|1|0
		[129.170.214.115@53922]: FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		Field Agent Ted1 was added to team topaz
		GAME_STATUS|34FD|0|3|2|0
		[129.170.213.207@39781]: FA_LOCATION|0|1798|topaz|Ted2|43.70662|-72.05000|1
		Field Agent Ted2 was added to team topaz
		GAME_STATUS|34FD|0|3|3|0

On the first client side:

		FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|1|0

On the second client side:

		FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|2|0

On the third client side:

		FA_LOCATION|0|1798|topaz|Ted2|43.70662|-72.05000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|3|0

#### ignore messages from new Guide Agents who declare a team for which there is already a Guide Agent.

This is again straightforward to test. Rather than ignoring the message, we thought it would be more helpful to return an error message so we implemented that instead.

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@53922]: GA_STATUS|0|1994|diamond|Zenon|1
		Guide Agent Zenon was added to team diamond
		[129.170.213.207@39781]: GA_STATUS|0|1995|diamond|Another Person|1
		GS_RESPONSE|34FD|MI_ERROR_INVALID_TEAMNAME|The team already exists

On the first client side:

		GA_STATUS|0|1994|diamond|Zenon|1
		[129.170.214.115@02136]: GAME_STATUS|34FD||F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE

On theh second client side:

		GA_STATUS|0|1995|diamond|Another Person|1
		[129.170.214.115@02136]: GS_RESPONSE|34FD|MI_ERROR_INVALID_TEAMNAME|The team already exists

#### forward hints from the Guide to FA or *.

Here we will do two cases. The first case will be sending one message to one specific Field Agent. The results are below.

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@53922]: FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		Field Agent Ted was added to team topaz
		GAME_STATUS|34FD|0|3|1|0
		[129.170.214.115@35739]: FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		Field Agent Ted1 was added to team topaz
		GAME_STATUS|34FD|0|3|2|0
		[129.170.214.115@48948]: GA_STATUS|0|1994|topaz|Zenon|1
		Guide Agent Zenon was added to team topaz
		GAME_STATUS|34FD|1788,topaz,Ted1,0,43.706620,-72.050000,27:1738,topaz,Ted,0,43.606552,-72.000000,36|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
		[129.170.214.115@48948]: GA_HINT|34FD|1994|topaz|Zenon|1788|THIS IS GREAT

On the GA side:

		GA_STATUS|0|1994|topaz|Zenon|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|1788,topaz,Ted1,0,43.706620,-72.050000,27:1738,topaz,Ted,0,43.606552,-72.000000,36|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
		GA_HINT|34FD|1994|topaz|Zenon|1788|THIS IS GREAT

On the FA side:

		FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|2|0
		[129.170.214.115@02136]: GA_HINT|34FD|1994|topaz|Zenon|1788|THIS IS GREAT

The second case will be sending one message to '*' and being recieved by the each of the FAs. The result of this is below.

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@53922]: FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		Field Agent Ted was added to team topaz
		GAME_STATUS|34FD|0|3|1|0
		[129.170.214.115@35739]: FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		Field Agent Ted1 was added to team topaz
		GAME_STATUS|34FD|0|3|2|0
		[129.170.214.115@48948]: GA_STATUS|0|1994|topaz|Zenon|1
		Guide Agent Zenon was added to team topaz
		GAME_STATUS|34FD|1788,topaz,Ted1,0,43.706620,-72.050000,27:1738,topaz,Ted,0,43.606552,-72.000000,36|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
		[129.170.214.115@48948]: GA_HINT|34FD|1994|topaz|Zenon|*|This also works really well.

On the GA side:

		GA_STATUS|0|1994|topaz|Zenon|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|1788,topaz,Ted1,0,43.706620,-72.050000,27:1738,topaz,Ted,0,43.606552,-72.000000,36|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
		GA_HINT|34FD|1994|topaz|Zenon|*|This also works really well.

On the FA1 side:

		FA_LOCATION|0|1788|topaz|Ted1|43.70662|-72.05000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|2|0
		[129.170.214.115@02136]: GA_HINT|34FD|1994|topaz|Zenon|*|1788|This also works really well.

On the FA2 side:

		FA_LOCATION|0|1738|topaz|Ted|43.606552|-72.000000|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|0|3|1|0
		[129.170.214.115@02136]: GA_HINT|34FD|1994|topaz|Zenon|*|1738|This also works really well.

#### receive/handle a request to neutralize

Here we have two examples. First will be the simple case that a FA will be within 10 meters of the code and submit a request to capture. He will then recieve back a successful message. The second case is that a player will not be in range to capture a peice of code. This will be ignored. The following will be tested with the server and two FAs, Chad and Dane. Chad will successfully capture the code (case 1). Dane will be too far away to capture the code (case 2). 

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@48948]: FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
		Field Agent Chad was added to team topaz
		[129.170.214.115@48948]: FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
		Field Agent Chad neutralized codeDrop F20D
		GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!
		[129.170.214.115@35739]: FA_LOCATION|0|4|topaz|Dane|43|-72|0
		Field Agent Dane was added to team topaz
		[129.170.214.115@35739]: FA_NEUTRALIZE|111F|4|topaz|Dane|43|-72|F20C


On the FA1 side (Chad):

		FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
		FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
		[129.170.214.115@02136]: GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!


On the FA2 side (Dane):

		FA_LOCATION|0|4|topaz|Dane|43|-72|0
		FA_NEUTRALIZE|111F|4|topaz|Dane|43|-72|F20C

#### receive/handle a request to capture and send notice of ‘captured’.

In this example we will have four players. Two who are on team topaz, Alex and Ben, and two who are on team diamond, Nick and Mike. Alex, Ben and Mike will all be in the same location. Nick will be far away. Alex will try to capture. Only Mike then will recieve the targeted notifcation with a random hex. Then Alex will complete the capture of Mike. 

On the server side:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@48948]: FA_LOCATION|0|1|topaz|Alex|43|-72|1
		Field Agent Alex was added to team topaz
		GAME_STATUS|111F|0|3|1|0
		[129.170.214.115@35739]: FA_LOCATION|0|2|topaz|Ben|43|-72|1
		Field Agent Ben was added to team topaz
		GAME_STATUS|111F|0|3|2|0
		[129.170.213.207@39781]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		Field Agent Mike was added to team ruby
		GAME_STATUS|111F|0|3|1|2
		[129.170.214.115@53922]: FA_LOCATION|0|6|ruby|Nick|20|20|1
		Field Agent Nick was added to team ruby
		GAME_STATUS|111F|0|3|2|2
		[129.170.214.115@48948]: FA_CAPTURE|111F|1|topaz|Alex|0
		GS_CAPTURE_ID|111F
		GS_CAPTURE_ID|111F|D1CB
		[129.170.214.115@48948]: FA_CAPTURE|111F|1|topaz|Alex|D1CB
		GS_RESPONSE|111F|MI_CAPTURED|You've been captuerd
		GS_RESPONSE|111F|MI_CAPTURE_SUCCESS|Congratulations! Your capture was successful!
		Field Agent Mike from team ruby was captured

Alex:

		FA_LOCATION|0|1|topaz|Alex|43|-72|1
		[129.170.214.115@02136]: GAME_STATUS|111F|0|3|1|0
		FA_CAPTURE|111F|1|topaz|Alex|0
		FA_CAPTURE|111F|1|topaz|Alex|D1CB
		[129.170.214.115@02136]: GS_RESPONSE|111F|MI_CAPTURE_SUCCESS|Congratulations! Your capture was successful!

Ben:

		FA_LOCATION|0|2|topaz|Ben|43|-72|1
		[129.170.214.115@02136]: GAME_STATUS|111F|0|3|2|0

Mike:

		FA_LOCATION|0|5|ruby|Mike|43|-72|1
		[129.170.214.115@02136]: GAME_STATUS|111F|0|3|1|2
		[129.170.214.115@02136]: GS_CAPTURE_ID|111F|D1CB
		[129.170.214.115@02136]: GS_RESPONSE|111F|MI_CAPTURED|You've been captuerd

Nick:

		FA_LOCATION|0|6|ruby|Nick|20|20|1
		[129.170.214.115@02136]: GAME_STATUS|111F|0|3|2|2

track the status of all code drops.

For this test, recall the neutralizing example. We will use the successful capturing sequence from that example. However, in this test we will call GA_STATUS both before and after the capture to show the difference in the status.

Server:

[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
Game Started
Host: flume.cs.dartmouth.edu
Port: 2136
[129.170.214.115@53922]: GA_STATUS|0|1994|topaz|Zenon|1
Guide Agent Zenon was added to team topaz
GAME_STATUS|111F||F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
[129.170.214.115@35739]: FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
Field Agent Chad was added to team topaz
[129.170.214.115@35739]: FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
Field Agent Chad neutralized codeDrop F20D
GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!
[129.170.214.115@53922]: GA_STATUS|111F|1994|topaz|Zenon|1
GAME_STATUS|111F|3,topaz,Chad,0,43.705259,-72.288328,54|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,topaz:F20C,-72.287328,43.706259,NONE

FA:

FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
[129.170.214.115@02136]: GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!

GA:

GA_STATUS|0|1994|topaz|Zenon|1
[129.170.214.115@02136]: GAME_STATUS|111F||F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
GA_STATUS|111F|1994|topaz|Zenon|1
[129.170.214.115@02136]: GAME_STATUS|111F|3,topaz,Chad,0,43.705259,-72.288328,54|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,topaz:F20C,-72.287328,43.706259,NONE

#### track the location and status of all players on all teams.

Here we added ten different players and has a GA ask for a status update. 

Server:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=34FD codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.213.207@39781]: FA_LOCATION|0|1|topaz|Alex|43.7049831248106|-72.29445292556758|1
		Field Agent Alex was added to team topaz
		GAME_STATUS|34FD|0|3|1|0
		[129.170.213.207@39781]: FA_LOCATION|0|2|topaz|Benn|43|-72|1
		Field Agent Benn was added to team topaz
		GAME_STATUS|34FD|0|3|2|0
		[129.170.213.207@39781]: FA_LOCATION|0|3|topaz|Chad|44|-72|1
		Field Agent Chad was added to team topaz
		GAME_STATUS|34FD|0|3|3|0
		[129.170.213.207@39781]: FA_LOCATION|0|4|topaz|Dane|43|-72|1
		Field Agent Dane was added to team topaz
		GAME_STATUS|34FD|0|3|4|0
		[129.170.213.207@39781]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		Field Agent Mike was added to team ruby
		GAME_STATUS|34FD|0|3|1|4
		[129.170.213.207@39781]: FA_LOCATION|0|6|ruby|Nick|44|-72|1
		Field Agent Nick was added to team ruby
		GAME_STATUS|34FD|0|3|2|4
		[129.170.213.207@39781]: FA_LOCATION|0|7|ruby|Otto|43|-72|1
		Field Agent Otto was added to team ruby
		GAME_STATUS|34FD|0|3|3|4
		[129.170.213.207@39781]: FA_LOCATION|0|10|diamond|Zeta|43|-72|1
		Field Agent Zeta was added to team diamond
		GAME_STATUS|34FD|0|3|1|7
		[129.170.213.207@39781]: FA_LOCATION|0|11|diamond|Yeti|44|-72|1
		Field Agent Yeti was added to team diamond
		GAME_STATUS|34FD|0|3|2|7
		[129.170.213.207@39781]: GA_STATUS|0|1995|topaz|Abe|1
		Guide Agent Abe was added to team topaz
		GAME_STATUS|34FD|11,diamond,Yeti,0,44.000000,-72.000000,83:10,diamond,Zeta,0,43.000000,-72.000000,83:7,ruby,Otto,0,43.000000,-72.000000,83:6,ruby,Nick,0,44.000000,-72.000000,83:5,ruby,Mike,0,43.000000,-72.000000,83:4,topaz,Dane,0,43.000000,-72.000000,83:3,topaz,Chad,0,44.000000,-72.000000,83:2,topaz,Benn,0,43.000000,-72.000000,83:1,topaz,Alex,0,43.704983,-72.294453,83|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE

GA:

		GA_STATUS|0|1995|topaz|Abe|1
		[129.170.214.115@02136]: GAME_STATUS|34FD|11,diamond,Yeti,0,44.000000,-72.000000,83:10,diamond,Zeta,0,43.000000,-72.000000,83:7,ruby,Otto,0,43.000000,-72.000000,83:6,ruby,Nick,0,44.000000,-72.000000,83:5,ruby,Mike,0,43.000000,-72.000000,83:4,topaz,Dane,0,43.000000,-72.000000,83:3,topaz,Chad,0,44.000000,-72.000000,83:2,topaz,Benn,0,43.000000,-72.000000,83:1,topaz,Alex,0,43.704983,-72.294453,83|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE

#### track game statistics: elapsed time, number of active agents, number of active teams, number of code drops neutralized by each team, number of code drops yet to neutralize.

Testing this is straightforward. What we will do is create a game with two teams. Have one team capture a player. Then print the game statistics by calling quit on the server. The game statistics are in the last line of code.

Server:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@48948]: FA_LOCATION|0|1|topaz|Alex|43|-72|1
		Field Agent Alex was added to team topaz
		GAME_STATUS|111F|0|3|1|0
		[129.170.214.115@48948]: FA_LOCATION|0|2|topaz|Ben|43|-72|1
		Field Agent Ben was added to team topaz
		GAME_STATUS|111F|0|3|2|0
		[129.170.214.115@48948]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		Field Agent Mike was added to team ruby
		GAME_STATUS|111F|0|3|1|2
		[129.170.214.115@48948]: FA_LOCATION|0|6|ruby|Nick|20|20|1
		Field Agent Nick was added to team ruby
		GAME_STATUS|111F|0|3|2|2
		[129.170.214.115@48948]: FA_CAPTURE|111F|1|topaz|Alex|0
		GS_CAPTURE_ID|111F
		GS_CAPTURE_ID|111F|41E2
		[129.170.214.115@48948]: FA_CAPTURE|111F|1|topaz|Alex|41E2
		GS_RESPONSE|111F|MI_CAPTURED|You've been captuerd
		GS_RESPONSE|111F|MI_CAPTURE_SUCCESS|Congratulations! Your capture was successful!
		Field Agent Mike from team ruby was captured
		[129.170.214.115@48948]: FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
		Field Agent Chad was added to team topaz
		[129.170.214.115@48948]: FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
		Field Agent Chad neutralized codeDrop F20D
		GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!
		quit
		GAME_OVER|111F|2|topaz,3,1,0,1:ruby,2,0,1,0

#### present a textual summary of game status on the terminal.

This I believe that we have done well enough in all of the Server code snipets that we have both above and below.

#### send game-status updates to FA: game statistics; not including any location information.

To test this, a FA calls FA_LOCATION with the status request being 1. The status update is then sent back to the FA.

Server:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@35739]: FA_LOCATION|0|1|topaz|Alex|43|-72|1
		Field Agent Alex was added to team topaz
		GAME_STATUS|111F|0|3|1|0
		[129.170.214.115@35739]: FA_LOCATION|0|2|topaz|Ben|43|-72|1
		Field Agent Ben was added to team topaz
		GAME_STATUS|111F|0|3|2|0
		[129.170.214.115@35739]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		Field Agent Mike was added to team ruby
		GAME_STATUS|111F|0|3|1|2
		[129.170.214.115@35739]: FA_LOCATION|0|6|ruby|Nick|20|20|1
		Field Agent Nick was added to team ruby
		GAME_STATUS|111F|0|3|2|2
		[129.170.214.115@35739]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		GS_RESPONSE|111F|MI_ERROR_INVALID_GAME_ID|You were added to this game before
		[129.170.214.115@35739]: FA_LOCATION|0|10|diamond|Zeta|43|-72|1
		Field Agent Zeta was added to team diamond
		GAME_STATUS|111F|0|3|1|4
		[129.170.214.115@35739]: FA_LOCATION|0|10|diamond|Zeta|43|-72|1
		Field Agent Zeta was added to team diamond
		GAME_STATUS|111F|0|3|1|4

FA:

		FA_LOCATION|0|10|diamond|Zeta|43|-72|1
		[129.170.214.115@02136]: GAME_STATUS|111F|0|3|1|4

#### send game-status updates to GA, on request, including game statistics; location and status of all code drops; location, name, team, and status of all field agents.

For this we will pick up from the last example and send a GA_STATUS message.

Server:

		[129.170.214.115@35739]: GA_STATUS|0|1994|topaz|Zenon|1
		Guide Agent Zenon was added to team topaz
		GAME_STATUS|111F|10,diamond,Zeta,0,43.000000,-72.000000,252:6,ruby,Nick,0,20.000000,20.000000,283:5,ruby,Mike,0,43.000000,-72.000000,315:2,topaz,Ben,0,43.000000,-72.000000,315:1,topaz,Alex,0,43.000000,-72.000000,315|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE

GA:

		GA_STATUS|0|1994|topaz|Zenon|1
		[129.170.214.115@02136]: GAME_STATUS|111F|10,diamond,Zeta,0,43.000000,-72.000000,252:6,ruby,Nick,0,20.000000,20.000000,283:5,ruby,Mike,0,43.000000,-72.000000,315:2,topaz,Ben,0,43.000000,-72.000000,315:1,topaz,Alex,0,43.000000,-72.000000,315|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE

#### recognize end of game; send game-over message.

Here we will end the game in three ways. The first is by a game timer, which is explaned in the extensions part of this document. Second is by typing "quit" into the server. It will then end the game and send the appropriate statistics to all parties. The last is by neutralizing all of the code. For this example, I will remove all but one piece of code from the load document.

##### Quit

This is continuing from the last test (sending game statistics to the GA).

Server:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@35739]: FA_LOCATION|0|1|topaz|Alex|43|-72|1
		Field Agent Alex was added to team topaz
		GAME_STATUS|111F|0|3|1|0
		[129.170.214.115@35739]: FA_LOCATION|0|2|topaz|Ben|43|-72|1
		Field Agent Ben was added to team topaz
		GAME_STATUS|111F|0|3|2|0
		[129.170.214.115@35739]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		Field Agent Mike was added to team ruby
		GAME_STATUS|111F|0|3|1|2
		[129.170.214.115@35739]: FA_LOCATION|0|6|ruby|Nick|20|20|1
		Field Agent Nick was added to team ruby
		GAME_STATUS|111F|0|3|2|2
		Empty or improper message was recieved
		Empty or improper message was recieved
		Empty or improper message was recieved
		[129.170.214.115@35739]: FA_LOCATION|0|5|ruby|Mike|43|-72|1
		GS_RESPONSE|111F|MI_ERROR_INVALID_GAME_ID|You were added to this game before
		Empty or improper message was recieved
		Empty or improper message was recieved
		[129.170.214.115@35739]: FA_LOCATION|0|10|diamond|Zeta|43|-72|1
		Field Agent Zeta was added to team diamond
		GAME_STATUS|111F|0|3|1|4
		Empty or improper message was recieved
		Empty or improper message was recieved
		[129.170.214.115@35739]: GA_STATUS|0|1994|topaz|Zenon|1
		Guide Agent Zenon was added to team topaz
		GAME_STATUS|111F|10,diamond,Zeta,0,43.000000,-72.000000,252:6,ruby,Nick,0,20.000000,20.000000,283:5,ruby,Mike,0,43.000000,-72.000000,315:2,topaz,Ben,0,43.000000,-72.000000,315:1,topaz,Alex,0,43.000000,-72.000000,315|F20E,-72.289328,43.704259,NONE:F20D,-72.288328,43.705259,NONE:F20C,-72.287328,43.706259,NONE
		quit
		GAME_OVER|111F|3|diamond,1,0,0,0:ruby,2,0,0,0:topaz,2,0,0,0
		Game Over

		GA and FA:

		[129.170.214.115@02136]: GAME_OVER|111F|3|diamond,1,0,0,0:ruby,2,0,0,0:topaz,2,0,0,0


##### Neutralizing all peices of code

Server:

		[tpoatsy@flume ~/cs50/project/project-starter-kit/game_server]$ gameserver --game=111F codeDrop 2136
		Game Started
		Host: flume.cs.dartmouth.edu
		Port: 2136
		[129.170.214.115@48948]: FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
		Field Agent Chad was added to team topaz
		[129.170.214.115@48948]: FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
		Field Agent Chad neutralized codeDrop F20D
		GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!
		GAME_OVER|111F|0|topaz,1,0,0,1
		Game Over

FA:

		FA_LOCATION|0|3|topaz|Chad|43.705259|-72.288328|0
		FA_NEUTRALIZE|111F|3|topaz|Chad|43.705259|-72.288328|F20D
		[129.170.214.115@02136]: GS_RESPONSE|111F|MI_NEUTRALIZED|Congratulations! You've neutralized a piece of code!
		[129.170.214.115@02136]: GAME_OVER|111F|0|topaz,1,0,0,1

#### logging

For each iteration of the game, a new log file was created. Since I made a new game for almost every one of the test cases above (lest they interfere with one another), I will copy and paste two different log files into this document.


### Extensions

#### Haversine Formula for Distance

We checked the funcionality of our function using print statements, set locations, and an online calculator.

#### ASCII Visual Display

This visual display was implemented in our client presentation. It visualizes campus locations (Library, East Wheelock, etc.), each code, field agents, and targeted field agents, each in a different color. 

#### Game Timer

This extension is hard to show with this type of document. I would invite you to use the `--time=1` flag and wait the minute for the game to end.