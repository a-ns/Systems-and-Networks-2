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
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include "node.h"

/* Our Dijkstra matrix will be a 2D array of ints. The rows/columns will correspond to the neighboring nodes represented as an integer
   from 0 to totalNumRouters - 1. To get the information for the corresponding router in that matrix, just index into struct neighbors.theNeighbors[i].
   Ex: dijkstra[2][2] 's information will be in struct neighbors.theNeighbors[2]
*/

sem_t flood_lock;
boolean DYNAMIC;
int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  sleep(10);
  sem_init(&flood_lock, 0, 0);
  struct neighbors * theNeighbors = readFile(argv[4], atoi(argv[3]));

  struct router * theRouter = newRouter(argv, theNeighbors);
  // whenever a new lsp comes in, add its values to theRouter->entries
  // if lsp contains a new undiscovered router, add it to theRouter->networkLabels[theRouter->networkLabelsLength++]

  /* Flooding */
  pthread_t flood_thread;
  spawn_flooding_thread(&flood_thread, theRouter);




  sem_wait(&flood_lock);
  //fprintf(stderr, "semaphore unlocked, now do dijkstras\n");
  struct matrix * dijkstra = build_dijkstra(theRouter);
  //fprintf(stderr, "dijkstra built success\n");
  printGraph(dijkstra);
  //fprintf(stderr, "printing dijkstras now\n");
  struct dijkstra_return_v *rv = dijkstra_shortest_path(dijkstra, getLabelIndex(theRouter->label, theRouter->networkLabels, theRouter->networkLabelsLength));

  print_forwarding_table(theRouter, rv);
  pthread_join(flood_thread, NULL);
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

void spawn_flooding_thread(pthread_t *thread, struct router *router) {
  pthread_create(thread, NULL, flooding_thread, (void *) router);
}

void * flooding_thread (void *vrouter) {
  struct router *router = (struct router *) vrouter;

  int sockfd;
  struct sockaddr_in src;
  struct hostent * hostptr;
  char hostname[500];
  initialize_network(&sockfd, &src, &hostptr, hostname, &(router->portNumber));
  sleep(5);
  // flood our neighbors to the network
  /* for each neighbor in neighbors
    sendto(neighbor, lsp_serialize(us))
  */

  int k;
  for(k = 0; k < router->neighbors->numOfNeighbors; k++) {
    struct linkStatePacket packet;
    packet.hopCounter = 6;
    packet.seqNumber = 0;
    packet.routerLabel = router->label;
    packet.numEntries = router->numEntries;
    packet.entries = router->entries;
    struct sockaddr_in neighbor;
    struct hostent *hostptr;
    hostptr = gethostbyname(router->neighbors->theNeighbors[k].hostname);
    memset((void *) &neighbor, 0, (size_t) sizeof(neighbor));
    neighbor.sin_family = AF_INET;
    memcpy((void *) &(neighbor.sin_addr), (void *) hostptr->h_addr, hostptr->h_length);
    neighbor.sin_port = htons((u_short) router->neighbors->theNeighbors[k].portNumber);

    sendto(sockfd, lsp_serialize(&packet), strlen(lsp_serialize(&packet)) + 1, 0, (struct sockaddr *) &neighbor, sizeof(neighbor));
    //fprintf(stderr, "sent flood packet\n");
  }
  //fprintf(stderr, "done flooding\n");

  int seqNumbers[router->numRouters];
  int i;
  for (i = 0; i < router->numRouters; ++i) {
    seqNumbers[i] = 0;
  }
  int numOfFloodedNodes = 0;
  while(1) {
    struct linkStatePacket *packet = receive_lsp(seqNumbers, router, sockfd, &src);
    // forward the packet to our neighbors
    if (packet != NULL && --packet->hopCounter > 0 && packet) {
      numOfFloodedNodes++;
      //fprintf(stderr, "got a packet from %c, hop counter is good, forwarding it\n", packet->routerLabel);
      //forward packet
      /* for each neighbor in neighbors
          sendto(neighbor, lsp_serialize(packet));
      */
      int j;
      for(j = 0; j < router->neighbors->numOfNeighbors; j++) {
        if (router->neighbors->theNeighbors[j].label != packet->routerLabel) {
          //fprintf(stderr, "preparing %i\n", j);
          struct sockaddr_in neighbor;
          struct hostent *hostptr;
          hostptr = gethostbyname(router->neighbors->theNeighbors[j].hostname);
          memset((void *) &neighbor, 0, (size_t) sizeof(neighbor));
          neighbor.sin_family = AF_INET;
          memcpy((void *) &(neighbor.sin_addr), (void *) hostptr->h_addr, hostptr->h_length);
          neighbor.sin_port = htons((u_short) router->neighbors->theNeighbors[j].portNumber);
          //fprintf(stderr, "about to send to %i\n", j);
          sendto(sockfd, lsp_serialize(packet), strlen(lsp_serialize(packet)) + 1, 0, (struct sockaddr *) &neighbor, sizeof(neighbor));
          //fprintf(stderr, "send successfully to %c\n", router->neighbors->theNeighbors[j].label);
        }
      }
    }
    else {
      //fprintf(stderr,"Hop counter is below. dropping \n");
    }
    if(packet != NULL) {
      free(packet->entries);
      free(packet);
    }
    else {
      sem_post(&flood_lock);
    }
  }
  pthread_exit(0);
}

