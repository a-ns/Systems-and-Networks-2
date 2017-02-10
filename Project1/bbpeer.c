/*
* @filename bbpeer.c
* @author Alex Lindemann , Nathan Moore
* @created 1/19/2017
*
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
void request_read();
void request_write();
void list_bulletin();
int get_bulletin_length();
char * craftBulletinMessage (char *, int);
//

//GLOBALS
#define MESSAGE_LENGTH 201
int sockfd;
int argc;
char **argv;
sem_t file_lock;
struct hostent *hostptr;
struct sockaddr_in dest;
int ME_PORT = 61002;
int SERVER_PORT;




int main (int argc_, char *argv_[]) {
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
  fgets(buffer, 500, stdin);
  int choice = atoi(buffer);
  switch (choice) {
    case 1:
      //do write stuff
      request_read();
      printf("1 \n");
      break;
    case 2:
      // do read stuff
      request_write();
      printf("2 \n");
      break;
    case 3:
      //do list stuff
      printf("3 \n");
      list_bulletin();
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

  if (argc == 6) {
    ME_PORT = atoi(argv[2]);
    SERVER_PORT = atoi(argv[4]);
  }
  else {
    ME_PORT = atoi(argv[1]);
    SERVER_PORT = atoi(argv[3]);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0 ){
    perror("error opening socket");
    exit(1);
  }
  if (argc == 6) {
    hostptr = gethostbyname(argv[3]);
  }
  else {
    hostptr = gethostbyname(argv[2]);
  }

  memset((void *) &dest, 0, (size_t)sizeof(dest));
  dest.sin_family = (short)(AF_INET);
  memcpy((void *) &dest.sin_addr, (void *) hostptr->h_addr, hostptr->h_length);
  dest.sin_port = htons((u_short) SERVER_PORT);
  fprintf(stderr, "sending data to\n %s , %d", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

  while(1){
    fprintf(stderr,"network_thread going to sleep\n");
    strcpy(buffer, "hello server");
    if (-1 == sendto(sockfd, buffer, 500, 0, (struct sockaddr *)&dest, sizeof(dest))){
      perror("sendto error");
	exit(1);
    }
    fprintf(stderr, "hello?\n");
    bzero(buffer, 500);
    recvfrom(sockfd, buffer, 500, 0, NULL, NULL);
    fprintf(stderr, "got %s from server\n", buffer);
    /*TODO

    HANDLE WHEN YOU GET THE token
    if (tcp_incoming == YOU_HAVE_TOKEN_MESSAGE) {
      unlock the mutex
      // other thread will now be able to do stuff
      lock the mutex
      pass on the token

      sem_post(&file_lock); // tell the other thread that it's ok to read/write
      sleep(5);
      printf("network_thread waking up\n");
      sem_wait(&file_lock); // tell the other thread that it's not ok to read/write

    }
    */

}
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

void request_read () {
  printf("Waiting for the lock to read!\n");
  sem_wait(&file_lock); //wait for the lock
  printf("Got the lock. I can read now!\n");


  sem_post(&file_lock);
  return;
}

void request_write () {
  printf("Waiting for the lock to write!\n");
  sem_wait(&file_lock); //wait for the lock
  printf("Got the lock. I can write now!\n");
  FILE *bulletinFile = fopen(argv[argc-1], "a");
  if (NULL == bulletinFile) {
    perror ("Bulletin file not found.");
    sem_post(&file_lock);
    return;
  }
  char message[MESSAGE_LENGTH];
  printf("Enter your message:\n");
  if (fgets (message, MESSAGE_LENGTH, stdin) == NULL) {
    perror("No message entered.");
    sem_post(&file_lock);
    return;
  }
  if (strstr(message, "<!") != NULL) { //contains our sentinel characters in bulletin file which is bad
    fprintf(stderr, "Invalid characters \"<!\" entered in message.\n");
    sem_post(&file_lock);
    return;
  }
  int messageCount = get_bulletin_length();
  char *newBulletinMessage = craftBulletinMessage(message, messageCount);
  fprintf(bulletinFile, "%s", newBulletinMessage);
  free(newBulletinMessage);
  fclose(bulletinFile);
  sem_post(&file_lock);
  return;
}

void list_bulletin() {
  sem_wait(&file_lock); //wait for the lock

  int messageCount = get_bulletin_length();
  printf("Current bulletin size: %i", messageCount);

  sem_post(&file_lock);
  return;
}

int get_bulletin_length () {
  FILE *bulletinFile = fopen(argv[argc-1], "r");
  if (NULL == bulletinFile) {
    perror("bulletin file not found!");
    sem_post(&file_lock);
    return;
  }
  int messageCount = 0;
  char currentChar = ' ', previousChar = ' ';
  while (currentChar != EOF) {
    previousChar = currentChar;
    currentChar = fgetc(bulletinFile);
    if(previousChar == '<' && currentChar == '!') {
      messageCount +=1;
    }
  }

  fclose(bulletinFile);
  return messageCount;
}

char * craftBulletinMessage(char *message, int bulletinLength) {
  char *newMessage = malloc(225); //13 + 201 + 11 = 225, <!message XX>\n = 13, message\n = 200, </message>\n = 11
  char messageIndex[3];
  char * newMessageBody = malloc(201); //size of a message + newline
  if (bulletinLength < 10) {
    messageIndex[0] = 0;
    messageIndex[1] = bulletinLength + '0';
    messageIndex[2] = '\0';
  }
  else {
    sprintf(messageIndex, "%d", bulletinLength);
  }
  strcat(newMessage, "<!message ");
  strcat(newMessage, messageIndex);
  strcat(newMessage, ">\n");
  // <!message XX>\n has been appended to newMassage
  //now pad spaces to newMessageBody to get it to length 200 and then append a newline
  strcat(newMessageBody, message);
  while (strlen(newMessageBody) != 200) {
    strcat(newMessageBody, " ");
  }
  strcat(newMessageBody, "\n");
  //now put messageBody into newMessage
  strcat(newMessage, messageBody);
  //now add </message>\n newMessage
  strcat(newMessage, "</message>\n");
  free(newMessageBody);
  return newMessage;
}
