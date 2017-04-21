
#ifndef _MAHTYPES_H_
#define _MAHTYPES_H_

#define boolean int
#define FALSE 0
#define TRUE 1

struct neighbor {
  char label;
  char hostname[30];
  int portNumber;
  int cost;
};

struct neighbors {
  int numOfNeighbors; // logical size
  int physicalSize;
  struct neighbor * theNeighbors; // array of struct
};

struct entry {
  char from;
  char to;
  int cost;
};

struct router {
  char label;
  char hostname[30];
  int numRouters;
  int numEntries;
  struct entry * entries;
  char *networkLabels;
  int networkLabelsLength;
};

struct linkStatePacket {
  int hopCounter;
  int seqNumber;
  char routerLabel; // this could probably just be changed to char label
  int numEntries;
  struct entry * entries;
};

struct dijkstra_return_v {
  int *dist;
  int *prev;
};

struct matrix {
  int rows;
  int cols;
  int **m;
};



#endif // _MAHTYPES_H_
