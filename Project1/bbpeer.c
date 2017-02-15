/*
* @filename bbpeer.c
* @author Alex Lindemann , Nathan Moore
* @created 1/19/2017
* run with ./bbpeer -new 61999 ServerIP/HOSTNAME hostIP bb.txt
* @desc This is a client program for a token ring simulation.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

//FUNCTION DECLARATIONS
void start ();
void listen_for_conn();
void show_menu();
void * network_thread(void *);
void process_choice();
void request_read(char *);
int request_write(char *);
void list_bulletin(char *);
int get_bulletin_length(char *);
char * craftBulletinMessage (char *, int);
char * get_bulletin_message (char *, int);
int setup_network(char *, int *, struct hostent **, struct sockaddr_in *, struct sockaddr_in *, int *, int *);
//

//GLOBALS
#define MESSAGE_LENGTH 201
#define TOKEN "^"
#define JOIN "~"
#define EXIT "#"
int argc;
char **argv;
sem_t file_lock;





int main (int argc_, char *argv_[]) {
  if ( !(argc_ == 5 || argc_ == 6)) {
  	perror("Incorrect input parameters");
  	exit(1);
  }
  argc = argc_;
  argv = argv_;
  start();

  return 0;
}

void start () {
  sem_init(&file_lock, 0 , 0);
  listen_for_conn();

  while(1) {
    show_menu();
    process_choice();
  }
}

void process_choice () {
  char buffer[500];
  char *bulletinFileName;
  if (argc == 6) {
    bulletinFileName = argv[5];
  }
  else {
    bulletinFileName = argv[4];
  }
  fgets(buffer, 500, stdin);
  int choice = atoi(buffer);
  switch (choice) {
    case 1:
      //do write stuff
      request_write(bulletinFileName);
      printf("1 \n");
      break;
    case 2:
      // do read stuff
      request_read(bulletinFileName);
      printf("2 \n");
      break;
    case 3:
      //do list stuff
      printf("3 \n");
      list_bulletin(bulletinFileName);
      break;
    case 4:
      printf("4 \n");
      //do exit stuff, leave token ring
      break;
    default:
      printf("Unknown input:  %i\n", choice);
  }
}

void listen_for_conn (int argc, char* argv[]) {
  pthread_t thread;
  pthread_create(&thread, NULL, network_thread, NULL);
  return;
}

void * network_thread (void * param) {
  //do the connection stuff here i think
  char buffer[500];
  int sockfd;
  struct hostent *hostptr;
  struct sockaddr_in dest, src;
  int ME_PORT;
  int DEST_PORT;
  int go = setup_network(buffer, &sockfd, &hostptr, &dest, &src, &ME_PORT, &DEST_PORT);
  //src is us, dest is now the peer we send data to
  // if go == 0, we start the token
  if (go == 0) {
    strcpy(buffer, TOKEN);
  }
  else {
    strcpy(buffer, " ");
  }
  fprintf(stderr, "the buffer is %s\n", buffer);
  struct sockaddr_in peer_sending_to_us;
  socklen_t peer_sending_to_us_length = sizeof(peer_sending_to_us);

  while(1){
    if (strcmp(buffer, TOKEN) == 0) {
      //unlock the mutex
      // other thread will now be able to do stuff
      //lock the mutex
      //pass on the token
      sem_post(&file_lock); // tell the other thread that it's ok to read/write

      sem_wait(&file_lock); // tell the other thread that it's not ok to read/write
      sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest));
    }
    else if(strcmp(buffer, JOIN) == 0) {
      //handle a new person joining the ring
      //tell the new peer to point to our current peer
      //point to the new peer instead of the current peer
      char peerIPAndPort[500] = "";
      char peerIP[250] = "";
      char peerPort[250] = "";
      strcpy(peerIP, inet_ntoa(dest.sin_addr));
      sprintf(peerPort, "%d", ntohs(dest.sin_port));
      strcat(peerIPAndPort, peerIP); strcat(peerIPAndPort, " "); strcat (peerIPAndPort, peerPort); strcat(peerIPAndPort, " "); strcat(peerIPAndPort, "1");
      strcpy(buffer, peerIPAndPort);

      fprintf(stderr, "join request: sending %s data to\n %s , %d\n", buffer, inet_ntoa(peer_sending_to_us.sin_addr), ntohs(peer_sending_to_us.sin_port));



      memcpy((void *) &dest.sin_addr, (void *) &peer_sending_to_us.sin_addr, sizeof(peer_sending_to_us.sin_addr));
      memcpy((void *) &dest.sin_port, (void *) &peer_sending_to_us.sin_port, sizeof(peer_sending_to_us.sin_port));
      sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest));

    }
    else if(strstr(buffer, EXIT) != NULL) { // EXIT IS A SUBSTRING OF BUFFER
      //handle someone exiting the ring
      //exit request has to go the whole way through the ring to get to the guy who points to us
      //once there he has to point to the person we currently point to, so we need to send that data
      //ie "EXIT IP_OF_PEER_BEFORE:PORT IP_OF_PEER_THEY_SHOULD_POINT_TO:PORT"
      //in the mean time, if we get the TOKEN, we should still send it off to our current peer
    }
    recvfrom(sockfd, buffer, 500, 0, (struct sockaddr *) &peer_sending_to_us, &peer_sending_to_us_length);
  }
}

int setup_network(char * buffer, int * sockfd, struct hostent **hostptr, struct sockaddr_in *dest, struct sockaddr_in *src, int *ME_PORT, int *DEST_PORT) {
  if (argc == 6) {
    *ME_PORT = atoi(argv[2]);
    *DEST_PORT = atoi(argv[4]);
  }
  else {
    *ME_PORT = atoi(argv[1]);
    *DEST_PORT = atoi(argv[3]);
  }
  *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0 ){
    perror("error opening socket");
    exit(1);
  }
  if (argc == 6) {
    *hostptr = gethostbyname(argv[3]);
  }
  else {
    *hostptr = gethostbyname(argv[2]);
  }
  memset((void *) dest, 0, (size_t)sizeof(*dest));
  dest->sin_family = (short)(AF_INET);
  memcpy((void *) &(dest->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);
  dest->sin_port = htons((u_short) *DEST_PORT);

  memset((void *) src, 0, sizeof(*src));
  src->sin_family = AF_INET;
  src->sin_addr.s_addr = htonl(INADDR_ANY);
  src->sin_port = htons(*ME_PORT);
  if (bind(*sockfd, (struct sockaddr *) src, sizeof(*src)) < 0) {
    perror("bind");
    exit(1);
  }

  fprintf(stderr, "sending data to\n %s , %d\n", inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));

  strcpy(buffer, JOIN);
  if (-1 == sendto(*sockfd, buffer, 500, 0, (struct sockaddr *)dest, sizeof(*dest))){
    perror("sendto error");
    exit(1);
  }
  bzero(buffer, 500);
  recvfrom(*sockfd, buffer, 500, 0, NULL, NULL);
  fprintf(stderr, "\ngot %s from server\n", buffer);

  //buffer should have the ip address and port of the person we need to connect to now "XXX.XXX.XXX.XXX PORT NUM" if NUM == 0 WE START WITH TOKEN
  *hostptr = gethostbyname(strtok(buffer, " "));
  memset((void *) dest, 0, (size_t)sizeof(*dest));
  dest->sin_family = (short)(AF_INET);
  memcpy((void *) &(dest->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);
  dest->sin_port = htons((u_short) atoi(strtok(NULL, " ")));

  fprintf(stderr, "\nwe are now sending to peer: %s on port %d\n", inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));

  return atoi (strtok(NULL, " "));
}

void show_menu () {
  printf("\n");
  printf("1. write\n");
  printf("2. read #\n");
  printf("3. list \n");
  printf("4. exit \n");
  printf("\n");
  return;
}

void request_read (char *bulletinFileName) { // thread should be able to read at any time, no need to wait for a lock
  printf("Enter index for message to be read: \n");
  char buffer[500];
  fgets(buffer, 500, stdin);
  int choice = atoi(buffer);
  char *theMessage = get_bulletin_message(bulletinFileName, choice);

  if (theMessage != NULL){

    printf("The message at index %i:\n", choice);
    printf("%s\n", theMessage);
    free(theMessage);
  }

  return;
}

char * get_bulletin_message(char *fileName, int index) {
  if (index > get_bulletin_length(fileName)) {
    printf("The message at index %i does not exist\n", index);
    return NULL;
  }
  FILE *bulletinFile = fopen(fileName, "r");
  if (NULL == bulletinFile) {
    perror("bulletin file not found!");
    return NULL;
  }
  int messageCount = 0;
  char currentChar = ' ', previousChar = ' ';
  while (currentChar != EOF) {
    previousChar = currentChar;
    currentChar = fgetc(bulletinFile);
    if(previousChar == '<' && currentChar == 'm') {
      messageCount +=1;
    }
    if(messageCount == index) {
      while(fgetc(bulletinFile) != '\n');
      break;
    }
  }

  char *message = malloc(201);
  fgets(message, 200, bulletinFile);

  fclose(bulletinFile);
  return message;
}

int request_write (char *bulletinFileName) {
  char message[MESSAGE_LENGTH];
  printf("Enter your message:\n");
  if (fgets (message, MESSAGE_LENGTH, stdin) == NULL) {
    perror("No message entered.");
    return 1;
  }
  if (strstr(message, "<!") != NULL) { //contains our sentinel characters in bulletin file which is bad
    fprintf(stderr, "Invalid characters \"<!\" entered in message.\n");
    return 1;
  }
  fprintf(stderr, "Waiting for access to write. . .\n");
  sem_wait(&file_lock); //wait for the lock from the network thread
  FILE *bulletinFile = fopen(bulletinFileName, "a");
  if (NULL == bulletinFile) {
    perror ("Bulletin file not found.");
    sem_post(&file_lock);
    return 1;
  }
  message[strlen(message)-1] = '\0';
  int messageCount = get_bulletin_length(bulletinFileName);
  char *newBulletinMessage = craftBulletinMessage(message, messageCount);
  fprintf(bulletinFile, "%s", newBulletinMessage);
  free(newBulletinMessage);
  fclose(bulletinFile);
  sem_post(&file_lock);
  return 0;
}

void list_bulletin(char *bulletinFileName) {
  int messageCount = get_bulletin_length(bulletinFileName);
  printf("Current bulletin size: %i", messageCount);

  return;
}

int get_bulletin_length (char *fileName) {
  FILE *bulletinFile = fopen(fileName, "r");
  if (NULL == bulletinFile) {
    perror("bulletin file not found!");
    return -1;
  }
  int messageCount = 0;
  char currentChar = ' ', previousChar = ' ';
  while (currentChar != EOF) {
    previousChar = currentChar;
    currentChar = fgetc(bulletinFile);
    if(previousChar == '<' && currentChar == 'm') {
      messageCount +=1;
    }
  }

  fclose(bulletinFile);
  return messageCount;
}

char * craftBulletinMessage(char *message, int bulletinLength) {
  char *newMessage = malloc(225);
  strcpy(newMessage, "");
  char messageIndex[5] = "";
  sprintf(messageIndex, "%d", bulletinLength + 1);
  char * newMessageBody = malloc(201); //size of a message + newline
  strcpy(newMessageBody, "");
  strcat(newMessage, "<message n=");
  strcat(newMessage, messageIndex);
  strcat(newMessage, ">\n");
  //now pad spaces to newMessageBody to get it to length 200 and then append a newline
  strcat(newMessageBody, message);
  while (strlen(newMessageBody) != 200) {
    strcat(newMessageBody, " ");
  }
  strcat(newMessageBody, "\n");
  //now put newMessageBody into newMessage
  strcat(newMessage, newMessageBody);
  //now add </message>\n newMessage
  strcat(newMessage, "</message>\n");
  free(newMessageBody);
  return newMessage;
}
