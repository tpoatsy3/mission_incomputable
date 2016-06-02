## Field Agent
A Pebble adventure game. 

### Compiling
Either "make" from top level directory or "pebble build" from field agent directory if you only wish to compile this part of the app. 

### Usage
This is an app for the Pebble watch that keeps track of a user's location and direction and sends messages every time the user is on the move. From the Pebble watch the user is able to neutralize a code, capture a player and view messages. The user's status and direction is displayed at the top of the home screen of the watch. 

From the home page of the app the user has four options: Neutralize Code, Code Drop, Request Status and Messages. If neutralize code is pressed, then a drop down menu of characters 0-9 and A-F is presented for the user to input the four-digit hex code of the code to neutralize. If capture is pressed, then the same keyboard is presented, allowing the user to also select a four digit hex. For both of these options, after the hex has been inputted the user is taken to a confirmation page, and upon pressing "Yes" the app sends the code to the server, specifying whether it was a code drop or a player captured. The request status button sends a message to the server to return the status of the game. The messages button takes the user to a list of messages that have come from the server. 

If the player is on the move, then it will send its location to the server every fifteen seconds. If the player is stationary then it only sends once a minute. 

### Limitations
Because we could not get the Pebble to connect to the server we could not test much of the functionality. 