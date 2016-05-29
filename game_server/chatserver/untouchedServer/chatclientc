/* 
 * chatclient - an example chat client using UDP.
 *
 * Read messages from stdin and send them as datagrams to server at host/port;
 * read messages from the UDP socket and print them to stdout.
 * 
 * usage: ./chatclient hostname port
 * 
 * David Kotz, May 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>	      // read, write, close
#include <string.h>	      // strlen
#include <strings.h>	      // bcopy, bzero
#include <netdb.h>	      // socket-related structures
#include <arpa/inet.h>	      // socket-related calls
#include <sys/select.h>	      // select-related stuff 
#include "file.h"	      // readline

/**************** file-local constants and functions ****************/
static const int BUFSIZE = 1024;     // read/write buffer size
static int socket_setup(const int argc, char *argv[], 
			struct sockaddr_in *themp);
static int handle_stdin(int comm_sock, struct sockaddr_in *themp);
static void handle_socket(int comm_sock, struct sockaddr_in *themp);

/**************** main() ****************/
int
main(const int argc, char *argv[])
{
  // the server's address
  struct sockaddr_in them;

  // set up a socket on which to communicate
  int comm_sock = socket_setup(argc, argv, &them);

  // prompt the user to write a message
  printf(": ");

  // read from either the socket or stdin, whichever is ready first;
  // if stdin, read a line and send it to the socket;
  // if socket, receive message from socket and write to stdout.
  while (true) {	      // loop exits on EOF from stdin
    // for use with select()
    fd_set rfds;	      // set of file descriptors we want to read
    struct timeval timeout;   // how long we're willing to wait
    const struct timeval fivesec = {5,0};   // five seconds
    
    // Watch stdin (fd 0) and the UDP socket to see when either has input.
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);	      // stdin
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
    } else if (select_response == 0) {
      // timeout occurred; encourage our user to write
      printf("? ");
      fflush(stdout);
    } else if (select_response > 0) {
      // some data is ready on either source, or both

      if (FD_ISSET(0, &rfds)) 
	if (handle_stdin(comm_sock, &them) == EOF)
	  break; // exit loop if EOF on stdin

      if (FD_ISSET(comm_sock, &rfds)) {
	handle_socket(comm_sock, &them);
      }

      // print a fresh prompt
      printf(": ");
      fflush(stdout);
    }
  }

  close(comm_sock);
  putchar('\n');
  exit(0);
}

/**************** socket_setup ****************/
/* Parse the arguments and set up our socket.
 * Exit on any error of arguments or socket setup.
 */
static int
socket_setup(const int argc, char *argv[], struct sockaddr_in *themp)
{
  char *program;	      // this program's name
  char *hostname;	      // server hostname
  int port;		      // server port

  // check arguments
  program = argv[0];
  if (argc != 3) {
    fprintf(stderr, "usage: %s hostname port\n", program);
    exit(1);
  } else {
    hostname = argv[1];
    port = atoi(argv[2]);
  }

  // Look up the hostname specified on command line
  struct hostent *hostp = gethostbyname(hostname);
  if (hostp == NULL) {
    fprintf(stderr, "%s: unknown host '%s'\n", program, hostname);
    exit(3);
  }

  // Initialize fields of the server address
  themp->sin_family = AF_INET;
  bcopy(hostp->h_addr_list[0], &themp->sin_addr, hostp->h_length);
  themp->sin_port = htons(port);

  // Create socket
  int comm_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (comm_sock < 0) {
    perror("opening datagram socket");
    exit(2);
  }

  return comm_sock;
}

/**************** handle_stdin ****************/
/* stdin has input ready; read a line and send it to the client.
 * return EOF if EOF was encountered on stdin;
 * return 0 if there is no client to whom we can send;
 * return 1 if message sent successfully.
 * exit on any socket error.
 */
static int
handle_stdin(int comm_sock, struct sockaddr_in *themp)
{
  char *response = readline(stdin);
  if (response == NULL) 
    return EOF;

  if (themp->sin_family != AF_INET) {
    printf("I am confused: server is not AF_INET.\n");
    fflush(stdout);
    return 0;
  } 

  if (sendto(comm_sock, response, strlen(response), 0, 
	     (struct sockaddr *) themp, sizeof(*themp)) < 0){
    perror("sending in datagram socket");
    exit(6);
  }
  free(response);

  return 1;
}

/**************** handle_socket ****************/
/* Socket has input ready; receive a datagram and print it.
 * 'themp' should be a valid address; ignore messages from other senders.
 * Exit on any socket error.
 */
static void
handle_socket(int comm_sock, struct sockaddr_in *themp)
{
  // socket has input ready
  struct sockaddr_in sender;		 // sender of this message
  struct sockaddr *senderp = (struct sockaddr *) &sender;
  socklen_t senderlen = sizeof(sender);  // must pass address to length
  char buf[BUFSIZE];	      // buffer for reading data from socket
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
      // was it from the expected server?
      if (sender.sin_addr.s_addr == themp->sin_addr.s_addr && 
	  sender.sin_port == themp->sin_port) {
	// print the message
	printf("[%s@%05d]: %s\n", 
	       inet_ntoa(sender.sin_addr),
	       ntohs(sender.sin_port), 
	       buf);
      } else {
	printf("[%s@%05d]: go away! ;-)\n",
	       inet_ntoa(sender.sin_addr),
	       ntohs(sender.sin_port));
      }
    }
    fflush(stdout);
  }
}