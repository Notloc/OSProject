/*
 * File: sws.c
 * Author: Alex Brodsky
 * Purpose: This file contains the implementation of a simple web server.
 *          It consists of two functions: main() which contains the main
 *          loop accept client connections, and serve_client(), which
 *          processes each client request.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "network.h"

#define MAX_HTTP_SIZE 8192                 /* size of buffer to allocate */

struct RCB{
  int sequenceNumber;
  int fileDescriptor;
  char *fileName;
  int bytesRemaining;
  int byteQuantum;
};

struct Node {
	struct RCB data;
	struct Node* next;
};
// Two glboal variables to store address of front and rear nodes.
struct Node* front = NULL;
struct Node* rear = NULL;

// To Enqueue an integer
void Enqueue(struct RCB x) {
	struct Node* temp =
		(struct Node*)malloc(sizeof(struct Node));
	temp->data = x;
	temp->next = NULL;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

// To Dequeue an integer.
void Dequeue() {
	struct Node* temp = front;
	if(front == NULL) {
		printf("Queue is Empty\n");
		return;
	}
	if(front == rear) {
		front = rear = NULL;
	}
	else {
		front = front->next;
	}
	free(temp);
}

struct RCB Front() {
	if(front == NULL) {
		printf("Queue is empty.");
		// return;
	}
	return front->data;
}

int nextSequenceNumber = 0;

/* This function takes a file handle to a client, reads in the request,
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters:
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static struct RCB create_rcb( int fd ) {
  static char *buffer;                              /* request buffer */
  char *req = NULL;                                 /* ptr to req file */
  char *brk;                                        /* state used by strtok */
  char *tmp;                                        /* error checking ptr */
  FILE *fin;                                        /* input file handle */
  int len;                                          /* length of data read */

  if( !buffer ) {                                   /* 1st time, alloc buffer */
    buffer = malloc( MAX_HTTP_SIZE );
    if( !buffer ) {                                 /* error check */
      perror( "Error while allocating memory" );
      abort();
    }
  }

  memset( buffer, 0, MAX_HTTP_SIZE );
  if( read( fd, buffer, MAX_HTTP_SIZE ) <= 0 ) {    /* read req from client */
    perror( "Error while reading request" );
    abort();
  }

  /* standard requests are of the form
   *   GET /foo/bar/qux.html HTTP/1.1
   * We want the second token (the file path).
   */
  tmp = strtok_r( buffer, " ", &brk );              /* parse request */
  if( tmp && !strcmp( "GET", tmp ) ) {
    req = strtok_r( NULL, " ", &brk );
  }

  struct stat fileStat;

  if( !req ) {                                      /* is req valid? */
    len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
    write( fd, buffer, len );                       /* if not, send err */
  } else {                                          /* if so, open file */
    req++;                                          /* skip leading / */
    fin = fopen( req, "r" );
	   fstat(fin, &fileStat);
	fclose( fin );
  }

  struct RCB newRCB;

  newRCB.sequenceNumber = nextSequenceNumber++;
  newRCB.fileDescriptor = fd;
  newRCB.fileName = req;
  newRCB.bytesRemaining = fileStat.st_size;
  newRCB.byteQuantum = 8;
  return newRCB;
}


 static void serve_client( struct RCB rcb ) {

  int fd;
  static char *buffer;                              /* request buffer */
  char *req = NULL;                                 /* ptr to req file */
  FILE *fin;                                        /* input file handle */
  int len;

  // strcpy(req, rcb.fileName);
  req = rcb.fileName;
  fd = rcb.fileDescriptor;

  if( !buffer ) {                                   /* 1st time, alloc buffer */
    buffer = malloc( MAX_HTTP_SIZE );
    if( !buffer ) {                                 /* error check */
      perror( "Error while allocating memory" );
      abort();
    }
  }
  memset( buffer, 0, MAX_HTTP_SIZE );


  if( !req ) {                                      /* is req valid? */
    len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
    write( fd, buffer, len );                       /* if not, send err */
  } else {                                          /* if so, open file */
    fin = fopen( req, "r" );                        /* open file */
    if( !fin ) {                                    /* check if successful */
      len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );
      write( fd, buffer, len );                     /* if not, send err */
    } else {                                        /* if so, send file */
      len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */
      write( fd, buffer, len );

      do {                                          /* loop, read & send file */
        len = fread( buffer, 1, MAX_HTTP_SIZE, fin );  /* read file chunk */
        if( len < 0 ) {                             /* check for errors */
            perror( "Error while writing to client" );
        } else if( len > 0 ) {                      /* if none, send chunk */
          len = write( fd, buffer, len );
          if( len < 1 ) {                           /* check for errors */
            perror( "Error while writing to client" );
          }
        }
      } while( len == MAX_HTTP_SIZE );              /* the last chunk < 8192 */
      fclose( fin );
    }
  }
  close( fd );                                     /* close client connectuin*/
}


/* This function is where the program starts running.
 *    The function first parses its command line parameters to determine port #
 *    Then, it initializes, the network and enters the main loop.
 *    The main loop waits for a client (1 or more to connect, and then processes
 *    all clients by calling the seve_client() function for each one.
 * Parameters:
 *             argc : number of command line parameters (including program name
 *             argv : array of pointers to command line parameters
 * Returns: an integer status code, 0 for success, something else for error.
 */
int main( int argc, char **argv ) {
  int port = -1;                                    /* server port # */
  char scheduler[5];
  int schedulerToUse = -1;

  int fd;                                           /* client file descriptor */


  /* check for and process parameters
   */

   //Port
  if( argc < 3){
	printf( "usage: sms <port> <Scheduler>\n" );
    return 0;
  }

  //Scheduler
  if(( sscanf( argv[1], "%d", &port ) < 1 ) ) {
	printf( "Invalid port number." );
    printf( "usage: sms <port> <Scheduler>\n" );
	return 0;
  }

  //Validate scheduler
  sscanf( argv[2], "%s", scheduler );

  if(strcmp(scheduler, "SJF") == 0){
	  schedulerToUse = 0;
  }
  else if(strcmp(scheduler, "RR") == 0){
	  schedulerToUse = 1;
  }
  else if(strcmp(scheduler, "MLFB") == 0){
	  schedulerToUse = 2;
  }
  else{
	  printf( "Invalid scheduler name.\n" );
	  return 0;
  }

  network_init( port );                             /* init network module */

  //initialize queue ( schedulerToUse );

  for( ;; ) {                                       /* main loop */
    network_wait();                                 /* wait for clients */

    for( fd = network_open(); fd >= 0; fd = network_open() ) { /* get clients */
      Enqueue(create_rcb(fd));
    }

    //

    while (front != rear) {
      serve_client(Front());
      Dequeue();
    }
    serve_client(Front());


	/*
	foreach request{
		serve_client( queue.next );
	}
	*/
  }
}
