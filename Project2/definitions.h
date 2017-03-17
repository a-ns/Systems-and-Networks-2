#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H
#include <stdint.h>

#define PACKET_LENGTH 54
#define TIMER_IN_SECONDS 2
#define ACK 'A'
#define SYN 'S'


void print_packet (char *);
int checksum(char *, int);
#endif
