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
#include <pthread.h>

#include "network.h"
#include "rcbQueue.c"
#include "fdQueue.c"

#define MAX_HTTP_SIZE 65536

int nextSequenceNumber = 0;
pthread_mutex_t schedulerLock;
pthread_mutex_t fdLock;

struct fdQueue fdQ;			//Queue of new file descriptors

struct Queue top;			//3 queues
struct Queue middle;			//Use depends on chosen scheduler
struct Queue bottom;			//MLFB uses all 3, RR and SJF only use top

/* This function takes a file handle to a client, reads in the request,
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters:
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static struct RCB create_rcb( int fd ) {
  char *buffer;                              /* request buffer */
  char *req = NULL;                                 /* ptr to req file */
  char *brk;                                        /* state used by strtok */
  char *tmp;                                        /* error checking ptr */
  FILE *fin;                                        /* input file handle */
  int len;                                          /* length of data read */

  buffer = malloc( MAX_HTTP_SIZE );
  if( !buffer ) {                                 /* error check */
    perror( "Error while allocating memory" );
    abort();
  }

  memset( buffer, 0, MAX_HTTP_SIZE );
  if( read( fd, buffer, MAX_HTTP_SIZE ) <= 0 ) {    /* read req from client */
    perror( "Error while reading request" );
    free(buffer);
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

  //struct stat fileStat;
  struct RCB newRCB;

  if( !req ) {                                      /* is req valid? */
    len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
    write( fd, buffer, len );                       /* if not, send err */
  } else {                                          /* if so, open file */
    req++;                                          /* skip leading / */
    fin = fopen( req, "r" );
    //stat(fin, &fileStat);
    fseek(fin, 0, SEEK_END);

    newRCB.bytesRemaining = ftell(fin);    

    fclose( fin );
  }

  free(buffer);

  newRCB.sequenceNumber = nextSequenceNumber++;
  newRCB.fileDescriptor = fd;
  newRCB.fileName = malloc(sizeof(char) * (strlen(req) + 1));
  strcpy(newRCB.fileName, req);
  newRCB.byteQuantum = 8192/8;
  return newRCB;
}


 static void serve_client( struct RCB *rcb ) {

  int fd;
  char *buffer;                              /* request buffer */
  char *req = NULL;                                 /* ptr to req file */
  FILE *fin;                                        /* input file handle */
  int len;

  int byteQuantum = rcb->byteQuantum;

  // strcpy(req, rcb.fileName);
  req = rcb->fileName;
  fd = rcb->fileDescriptor;

  buffer = malloc( MAX_HTTP_SIZE );
  if( !buffer ) {                                 /* error check */
    perror( "Error while allocating memory" );
    abort();
  }
  memset( buffer, 0, MAX_HTTP_SIZE );

  if( !req ) {                                      /* is req valid? */
    len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
    write( fd, buffer, len );                       /* if not, send err */
  } else {                                          /* if so, open file */
    fin = fopen( req, "rb" );                        /* open file */
    if( !fin ) {                                    /* check if successful */
      len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );
      write( fd, buffer, len );                     /* if not, send err */
    } else {                                        /* if so, send file */
      
      //Get file size
      fseek(fin, 0, SEEK_END);
      int sizeOfFile = ftell(fin);

      //Reset our position in the file to beginning      
      fseek(fin, 0, SEEK_SET);
      
      if(rcb->bytesRemaining==sizeOfFile){
        len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */
        write( fd, buffer, len );
      }
      else{
        fseek(fin, sizeOfFile - rcb->bytesRemaining, SEEK_SET);
      }	

        len = fread( buffer, 1, byteQuantum, fin );  /* read file chunk */
        if( len < 0 ) {                             /* check for errors */
            perror( "Error while writing to client" );
        } else if( len > 0 ) {                      /* if none, send chunk */
          len = write( fd, buffer, len );
          if( len < 1 ) {                           /* check for errors */
            perror( "Error while writing to client" );
          }
	  rcb->bytesRemaining -= len;
          printf("Sent %d bytes of file <%s>.\n", len, rcb->fileName);
          fflush(stdout);
        }

      fclose( fin );
    }
  }

  free(buffer);

  if(rcb->bytesRemaining < 1){
    close ( fd );
    printf("Request for file <%s> completed.\n", rcb->fileName);
    printf("Request <%d> completed\n", rcb->sequenceNumber);
    fflush(stdout);
  }                                  /* close client connectuin*/
}

