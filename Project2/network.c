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

int forward_packet(char *, char *, int, char *, int, char *, int);

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
  fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(network.sin_addr), ntohs(network.sin_port));


  //TODO put into its own function everything below
  char segment[PACKET_LENGTH];
  memset(segment, 0, PACKET_LENGTH);
  struct sockaddr_in peer;
  socklen_t peerAddrLen = sizeof(peer);

  recvfrom(sockfd, segment, PACKET_LENGTH, 0, (struct sockaddr *) &peer, &peerAddrLen);

  char segmentCopy[PACKET_LENGTH];
  memset(segmentCopy, '\0', PACKET_LENGTH);
  memcpy(segmentCopy, segment, PACKET_LENGTH);

  char* srcIP;
  char* srcPortStr;
  int srcPort;
  char* destIP;
  char* destPortStr;
  int destPort;

  char* message;
  srcIP = segmentCopy;
  srcPortStr = segmentCopy + 16;
  destIP = srcPortStr + 6;
  destPortStr = destIP + 16;
  message = destPortStr + 6;

  srcPort = atoi(srcPortStr);
  destPort = atoi(destPortStr);

  printf("\nsrcIP: %s\n", srcIP);
  printf("srcPort: %d\n", srcPort);
  printf("srcPortStr: %s\n", srcPortStr);
  printf("destIP: %s\n", destIP);
  printf("destPort: %d\n", destPort);
  printf("destPortStr: %s\n", destPortStr);
  printf("message: %s\n", message);

  forward_packet(segment, srcIP, srcPort, destIP, destPort, message, sockfd);


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

}

/*
* @params wholeData is the packet received in its entirety of char[54] "srcIPsrcPortdestIPdestPortSegment"
*/
int forward_packet(char *wholeData, char *srcIP, int srcPort, char *destIP, int destPort, char *segment,int sockfd) {
  struct sockaddr_in dest;

  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(destPort);
  inet_aton(destIP, &dest.sin_addr);

  sendto(sockfd, wholeData, PACKET_LENGTH, 0, (struct sockaddr *) &dest, sizeof(dest));
  return 1;
}

void print_packet (char * packet) {
  int i = 0;
  while(i < 54) {
    if(packet[i])
      putchar(packet[i]);
    else {
      putchar(' ');
    }
    i++;
  }
}
