/*
* @author Alex Lindemann , Nathan Moore
* @filename node.c
* @created 04/13/2017
* @desc Program that simulates a router for determining shortest routes in a network
*
* node routerLabel portNum totalNumRouters discoverFile [-dynamic]
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mahtypes.h"

/* Our Dijkstra matrix will be a 2D array of ints. The rows/columns will correspond to the neighboring nodes represented as an integer
   from 0 to totalNumRouters - 1. To get the information for the corresponding router in that matrix, just index into struct neighbors.theNeighbors[i].
   Ex: dijkstra[2][2] 's information will be in struct neighbors.theNeighbors[2]
*/

int countFile(char *);
char* lsp_serialize(struct linkStatePacket *);
struct linkStatePacket * lsp_deserialize(char *);
struct neighbors * readFile(char*, int);
void checkCommandLineArguments(int, char**);
void cleanup(struct neighbors *);
void print_lsp(struct linkStatePacket *);
void print_struct_neighbor(struct neighbor);

int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  struct neighbors * theNeighbors = readFile(argv[4], atoi(argv[3]));
  // struct linkStatePacket packet;
  // packet.hopCounter = 12;
  // packet.seqNumber = 12;
  // packet.routerInfo.label = 'A';
  // strcpy(packet.routerInfo.hostname , "cs-ssh1.cs.uwf.edu");
  // packet.routerInfo.portNumber = 62100;
  // packet.routerInfo.cost = 6969;
  // struct linkStatePacket * deserialize_test = lsp_deserialize(lsp_serialize(&packet));
  //
  // print_lsp(deserialize_test);

  cleanup(theNeighbors);
  return 0;
}

void checkCommandLineArguments(int argc, char *argv[]) {
  if (argc != 6 && argc != 5){
    printf("usage: node routerLabel portNum totalNumRouters discoverFile [-dynamic]\n");
    exit(0);
  }
  return;
}

void cleanup(struct neighbors * theNeighbors) {
  if (theNeighbors == NULL) return;
  free(theNeighbors->theNeighbors);
  free(theNeighbors);

  return;
}

struct neighbors * readFile(char * filename, int totalNumRouters) {
  if(filename == NULL || totalNumRouters <=0) return NULL;
  int numOfNeighbors = countFile(filename);
  struct neighbors * theNeighbors = malloc(sizeof(struct neighbors));
  theNeighbors->theNeighbors = malloc(sizeof(struct neighbor)*numOfNeighbors);
  theNeighbors->numOfNeighbors = numOfNeighbors;
  theNeighbors->physicalSize = numOfNeighbors;

  FILE *fp = fopen(filename, "r");
  char ch = 0;
  int i;
  int j;
  for(i = 0; i < numOfNeighbors; ++i) {
    char processingNeighbor_String[500];
    j = 0;
    while(ch != '\n') {
      ch = fgetc(fp);
      processingNeighbor_String[j] = ch;
      j++;
    }
    ch = 0;

    processingNeighbor_String[j-1] = '\0';
    printf("processingNeighbor_String: %s\n", processingNeighbor_String);
    // now strtok the string, and shove the data into theNeighbors
    char * token = NULL;
    //get the label
    token = strtok(processingNeighbor_String, ",");
    theNeighbors->theNeighbors[i].label = token[0];
    token = strtok(NULL, ",");
  //  printf("t:%s\n", token);
    // get the hostname
    strcpy(theNeighbors->theNeighbors[i].hostname , token);
    token = strtok(NULL, ",");
  //  printf("t:%s\n", token);
    // get the portNum
    theNeighbors->theNeighbors[i].portNumber = atoi(token);
    token = strtok(NULL, ",");
    //printf("t:%s\n", token);
    // get the cost
    theNeighbors->theNeighbors[i].cost = atoi(token);

    print_struct_neighbor(theNeighbors->theNeighbors[i]);
  }
  fclose(fp);
  return theNeighbors;
}

