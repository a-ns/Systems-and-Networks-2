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
void printEntry( struct entry );
struct router * newRouter(char **, struct neighbors *);
int getLabelIndex(char, char *, int);

int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  struct neighbors * theNeighbors = readFile(argv[4], atoi(argv[3]));

  struct router * theRouter = newRouter(argv, theNeighbors);
  // whenever a new lsp comes in, add its values to theRouter->entries
  // if lsp contains a new undiscovered router, add it to theRouter->networkLabels[theRouter->networkLabelsLength++]




  free(theRouter->entries); // add to cleanup later
  free(theRouter); // add to cleanup later
  cleanup(theNeighbors);
  return 0;
}

int ** build_dijkstra (struct router * router) {
    if(router == NULL) return NULL;

    int ** dijkstra = malloc(sizeof(int) * router->numRouters * router->numRouters);
    int i = 0;
    while ( i < router->numRouters) {
      dijkstra[i][i] = -1;
    }

    i = 0;
    while ( i < router->numEntries) {
      dijkstra[getLabelIndex(router->entries[i].from, router->networkLabels, router->networkLabelsLength)][getLabelIndex(router->entries[i].to, router->networkLabels, router->networkLabelsLength)] = router->entries[i].cost;
      i++;
    }

    return dijkstra;
}

struct entry * receive_lsp( char * packet) {
  struct linkStatePacket * packet_d = lsp_deserialize(packet);
  struct entry * entry = malloc(sizeof(entry));
  entry->to = packet_d->entry.to;
  entry->from = packet_d->entry.from;
  entry->cost = packet_d->entry.cost;

  free(packet_d);
  return entry;
}

struct router * newRouter(char **argv, struct neighbors * theNeighbors) {
  struct router * theRouter = malloc(sizeof(struct router));
  theRouter->label = argv[1][0];
  gethostname(theRouter->hostname, 30);
  theRouter->numRouters = atoi(argv[3]);
  theRouter->numEntries = 0;
  theRouter->networkLabels = malloc(theRouter->numRouters + 1);
  theRouter->networkLabelsLength = 1;
  theRouter->networkLabels[0] = theRouter->label;
  theRouter->entries = malloc(sizeof(struct entry)*theRouter->numRouters*theRouter->numRouters);
  // add neighbors to the router
  while (theRouter->numEntries < theNeighbors->numOfNeighbors) {
    theRouter->entries[theRouter->numEntries].to = theNeighbors->theNeighbors[theRouter->numEntries].label;
    theRouter->entries[theRouter->numEntries].cost = theNeighbors->theNeighbors[theRouter->numEntries].cost;
    theRouter->entries[theRouter->numEntries].from = theRouter->label;

    theRouter->networkLabels[theRouter->networkLabelsLength] = theNeighbors->theNeighbors[theRouter->numEntries].label;

    theRouter->numEntries++;
    theRouter->networkLabelsLength++;
  }


  return theRouter;
}

void checkCommandLineArguments(int argc, char *argv[]) {
  if (argc != 6 && argc != 5){
    printf("usage: node routerLabel portNum totalNumRouters discoverFile [-dynamic]\n");
    exit(0);
  }
  return;
}

/*
* Frees the malloc'd struct neighbors *
* @param theNeighbors struct neighbors * pointer to the initial known neighbors that were read from the file
* @param networkNodes struct neighbors * pointer to information about other routers on the network that this node isn't directly connected to
*/
void cleanup(struct neighbors * theNeighbors) {
  if (theNeighbors == NULL ) return;
  free(theNeighbors->theNeighbors);
  free(theNeighbors);
  return;
}

/*
* Reads a file containing information about the immediate neighbors to this node
* @param filename char * path to the file to be read
* @param totalNumRouters int the number of routers on the network passed in from the command line
* @return struct neighbors * information about the immediate neighbors to this node
*/
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
  //  printf("processingNeighbor_String: %s\n", processingNeighbor_String);
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

    //print_struct_neighbor(theNeighbors->theNeighbors[i]);
  }
  fclose(fp);
  return theNeighbors;
}

