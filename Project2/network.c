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

void initialize_from_args(int, char **, int *, int *, int *, int *);
void initialize_network(int *, struct sockaddr_in *, struct hostent** , char *, int *);
int rand_int(int, int);
int forward_packet(char *, char *, int, char *, int, char *, int, int, int, int);

int main (int argc, char *argv[]) {

  /* parse the command line arguments */
  int ME_PORT;
  int lostPercent;
  int delayedPercent;
  int errorPercent;
  initialize_from_args(argc, argv, &ME_PORT, &lostPercent, &delayedPercent, &errorPercent);

  /* setup the network */
  int sockfd;
  struct sockaddr_in network;
  struct hostent *hostptr;
  char hostname[100];
  initialize_network(&sockfd, &network, &hostptr, hostname, &ME_PORT);
  fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(network.sin_addr), ntohs(network.sin_port));

  while(1) {
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
/*
    printf("\nsrcIP: %s\n", srcIP);
    printf("srcPort: %d\n", srcPort);
    printf("srcPortStr: %s\n", srcPortStr);
    printf("destIP: %s\n", destIP);
    printf("destPort: %d\n", destPort);
    printf("destPortStr: %s\n", destPortStr);
    printf("message: %s\n", message);
    */
    print_packet(segment);


    forward_packet(segment, srcIP, srcPort, destIP, destPort, message, sockfd, lostPercent, delayedPercent, errorPercent);
  }

  return 0;
}

void initialize_from_args(int argc, char **argv, int *ME_PORT, int *lostPercent, int *delayedPercent, int *errorPercent){
  if (argc != 5) {
    fprintf(stderr, "Usage: network port lostPercent delayedPercent errorPercent\n");
    exit(1);
  }
  *ME_PORT = atoi(argv[1]);
  *lostPercent = atoi(argv[2]);
  *delayedPercent = atoi(argv[3]);
  *errorPercent = atoi(argv[4]);
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
int forward_packet(char *wholeData, char *srcIP, int srcPort, char *destIP, int destPort, char *segment,int sockfd, int lostPercent, int delayedPercent, int errorPercent) {

  struct sockaddr_in dest;

  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(destPort);
  inet_aton(destIP, &dest.sin_addr);


    if ( rand_int(100, 0) < lostPercent) {
      printf("Packet dropped\n");
      return 1;
    }
    if ( rand_int(100, 0) < errorPercent) {
      printf("Corrupting packet\n");
      //increment checksum by one
      wholeData[53]++;
    }
    if( rand_int(100, 0) < delayedPercent) {
      //wait a bit
      printf("Packet delay\n");
      sleep(rand_int(4, 0));
    }

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
  printf("\n");
}

int rand_int (int max_number, int minimum_number) {
  return rand() % (max_number + 1 - minimum_number) + minimum_number;
}