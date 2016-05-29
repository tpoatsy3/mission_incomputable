utility_t utility_p = malloc(sizeof(utility_t));
		int errorFlag = 0;

		utility_p->param1 = &errorFlag;
		utility_p->param2 = messageArray[3];

		hash_iterate(allGameInfo->GA, GAMatchingTeam, utility_p);
		if(errorFlag == 0){
			//player is not known, so they can make whatever team name they want
			free(utility_p);
			return true;






void FA_LOCATION_handler(hashStruct_t *allGameInfo, char** messageArray, int arraySize, receiverAddr_t *playerAddr){
	//returns a message to the sender
	int gameIDFlag, playerIDFlag, statusFlag;

	if (arraySize != 8){
		printf("FA LOCATION message is of the wrong length.\n");
		return;
	}

	gameIDFlag = gameIDHandler(allGameInfo, messageArray);

	//modify this later
	if (gameIDFlag == 1){
		//add
		printf("it will added successfully\n");
	} else if (gameIDFlag == -1 ){
		printf("you have there wrong game ID\n");
	} else {
		printf("It is there\n");
	}
	playerIDFlag = playerIDHandler(allGameInfo, messageArray);
	if (teamNameHandler(allGameInfo, messageArray)) printf("team true\n");
	else printf("team false\n");

	if (playerNameHandler(allGameInfo, messageArray)) printf("player true\n");
	else printf("player false\n");
	
	if (latHandler(messageArray)) printf("lat true\n");
	else printf("lat false\n");
	
	if (lngHandler(messageArray)) printf("lng true\n");
	else printf("lng false\n");

	statusFlag = statusReqHandler(messageArray);


	if (gameIDFlag == -1){
		printf("Your gameID is not valid");
		break;
	} else if(playerIDFlag == -1){
		printf("Your playerID is not valid");
		break;
	} else if (playerIDFlag == 2){
		break;
	} else if ((gameIDFlag == 1) && (playerIDFlag == 1)){
		addPlayer(allGameInfo, messageArray, playerAddr);
	} else if ((gameIDFlag == 1) && (playerIDFlag != 1)){
		printf("Your gameID is not valid");
		break;
	} else if ((gameIDFlag != 1) && (playerIDFlag == 1)){
		printf("Your playerID is not valid");
		break;
	} 

	//update the FA's information
	FAPlayer_t currentFA = hashtable_find(allGameInfo->FA, messageArray[2]);
	currentFA->lat = atof(messageArray[5]);
	currentFA->lng = atof(messageArray[6]);
	currentFA->lastContact = time(NULL);
	currentFA->addr = playerAddr;

	if (statusFlag == 1){
		//send a message back.
		hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);
		hashtable_insert(tempHash, messageArray[3], playerAddr); //the key dooesnt matter
		sending(allGameInfo->game->comm_sock, tempHash, "GAME_STATUS");
		
		hashtable_delete(tempHash);
	}
}




typedef struct game {
	long gameID; 
	int level;
	int timeVal;
	int rawlogFl;
	int GSPort;
	int deadDropRemaining;
	int comm_sock;
} game_t;

typedef struct code_drop {
	double lng;
	double lat;
	int status; // 0 means not neutralized
	char *team;
} code_drop_t;

typedef struct sendingInfo {
	int comm_sock;
	char *message;
} sendingInfo_t;

typedef struct utility {
	void *param1;
	void *param2;
	void *param3;
} utility_t;

typedef struct hashStruct{
	hashtable_t *GA;
	hashtable_t *FA;
	hashtable_t *temp;
	hashtable_t *CD;
	game_t *game;
} hashStruct_t;

typedef struct receiverAddr{
	in_port_t port;
	struct in_addr inaddr;
	int sin_family;
}receiverAddr_t;


typedef struct FAPlayer{
	char *PlayerName;
	char *TeamName;
	int status;
	double lat;
	double lng;
	long int lastContact; //unclearType
	receiverAddr_t *addr;
} FAPlayer_t;

typedef struct GAPlayer{
	char *PlayerName;
	char *TeamName;
	int status;
	long int lastContact; //unclearType
	receiverAddr_t *addr;
} GAPlayer_t;


FA_LOCATION|1|60000||4||0000|7
FA_LOCATION|1|60000| |4|5|0000|7
FA_LOCATION|1|60000|jdasf lkjadsf|4|5|0000|7
FA_LOCATION|1|60000|team|4|5|0000|7
FA_LOCATION|1|60000|3|4|xx|0000|7
FA_LOCATION|1|60000|3|4||0000|7
FA_LOCATION|1|60000|3|4| |0000|7
FA_LOCATION|1|60000|3|4|91|0000|7
FA_LOCATION|1|60000|3|4|-91|0000|7
FA_LOCATION|1|60000|3|4|0|0000|7
FA_LOCATION|1|60000|3|4|36|181|7
FA_LOCATION|1|60000|3|4|36|181| 
FA_LOCATION|1|60000|3|4|36|181|jhShjs
FA_LOCATION|1|60000|3|4|36|181|
FA_LOCATION|1|60000|3|4|36|181|1
FA_LOCATION|1|60000|3|4|36|181|0
FA_LOCATION|1|60000|3|4|36|181|12
FA_LOCATION|1|60000|3|4|36|181|x