/*
* @filename sender.c
* @author Alex Lindemann , Nathan Moore
* @created 03/04/2017
* @desc This is a sender program implementing rdt 3.0
* sender port rcvHost rcvPort networkHost networkPort
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "rdtSender.h"

void getMessageFromUser(char *);
void checkCommandLineArguments(int, char**);
void setup_network (char **, int *, struct hostent**, struct sockaddr_in *, struct sockaddr_in *, int *, int *, int *);
int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  char messageToSend[500];
  getMessageFromUser(messageToSend);
  int sockfd;
  struct hostent *hostptr;
  struct sockaddr_in network, src;
  int ME_PORT;
  int DEST_PORT;
  int NETWORK_PORT;
  setup_network(argv, &sockfd, &hostptr, &network, &src, &ME_PORT, &DEST_PORT, &NETWORK_PORT);

  return 0;
}

void checkCommandLineArguments(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr, "Usage: sender port rcvHost rcvPort networkHost networkPort\n");
    exit(1);
  }
}

void setup_network(char **argv, int *sockfd, struct hostent**hostptr, struct sockaddr_in * network, struct sockaddr_in *src, int *ME_PORT, int *DEST_PORT, int *NETWORK_PORT) {
  *ME_PORT = atoi(argv[1]);
  *DEST_PORT = atoi(argv[3]);
  *NETWORK_PORT = atoi(argv[5]);

  *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0 ) {
    perror("error opening socket");
    exit(1);
  }
  *hostptr = gethostbyname(argv[4]);
  memset((void *) network, 0, (size_t) sizeof(*network));
  network->sin_family = AF_INET;
  memcpy((void *) &(network->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);
  network->sin_port = htons((u_short) *NETWORK_PORT);

  memset((void *) src, 0, sizeof(*src));
  src->sin_family = AF_INET;
  src->sin_addr.s_addr = htonl(INADDR_ANY);
  src->sin_port = htons(*ME_PORT);
  if(bind (*sockfd, (struct sockaddr *) src, sizeof(*src)) < 0) {
    perror("bind");
    exit(1);
  }
  fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(src->sin_addr), ntohs(src->sin_port));
  fprintf(stderr, "\nWe are sending to the network: %s:%d\n", inet_ntoa(network->sin_addr), ntohs(network->sin_port));
  return;
}

/*
* Prompts the user for a string
* @params, char* buffer - where the string will be stored
*/
void getMessageFromUser(char *buffer) {
  printf("Enter the message to send: ");
  fgets(buffer, 500, stdin);
  return;
}
