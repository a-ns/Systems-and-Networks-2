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
int get_ip(char *, char *);
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
void exit_loop();
int i_want_out();
void craft_exit_message(char *, struct sockaddr_in, struct sockaddr_in);
//

//GLOBALS
#define MESSAGE_LENGTH 201
#define TOKEN "^"
#define JOIN "~"
#define EXIT "#"
#define EXIT_ACK "&"
int argc;
char **argv;
sem_t file_lock;
pthread_mutex_t quit_lock;
int QUITTER = 0;




int main (int argc_, char *argv_[]) {
  if ( !(argc_ == 5 || argc_ == 6)) {
  	perror("Incorrect input parameters");
  	exit(1);
  }
  argc = argc_; /* make argc and argv global */
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

/*
* Gets the user input for what they would like to do and then does that choice.
*/
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
      break;
    case 2:
      // do read stuff
      request_read(bulletinFileName);
      break;
    case 3:
      //do list stuff
      list_bulletin(bulletinFileName);
      break;
    case 4:
      exit_loop();
      //do exit stuff, leave token ring
      break;
    default:
      printf("Unknown input:  %i\n", choice);
  }
}
/*
* Creates the network thread
*/
void listen_for_conn (int argc, char* argv[]) {
  pthread_t thread;
  pthread_create(&thread, NULL, network_thread, NULL);
  return;
}

