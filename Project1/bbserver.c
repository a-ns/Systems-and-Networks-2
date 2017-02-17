/*
* @filename bbserver.c
* @author Alex Lindemann , Nathan Moore
* @created 1/19/2017
* run with ./bbserver 62001 3
* @desc This is a server program for a token ring simulation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
//GLOBALS
#define JOIN "~"
int PORT = 61001;
int listensockfd;
char hostname[100];
struct sockaddr_in server;
struct hostent *hostptr;
int argc;
char **argv;
struct sockaddr_in clients[10];
int MAX_CLIENTS;
//=============

//PROTOTYPES
void setup();
void start();
void craft_join_response(char *, struct sockaddr_in, int);

//=============

int main (int argc_, char *argv_[]) {
  argc = argc_;
  argv = argv_;

  setup();
  start();
  return 0;
}

void setup() {
  if (argc < 3) {
    perror("bbserver usage: bbserver portNum numberHosts\n");
    return;
  }
  if ((listensockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // listen on a new unused socket
    perror("Cannot create socket\n");
    return;
  }
  MAX_CLIENTS = atoi(argv[2]);
  PORT = atoi(argv[1]);
  gethostname(hostname, 100);
  hostptr = gethostbyname(hostname);

  memset((void *)&server, 0, (size_t)sizeof(server));
  memcpy((void *) &server.sin_addr, (void *) hostptr->h_addr, hostptr->h_length);
  server.sin_port = htons((u_short)PORT);


  server.sin_family = (short)AF_INET;

  //bindmemcpy((void *) &server.sin_addr, (void *))
  bind(listensockfd, (struct sockaddr *)&server, (socklen_t) sizeof(server));

  return;
}

void start() {
  fprintf(stderr, "hostname: %s , IP: %s listening on %d\n", hostptr->h_name,  inet_ntoa(server.sin_addr), ntohs(server.sin_port));
  char buffer[500];
  struct sockaddr_in clientAddr[MAX_CLIENTS];
  socklen_t clientAddrLen = sizeof(clientAddr[0]);
  int numberOfClients = 0;
  while (numberOfClients < MAX_CLIENTS) {
    //process
    recvfrom(listensockfd, buffer, 500, 0,(struct sockaddr *) &clientAddr[numberOfClients], &clientAddrLen);
    if (strcmp(buffer, JOIN) == 0) {
    	//handle join
      printf("join request\n");
    	numberOfClients += 1;
    }
  //  else {
  //    fprintf(stderr, "message: %s from %s", buffer, inet_ntoa(clientAddr[numberOfClients].sin_addr));
  //    strcpy(buffer, "boop");
  //    sendto(listensockfd, buffer, 500, 0, (struct sockaddr *) &clientAddr[numberOfClients], clientAddrLen);
  //  }
  }
  //we got the number of clients needed to form a ring
  int i;
  for(i = 0 ; i < numberOfClients - 1; ++i) {
    //go through and tell each client which new person to talk to
    craft_join_response(buffer, clientAddr[i], i);
    fprintf(stderr, "sending \"%s\"\n", buffer);
    if(sendto(listensockfd, buffer, 500, 0, (struct sockaddr *) &clientAddr[i+1], clientAddrLen) < 0){
      perror("sendto fail1");
      exit(1);
    }
  }
  //TO SEND TnO THE FIRST PERSON WHO JOINED THE INFORMATION FOR THE LAST PERSON TO JOIN
  craft_join_response(buffer, clientAddr[i], i);
  fprintf(stderr, "sending \"%s\"\n", buffer);
  if(sendto(listensockfd, buffer, 500, 0, (struct sockaddr *) &clientAddr[0], clientAddrLen)<0){
    perror("sendto fail2");
    exit(1);
  }
  //------------------------------------------------------------------------------
  return;
}

void craft_join_response(char *message, struct sockaddr_in client, int i) {
  char port[50] = "";
  sprintf(port, "%d", ntohs(client.sin_port));
  char index[10] = "";
  sprintf(index, "%d", i);

  strcpy(message, "");
  strcat(message, inet_ntoa(client.sin_addr));
  strcat(message, " ");
  strcat(message, port);
  strcat(message, " ");
  strcat(message, index);
  return;
}
