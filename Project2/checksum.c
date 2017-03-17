
#include <stdio.h>
#include "definitions.h"

int checksum (char *data, int length){
  int sum = 0;
  int i;
  for(i = 0; i < length; i++){
    sum += ((int)*data)%(i+50);
    printf("%i ", *data);
    data++;
  }

  printf("\nchecksum: %i\n", sum);
  return sum;
}