//The function that threads run
void doWork(){
  //while(1){}

  while(1){
    if(fdQ.front != NULL){
      pthread_mutex_lock(&fdLock);
      int fd = fdDequeue(&fdQ);
      pthread_mutex_unlock(&fdLock);

      if(fd > 0){
        struct RCB tempRCB = create_rcb(fd);

        pthread_mutex_lock(&schedulerLock);
        schedulerEnqueue(tempRCB);
        printf("Request for file <%s> admitted.\n", tempRCB.fileName);
        fflush(stdout);
        pthread_mutex_unlock(&schedulerLock);
      }
    }

    if(isSchedulerEmpty() != 1){
      schedulerDequeue();
    }
  }
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

int schedulerToUse = -1;
int main( int argc, char **argv ) {
  int port = -1;                                    /* server port # */
  int numberOfThreads = 0;
  char scheduler[5];

  int fd;                                           /* client file descriptor */


  /* check for and process parameters
   */

   //Port
  if( argc < 4){
	printf( "usage: sms <port> <Scheduler> <NumberOfThreads>\n" );
    return 0;
  }

  //Scheduler
  if(( sscanf( argv[1], "%d", &port ) < 1 ) ) {
	printf( "Invalid port number." );
    printf( "usage: sms <port> <Scheduler> <NumberOfThreads>\n" );
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

  //Number of threads
  if(( sscanf( argv[3], "%d", &numberOfThreads ) < 1 ) ) {
    printf( "usage: sms <port> <Scheduler> <NumberOfThreads>\n" );
    return 0;
  }
  if(numberOfThreads < 1){
      printf( "Invalid number of threads." );
      printf( "usage: sms <port> <Scheduler> <PositiveNumberOfThreads>\n" ); 
     return 0;
  }


  network_init( port );                             /* init network module */

  pthread_t workers[numberOfThreads];

  int i = 0;
  while(i < numberOfThreads){
    pthread_create(&workers[i], NULL, doWork, NULL);
    i++;
  }

  for( ;; ) {                                       /* main loop */
    network_wait();                                 /* wait for clients */

    for( fd = network_open(); fd >= 0; fd = network_open() ) { /* get clients */
      
      pthread_mutex_lock(&fdLock);
      fdEnqueue(&fdQ, fd);
      pthread_mutex_unlock(&fdLock);

      //schedulerEnqueue(create_rcb(fd));
      //printf("%d\n", create_rcb(fd).bytesRemaining);
    }
    //while (schedulerDequeue() != 0){}
  }
}

void schedulerEnqueue(struct RCB rcb){
  if(schedulerToUse == -1){
    printf("Invalid scheduler./n");
  }
  else if(schedulerToUse == 0){
    rcb.byteQuantum = MAX_HTTP_SIZE;
    Insert(&top, rcb);
  }
  else if(schedulerToUse == 1){
    //RR
    Enqueue(&top, rcb);
  }
  else if(schedulerToUse == 2){
    //MLFB
    Enqueue(&top, rcb);
  }
}

int schedulerDequeue(){
  if(schedulerToUse == -1){
    printf("Invalid scheduler./n");
  }
  else if(schedulerToUse == 0){
    //SJF();
    pthread_mutex_lock(&schedulerLock);
    if(top.front == NULL){
      pthread_mutex_unlock(&schedulerLock);
      return 0;
    }
    struct RCB temp;
    temp = Dequeue(&top);
    pthread_mutex_unlock(&schedulerLock);

    serve_client(&temp);
    return 1;
  }
  else if(schedulerToUse == 1){
    //RR
    pthread_mutex_lock(&schedulerLock);
    if(top.front == NULL){
      pthread_mutex_unlock(&schedulerLock);      
      return 0;
    }
    struct RCB temp;
    temp = Dequeue(&top);
    pthread_mutex_unlock(&schedulerLock);

    serve_client(&temp);

    if(temp.bytesRemaining > 0){      
      pthread_mutex_lock(&schedulerLock);
      schedulerEnqueue(temp);
      pthread_mutex_unlock(&schedulerLock);
    }

    return 1;
  }
  else if(schedulerToUse == 2){
    //MLFB();
    pthread_mutex_lock(&schedulerLock);
    if(top.front != NULL){
      struct RCB tempRCB;
      tempRCB = Dequeue(&top);
      pthread_mutex_unlock(&schedulerLock);      

      serve_client(&tempRCB);

      if(tempRCB.bytesRemaining > 0){
        tempRCB.byteQuantum = (1024*2);
        pthread_mutex_lock(&schedulerLock);      
        Enqueue(&middle, tempRCB);
        pthread_mutex_unlock(&schedulerLock);      

      }
      return 1;
    }
    else if(middle.front != NULL){
      struct RCB tempRCB;
      tempRCB = Dequeue(&middle);
      pthread_mutex_unlock(&schedulerLock);      

      serve_client(&tempRCB);

      if(tempRCB.bytesRemaining > 0){
        pthread_mutex_lock(&schedulerLock);      
        Enqueue(&bottom, tempRCB);
        pthread_mutex_unlock(&schedulerLock);      
      }
      return 1;
    }
    else if(bottom.front != NULL){
      struct RCB tempRCB;
      tempRCB = Dequeue(&bottom);
      pthread_mutex_unlock(&schedulerLock);      

      serve_client(&tempRCB);

      if(tempRCB.bytesRemaining > 0){
        pthread_mutex_lock(&schedulerLock);      
        Enqueue(&bottom, tempRCB);
        pthread_mutex_unlock(&schedulerLock);      
      }
      return 1;
    }
    else{
      pthread_mutex_unlock(&schedulerLock);      
      return 0;
    }

  }
 
}

//Returns 1 if scheduler is empty
int isSchedulerEmpty(void){
  if(schedulerToUse == 0 || schedulerToUse == 1){
    if(top.front == NULL){
      return 1;
    }
    else{
      return 0;
    }
  }
  else{
    if(top.front == NULL && middle.front == NULL && bottom.front == NULL){
      return 1;
    }
    else{
      return 0;
    }
  }
}

