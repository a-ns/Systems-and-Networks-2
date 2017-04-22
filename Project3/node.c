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
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include "mahtypes.h"

/* Our Dijkstra matrix will be a 2D array of ints. The rows/columns will correspond to the neighboring nodes represented as an integer
   from 0 to totalNumRouters - 1. To get the information for the corresponding router in that matrix, just index into struct neighbors.theNeighbors[i].
   Ex: dijkstra[2][2] 's information will be in struct neighbors.theNeighbors[2]
*/
int node_to_forward_to(int*, int, int);
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
struct matrix * build_dijkstra(struct router *);
void printGraph (struct matrix *);
struct dijkstra_return_v * dijkstra_shortest_path(struct matrix *, int, int);
int minimumDistance(boolean *, int *, int);
void print_forwarding_table(struct router *, struct dijkstra_return_v *);
int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  struct neighbors * theNeighbors = readFile(argv[4], atoi(argv[3]));

  struct router * theRouter = newRouter(argv, theNeighbors);
  // whenever a new lsp comes in, add its values to theRouter->entries
  // if lsp contains a new undiscovered router, add it to theRouter->networkLabels[theRouter->networkLabelsLength++]


  struct matrix * dijkstra = build_dijkstra(theRouter);

  printGraph(dijkstra);

//  printf("Shortest distances:\n");
//  printf(" %3c %3c %3c\n", theRouter->networkLabels[0], theRouter->networkLabels[1], theRouter->networkLabels[2]);
//  printf("%c", theRouter->label);
  struct dijkstra_return_v *rv = dijkstra_shortest_path(dijkstra, getLabelIndex(theRouter->label, theRouter->networkLabels, theRouter->networkLabelsLength), getLabelIndex('B', theRouter->networkLabels ,theRouter->networkLabelsLength));

  print_forwarding_table(theRouter, rv);



  free(rv->prev);
  free(rv->dist);
  free(rv);
  free(theRouter->entries); // add to cleanup later
  free(theRouter); // add to cleanup later
  int i = 0;
  /* deallocate the array */
  for (i=0; i< dijkstra->rows; i++) {
    free(dijkstra->m[i]);
  }
  free(dijkstra);
  cleanup(theNeighbors);
  return 0;
}
void print_forwarding_table(struct router * theRouter, struct dijkstra_return_v *rv) {
  printf("Forwarding table\n");
  int i;
  printf(" ");
  for(i = 1; i < theRouter->networkLabelsLength; i++){
    printf("%3c", theRouter->networkLabels[i]);
  }
  printf("\n");
  printf("%c", theRouter->networkLabels[0]);

  for(i = 1; i < theRouter->networkLabelsLength; i++){
    printf("%3c", theRouter->networkLabels[node_to_forward_to(rv->prev, 0, i)]);
  }
  printf("\n");
}

void printGraph(struct matrix * graph){
  int i,j;
  for(i = 0; i < graph->rows; i++){
    for(j = 0; j < graph->cols; j++){
      printf(" %i", graph->m[i][j]);
    }
    printf("\n");
  }
  return;
}

struct matrix * build_dijkstra (struct router * router) {
    if(router == NULL) return NULL;
    struct matrix * matrix = malloc(sizeof(struct matrix));
    matrix->rows = router->numRouters;
    matrix->cols = router->numRouters;
    //int ** dijkstra;//malloc(sizeof(int) * router->numRouters * router->numRouters);
    /* allocate the array */
    matrix->m =  malloc(router->numRouters * sizeof * matrix->m);
    int i = 0;
    for (i=0; i<router->numRouters; i++){
      matrix->m[i] = malloc(router->numRouters * sizeof *matrix->m[i]);
    }
    i = 0;
    int j = 0;
    for(i = 0; i < matrix->rows; i++){
      for(j = 0; j < matrix->cols; j++){
        matrix->m[i][j] = 0;
      }
    }

    i = 0;
    while ( i < router->numEntries) {
      matrix->m[getLabelIndex(router->entries[i].from, router->networkLabels, router->networkLabelsLength)][getLabelIndex(router->entries[i].to, router->networkLabels, router->networkLabelsLength)] = router->entries[i].cost;
      i++;
    }

    return matrix;
}

//TODO Change this to match new struct linkStatePacket
struct entry * receive_lsp( char * packet) {
  struct linkStatePacket * packet_d = lsp_deserialize(packet);
//  entry->to = packet_d->entry.to;
//  entry->from = packet_d->entry.from;
//  entry->cost = packet_d->entry.cost;

