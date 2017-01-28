/*
* @filename bbpeer.c
* @author Alex Lindemann , Nathan Moore
* @created 1/19/2017
*
* @desc This is a client program for a token ring simulation.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
//FUNCTION DECLARATIONS
void start ();
void listen_for_conn();
void show_menu();
void * network_thread(void *);
void process_choice();
//

//GLOBALS
int sockfd;
int argc;
char **argv;






int main (int argc_, char *argv_[]) {
  argc = argc_;
  argv = argv_;
  start();

  return 0;
}

void start () {

  listen_for_conn();

  while(1) {
    show_menu();
    process_choice();
  }
}

void process_choice () {
  char buffer[500];
  fgets(buffer, 500, stdin);
  int choice = atoi(buffer);
  switch (choice) {
    case 1:
      //do write stuff
      printf("1 \n");
      break;
    case 2:
      // do read stuff
      printf("2 \n");
      break;
    case 3:
      printf("3 \n");
      //do list stuff
      break;
    case 4:
      printf("4 \n");
      //do exit stuff, leave token ring
      break;
    default:
      printf("Unknown input:  %i\n", choice);
  }
}

void listen_for_conn (int argc, char* argv[]) {
  pthread_t thread;
  pthread_create(&thread, NULL, network_thread, NULL);
  return;
}

void * network_thread (void * param) {
  //do the connection stuff here i think
  while(1){
    printf("n_thread going to sleep\n");
    sleep(5);
    printf("n_thread waking up\n");
  }
}

void show_menu () {
  printf("\n");
  printf("1. write\n");
  printf("2. read #\n");
  printf("3. list \n");
  printf("4. exit \n");
  printf("\n");
  return;
}
