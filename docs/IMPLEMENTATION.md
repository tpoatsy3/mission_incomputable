# Implementation Spec for Mission Incomputable
# Team Topaz

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
