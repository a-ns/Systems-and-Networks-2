/*
* @filename network.c
* @author Alex Lindemann , Nathan Moore
* @created 03/04/2017
* @desc This is a network program implementing rdt 3.0
* network port lostPercent delayedPercent errorPercent
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "definitions.h"

void initialize_from_args(int, char **, int *, double *, double *, double *);
void initialize_network(int *, struct sockaddr_in *, struct hostent** , char *, int *);
int main (int argc, char *argv[]) {

  /* parse the command line arguments */
  int ME_PORT;
  double lostPercent;
  double delayedPercent;
  double errorPercent;
  initialize_from_args(argc, argv, &ME_PORT, &lostPercent, &delayedPercent, &errorPercent);

  /* setup the network */
  int sockfd;
  struct sockaddr_in network;
  struct hostent *hostptr;
  char hostname[100];
  initialize_network(&sockfd, &network, &hostptr, hostname, &ME_PORT);

  char message[500];
  struct sockaddr_in peer;
  socklen_t peerAddrLen = sizeof(peer);
  
  recvfrom(sockfd, message, 54, 0, (struct sockaddr *) &peer, &peerAddrLen);
  printf("\n Got message: %s\n", message);


  return 0;
}

void initialize_from_args(int argc, char **argv, int *ME_PORT, double *lostPercent, double *delayedPercent, double *errorPercent){
  if (argc != 5) {
    fprintf(stderr, "Usage: network port lostPercent delayedPercent errorPercent\n");
    exit(1);
  }
  *ME_PORT = atoi(argv[1]);
  *lostPercent = atof(argv[2]);
  *delayedPercent = atof(argv[3]);
  *errorPercent = atof(argv[4]);
  return;
}

void initialize_network(int *sockfd, struct sockaddr_in *network, struct hostent** hostptr, char *hostname, int *ME_PORT){
  gethostname(hostname, 100);
  *hostptr = gethostbyname(hostname);
  *sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  memset((void *) network, 0, (size_t) sizeof(*network));

  network->sin_family = AF_INET;
  network->sin_addr.s_addr = htonl(INADDR_ANY);
  network->sin_port = htons(*ME_PORT);
  memcpy((void *) &(network->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);

  if(bind(*sockfd, (struct sockaddr *) network, sizeof(*network)) < 0) {
    perror("bind");
    exit(1);
  }
  fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(network->sin_addr), ntohs(network->sin_port));






}
