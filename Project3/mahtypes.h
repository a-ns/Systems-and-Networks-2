
#ifndef _MAHTYPES_H_
#define _MAHTYPES_H_

struct neighbor {
  char * label;
  char * hostname;
  int portNumber;
  int cost;
};

struct neighbors {
  int numOfNeighbors; // logical size
  int physicalSize;
  struct neighbor * theNeighbors; // array of struct
};

struct linkStatePacket {
  int hopCounter;
  int seqNumber;
  struct neighbor routerInfo;
};

#endif // _MAHTYPES_H_
