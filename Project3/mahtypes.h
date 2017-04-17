
#ifndef _MAHTYPES_H_
#define _MAHTYPES_H_

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
  struct neighbor routerInfo;
  struct entry entry;
};



#endif // _MAHTYPES_H_
