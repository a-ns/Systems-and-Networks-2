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
void print_message(char *);
void initialize_network(int *, struct sockaddr_in *, struct hostent** , char *, int *);
int main (int argc, char *argv[]) {

  /* parse the command line arguments */
  int ME_PORT;
  initialize_from_args(argc, argv, &ME_PORT);
  char * message = receiveMessage(ME_PORT);
  printf("\nMessage: %s\n", message);
  free(message);
  return 0;
}

char * receiveMessage(int ME_PORT){

    /* setup the network */
    char * wholeMessage = malloc(2048);
    wholeMessage[0] = '\0';
    int sockfd;
    struct sockaddr_in src;
    socklen_t src_length = sizeof(src);
    struct hostent *hostptr;
    char hostname[100];
    initialize_network(&sockfd, &src, &hostptr, hostname, &ME_PORT);

    char seq = '0';
    while(1) {
      char packet[PACKET_LENGTH];
      memset(packet, '\0', PACKET_LENGTH);
      recvfrom(sockfd, packet, PACKET_LENGTH, 0, (struct sockaddr *)&src, &src_length);
      if(packet[44] == 'F') {
        return wholeMessage;
      }
      else {
        //now respond
        int packetChecksum = atoi(packet + 50);
        char packetSeq = packet[45];
        if(checksum((packet + 44), 6) == packetChecksum && packetSeq == seq) {
          print_message(packet);
          memcpy(wholeMessage + strlen(wholeMessage), packet + 46, 4);
          char packetCopy[PACKET_LENGTH];
          memset(packetCopy, '\0', PACKET_LENGTH);
          memcpy(packetCopy, packet, PACKET_LENGTH);
          memcpy(packetCopy, packetCopy + 22, 22);
          memcpy(packetCopy + 22, packet, 22);
          memset(packetCopy + 44, ACK, 1);
          memset(packetCopy + 45, seq, 1);
          char chksumStr[5];
          sprintf(chksumStr, "%d", checksum(packetCopy+44, 6));
          memcpy(packetCopy + 50, chksumStr, 4);

          //print_packet(packetCopy);
          //printf("Sending to: %s:%d", inet_ntoa(src.sin_addr), ntohs(src.sin_port));
          if(sendto(sockfd, packetCopy, PACKET_LENGTH, 0 , (struct sockaddr *) &src, src_length) <0 ){
            perror("sendto fail");
            return NULL;
          }
          if (seq == '0' )
            seq = '1';
          else seq = '0';
        }
        else {
          if(checksum((packet + 44), 6) != packetChecksum) {
            printf("Received corrupt packet\n");
          }
          else {// received out of sequence
            char packetCopy[PACKET_LENGTH];
            memset(packetCopy, '\0', PACKET_LENGTH);
            memcpy(packetCopy, packet, PACKET_LENGTH);
            memcpy(packetCopy, packetCopy + 22, 22);
            memcpy(packetCopy + 22, packet, 22);
            memset(packetCopy + 44, ACK, 1);
            char chksumStr[5];
            sprintf(chksumStr, "%d", checksum(packetCopy + 44, 6));
            memcpy(packetCopy + 50, chksumStr, 4);
            //printf("Sending: "); print_packet(packetCopy); printf("\n");
            sendto(sockfd, packetCopy, PACKET_LENGTH, 0, (struct sockaddr *) &src, src_length);
          }
        }
      }
    }
    return NULL;
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

void print_packet (char * packet) {
  int i = 0;
  while(i < PACKET_LENGTH) {
    if(packet[i])
      putchar(packet[i]);
    else {
      putchar(' ');
    }
    i++;
  }
  printf("\n");
}

void print_message(char * message) {
  message = message + 46;
  char *end = message + 4;
  while(message != end) {
    fprintf(stderr, "%c", *message);
    message++;
  }
}