/*
* The network thread that runs in the background. This is a mess.
*/
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
//  fprintf(stderr, "the buffer is %s\n", buffer);
  struct sockaddr_in peer_sending_to_us;
  socklen_t peer_sending_to_us_length = sizeof(peer_sending_to_us);

  while(1){ //just keep doing this over and over
//    fprintf(stderr, "buffer: %s           from %s:%d\n", buffer, inet_ntoa(peer_sending_to_us.sin_addr), ntohs(peer_sending_to_us.sin_port));
    if (strcmp(buffer, TOKEN) == 0) {
      //unlock the mutex
      // other thread will now be able to do stuff
      //lock the mutex
      //pass on the token
      sem_post(&file_lock); // tell the other thread that it's ok to read/write

      sem_wait(&file_lock); // tell the other thread that it's not ok to read/write
      if(sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
        perror("sendto fail1");
        exit(1);
      }
//      fprintf(stderr, "sending token to %s:%d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
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

    //  fprintf(stderr, "join request: sending %s data to\n %s , %d\n", buffer, inet_ntoa(peer_sending_to_us.sin_addr), ntohs(peer_sending_to_us.sin_port));

      memcpy((void *) &dest.sin_addr, (void *) &peer_sending_to_us.sin_addr, sizeof(peer_sending_to_us.sin_addr));
      memcpy((void *) &dest.sin_port, (void *) &peer_sending_to_us.sin_port, sizeof(peer_sending_to_us.sin_port));
      if(sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) < 0){
        perror("sendto fail 2");
        exit(1);
      }

    }
    else if(strstr(buffer, EXIT) != NULL) { // EXIT IS A SUBSTRING OF BUFFER
      //handle someone exiting the ring
      //exit request has to go the whole way through the ring to get to the guy who points to us
      //once there he has to point to the person we currently point to, so we need to send that data
      //ie "EXIT IP_OF_PEER_BEFORE:PORT IP_OF_PEER_THEY_SHOULD_POINT_TO:PORT"
      //in the mean time, if we get the TOKEN, we should still send it off to our current peer
      char tmpBuffer[500] = "";
      strcpy(tmpBuffer, buffer);
      char *ipAndPort_of_peer_that_should_switch;
      strtok(tmpBuffer, " "); //point past "EXIT" in buffer
      ipAndPort_of_peer_that_should_switch = strtok(NULL, " "); //gets ip and port
      char *ip = strtok(ipAndPort_of_peer_that_should_switch, ":");
      char *port = strtok(NULL, ":");
      char us_host[500];
      char us_ip[500];
      gethostname(us_host, 500);
      get_ip(us_host, us_ip);
      //fprintf(stderr, "%s    %s    %s     %d", ip, us_host, port, ntohs(src.sin_port));
      if (strcmp(ip, us_ip) == 0 && atoi(port) == htons(src.sin_port)) { //if we are the one who the EXIT message is intended for

      //  printf("I'm supposed to change!\n");
        //change who we point to
        strtok(buffer, " ");
        strtok(NULL, " ");
        char *newIPAndPort = strtok(NULL, " ");
        char *newIP = strtok(newIPAndPort, ":");
        int newPort = atoi(strtok(NULL, ":"));
        char okMessage[500] ="";
        strcpy(okMessage, EXIT_ACK);
        if(sendto(sockfd, okMessage, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
          perror("sento fail 3");
          exit(1);
        }
    //    fprintf(stderr, "new peer is: %s:%d",  newIP, newPort);
        inet_pton(AF_INET, newIP, &dest.sin_addr);
        dest.sin_port = htons((u_short)newPort);
      }
      else { //pass the message along
      //  printf("passing along EXIT message.\n");
        if(sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) < 0){
          perror("sento fail 4");
          exit(1);
        }
      }
    }
    if (i_want_out()) {
      //signal that you want out of the loop
      //put in the buffer "EXIT IP_PERSON_BEFORE:PORT_BEFORE IP_PERSON_AFTER:PORT_AFTER"
      char message[500] = "";
      craft_exit_message(message, peer_sending_to_us, dest);

      strcpy(buffer, message);
  //    fprintf(stderr, "the exit message is: \"%s\"\n", buffer);
      if(sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) < 0 ) {
        perror("sento fail 5");
        exit(1);
      }
      while(1) {
        recvfrom(sockfd, buffer, 500, 0, (struct sockaddr *) &peer_sending_to_us, &peer_sending_to_us_length);
        if (strcmp(buffer, EXIT_ACK) == 0) {
          //  fprintf(stderr, "safe to quit.\n");
            exit(0);
        }
        else  {
          //fprintf(stderr, "i got a new message while exiting! \"%s\"passing it along\n", buffer);
          if (strstr(buffer, EXIT) != NULL){
            char tmpBuffer[500] = "";
            strcpy(tmpBuffer, buffer);
            char *ipAndPort_of_peer_that_should_switch;
            strtok(tmpBuffer, " "); //point past "EXIT" in buffer
            ipAndPort_of_peer_that_should_switch = strtok(NULL, " "); //gets ip and port
            char *ip = strtok(ipAndPort_of_peer_that_should_switch, ":");
            char *port = strtok(NULL, ":");
            char us_host[500];
            char us_ip[500];
            gethostname(us_host, 500);
            get_ip(us_host, us_ip);
            if (strcmp(ip, us_ip) == 0 && atoi(port) == htons(src.sin_port)) { //if we send the message back to ourselves
              exit(0);
            }
          }
          //fprintf(stderr, "\n%s:%d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
          if(sendto(sockfd, buffer, 500, 0, (struct sockaddr *) &dest, sizeof(dest)) <0 ) {
            perror("sento fail 6");
            exit(1);
          }
        }
      }
    }
    recvfrom(sockfd, buffer, 500, 0, (struct sockaddr *) &peer_sending_to_us, &peer_sending_to_us_length);
  }
}

