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
  int numOfNeighbors;
  struct neighbor * theNeighbors;
};

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
  theNeighbors->theNeighbors = malloc(sizeof(struct neighbor)*numOfNeighbors);
  theNeighbors->numOfNeighbors = numOfNeighbors;
  // theNeighbors->theNeighbors is an array of struct neighbor

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


    processingNeighbor_String[j] = '\0';
    // now strtok the string, and shove the data into theNeighbors
    printf("processingNeighbor_String: %s\n", processingNeighbor_String);
  }

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
  printf("numOfNeighbors: %i\n", lines);
  return lines;
}
