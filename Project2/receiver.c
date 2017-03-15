/*
* @filename receiver.c
* @author Alex Lindemann , Nathan Moore
* @created 03/04/2017
* @desc This is a receiver program implementing rdt 3.0
* receiver port
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "rdtReceiver.h"
#include "definitions.h"

void initialize_from_args(int, char **, int *);
void initialize_network(int *, struct sockaddr_in *, struct hostent** , char *, int *);
int main (int argc, char *argv[]) {

  /* parse the command line arguments */
  int ME_PORT;
  initialize_from_args(argc, argv, &ME_PORT);

  /* setup the network */
  int sockfd;
  struct sockaddr_in src;
  socklen_t src_length = sizeof(src);
  struct hostent *hostptr;
  char hostname[100];
  initialize_network(&sockfd, &src, &hostptr, hostname, &ME_PORT);
  //TODO put into its own function everything below
  char segment[PACKET_LENGTH];

  recvfrom(sockfd, segment, PACKET_LENGTH, 0, (struct sockaddr *)&src, &src_length);
  printf("Got packet: \"%s\"\n", segment);
  //now respond

  return 0;
}

void initialize_from_args(int argc, char **argv, int *ME_PORT){
  if (argc != 2) {
    fprintf(stderr, "Usage: receiver port\n");
    exit(1);
  }
  *ME_PORT = atoi(argv[1]);
  return;
}

void initialize_network(int *sockfd, struct sockaddr_in *src, struct hostent** hostptr, char *hostname, int *ME_PORT){
  gethostname(hostname, 100);
  *hostptr = gethostbyname(hostname);
  *sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  memset((void *) src, 0, (size_t) sizeof(*src));

  src->sin_family = AF_INET;
  src->sin_addr.s_addr = htonl(INADDR_ANY);
  src->sin_port = htons(*ME_PORT);
  memcpy((void *) &(src->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);

  if(bind(*sockfd, (struct sockaddr *) src, sizeof(*src)) < 0) {
    perror("bind");
    exit(1);
  }
  fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(src->sin_addr), ntohs(src->sin_port));
}
