/* 
 * chatserver - an example chat server using UDP.
 *
 * Read messages from a UDP socket and print them to stdout;
 * read messages from stdin and send them back to client.
 * 
 * usage: ./chatserver
 *  and ctrl-D to exit from a prompt, or ctrl-C if necessary.
 *
 * David Kotz, May 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        // bool type
#include <unistd.h>       // read, write, close
#include <string.h>       // strlen
#include <strings.h>        // bcopy, bzero
#include <arpa/inet.h>        // socket-related calls
#include <sys/select.h>       // select-related stuff
#include <sys/socket.h>
#include <netinet/in.h> 
#include "file.h"       // readline
#include "hashtable/hashtable.h"      // readline

typedef struct sendingInfo {
	int comm_sock;
	char *message;
} sendingInfo_t;

// typedef struct latlong;

typedef struct receiverAddr { //going to be part of the player struct
	in_port_t port;
	struct in_addr inaddr;
	int sin_family;
} receiverAddr_t;

typedef struct playSel {
	hashtable_t *hash;
	void *param;
} playSel_t;

// typedef struct theSuperHashtableReferenceStructure{}

/**************** file-local constants and functions ****************/
static const int BUFSIZE = 1024;     // read/write buffer size
static int socket_setup();
static void handle_socket(int comm_sock, hashtable_t *hash);
static void sending(int comm_sock, hashtable_t *hash, char *message);
void sendIterator(void* key, void* data, void* farg);
void delete(void *data);
bool handle_stdin();
void deleteTempHash(void *data);

void FA_LOCATION_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void FA_NEUTRALIZE_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void FA_CAPTURE_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
// void GA_STATUS_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
void GA_HINT_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);
void GAME_OVER_handler(int comm_sock, hashtable_t *hash);

void GA_HINT_iterator(void* key, void* data, void* farg);

bool processing(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted);

/**************** main() ****************/
int
main(const int argc, char *argv[])
{
	// set up a socket on which to receive messages
	int comm_sock = socket_setup();
  
	hashtable_t *hash = hashtable_new( 1, delete, NULL);
  
	// Receive datagrams, print them out, read response from term, send it back
	while (true) {        // loop exits on EOF from stdin
    
		// for use with select()
		fd_set rfds;        // set of file descriptors we want to read

		// Watch stdin (fd 0) and the UDP socket to see when either has input.
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);       // stdin
		FD_SET(comm_sock, &rfds); // the UDP socket
		int nfds = comm_sock+1;   // highest-numbered fd in rfds

		int select_response = select(nfds, &rfds, NULL, NULL, NULL);
		// note: 'rfds' updated, and value of 'timeout' is now undefined
    
		if (select_response < 0) {
			// some error occurred
			perror("select()");
			close(comm_sock);
			hashtable_delete(hash);
			exit(9);
    
		} else if (select_response > 0) {
			// some data is ready on either source, or both
			if (FD_ISSET(0, &rfds)){
				if (!handle_stdin())
					break;
			} 

			if (FD_ISSET(comm_sock, &rfds)) {
				handle_socket(comm_sock, hash);
			}
			fflush(stdout);
		}
	}
	GAME_OVER_handler(comm_sock, hash);
	close(comm_sock);
	hashtable_delete(hash);
	putchar('\n');
	exit(0);
}

/**************** socket_setup ****************/
/* All the ugly work of preparing the datagram socket;
 * exit program on any error.
 */
int socket_setup(){
	// Create socket on which to listen (file descriptor)
	int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (comm_sock < 0) {
		perror("opening datagram socket");
		close(comm_sock);
		exit(11);
	}

	struct sockaddr_in server;  // server address
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(58503);
	if (bind(comm_sock, (struct sockaddr *) &server, sizeof(server))) {
		perror("binding socket name");
		close(comm_sock);
		exit(12);
	}
	return (comm_sock);
}

bool handle_stdin(){
	char *terminalResponse = readline(stdin);
	if (terminalResponse == NULL || strcmp(terminalResponse, "quit") == 0){
		printf("LOG: Game is over. Sorry about that. If you have a problem please email Mr. David Kotz!\n");
		free(terminalResponse);
		return false;
	}
	free(terminalResponse);
	return true;
}


static void sending(int comm_sock, hashtable_t *hash, char *message){
  	sendingInfo_t *sendingInfo_p;
	sendingInfo_p = malloc(sizeof(sendingInfo_t));
	sendingInfo_p->comm_sock = comm_sock;
	sendingInfo_p->message = message;

	hash_iterate(hash, sendIterator, sendingInfo_p);

	free(sendingInfo_p);
}

void sendIterator(void* key, void* data, void* farg) {
	//We will never change this block of code
	sendingInfo_t *sendingInfo_p = (sendingInfo_t *) farg;
	receiverAddr_t *recieverp = (receiverAddr_t *) data;

	struct sockaddr_in sender;    
	sender.sin_port = recieverp->port;
	sender.sin_addr = recieverp->inaddr;
	sender.sin_family = recieverp->sin_family;

	struct sockaddr_in them = {0,0,{0}};
	struct sockaddr_in *themp = &them;
    *themp = sender;

	if (sendto(sendingInfo_p->comm_sock, sendingInfo_p->message, strlen(sendingInfo_p->message), 
		0, (struct sockaddr *) themp, sizeof(*themp)) < 0){
    printf("it failed to send the datagram\n");
	}
}