  free(packet_d);
  return NULL;//entry;
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
/*
* Serializes a struct linkStatePacket to send it across a network
* @param struct linkStatePacket * pointer to the desired packet to be serialized
* @return char * representation of a struct linkStatePacket
*/
char * lsp_serialize(struct linkStatePacket * packet) {
  if (packet == NULL) return NULL;
  char * packet_toString = NULL;
  packet_toString = malloc(60);
  memset(packet_toString, 0, 60);

  //hop counter
  char hopToString[4] = "";
  sprintf(hopToString, "%i", packet->hopCounter);
  memcpy(packet_toString, hopToString, 2);
  memset(packet_toString + strlen(packet_toString), ',', 1);
  //seq number
  char seqToString[3] = "";
  sprintf(seqToString, "%i", packet->seqNumber);
  memcpy(packet_toString + strlen(packet_toString), seqToString, 2);
  memset(packet_toString + strlen(packet_toString), ',', 1);
  //router label
  memset(packet_toString + strlen(packet_toString), packet->routerLabel,1);
  memset(packet_toString + strlen(packet_toString), ',', 1);
  //num entries
  char numEntries[4] = "";
  sprintf(numEntries, "%i", packet->numEntries);
  memcpy(packet_toString + strlen(packet_toString), numEntries, 4);
  memset(packet_toString + strlen(packet_toString), ',', 1);

  //struct entry entries
  int i;
  for(i = 0 ; i < packet->numEntries; i++) {
    memset(packet_toString + strlen(packet_toString), packet->entries[i].to, 1);
    memset(packet_toString + strlen(packet_toString), ',', 1);
    char costToString[4] = "";
    sprintf(costToString, "%i", packet->entries[i].cost);
    memcpy(packet_toString + strlen(packet_toString), costToString, 3);
    memset(packet_toString + strlen(packet_toString), ',', 1);
  }
  return packet_toString;
}

/*
* deserialize a struct linkStatePacket to be able to receive it across a network
* @param packet_s - char * a serialized version of a struct linkStatePacket
* @return struct linkStatePacket * pointer to the constructed/deserialized linkStatePacket
*/
//TODO change this to match new struct linkStatePacket
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

//TODO extract numEntries
  //packet_d->numEntries = atoi(token);
  // deserialize struct neighbor
  // get the label
//  packet_d->routerInfo.label = token[0];
/* TODO
  for i < numEntries
    add new entry to packet_d
*/
  //packet_d->entry.from = token[0];
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);
  // get the hostname
//  strcpy(packet_d->routerInfo.hostname , token);
  token = strtok(NULL, ",");
  printf("t:%s\n", token);
  // get the portNum
//  packet_d->routerInfo.portNumber = atoi(token);
  token = strtok(NULL, ",");
//  printf("t:%s\n", token);
  // get the cost
//  packet_d->entry.cost = atoi(token);
  token = strtok(NULL, ",");
  // get the labelTo
//  packet_d->entry.to = token[0];

  return packet_d;
}
/*
* Prints the struct linkStatePacket pointed to by packet
* @param packet - struct linkStatePacket *
*/
void print_lsp(struct linkStatePacket * packet){
  printf("Hop Counter: %i\n", packet->hopCounter);
  printf("seqNumber: %i\n", packet->seqNumber);
//  print_struct_neighbor(packet->routerInfo);
  int i;
  for( i = 0 ; i < packet->numEntries; ++i)
    printEntry(packet->entries[i]);
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
    printf("Invalid label %c.\n", label);
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

struct dijkstra_return_v * dijkstra_shortest_path (struct matrix * graph, int start, int dest) {
    //boolean[] visited = new boolean[V];
    boolean visited[graph->rows];
    //int[] dist = new int[V]; //stores best distance from start to all other nodes.
    int dist[graph->rows];
    //int[] prev = new int[V]; //For figuring out the path from start to dest
    int prev[graph->rows];
    int i;
    for (i = 0; i < graph->rows; i++) {
      visited[i] = FALSE;
      dist[i] = INT_MAX; //"infinity" value because they are unknown
    }
    dist[start] = 0; //distance from start to itself is zero
    prev[start] = -1;
    int fromV;
    for (fromV = 0; fromV < graph->rows - 1; fromV++) {
      int current = minimumDistance(visited, dist, graph->rows); //find the next vertex to work on. starts with start vertex because we set it to zero.
      visited[current] = TRUE; //mark the current node as visited so we don't keep doing it

      for (int toV = 0; toV < graph->rows; toV++) {
        /* 1. check if the toV (vertex) is not visited
           2. if there is an edge from current to toV (0 in the graph indicates no edge)
           3. dist[current] is obviously not MAX_VALUE for the one to do, but we want to make this check so that 4) does not overflow
           4. if the distance to the current vertex plus the weight to the next vertex is better than the existing value to the next vertex
                then update that distance.
        */
        if (visited[toV] == FALSE && graph->m[current][toV] != 0 && dist[current] != INT_MAX
                                  && dist[current] + graph->m[current][toV] < dist[toV]) {
            dist[toV] = dist[current] + graph->m[current][toV];
            prev[toV] = current; //this helps us "remember" the pathway to that vertex
          }
      }
    }
    struct dijkstra_return_v *rv = malloc(sizeof(struct dijkstra_return_v));
    rv->dist = malloc(sizeof(int)* graph->rows);
    rv->prev = malloc(sizeof(int) * graph->rows);
    memcpy(rv->dist, dist, sizeof(dist));
    memcpy(rv->prev, prev, sizeof(prev));
    return rv;
  }

  int minimumDistance (boolean *visited, int * dist, int V) {
    int minimum = INT_MAX;
    int u = 0;
    int i;
    for(i = 0; i < V; i++) {
      if (visited[i] == FALSE && dist[i] < minimum) {
        minimum = dist[i];
        u = i;
      }
    }
    return u;
}

int node_to_forward_to(int *prev, int src, int dest) {
  if ( prev[dest] == src) return dest;
  return node_to_forward_to(prev, src, prev[dest]);
}
