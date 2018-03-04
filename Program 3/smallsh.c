#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv){
  // Variables
  char line[2048] = "";
  char* token;
  int status = 0;
  // Commandline loop
  do {
    // Print prompt
    printf(": ");
    fflush();
    // Read line
    int er = scanf("%s", line);
    // Check for error and if built-in Command
    if (er < 0) {
      printf("%s\n", "Failed to get line.");
    }
    else{
      token = strtok(line, ' '); // Get first token, which should be the command.
      if (token == "exit" || token == "status" || token == "cd") {

      }
      // Else: command is NOT built-in
      else{
        
      }
    }


  } while();


  // Exit

}