/*
* Stuffs an exit message into message consisting of the desired recipient and the desired recipients new peer
*/
void craft_exit_message(char *message, struct sockaddr_in peer_sending_to_us, struct sockaddr_in dest) {
  char ipBefore[500] = "";
  strcat(ipBefore, inet_ntoa(peer_sending_to_us.sin_addr));
  char portBefore[50] = "";
  sprintf(portBefore, "%d", ntohs(peer_sending_to_us.sin_port));

  char ipAfter[500] = "";
  strcat(ipAfter, inet_ntoa(dest.sin_addr));
  char portAfter[50] = "";
  sprintf(portAfter, "%d", ntohs(dest.sin_port));

  strcat(message, EXIT);
  strcat(message, " ");
  strcat(message, ipBefore);
  strcat(message, ":");
  strcat(message, portBefore);
  strcat(message, " ");
  strcat(message, ipAfter);
  strcat(message, ":");
  strcat(message, portAfter);
  return;
}
/*
* Initializes global variables and other network required things upon running the program.
*/
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
//  fprintf(stderr, "sending data to\n %s , %d\n", inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));

  strcpy(buffer, JOIN);
  if (-1 == sendto(*sockfd, buffer, 500, 0, (struct sockaddr *)dest, sizeof(*dest))){
    perror("sendto error");
    exit(1);
  }
  bzero(buffer, 500);
  recvfrom(*sockfd, buffer, 500, 0, NULL, NULL);
  //fprintf(stderr, "\ngot %s from server\n", buffer);

  //buffer should have the ip address and port of the person we need to connect to now "XXX.XXX.XXX.XXX PORT NUM" if NUM == 0 WE START WITH TOKEN
  *hostptr = gethostbyname(strtok(buffer, " "));
  memset((void *) dest, 0, (size_t)sizeof(*dest));
  dest->sin_family = (short)(AF_INET);
  memcpy((void *) &(dest->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);
  dest->sin_port = htons((u_short) atoi(strtok(NULL, " ")));

//  fprintf(stderr, "\nwe are now sending to peer: %s on port %d\n", inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));

  return atoi (strtok(NULL, " "));
}

/*
* The menu the user sees.
*/
void show_menu () {
  printf("\n");
  printf("1. write\n");
  printf("2. read #\n");
  printf("3. list \n");
  printf("4. exit \n");
  printf("\n");
  return;
}

/*
* prints out a message from the bulletin board.
*/
void request_read (char *bulletinFileName) {
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

/*
*Returns a message from the bulletin board.
*/
char * get_bulletin_message(char *fileName, int index) {
  if (index > get_bulletin_length(fileName)) {
    printf("The message at index %i does not exist\n", index);
    return NULL;
  }
  sem_wait(&file_lock);
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
  sem_post(&file_lock);
  return message;
}

/*
* Writes a message to the specified file.
*/
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
//  fprintf(stderr, "Waiting for access to write. . .\n");
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
  fprintf(stderr, "Message succefully posted.\n");
  return 0;
}

/*
* Prints the length of the bulletin board
*/
void list_bulletin(char *bulletinFileName) {
  int messageCount = get_bulletin_length(bulletinFileName);
  printf("Current bulletin size: %i", messageCount);
  return;
}

/*
* Returns the lenght of the bulletin board
*/
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

/*
* Creates a bulletin message in a specific format based upon the string entered and the lenght of the bulletinBoard
*/
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

/*
* User specifies that they want to exit and this function will communicate to the network thread that it needs to shutdown
*/
void exit_loop () {
//  fprintf(stderr, "you want to quit, locking\n");
  pthread_mutex_lock(&quit_lock);
  QUITTER = 1;
  pthread_mutex_unlock(&quit_lock);
  return;
}

/*
* Returns 1 if exit_loop() has been run, 0 otherwise
*/
int i_want_out() {
  pthread_mutex_lock(&quit_lock);
  int returnv = QUITTER;
  pthread_mutex_unlock(&quit_lock);
  return returnv;
}

/*
* Get IP Address of a hostname.
*/
int get_ip(char * hostname , char* ip) {
  struct hostent *he;
  struct in_addr **addr_list;
  int i;
  if ((he = gethostbyname(hostname)) == NULL){
    herror("gethostbyname");
    return 1;
  }
  addr_list = (struct in_addr **) he->h_addr_list;
  for(i = 0; addr_list[i] != NULL; i++) {
    strcpy(ip , inet_ntoa(*addr_list[i]) );
    return 0;
  }
  return 1;
}