int countFile(char * filename) {
  if (filename == NULL) return -1;
  FILE * fp = fopen(filename, "r");
  if (fp == NULL)
    return 0;
  int lines = 0;
  int commas = 0;
  char ch;
  while(!feof(fp)){
    ch = fgetc(fp);
    if(ch == ','){
      commas++;
      if(commas % 3 == 0) {
        commas = 0;
        lines++;
      }
    }
  }
  fclose(fp);
  printf("numOfNeighbors: %i\n", lines);
  return lines;
}
// [ hop | hop | seq | seq | label | h|o | s| t| n| a| m|e |. |. |. | .|. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |p | o| r| t | .| c|o |s |t | \0]
char * lsp_serialize(struct linkStatePacket * packet) {
  if (packet == NULL) return NULL;
  char * packet_toString = NULL;
  packet_toString = malloc(50);
  memset(packet_toString, ' ', 50);
  char hopToString[4] = "";
  sprintf(hopToString, "%i", packet->hopCounter);
  memcpy(packet_toString, hopToString, 2);
  memset(packet_toString + 2, ',', 1);


  char seqToString[3] = "";
  sprintf(seqToString, "%i", packet->seqNumber);
  memcpy(packet_toString + 3, seqToString, 2);
  memset(packet_toString + 5, ',', 1);

  memset(packet_toString + 6, packet->routerInfo.label, 1);
  memset(packet_toString + 7, ',', 1);

  memcpy(packet_toString + 8, packet->routerInfo.hostname, 29);
  memset(packet_toString + 8 + strlen(packet->routerInfo.hostname), ' ', 29 - strlen(packet->routerInfo.hostname));
  memset(packet_toString + 37, ',', 1);

  char portToString[6] = "";
  sprintf(portToString, "%i", packet->routerInfo.portNumber);
  memcpy(packet_toString + 38, portToString, 5);
  memset(packet_toString + 43, ',', 1);

  char costToString[5] = "";
  sprintf(costToString, "%i", packet->routerInfo.cost);
  memcpy(packet_toString + 44, costToString, 4);
  memset(packet_toString + 48, ',', 1);


  memset(packet_toString + 49, 0, 1);
  int i = 0;
  while(i < 49){
    if(packet_toString[i] == '\0') {
      fprintf(stderr, "/");
    }
    fprintf(stderr, "%c", packet_toString[i]);

    i++;
  }
  fprintf(stderr, "\n");

  return packet_toString;
}


struct linkStatePacket * lsp_deserialize(char *packet_s) {
  char * token;
  struct linkStatePacket *packet_d = NULL;
  packet_d = malloc(sizeof(struct linkStatePacket));
  token = strtok(packet_s, ",");
//  printf("t:%s\n", token);
  // get the hop counter
  packet_d->hopCounter = atoi(token);
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);
  // get the seqNumber
  packet_d->seqNumber = atoi(token);
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);


  // deserialize struct neighbor
  // get the label
  packet_d->routerInfo.label = token[0];
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);
  // get the hostname
  strcpy(packet_d->routerInfo.hostname , token);
  token = strtok(NULL, ",");
  printf("t:%s\n", token);
  // get the portNum
  packet_d->routerInfo.portNumber = atoi(token);
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);
  // get the cost
  packet_d->routerInfo.cost = atoi(token);


  return packet_d;
}

void print_lsp(struct linkStatePacket * packet){
  printf("Hop Counter: %i\n", packet->hopCounter);
  printf("seqNumber: %i\n", packet->seqNumber);
  print_struct_neighbor(packet->routerInfo);
  return;
}

void print_struct_neighbor(struct neighbor routerInfo) {
  printf("routerInfo.label: %c\n" , routerInfo.label);
  printf("routerInfo.hostname: %s\n", routerInfo.hostname);
  printf("routerInfo.portNumber: %i\n" , routerInfo.portNumber);
  printf("routerInfo.cost: %i\n", routerInfo.cost);
}