/**************** handle_socket ****************/
/* Socket has input ready; receive a datagram and print it.
 * If 'themp' is a valid client, ignore messages from other clients.
 * If 'themp' is not a valid client, update *themp to this sender.
 * Exit on any socket error.
 */
static void handle_socket(int comm_sock, hashtable_t *hash){
    // socket has input ready
	struct sockaddr_in sender;     // sender of this message
	struct sockaddr *senderp = (struct sockaddr *) &sender;
	socklen_t senderlen = sizeof(sender);  // must pass address to length
	char buf[BUFSIZE];        // buffer for reading data from socket
	int nbytes = recvfrom(comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
	if (nbytes < 0) {
	    perror("receiving from socket");
	    hashtable_delete(hash);
	    close(comm_sock);
	    exit(13);
	} else {      
	    buf[nbytes] = '\0';     // null terminate string

	    // where was it from?
	    if (sender.sin_family == AF_INET) {

	        /****DEBUGGING***/
	        char name[100];
	        sprintf(name, "%s %d", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
	        /****DEBUGGING***/

	        /***FOR THIS FILE ONLY, SHOULD BE PARSE***/
		    if ((hashtable_find(hash, name)) == NULL){
		        printf("%s\n", name);
		        receiverAddr_t *trial = malloc(sizeof(receiverAddr_t));  
		        trial->port = sender.sin_port;
		        trial->inaddr = sender.sin_addr;
				trial->sin_family = sender.sin_family;
		        hashtable_insert(hash, name, trial);
		    }
		    /***FOR THIS FILE ONLY, SHOULD BE PARSE***/

		    /****LOGGING***/
			printf("[%s@%05d]: %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), buf);
			/****LOGGING***/

			processing(comm_sock, hash, buf, name);

	    }
		fflush(stdout);
	}
}

bool processing(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	if(strcmp(buf, "FA_LOCATION") == 0){
		//return to user
		FA_LOCATION_handler(comm_sock, hash, buf, thisWillBeDeleted);
		return true;
	// } else if(strcmp(buf, FA_NEUTRALIZE) == 0){
	// 	//return to user
	// 	FA_NEUTRALIZE_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(buf, FA_CAPTURE) == 0){
	// 	//People based on Location and Status and Team
	// 	FA_CAPTURE_handler(comm_sock, hash, buf, thisWillBeDeleted);
	// } else if(strcmp(buf, GA_STATUS) == 0){
	// 	//Back to the user
	// 	GA_STATUS_handler(comm_sock, hash, buf, thisWillBeDeleted);
	} else if(strcmp(buf, "GA_HINT") == 0){
		//Either one FA or all on team
		GA_HINT_handler(comm_sock, hash, buf, thisWillBeDeleted);
		return true;
	} else {
		printf("you suck, that doesnt work\n");
		return false;
	}
	return true;
}

void FA_LOCATION_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	//returns a message to the sender
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	receiverAddr_t *sendingPlayer;

	if ((sendingPlayer = (receiverAddr_t *) hashtable_find(hash, thisWillBeDeleted)) != NULL){
		hashtable_insert(tempHash, thisWillBeDeleted, sendingPlayer); //the key dooesnt matter
		printf("FA_LOCATION: before\n");
		sending(comm_sock, tempHash, "FA_LOCATION received\n");
	}
	hashtable_delete(tempHash);
}



// void FA_NEUTRALIZE_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
// void FA_CAPTURE_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
// void GA_STATUS_handler(int comm_sock, hashtable_t hash, char *buf, char* thisWillBeDeleted);
void GA_HINT_handler(int comm_sock, hashtable_t *hash, char *buf, char* thisWillBeDeleted){
	hashtable_t *tempHash = hashtable_new(1, deleteTempHash, NULL);

	char *Ihab = "129.170.214.160";

	playSel_t *playSel_p = malloc(sizeof(playSel_t));
	playSel_p->hash = tempHash;
	playSel_p->param = Ihab;
	// receiverAddr_t *sendingPlayer;
	hash_iterate(hash, GA_HINT_iterator, playSel_p);
	sending(comm_sock, tempHash, "GA_HINT received\n");

	hashtable_delete(tempHash);
	free(playSel_p);
}

void GA_HINT_iterator(void* key, void* data, void* farg){
	char IP1[20];

	playSel_t *playSel_p = (playSel_t *) farg;
	char *key1 = (char *)key;

	hashtable_t *tempHash = playSel_p->hash;
	char *param1 = (char *) playSel_p->param;
	

	sscanf(key1, "%s *", IP1);

	if (strcmp(IP1, param1) == 0){
		hashtable_insert(tempHash, key1, data); //the key dooesnt matter
	} 
}

void GAME_OVER_handler(int comm_sock, hashtable_t *hash){
	sending(comm_sock, hash, "GAME_OVER. We win. Haha.\n");
}


void deleteTempHash(void *data){
	if (data){	//if valid
	}
}

void delete(void *data){
	if (data){	//if valid
		free(data);
	}
}