/*
* Counts the number of neighbors in a filename
* @param filename char * location of the file
* @return number of known neighbors for this node
*/
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
// [ hop | hop | seq | seq | labelFrom | h|o | s| t| n| a| m|e |. |. |. | .|. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |. |p | o| r| t | .| c|o |s |t | labelTo |\0]
/*
* Serializes a struct linkStatePacket to send it across a network
* @param struct linkStatePacket * pointer to the desired packet to be serialized
* @return char * representation of a struct linkStatePacket
*/
char * lsp_serialize(struct linkStatePacket * packet) {
  if (packet == NULL) return NULL;
  char * packet_toString = NULL;
  packet_toString = malloc(55);
  memset(packet_toString, ' ', 55);
  char hopToString[4] = "";
  sprintf(hopToString, "%i", packet->hopCounter);
  memcpy(packet_toString, hopToString, 2);
  memset(packet_toString + 2, ',', 1);


  char seqToString[3] = "";
  sprintf(seqToString, "%i", packet->seqNumber);
  memcpy(packet_toString + 3, seqToString, 2);
  memset(packet_toString + 5, ',', 1);

  memset(packet_toString + 6, packet->entry.from, 1);
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
  memset(packet_toString + 49, packet->entry.to, 1);
  memset(packet_toString + 50, ',', 1);
  memset(packet_toString + 51, 0, 1);
  int i = 0;
  while(i < 51){
    if(packet_toString[i] == '\0') {
      fprintf(stderr, "/");
    }
    fprintf(stderr, "%c", packet_toString[i]);

    i++;
  }
  fprintf(stderr, "\n");

  return packet_toString;
}

/*
* deserialize a struct linkStatePacket to be able to receive it across a network
* @param packet_s - char * a serialized version of a struct linkStatePacket
* @return struct linkStatePacket * pointer to the constructed/deserialized linkStatePacket
*/
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
  packet_d->entry.from = token[0];
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
  packet_d->entry.cost = atoi(token);
  token = strtok(NULL, ",");
  // get the labelTo
  packet_d->entry.to = token[0];

  return packet_d;
}
/*
* Prints the struct linkStatePacket pointed to by packet
* @param packet - struct linkStatePacket *
*/
void print_lsp(struct linkStatePacket * packet){
  printf("Hop Counter: %i\n", packet->hopCounter);
  printf("seqNumber: %i\n", packet->seqNumber);
  print_struct_neighbor(packet->routerInfo);
  printEntry(packet->entry);
  return;
}
/*
* Prints the struct neighbor
* @param routerInfo - struct neighbor
*/
void print_struct_neighbor(struct neighbor routerInfo) {
  printf("routerInfo.label: %c\n" , routerInfo.label);
  printf("routerInfo.hostname: %s\n", routerInfo.hostname);
  printf("routerInfo.portNumber: %i\n" , routerInfo.portNumber);
  printf("routerInfo.cost: %i\n", routerInfo.cost);
}

/*
* Gets the index in the 2D Dijkstra array for the label
* @param label - char the desired label to be converted to an index
* @param allLabels - all the labels of the peers on the network in the order in which they were discovered
* @param allLabelsLength - length of allLabels
* @return the index
*/
int getLabelIndex(char label, char *allLabels, int allLabelsLength) {
  if (label > 'Z' || label < 'A') {
    printf("Invalid label.\n");
    return -1;
  }
  int i;
  for(i = 0; i < allLabelsLength; i ++) {
    if(label == allLabels[i]) {
      return i;
    }
  }
  return -1;
}

void printEntry (struct entry entry){
  printf("Entry: %3c %3c %i\n", entry.from, entry.to, entry.cost);
  return;
}
