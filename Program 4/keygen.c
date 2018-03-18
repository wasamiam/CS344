#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){

  int keylength, i;
  time_t t;
  srand((unsigned) time(&t));

  // Error checking
  if (argc != 2) {
    fprintf(stderr, "Error: keygen only takes one integer as an argument.\n");
    exit(1);
  }
  else {
    if ( (keylength = atoi(argv[1]) ) == 0 ) {
      fprintf(stderr, "Error: The key length could not be recognized.\n");
      exit(1);
    }
  }

  char finalkey[keylength]; // Create string
  finalkey[keylength] = '\0'; // Make sure end is null

  for (i = 0; i < keylength; i++) {
    int r = ( rand() % 27 );
    if (r == 26) {
      finalkey[i] = ' ';
    } // Space case
    else{
      finalkey[i] = ('A' + r);
    } // Letter case
  }

  printf("%s\n", finalkey);

  exit(0);
}