struct linkStatePacket * receive_lsp (int *seqNumbers, struct router *router, int sock, struct sockaddr_in *connection) {
  char buffer[500];
  int length = 500;
  if(timeout_recvfrom(sock, buffer, &length, (struct sockaddr_in *)connection, 10)){
    struct linkStatePacket *packet = lsp_deserialize(buffer); // now stuff info from packet into router
    int i;
    // check the seqNumber
    if (seqNumbers[getLabelIndex(packet->routerLabel, router->networkLabels, router->networkLabelsLength)] > packet->seqNumber) {
      //fprintf(stderr,"Old packet\n");
      return packet; // old packet for us, don't add its entries, just flood it to neighbors
    }
    seqNumbers[getLabelIndex(packet->routerLabel, router->networkLabels, router->networkLabelsLength)] = packet->seqNumber; // make this the current seq Number
    /* now add the entries in the packet to the entries in the router */
    for(i = 0; i < packet->numEntries; i++){

      router->entries[router->numEntries].to = packet->entries[i].to;
      router->entries[router->numEntries].from = packet->entries[i].from;
      router->entries[router->numEntries].cost = packet->entries[i].cost;
      router->numEntries++;

    }
    char tmp[2] = "";
    tmp[0] = packet->routerLabel;
    if (strstr(router->networkLabels, tmp) == NULL) { // add this label
      router->networkLabels[router->networkLabelsLength++] = tmp[0];
    }
    return packet;
  }
  else {
    return NULL;
  }
}

/* Prints the forwarding table for this router.
* @param struct router * theRouter - the router whose table is to be printed
* @param struct dijkstra_return_v *rv - the return values from dijkstras shortest path (a int dist[] and int prev[])
*/
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

/* Prints a 2D array/adjacency matrix pointed to by  struct matrix * graph
* @param graph - the graph to be printe
*/
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

/* Builds an adjacency matrix out of the router passed in
*
*/
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
    //fprintf(stderr,"%i\n\n", router->numEntries);
    while ( i < router->numEntries) {
      //fprintf(stderr, "good here\n");
      matrix->m[getLabelIndex(router->entries[i].from, router->networkLabels, router->networkLabelsLength)][getLabelIndex(router->entries[i].to, router->networkLabels, router->networkLabelsLength)] = router->entries[i].cost;
      i++;
      //fprintf(stderr, "%i\n", i);
    }

    return matrix;
}



/* Constructs a new router/node from the command line arguments and from the neighbors read from a text file
* @return pointer to the router created
*/
struct router * newRouter(char **argv, struct neighbors * theNeighbors) {
  struct router * theRouter = malloc(sizeof(struct router));
  theRouter->neighbors = theNeighbors;
  theRouter->label = argv[1][0];
  gethostname(theRouter->hostname, 30);
  theRouter->portNumber = atoi(argv[2]);
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

/* Makes sure there are the correct number of command line arguments (5 or 6)
*  On failure, program will exit
*/
void checkCommandLineArguments(int argc, char *argv[]) {
  if (argc != 6 && argc != 5){
    printf("usage: node routerLabel portNum totalNumRouters discoverFile [-dynamic]\n");
    exit(1);
  }
  if(argc == 6) {
    DYNAMIC = TRUE;
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
    // now strtok the string, and shove the data into theNeighbors
    char * token = NULL;
    //get the label
    token = strtok(processingNeighbor_String, ",");
    theNeighbors->theNeighbors[i].label = token[0];
    token = strtok(NULL, ",");
    // get the hostname
    strcpy(theNeighbors->theNeighbors[i].hostname , token);
    token = strtok(NULL, ",");
    // get the portNum
    theNeighbors->theNeighbors[i].portNumber = atoi(token);
    token = strtok(NULL, ",");
    // get the cost
    theNeighbors->theNeighbors[i].cost = atoi(token);
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
struct linkStatePacket * lsp_deserialize(char *packet_s) {
  char * token;
  struct linkStatePacket *packet_d = NULL;
  packet_d = malloc(sizeof(struct linkStatePacket));
  //malloc packet_d->entries when we get to that part in the packet_s

  //hopCounter
  token = strtok(packet_s, ",");
  packet_d->hopCounter = atoi(token);

  //seqNumber
  token=strtok(NULL, ",");
  packet_d->seqNumber = atoi(token);

  //routerLabel
  token = strtok(NULL, ",");
  packet_d->routerLabel = token[0];

  //numEntries
  token=strtok(NULL, ",");
  packet_d->numEntries = atoi(token);

  //malloc entries now
  packet_d->entries = malloc(sizeof(struct entry) * packet_d->numEntries);

  int i;
  for(i = 0 ; i < packet_d->numEntries; ++i) {
    packet_d->entries[i].from = packet_d->routerLabel;
    packet_d->entries[i].to = strtok(NULL, ",")[0];
    packet_d->entries[i].cost = atoi(strtok(NULL, ","));
  }
  return packet_d;
}

/*
* Prints the struct linkStatePacket pointed to by packet
* @param packet - struct linkStatePacket *
*/
void print_lsp(struct linkStatePacket * packet){
  printf("Hop Counter: %i\n", packet->hopCounter);
  printf("seqNumber: %i\n", packet->seqNumber);
  printf("routerLabel: %c\n", packet->routerLabel);
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

/* Prints an individual struct entry
* @param struct entry entry - the entry to be printed
*/
void printEntry (struct entry entry){
  printf("Entry: %3c %3c %i\n", entry.from, entry.to, entry.cost);
  return;
}

/* Computes a shortest path vector and a prev node vector for an adjacency matrix
*
*/
struct dijkstra_return_v * dijkstra_shortest_path (struct matrix * graph, int start) {
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

      return 1;
    }
    else {
      return 0;
    }
    return 0;
}
