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

typedef struct sendInfo {
  int comm_sock;
  char *message;
  char *selectionParam;
  int error;
} sendInfo_t;

typedef struct saveable {
 in_port_t port;
 struct in_addr inaddr;
} saveable_t;

/**************** file-local constants and functions ****************/
static const int BUFSIZE = 1024;     // read/write buffer size
static int socket_setup();
static void handle_socket(int comm_sock, hashtable_t *hash);
static int sendToAll(int comm_sock, hashtable_t *hash, int flag);
void sendall(void* key, void* data, void* farg);
void delete(void *data);




/**************** main() ****************/
int
main(const int argc, char *argv[])
{
  // no arguments to parse



  // set up a socket on which to receive messages
  int comm_sock = socket_setup();
  hashtable_t *hash = hashtable_new(1, delete, NULL);
  // the client's address, filled in by recvfrom
  struct sockaddr_in them = {0, 0, {0}};  // sender's address
  
  // Receive datagrams, print them out, read response from term, send it back
  while (true) {        // loop exits on EOF from stdin
    // for use with select()
    fd_set rfds;        // set of file descriptors we want to read
    struct timeval timeout;   // how long we're willing to wait
    const struct timeval fivesec = {5,0};   // five seconds
    
    // Watch stdin (fd 0) and the UDP socket to see when either has input.
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);       // stdin
    FD_SET(comm_sock, &rfds); // the UDP socket
    int nfds = comm_sock+1;   // highest-numbered fd in rfds

    // Wait for input on either source, up to five seconds.
    timeout = fivesec;
    int select_response = select(nfds, &rfds, NULL, NULL, &timeout);
    // note: 'rfds' updated, and value of 'timeout' is now undefined
    
    if (select_response < 0) {
      // some error occurred
      perror("select()");
      exit(9);
    } else if (select_response > 0) {
      // some data is ready on either source, or both

      if (FD_ISSET(0, &rfds)) 
        if ((sendToAll(comm_sock, hash, 0)) == EOF)
          break; // exit loop if EOF on stdin
      if (FD_ISSET(comm_sock, &rfds)) {
        handle_socket(comm_sock, hash); // may update 'them'
      }

      // print a fresh prompt
      printf(": ");
      fflush(stdout);
    }
  }
  close(comm_sock);
  hashtable_delete(hash);
  putchar('\n');
  exit(0);
}

/**************** socket_setup ****************/
/* All the ugly work of preparing the datagram socket;
 * exit program on any error.
 */
int
socket_setup()
{
  // Create socket on which to listen (file descriptor)
  int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (comm_sock < 0) {
    perror("opening datagram socket");
    exit(1);
  }

  // Name socket using wildcards
  struct sockaddr_in server;  // server address
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(58503);
  if (bind(comm_sock, (struct sockaddr *) &server, sizeof(server))) {
    perror("binding socket name");
    exit(2);
  }

  return (comm_sock);
}

/**************** handle_stdin ****************/
/* stdin has input ready; read a line and send it to the client.
 * return EOF if EOF was encountered on stdin;
 * return 0 if there is no client to whom we can send;
 * return 1 if message sent successfully.
 * exit on any socket error.
 */
static int sendToAll(int comm_sock, hashtable_t *hash, int flag){
  char *response = readline(stdin);
  if (response == NULL) 
    return EOF;

//send all
  sendInfo_t *test;
  test = malloc(sizeof(sendInfo_t));
  test->comm_sock = comm_sock;
  test->message = response;

  hash_iterate(hash, sendall, test);

  free(test);
  free(response);

  return 1;
}

void sendall(void* key, void* data, void* farg) {
  sendInfo_t *test = (sendInfo_t *) farg;
  struct saveable *recieverp = (struct saveable *) data;
  struct sockaddr_in sender;    
  sender.sin_port = recieverp->port;
  sender.sin_addr = recieverp->inaddr;
  sender.sin_family = 2;
  struct sockaddr_in them = {0,0,{0}};
  struct sockaddr_in *themp = &them;


    *themp = sender;
  // struct sockaddr_in *recieverp = (struct sockaddr_in *) data;
  // struct sockaddr_in reciever = *recieverp;
  printf("%s\n", (char *)key);
  if (sendto(test->comm_sock, test->message, strlen(test->message), 
    0, (struct sockaddr *) themp, sizeof(*themp)) < 0)
    printf("NOOO!!!\n");
}


/**************** handle_socket ****************/
/* Socket has input ready; receive a datagram and print it.
 * If 'themp' is a valid client, ignore messages from other clients.
 * If 'themp' is not a valid client, update *themp to this sender.
 * Exit on any socket error.
 */
static void
handle_socket(int comm_sock, hashtable_t *hash)
{
  // socket has input ready
  struct sockaddr_in sender;     // sender of this message
  struct sockaddr *senderp = (struct sockaddr *) &sender;
  socklen_t senderlen = sizeof(sender);  // must pass address to length
  char buf[BUFSIZE];        // buffer for reading data from socket
  int nbytes = recvfrom(comm_sock, buf, BUFSIZE-1, 0, senderp, &senderlen);
  if (nbytes < 0) {
    perror("receiving from socket");
    exit(1);
  } else {      
    buf[nbytes] = '\0';     // null terminate string

    // where was it from?
    if (sender.sin_family != AF_INET)
      printf("From non-Internet address: Family %d\n", sender.sin_family);
    else {
      // if we don't yet have a client, use this sender as our client.
        char name[100];
        sprintf(name, "%s%d", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
        
      if ((hashtable_find(hash, name)) == NULL){
        printf("%s\n", name);
        struct saveable *trial = malloc(sizeof(saveable_t));  
        trial->port = sender.sin_port;
        trial->inaddr = sender.sin_addr;
        hashtable_insert(hash, name, trial);
      }
      
      // was it from the expected client?
    //   if (sender.sin_addr.s_addr == themp->sin_addr.s_addr && 
    // sender.sin_port == themp->sin_port) {
  // print the message
  printf("[%s@%05d]: %s\n", 
         inet_ntoa(sender.sin_addr),
         ntohs(sender.sin_port), 
         buf);
  //     } else {
  // printf("[%s@%05d]: go away! ;-)\n",
  //        inet_ntoa(sender.sin_addr),
  //        ntohs(sender.sin_port));
      // }
    }
    fflush(stdout);
    
  }
}

void delete(void *data){
  if (data){//if valid
    free(data);
  }
}
