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

/* Our Dijkstra matrix will be a 2D array of ints. The rows/columns will correspond to the neighboring nodes represented as an integer
   from 0 to totalNumRouters - 1. To get the information for the corresponding router in that matrix, just index into struct neighbors.theNeighbors[i].
   Ex: dijkstra[2][2] 's information will be in struct neighbors.theNeighbors[2]
*/

int countFile(char *);
struct neighbors * readFile(char*, int);
void checkCommandLineArguments(int, char**);

int main (int argc, char *argv[]) {
  checkCommandLineArguments(argc, argv);
  readFile(argv[4], atoi(argv[3]));
  return 0;
}

void checkCommandLineArguments(int argc, char *argv[]) {
  if (argc != 6 && argc != 5){
    printf("usage: node routerLabel portNum totalNumRouters discoverFile [-dynamic]\n");
    exit(0);
  }
  return;
}

struct neighbors * readFile(char * filename, int totalNumRouters) {
  int numOfNeighbors = countFile(filename);
  struct neighbors * theNeighbors = malloc(sizeof(struct neighbors));
  theNeighbors->physicalSize = totalNumRouters;
  theNeighbors->theNeighbors = malloc(sizeof(struct neighbor)*totalNumRouters);
  theNeighbors->numOfNeighbors = numOfNeighbors;

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
    printf("processingNeighbor_String: %s\n", processingNeighbor_String);
  }
  fclose(fp);
  return theNeighbors;
}

int countFile(char * filename) {
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
