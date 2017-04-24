#ifndef __NODE_H_
#define __NODE_H_
#include "mahtypes.h"
#include <pthread.h>
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
struct dijkstra_return_v * dijkstra_shortest_path(struct matrix *, int);
int minimumDistance(boolean *, int *, int);
void print_forwarding_table(struct router *, struct dijkstra_return_v *);
void initialize_network(int *, struct sockaddr_in *, struct hostent**, char *, int *);
void * flooding_thread(void *);
void spawn_flooding_thread(pthread_t *, struct router *);
int receive_lsp (int , char * ,int *, struct sockaddr_in *);

#endif // __NODE_H_
