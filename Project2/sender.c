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
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "rdtSender.h"
#include "definitions.h"

void getMessageFromUser(char *);
void checkCommandLineArguments(int, char**);
int setup_network (int* , char *, int , char *, int , struct hostent** , struct sockaddr_in * , struct sockaddr_in * , int);
int timeout_recvfrom (int , char *, int *, struct sockaddr_in *, int);
int main (int argc, char *argv[]) {

  checkCommandLineArguments(argc, argv);
  char messageToSend[500];
  getMessageFromUser(messageToSend);
  sendMessage(atoi(argv[1]), argv[4], atoi(argv[5]), argv[2], atoi(argv[3]), messageToSend);

  return 0;
}

void checkCommandLineArguments(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr, "Usage: sender port rcvHost rcvPort networkHost networkPort\n");
    exit(1);
  }
}

int setup_network(int* sockfd, char *netwhost, int netwPort, char *desthost, int destPort, struct hostent** hostptr, struct sockaddr_in * network, struct sockaddr_in * src, int localPort){
*sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 if (sockfd < 0 ) {
   perror("error opening socket");
   return -2;
 }
 *hostptr = gethostbyname(netwhost);
 memset((void *) network, 0, (size_t) sizeof(*network));
 network->sin_family = AF_INET;
 memcpy((void *) &(network->sin_addr), (void *) (*hostptr)->h_addr, (*hostptr)->h_length);
 network->sin_port = htons((u_short) netwPort);

 memset((void *) src, 0, sizeof(*src));
 src->sin_family = AF_INET;
 src->sin_addr.s_addr = htonl(INADDR_ANY);
 src->sin_port = htons(localPort);
 if(bind (*sockfd, (struct sockaddr *) src, sizeof(*src)) < 0) {
   perror("bind");
   return -1;
 }
 fprintf(stderr, "\nWe are listening on: %s:%d\n", inet_ntoa(src->sin_addr), ntohs(src->sin_port));
 fprintf(stderr, "\nWe are sending to the network: %s:%d\n", inet_ntoa(network->sin_addr), ntohs(network->sin_port));
return 0;
}

/*
* Prompts the user for a string
* @params, char* buffer - where the string will be stored
*/
void getMessageFromUser(char *buffer) {
  printf("Enter the message to send: ");
  fgets(buffer, 500, stdin);
  buffer[strlen(buffer) -1] = '\0';
  return;
}

int sendMessage (int localPort, char* netwhost, int netwPort, char* desthost, int destPort, char* message) {
  int sockfd;
  struct hostent *hostptr;
  struct sockaddr_in network, src;
  int err;
  if ((err = setup_network(&sockfd, netwhost, netwPort, desthost, destPort, &hostptr, &network, &src, localPort)) < 0) {
    return err;
  }
  char srcHost[100];
  gethostname(srcHost, 100);
  struct sockaddr_in tmpSockAddr;
  memcpy((void *) &(tmpSockAddr.sin_addr), (void *) (hostptr)->h_addr, (*hostptr).h_length);
  strcpy(srcHost, inet_ntoa(tmpSockAddr.sin_addr));
  char packet[PACKET_LENGTH];
  memset(packet, '\0', PACKET_LENGTH);

  char localPortStr[10];
  sprintf(localPortStr, "%d", localPort);
  char destPortStr[10];
  sprintf(destPortStr, "%d", destPort);

  strcpy(packet, srcHost);
  strcpy(packet + 16, localPortStr);
  strcpy(packet + 22, desthost);
  strcpy(packet + 38, destPortStr);
  //start breaking up message into groups of 4 chars, enter rdt3.0 stuff
  //testing
  int segmentsRequired = (strlen(message) + 4 - 1) / 4;
  char segments[segmentsRequired][5];

  int k = 0;
  int i = 0;
  while (i < segmentsRequired){
    memset(segments[i], '\0', 5);
    int j = 0;
    while (j < 4){
      segments[i][j] = message[k];
      j++;
      k++;
    }
    printf("Segment%i: %s\n", i , segments[i]);
    i++;
  }

  //segments now contains each of the strings that needs to be sent to the receiver one by one
  i = 0;
  char seq = '0';
  while (i < segmentsRequired) {


    int ack_received = 0;

    memset(packet + 44, SYN, 1);
    memset(packet + 45, seq, 1);
    memset(packet + 46, '\0', 9);
    memcpy(packet + 46, segments[i], 4);
    int chksum = checksum((packet + 44), 6);
    char chksumStr[5];
    sprintf(chksumStr, "%d", chksum);
    memcpy(packet + 50, chksumStr, 4);

    while (!ack_received) {
      //make packet with current segments[i] and send it

      sendto(sockfd, packet, PACKET_LENGTH, 0, (struct sockaddr *) &network, sizeof(network));
      print_packet(packet);
      char response[PACKET_LENGTH];
      struct sockaddr_in peer;
      int len = PACKET_LENGTH;
      if(timeout_recvfrom(sockfd, response, &len, &peer, 3)) {
        //printf("Received: "); print_packet(response); printf("\n");
        int responseChecksum = atoi(response+50);
        char ack_response = response[45];
        if(checksum(response + 44, 6) == responseChecksum && ack_response == seq){
          ack_received = 1;
        }
        else {
          printf("Received corrupt or out of sequence.\n");
        }
      }
      else {
        printf("Timeout\n");
      }
    }
    i++;
    if (seq == '0' )
      seq = '1';
    else seq = '0';
    memset(packet + 46, '\0', 8);// fill message and checksum with zeroes for reuse
  }

  packet[44] = 'F';
  sendto(sockfd, packet, PACKET_LENGTH, 0, (struct sockaddr *) &network, sizeof(network));


  //end testing



  return 0;
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

int timeout_recvfrom (int sock, char *buf, int *length, struct sockaddr_in *connection, int timeoutinseconds) {
    printf("\nIn timeout recvfrom\n");
    fd_set socks;
    struct timeval t;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    t.tv_sec = timeoutinseconds;
    t.tv_usec = 0;

    if (select(sock + 1, &socks, NULL, NULL, &t) < 0) {
      perror("select");
      exit(1);
    }

    if (FD_ISSET(sock, &socks)) {
      recvfrom(sock, buf, *length, 0, (struct sockaddr *)connection, (void *) length);
      print_packet(buf);
      return 1;
    }
    else {
      return 0;
    }
    return 0;
}
