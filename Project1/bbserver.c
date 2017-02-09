/*
* @filename bbserver.c
* @author Alex Lindemann , Nathan Moore
* @created 1/19/2017
*
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
  int bytesRcvd;
  int clients_counter = 0;
  struct sockaddr_in clientAddr[MAX_CLIENTS];
  socklen_t clientAddrLen = sizeof(clientAddr[0]);
  int numberOfClients = 0;
  while (numberOfClients < MAX_CLIENTS) {
    //process
    bytesRcvd = recvfrom(listensockfd, buffer, 500, 0,(struct sockaddr *) &clientAddr[0], &clientAddrLen);
    strcpy(buffer, "hey there you");
    sendto(listensockfd, buffer, 500, 0, (struct sockaddr *) &clientAddr[0], clientAddrLen);
  }
  return;
}
